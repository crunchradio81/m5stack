#pragma once
// Stock original Cardputer P20: 3 owner-operated field/network apps, no extras.
// Wi-Fi log history is read-only; LAN receiver listens only on explicit action.
// Clock labels are *fixed* offsets, not timezone/DST database conversions.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <time.h>
#include "core/Field20Core.h"
#include "core/Net8Core.h"
#include "core/Theme16Runtime.h"

class Field20Collection {
 public:
 void begin(){
  mode_=-1;selected_=0;menu_=0;dirty_=true;connecting_=false;listening_=false;
  WiFi.persistent(false);WiFi.mode(WIFI_OFF);
  SPI.begin(40,39,14,12);sd_=SD.begin(12,SPI,10000000);
  status_=sd_?"R load saved survey; C connect":"Insert FAT32 SD";
  readConfig();
 }
 void exit(){
  if(client_)client_.stop();
  if(receiver_)receiver_.stop();
  server_.stop();
  WiFi.disconnect(true);WiFi.mode(WIFI_OFF);
 }
 bool dirty() const {return dirty_;}
 void input(const Keyboard_Class::KeysState&k){
  if(mode_<0){
   if(k.up&&menu_>0)--menu_;
   if(k.down&&menu_<2)++menu_;
   if(k.enter){mode_=menu_;selected_=0;if(mode_==0)loadHistory();}
   dirty_=true;return;
  }
  if(letter(k,'b')){endListener();mode_=-1;dirty_=true;return;}
  if(mode_==0){
   if(k.up&&selected_>0)--selected_;
   if(k.down&&selected_+1<historyCount_)++selected_;
   if(letter(k,'r'))loadHistory();
  }else{
   if(letter(k,'c'))connectWifi();
   if(mode_==1){
    if(k.left&&zone_>0)--zone_;
    if(k.right&&zone_<6)++zone_;
    if(letter(k,'n'))syncClock();
   }else if(mode_==2){
    if(letter(k,'l'))listen();
    if(letter(k,'t'))transmit();
    if(letter(k,'x'))endListener();
   }
  }
  dirty_=true;
 }
 void poll(){
  const uint32_t now=millis();
  if(connecting_){
   if(WiFi.status()==WL_CONNECTED){
    connecting_=false;status_="Wi-Fi connected (local LAN)";dirty_=true;
   }else if(now-connectAt_>12000){
    connecting_=false;WiFi.disconnect(true);
    status_="Connection timed out";dirty_=true;
   }
  }
  if(mode_==1 && now-lastClock_>=1000){lastClock_=now;dirty_=true;}
  if(!listening_)return;
  if(WiFi.status()!=WL_CONNECTED){endListener();status_="LAN connection lost";dirty_=true;return;}
  if(!receiver_){
   WiFiClient next=server_.available();
   if(next){
    IPAddress ip=next.remoteIP();
    if(!suite::field20::privateLan(ip[0],ip[1],ip[2],ip[3])){
     next.stop();status_="Rejected non-private LAN client";
    }else{
     receiver_=next;rxBytes_=0;rxAt_=now;
     status_=String("Receiving from ")+ip.toString();
    }
    dirty_=true;
   }
  }
  if(receiver_){
   uint8_t buffer[512];unsigned count=0;
   while(receiver_.available()&&count<8){
    int n=receiver_.read(buffer,sizeof(buffer));
    if(n<=0)break;rxBytes_+=static_cast<uint32_t>(n);++count;
   }
   if(!receiver_.connected() || now-rxAt_>15000){
    const uint32_t elapsed=now-rxAt_;
    lastRx_=suite::field20::kilobitsPerSecond(rxBytes_,elapsed);
    status_=String("RX ")+rxBytes_+" bytes / "+elapsed+"ms";
    receiver_.stop();dirty_=true;
   }
  }
 }
 void render(){
  dirty_=false;
  auto&d=M5Cardputer.Display;auto&p=suite::skin16::colors();
  d.fillScreen(p.bg);d.fillRect(0,0,240,23,p.panel);
  d.setTextColor(p.text,p.panel);d.setCursor(5,7);
  d.print(mode_<0?"FIELD LAB II":mode_==0?"WI-FI LOG HISTORY":mode_==1?"WORLD CLOCK":"LAN THROUGHPUT");
  d.fillRect(0,116,240,19,p.panel);d.setTextColor(p.muted,p.panel);
  d.setCursor(4,121);
  d.print(mode_<0?"Arrows select / Enter":
   mode_==0?"R refresh  B menu":
   mode_==1?"C WiFi N sync  <> zone B":"C WiFi L listen T send X stop B");
  if(mode_<0){
   static const char*names[]={"Wi-Fi History Explorer","World Clock / NTP","LAN Throughput Test"};
   for(int i=0;i<3;++i){
    const int y=32+i*22;
    if(i==menu_)d.fillRect(4,y-1,231,17,p.selected);
    d.setTextColor(p.text,i==menu_?p.selected:p.bg);
    d.setCursor(8,y+2);d.print(names[i]);
   }
  }else if(mode_==0){
   if(!historyCount_){
    d.setTextColor(p.muted,p.bg);d.setCursor(5,48);
    d.print("No valid /SUITE/FIELD/WIFI.CSV");
   }else{
    const int start=selected_>=5?selected_-4:0;
    for(int i=start;i<historyCount_&&i<start+5;++i){
     const int y=27+(i-start)*16;
     if(i==selected_)d.fillRect(4,y-1,232,15,p.selected);
     d.setTextColor(p.text,i==selected_?p.selected:p.bg);
     d.setCursor(6,y);
     d.print(String(history_[i].bssid).substring(0,18));
     d.setCursor(155,y);
     d.print(String(history_[i].avg())+" dBm");
    }
    const auto&h=history_[selected_];
    d.setTextColor(p.good,p.bg);d.setCursor(5,108);
    d.print(String(h.samples)+" samples ["+h.min+","+h.max+"]");
   }
  }else if(mode_==1){
   line(29,String("Network: ")+netState(),p.accent);
   line(48,suite::field20::zoneName(zone_),p.text);
   const time_t now=time(nullptr);
   if(now<1600000000){
    line(65,"Clock not synchronized",p.muted);
    line(80,"Press C WiFi, then N",p.muted);
   }else{
    const time_t local=now+suite::field20::offsetHours(zone_)*3600;
    struct tm tmv={};gmtime_r(&local,&tmv);
    char clock[42]={};strftime(clock,sizeof(clock),"%Y-%m-%d  %H:%M:%S",&tmv);
    line(67,clock,p.good);
    line(85,"Fixed offset; DST manual",p.muted);
   }
  }else if(mode_==2){
   line(29,String("Network: ")+netState(),p.accent);
   line(46,String("Peer: ")+ (configured_?config_.tcpHost:"NET.CFG needed"),p.text);
   line(62,listening_?"TCP :38946 LISTENING":"No TCP listener",p.good);
   line(78,String("TX kb/s: ")+lastTx_+" RX kb/s: "+lastRx_,p.text);
   line(93,"Unencrypted; trusted LAN only",p.muted);
  }
  if(mode_!=0){
   line(105,status_,p.muted);
  }else if(historyCount_==0){
   line(105,status_,p.muted);
  }
 }
 private:
 static bool letter(const Keyboard_Class::KeysState&k,char c){
  for(char x:k.word)if(x==c||x==(c-'a'+'A'))return true;
  return false;
 }
 void line(int y,const String&text,uint16_t fg){
  auto&d=M5Cardputer.Display;d.setCursor(5,y);
  d.setTextColor(fg,suite::skin16::colors().bg);
  d.print(text.substring(0,36));
 }
 String netState()const{
  return WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():
         connecting_?"connecting":"offline";
 }
 void readConfig(){
  configured_=false;if(!sd_)return;
  File f=SD.open("/SUITE/NET/NET.CFG",FILE_READ);
  if(!f||f.isDirectory()||f.size()>1024){if(f)f.close();return;}
  char text[1025]={};
  const size_t n=f.size();
  bool ok=f.readBytes(text,n)==n;f.close();
  suite::net8::Config next;
  if(ok&&suite::net8::parse(text,n,next)){
   config_=next;configured_=true;
  }
 }
 void connectWifi(){
  if(!configured_){status_="Create /SUITE/NET/NET.CFG";return;}
  if(WiFi.status()==WL_CONNECTED){status_="Already connected";return;}
  WiFi.persistent(false);WiFi.mode(WIFI_STA);
  WiFi.begin(config_.ssid,config_.password);
  connecting_=true;connectAt_=millis();status_="Connecting, 12s timeout";
 }
 void syncClock(){
  if(WiFi.status()!=WL_CONNECTED){status_="Connect Wi-Fi first";return;}
  configTime(0,0,"pool.ntp.org","time.google.com");
  status_="NTP requested; observe clock";dirty_=true;
 }
 void endListener(){
  if(receiver_)receiver_.stop();
  server_.stop();listening_=false;
 }
 void listen(){
  if(WiFi.status()!=WL_CONNECTED){status_="Connect Wi-Fi first";return;}
  endListener();
  server_.begin();listening_=true;
  status_="Peer must connect to this IP:38946";
 }
 void transmit(){
  if(!configured_||WiFi.status()!=WL_CONNECTED){
   status_="Connect with NET.CFG first";return;
  }
  IPAddress ip;
  if(!ip.fromString(config_.tcpHost)||
      !suite::field20::privateLan(ip[0],ip[1],ip[2],ip[3])){
   status_="TCP_HOST must be private IPv4";return;
  }
  // Explicit single run, 64KiB only, 4s maximum; no file data transmitted.
  WiFiClient tx;
  if(!tx.connect(ip,38946,1500)){
   status_="LAN test peer not listening";return;
  }
  uint8_t pattern[512]={};for(unsigned i=0;i<sizeof(pattern);++i)pattern[i]=i&255u;
  const uint32_t start=millis();
  uint32_t sent=0;
  while(sent<65536 && millis()-start<4000){
   const size_t n=tx.write(pattern,sizeof(pattern));
   if(n==0)break;sent+=static_cast<uint32_t>(n);
   yield();
  }
  tx.stop();
  const uint32_t duration=millis()-start;
  lastTx_=suite::field20::kilobitsPerSecond(sent,duration);
  status_=String("TX ")+sent+" bytes / "+duration+"ms";
  dirty_=true;
 }
 void loadHistory(){
  memset(history_,0,sizeof(history_));historyCount_=0;selected_=0;
  if(!sd_){status_="Insert SD card";return;}
  File f=SD.open("/SUITE/FIELD/WIFI.CSV",FILE_READ);
  if(!f||f.isDirectory()){
   if(f)f.close();status_="No P7 survey log on SD";return;
  }
  if(f.size()>131072){
   if(!f.seek(f.size()-131072)){f.close();return;}
   while(f.available() && f.read()!='\n'){} // discard partial first line
  }
  unsigned rows=0,valid=0,ignored=0;
  while(f.available()&&rows<768){
   char line[224]={};size_t n=0;bool tooLong=false;
   while(f.available()){
    const int c=f.read();
    if(c=='\n'||c<0)break;
    if(n+1<sizeof(line))line[n++]=static_cast<char>(c);
    else tooLong=true;
   }
   ++rows;
   if(tooLong){++ignored;continue;}
   suite::field20::Survey s;
   if(suite::field20::survey(line,s)){
    if(suite::field20::accumulate(history_,16,s))++valid;
   }else ++ignored;
  }
  f.close();
  for(int i=0;i<16;++i)if(history_[i].samples)++historyCount_;
  status_=String(valid)+" survey rows / "+ignored+" invalid";
  dirty_=true;
 }
 int mode_=-1,menu_=0,selected_=0,zone_=1,historyCount_=0;
 bool sd_=false,configured_=false,dirty_=true,connecting_=false,listening_=false;
 uint32_t connectAt_=0,lastClock_=0,rxAt_=0,rxBytes_=0,lastTx_=0,lastRx_=0;
 suite::net8::Config config_;
 suite::field20::History history_[16]={};
 WiFiServer server_{38946};
 WiFiClient receiver_,client_;
 String status_;
};
