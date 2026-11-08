#pragma once
// P23 original stock Cardputer BLE GATT metadata viewer.
// BLE connection requires explicit Enter; never reads values, writes,
// subscribes, pairs, or controls a discovered device.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLERemoteService.h>
#include <BLERemoteCharacteristic.h>
#include "core/Gatt23Core.h"
#include "core/Theme16Runtime.h"
class Gatt23Collection {
 struct Target {
  String address,name;
  int rssi=0;
  esp_ble_addr_type_t type=BLE_ADDR_TYPE_PUBLIC;
 };
 Target discovered_[suite::gatt23::MaxAdvertisers];
 int count_=0,selected_=0,row_=0;
 bool inspecting_=false,dirty_=true;
 BLEScan*scan_=nullptr;
 BLEClient*client_=nullptr;
 suite::gatt23::Report report_;
 String status_="S scan  Enter connect  D disconnect";

 static bool letter(const Keyboard_Class::KeysState&keys,char c){
  for(char x:keys.word)
   if(x==c||(c>='a'&&c<='z'&&x==c-'a'+'A'))return true;
  return false;
 }
 void close(){
  if(client_){
   if(client_->isConnected())client_->disconnect();
   delete client_;client_=nullptr;
  }
  inspecting_=false;row_=0;report_.reset();
 }
 void scan(){
  close();count_=selected_=0;
  if(!scan_){status_="BLE scanner unavailable";return;}
  auto&d=M5Cardputer.Display;
  d.fillScreen(0x0841);d.setTextColor(0xffff,0x0841);
  d.setCursor(5,40);d.print("Passive BLE scan (3 sec)...");
  BLEScanResults results=scan_->start(3,false);
  const int found=results.getCount();
  count_=found<static_cast<int>(suite::gatt23::MaxAdvertisers)?found:
            static_cast<int>(suite::gatt23::MaxAdvertisers);
  for(int i=0;i<count_;++i){
   BLEAdvertisedDevice device=results.getDevice(i);
   discovered_[i].address=String(device.getAddress().toString().c_str());
   discovered_[i].type=device.getAddressType();
   discovered_[i].name=device.getName().empty()?"(unnamed)":
      String(device.getName().c_str());
   discovered_[i].rssi=device.getRSSI();
  }
  scan_->clearResults();
  status_=String(count_)+" BLE devices; Enter GATT on selected";
  dirty_=true;
 }
 void inspect(){
  if(selected_<0||selected_>=count_){
   status_="Select a scanned device first";return;
  }
  close();
  client_=BLEDevice::createClient();
  if(!client_){status_="BLE client allocation failed";return;}
  auto&d=M5Cardputer.Display;
  d.fillScreen(0x0841);d.setTextColor(0xffff,0x0841);
  d.setCursor(5,40);d.print("Connecting to selected BLE...");
  BLEAddress address(discovered_[selected_].address.c_str());
  if(!client_->connect(address,discovered_[selected_].type)){
   status_="GATT connect failed; no pairing";
   close();dirty_=true;return;
  }
  // UUID/properties metadata only. No characteristic read/write/notification.
  const auto*services=client_->getServices();
  if(!services){status_="GATT service discovery failed";close();return;}
  report_.reset();
  for(const auto&service:*services){
   if(report_.count()>=suite::gatt23::MaxResults)break;
   if(!service.second)continue;
   const std::string id=service.second->getUUID().toString();
   if(!report_.add(id.c_str(),'S',"-"))continue;
   const auto*characteristics=service.second->getCharacteristics();
   if(!characteristics)continue;
   for(const auto&ch:*characteristics){
    if(!ch.second)continue;
    char props[8]={};
    suite::gatt23::properties(ch.second->canRead(),ch.second->canWrite(),
      ch.second->canNotify(),ch.second->canIndicate(),props);
    report_.add(ch.second->getUUID().toString().c_str(),'C',props);
    if(report_.count()>=suite::gatt23::MaxResults)break;
   }
  }
  inspecting_=true;row_=0;
  status_=String("GATT metadata ")+report_.count()+
     (report_.cropped()?" capped":" entries");
  dirty_=true;
 }
public:
 void begin(){
  WiFi.mode(WIFI_OFF);
  BLEDevice::init("");
  scan_=BLEDevice::getScan();
  if(scan_){
   scan_->setActiveScan(false);
   scan_->setInterval(100);scan_->setWindow(80);
  }
  count_=selected_=row_=0;inspecting_=false;dirty_=true;
  report_.reset();status_="S passive scan / Enter GATT / D close";
 }
 void exit(){
  if(scan_)scan_->stop();
  close();
 }
 bool dirty()const{return dirty_;}
 void poll(){
  if(inspecting_ && client_&&!client_->isConnected()){
   close();status_="Device disconnected; scan again";dirty_=true;
  }
 }
 void input(const Keyboard_Class::KeysState&keys){
  if(letter(keys,'d')||letter(keys,'b')){
   close();status_="BLE disconnected";dirty_=true;return;
  }
  if(letter(keys,'s')){
   scan();return;
  }
  if(inspecting_){
   if(keys.up&&row_>0)--row_;
   if(keys.down&&row_+1<static_cast<int>(report_.count()))++row_;
  }else{
   if(keys.up&&selected_>0)--selected_;
   if(keys.down&&selected_+1<count_)++selected_;
   if(keys.enter)inspect();
  }
  dirty_=true;
 }
 void render(){
  dirty_=false;
  auto&d=M5Cardputer.Display;const auto&p=suite::skin16::colors();
  d.fillScreen(p.bg);d.fillRect(0,0,240,22,p.panel);
  d.setTextColor(p.text,p.panel);d.setCursor(5,7);
  d.print(inspecting_?"BLE GATT (UUID ONLY)":"BLE GATT EXPLORER");
  d.setTextColor(p.accent,p.panel);d.setCursor(206,7);d.print("P23");
  d.fillRect(0,117,240,18,p.panel);
  d.setTextColor(p.muted,p.panel);d.setCursor(5,122);
  d.print(inspecting_?"Arrows metadata D close S rescan":
                       "S scan Enter GATT D disconnect");
  if(inspecting_){
   d.setTextColor(p.good,p.bg);d.setCursor(4,26);
   d.print(discovered_[selected_].address);
   const int first=row_>=5?row_-4:0;
   for(int i=first;i<static_cast<int>(report_.count())&&i<first+5;++i){
    const auto*entry=report_.get(i);
    if(!entry)continue;
    const int y=45+(i-first)*13;
    if(i==row_)d.fillRect(2,y-1,236,12,p.selected);
    d.setTextColor(p.text,i==row_?p.selected:p.bg);
    d.setCursor(4,y);
    d.print(entry->kind=='S'?"S ":"C ");
    d.print(String(entry->uuid).substring(0,26));
    d.setCursor(200,y);d.print(entry->perms);
   }
  }else{
   d.setTextColor(p.muted,p.bg);d.setCursor(4,27);
   d.print(String("Advertisements: ")+count_+" (manual 3s scan)");
   const int top=selected_>=5?selected_-4:0;
   for(int i=top;i<count_&&i<top+5;++i){
    const int y=45+(i-top)*13;
    if(i==selected_)d.fillRect(2,y-1,236,12,p.selected);
    d.setTextColor(p.text,i==selected_?p.selected:p.bg);
    d.setCursor(4,y);d.print(discovered_[i].name.substring(0,20));
    d.setCursor(177,y);d.print(discovered_[i].rssi);
   }
  }
  d.setTextColor(p.muted,p.bg);d.setCursor(4,108);
  d.print(status_.substring(0,38));
 }
};
