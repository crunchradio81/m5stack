#pragma once
// Five passive field tools: 2.4GHz strength graph, manually named survey,
// appearance monitor, BLE advertisement log, BLE RSSI viewer.
// No location inference, Wi-Fi association, BLE pairing or active connections.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include "core/P7Core.h"
class WirelessCollection {
public:
 void begin(){
   mode_=-1;menu_=0;selected_=0;count_=0;running_=false;dirty_=true;
   SPI.begin(40,39,14,12);sd_=SD.begin(12,SPI,10000000);
   if(sd_){
     if(!SD.exists("/SUITE"))SD.mkdir("/SUITE");
     if(!SD.exists("/SUITE/FIELD"))SD.mkdir("/SUITE/FIELD");
   }
   WiFi.mode(WIFI_OFF);BLEDevice::init("");
   ble_=BLEDevice::getScan();ble_->setActiveScan(false);
   ble_->setInterval(100);ble_->setWindow(80);
   status_=sd_?"Ready: local logs on SD":"No SD: viewing only";
 }
 void exit(){
   running_=scanning_=false;
   WiFi.scanDelete();WiFi.mode(WIFI_OFF);
   if(ble_)ble_->stop();
 }
 bool dirty()const{return dirty_;}
 void poll(){
   if(mode_<0)return;
   if(mode_<=2){
     if(scanning_){
       const int n=WiFi.scanComplete();
       if(n!=WIFI_SCAN_RUNNING)finishWifi(n);
     }else if(running_ && static_cast<uint32_t>(millis()-lastScan_)>7000u)startWifi();
   }
 }
 void input(const Keyboard_Class::KeysState&k){
   if(eraseConfirm_){
     if(letter(k,'y')){
       const char*path=mode_>=3?"/SUITE/FIELD/BLE.CSV":"/SUITE/FIELD/WIFI.CSV";
       if(sd_&&SD.exists(path)&&SD.remove(path))status_="Log erased";
       else status_="No log / erase failed";
     }else status_="Erase canceled";
     eraseConfirm_=false;dirty_=true;return;
   }
   if(editSite_){
     if(k.enter){editSite_=false;status_=String("Survey: ")+site_;}
     else if(k.backspace){const size_t n=strlen(site_);if(n)site_[n-1]=0;}
     else if(!k.fn&&!k.alt&&!k.ctrl){
       for(char c:k.word){
         if(c<32||c>126||c=='"'||c==','||c=='\n')continue;
         const size_t n=strlen(site_);if(n<20){site_[n]=c;site_[n+1]=0;}
       }
     }
     dirty_=true;return;
   }
   if(mode_<0){
     if(k.up&&menu_>0)--menu_;
     if(k.down&&menu_<4)++menu_;
     if(k.enter){mode_=menu_;selected_=0;dirty_=true;running_=false;}
     return;
   }
   if(letter(k,'b')){
     running_=false;mode_=-1;
     WiFi.scanDelete();WiFi.mode(WIFI_OFF);scanning_=false;dirty_=true;return;
   }
   if(letter(k,'e')){eraseConfirm_=true;status_="Delete log? Y confirms / N cancel";dirty_=true;return;}
   if(mode_==1&&letter(k,'n')){editSite_=true;site_[0]=0;}
   if(mode_<=2){
     if(letter(k,'s')) {
       running_=!running_;
       status_=running_?"Automatic scan every ~7s":"Paused";
       if(running_)startWifi();
     }else if(letter(k,'r'))startWifi();
     const int previous=selected_;
     if(k.up&&selected_>0)--selected_;
     if(k.down&&selected_+1<count_)++selected_;
     if(selected_!=previous&&mode_==0){
       focused_="";trend_.clear();
     }
   } else if(mode_>=3) {
     if(letter(k,'s')||letter(k,'r'))scanBle();
     if(k.up&&selected_>0)--selected_;
     if(k.down&&selected_+1<count_)++selected_;
   }
   dirty_=true;
 }
 void render(){
   dirty_=false;auto&d=M5Cardputer.Display;
   constexpr uint16_t bg=0x0841,head=0x10c4,white=0xffff,
     cyan=0x05ff,green=0x5fe9,sel=0x2673,dim=0xad55;
   d.fillScreen(bg);d.fillRect(0,0,240,23,head);
   d.setTextColor(white,head);d.setCursor(5,7);
   d.print(mode_<0?"FIELD WIRELESS":names(mode_));
   d.setTextColor(cyan,head);d.setCursor(206,7);d.print("SD");
   d.fillRect(0,116,240,19,head);
   d.setTextColor(dim,head);d.setCursor(4,121);
   d.print(mode_<0?"Fn arrows / Enter":
     mode_==1?"S auto R scan N site E erase B":
     "S start/scan R rescan E erase B");
   if(mode_<0) {
     for(int i=0;i<5;++i) {
       const int y=28+i*16;
       if(menu_==i)d.fillRect(3,y-1,233,14,sel);
       d.setTextColor(white,menu_==i?sel:bg);
       d.setCursor(7,y+2);d.print(String(i+1)+". "+names(i));
     }
   }else {
     d.setTextColor(cyan,bg);d.setCursor(5,27);
     d.print(mode_<=2?String("2.4 GHz APs: ")+count_+
       (running_?" AUTO":" PAUSED"):String("BLE advertisers: ")+count_);
     if(mode_==1){
       d.setTextColor(green,bg);d.setCursor(5,38);
       d.print(String("Site: ")+site_+(editSite_?"_":""));
     }
     if(mode_==0&&count_) {
       const auto &ap=wifi_[selected_];
       d.setTextColor(white,bg);d.setCursor(5,41);d.print(ap.name.substring(0,33));
       d.setCursor(5,55);
       d.print(String(ap.rssi)+" dBm Ch "+ap.channel);
       d.setTextColor(green,bg);d.setCursor(5,68);
       d.print(String("History average: ")+trend_.average()+" dBm");
       for(unsigned i=0;i<trend_.count();++i){
         const int height=(trend_.at(i)+127)*33/127;
         d.fillRect(5+i*14,111-height,11,height,sel);
       }
     }else if(mode_==2){
       d.setTextColor(white,bg);d.setCursor(5,47);
       d.print(String("Appeared: ")+appeared_+"  Gone: "+disappeared_);
       d.setCursor(5,64);d.print("Compares consecutive scans");
       d.setCursor(5,79);d.print("AP address only; no GPS");
     }else{
       const int top=selected_>=5?selected_-4:0;
       for(int i=top;i<count_&&i<top+5;++i) {
         const int y=mode_==1?51+(i-top)*11:42+(i-top)*13;
         if(i==selected_)d.fillRect(3,y-1,233,11,sel);
         d.setTextColor(white,i==selected_?sel:bg);
         d.setCursor(5,y);
         d.print((mode_<=2?wifi_[i].name:bleNames_[i]).substring(0,25));
         d.print(" ");
         d.print(mode_<=2?wifi_[i].rssi:bleRssi_[i]);
       }
     }
   }
   d.setTextColor(dim,bg);d.setCursor(5,108);
   d.print(eraseConfirm_?"Confirm ERASE with Y/N":status_.substring(0,38));
 }
private:
 struct AP {String name,mac;int rssi=0,channel=0;};
 static const char*names(int i){
   static const char* const values[]={"Wi-Fi Signal Graph","Wi-Fi Survey Notebook",
     "AP Appearance Monitor","BLE Advertisement Logger","BLE Signal Viewer"};
   return values[i];
 }
 AP wifi_[24];
 String prev_[24];
 String bleNames_[24],bleMac_[24];
 int bleRssi_[24]={};
 suite::field::SignalTrend trend_;
 int mode_=-1,menu_=0,selected_=0,count_=0,prevCount_=0,appeared_=0,disappeared_=0;
 uint32_t lastScan_=0;
 bool sd_=false,dirty_=true,running_=false,scanning_=false,
      editSite_=false,eraseConfirm_=false;
 char site_[22]="UNNAMED";
 String status_,focused_;
 BLEScan*ble_=nullptr;
 static bool letter(const Keyboard_Class::KeysState&k,char v){
   for(char c:k.word)if(c==v||c==(v-'a'+'A'))return true;
   return false;
 }
 void startWifi(){
   if(scanning_)return;
   WiFi.mode(WIFI_STA);WiFi.disconnect();WiFi.scanDelete();
   const int code=WiFi.scanNetworks(true,true,true,250);
   if(code==WIFI_SCAN_FAILED){WiFi.mode(WIFI_OFF);status_="Scan failed";return;}
   scanning_=true;lastScan_=millis();status_="Scanning...";dirty_=true;
 }
 void finishWifi(int found){
   scanning_=false;
   if(found<0){status_="Scan error";WiFi.scanDelete();WiFi.mode(WIFI_OFF);return;}
   String current[24];const int oldCount=prevCount_;
   const int limit=found>24?24:found;
   appeared_=disappeared_=0;
   for(int i=0;i<limit;++i) {
     wifi_[i].name=WiFi.SSID(i);
     wifi_[i].mac=WiFi.BSSIDstr(i);
     wifi_[i].rssi=WiFi.RSSI(i);
     wifi_[i].channel=WiFi.channel(i);
     current[i]=wifi_[i].mac;
     bool previously=false;
     for(int j=0;j<prevCount_;++j)
       if(prev_[j]==current[i])previously=true;
     if(!previously)++appeared_;
   }
   for(int j=0;j<prevCount_;++j){
     bool present=false;
     for(int i=0;i<limit;++i)if(prev_[j]==current[i])present=true;
     if(!present)++disappeared_;
   }
   count_=limit;
   for(int i=0;i<limit;++i)prev_[i]=current[i];
   prevCount_=limit;
   if(selected_>=count_)selected_=count_?count_-1:0;
   if(count_) {
     if(focused_!=wifi_[selected_].mac){
       focused_=wifi_[selected_].mac;trend_.clear();
     }
     trend_.sample(wifi_[selected_].rssi);
   }
   if(sd_&&mode_==1){
     File out=SD.open("/SUITE/FIELD/WIFI.CSV",FILE_APPEND);
     if(out){
       if(out.size()<1024*1024) {
         for(int i=0;i<count_;++i) {
           char name[135];
           const String ssid=wifi_[i].name.substring(0,52);
           if(!suite::field::csvEscape(ssid.c_str(),name,sizeof(name)))continue;
           out.printf("%lu,%s,%s,%d,%d,%s\n",
             static_cast<unsigned long>(millis()/1000),site_,
             wifi_[i].mac.c_str(),wifi_[i].rssi,wifi_[i].channel,name);
         }
         out.flush();
       }else status_="Wi-Fi log full (1MiB cap)";
       out.close();
     }else status_="SD log open failed";
   }
   WiFi.scanDelete();WiFi.mode(WIFI_OFF);
   if(!status_.startsWith("Wi-Fi log full")&&!status_.startsWith("SD log open failed"))
     status_=String("Found ")+found+" access points";
   (void)oldCount;dirty_=true;
 }
 void scanBle(){
   if(!ble_){status_="BLE unavailable";return;}
   running_=false;scanning_=false;
   WiFi.scanDelete();WiFi.mode(WIFI_OFF);
   auto&d=M5Cardputer.Display;
   d.fillScreen(0x0841);d.setTextColor(0xffff,0x0841);
   d.setCursor(5,40);d.print("BLE passive scan (2s)...");
   BLEScanResults found=ble_->start(2,false);
   count_=found.getCount()>24?24:found.getCount();
   selected_=0;
   for(int i=0;i<count_;++i) {
     BLEAdvertisedDevice device=found.getDevice(i);
     bleMac_[i]=device.getAddress().toString().c_str();
     const auto name=device.getName();
     bleNames_[i]=name.empty()?"(unnamed)":String(name.c_str());
     bleRssi_[i]=device.getRSSI();
   }
   ble_->clearResults();
   if(sd_&&mode_==3) {
     File out=SD.open("/SUITE/FIELD/BLE.CSV",FILE_APPEND);
     if(out) {
       if(out.size()<1024*1024) {
         for(int i=0;i<count_;++i) {
           char escaped[140];const String name=bleNames_[i].substring(0,52);
           if(!suite::field::csvEscape(name.c_str(),escaped,sizeof(escaped)))continue;
           out.printf("%lu,%s,%d,%s\n",
             static_cast<unsigned long>(millis()/1000),bleMac_[i].c_str(),
             bleRssi_[i],escaped);
         }
         out.flush();
       }else status_="BLE log full (1MiB cap)";
       out.close();
     }
   }
   status_=String("BLE advertisements: ")+found.getCount();
   dirty_=true;
 }
};
