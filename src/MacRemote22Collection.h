#pragma once
// P22 stock original K132 remote companion for the Mac native menu bar app.
// User starts Wi-Fi and authenticates each 512-byte command with shared HMAC.
// Plain TCP on trusted LAN: command contents are NOT encrypted.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <mbedtls/md.h>
#include "core/MacRemote22Core.h"
#include "core/Net8Core.h"
#include "core/Theme16Runtime.h"

class MacRemote22Collection {
public:
 void begin(){
  mode_=0;dirty_=true;connected_=connecting_=false;
  WiFi.persistent(false);WiFi.mode(WIFI_OFF);
  SPI.begin(40,39,14,12);sd_=SD.begin(12,SPI,10000000);
  status_=sd_?"R load configs, C connect":"Insert FAT32 microSD";
  load();
 }
 void exit(){
  disconnect();
  WiFi.disconnect(true);WiFi.mode(WIFI_OFF);
  memset(pair_.secret,0,sizeof(pair_.secret));
 }
 bool dirty()const{return dirty_;}
 void poll(){
  if(connected_ && (!client_.connected()||WiFi.status()!=WL_CONNECTED)){
    disconnect();status_="Mac disconnected";dirty_=true;
  }
 }
 void input(const Keyboard_Class::KeysState&k){
  if(k.fn && key(k,'1')){mode_=0;dirty_=true;return;}
  if(k.fn && key(k,'2')){mode_=1;dirty_=true;return;}
  if(k.fn && key(k,'3')){mode_=2;dirty_=true;return;}
  if(k.fn && key(k,'4')){mode_=3;dirty_=true;return;}
  if(mode_==0){
    if(key(k,'r'))load();
    if(key(k,'c'))connect();
    if(key(k,'d')){disconnect();status_="Disconnected";}
  }else if(!connected_){
    status_="Connect first: Fn+1 then C";
  }else if(mode_==1){
    for(char c:k.word){
      const int index=suite::mac22::macroIndex(c);
      if(index>0){
        if(send("MACRO",String(index))){
          status_=String("Macro ")+index+" sent";
        }
      }
    }
  }else if(mode_==2){
    // Virtual D-pad, mouse click and scroll. No cursor hardware required.
    if(k.up||key(k,'i'))send("MOVE","0,-12");
    if(k.down||key(k,'k'))send("MOVE","0,12");
    if(k.left||key(k,'j'))send("MOVE","-12,0");
    if(k.right||key(k,'l'))send("MOVE","12,0");
    if(key(k,' '))send("CLICK","L");
    if(key(k,'r'))send("CLICK","R");
    if(key(k,'u'))send("SCROLL","2");
    if(key(k,'n'))send("SCROLL","-2");
  }else if(mode_==3){
    // Stock K132 keyboard generates printable ASCII; Mac macro text can be
    // full Unicode, delivered using CGEventKeyboardSetUnicodeString.
    if(!k.fn&&!k.ctrl&&!k.alt){
      char hex[65]={};size_t used=0;
      static const char alpha[]="0123456789abcdef";
      for(char c:k.word){
        if(!suite::mac22::utf8Ascii(c)||used>=32)continue;
        const uint8_t b=static_cast<uint8_t>(c);
        hex[used*2]=alpha[b>>4];
        hex[used*2+1]=alpha[b&15];
        ++used;
      }
      if(used&&send("TEXT",String(hex)))status_=String("Sent ")+used+" text bytes";
    }
  }
  dirty_=true;
 }
 void render(){
  dirty_=false;
  auto& d=M5Cardputer.Display;
  const auto&p=suite::skin16::colors();
  d.fillScreen(p.bg);d.fillRect(0,0,240,23,p.panel);
  d.setTextColor(p.text,p.panel);d.setCursor(5,7);
  d.print("MAC REMOTE");
  d.setTextColor(p.accent,p.panel);d.setCursor(207,7);d.print("P22");
  d.setTextColor(p.good,p.bg);d.setCursor(5,31);
  d.print(connected_?"PAIR AUTHENTICATED":(configured_?"READY (OFF)":"NOT CONFIGURED"));
  d.setTextColor(p.text,p.bg);d.setCursor(5,49);
  d.print(String("Mac: ")+(configured_?pair_.address:"Need REMOTE.CFG"));
  d.setCursor(5,66);
  d.print(String("Mode: ")+(mode_==0?"Setup":mode_==1?"12 Macro Keys":
          mode_==2?"Mouse / Scroll":"Type to Mac"));
  d.setTextColor(p.muted,p.bg);d.setCursor(5,89);
  d.print(status_.substring(0,38));
  d.setCursor(5,103);
  d.print("Mac must enable LAN + Accessibility");
  d.fillRect(0,116,240,19,p.panel);
  d.setTextColor(p.text,p.panel);d.setCursor(4,121);
  d.print(mode_==0?"R reload C connect D disconnect":
         mode_==1?"1..0 -= macros  Fn+3 mouse":
         mode_==2?"IJKL move space click R right":"Type ASCII | Fn+1 setup");
 }
private:
 static bool key(const Keyboard_Class::KeysState&k,char expected){
  for(char c:k.word)
    if(c==expected || (expected>='a'&&expected<='z' &&
                       c==expected-'a'+'A'))return true;
  return false;
 }
 void load(){
  configured_=netConfigured_=false;
  disconnect();
  if(!sd_){status_="SD card required";return;}
  File f=SD.open("/SUITE/MAC/REMOTE.CFG",FILE_READ);
  if(!f||f.isDirectory()||f.size()>160){
   if(f)f.close();status_="Add /SUITE/MAC/REMOTE.CFG";return;
  }
  char raw[161]={};
  const size_t n=f.size();bool ok=f.readBytes(raw,n)==n;f.close();
  suite::mac22::Config next;
  if(!ok||!suite::mac22::parse(raw,n,next)){
   status_="REMOTE.CFG invalid; no connection";return;
  }
  File net=SD.open("/SUITE/NET/NET.CFG",FILE_READ);
  if(!net||net.isDirectory()||net.size()>1024){
   if(net)net.close();status_="NET.CFG missing or invalid";return;
  }
  char nr[1025]={};const size_t count=net.size();
  ok=net.readBytes(nr,count)==count;net.close();
  suite::net8::Config profile;
  if(!ok||!suite::net8::parse(nr,count,profile)){
   status_="NET.CFG invalid; no connection";return;
  }
  pair_=next;network_=profile;
  netConfigured_=configured_=true;
  status_="Pairing config loaded (token hidden)";
 }
 void disconnect(){
  client_.stop();
  connected_=connecting_=false;
  seq_=0;challenge_="";
 }
 void connect(){
  if(!configured_||!netConfigured_){
   status_="Valid REMOTE.CFG + NET.CFG needed";return;
  }
  if(connected_){status_="Already connected";return;}
  // All network activity requires an explicit C action.
  WiFi.mode(WIFI_STA);WiFi.begin(network_.ssid,network_.password);
  const uint32_t start=millis();
  while(WiFi.status()!=WL_CONNECTED && millis()-start<12000){
   M5Cardputer.update();delay(30);
  }
  if(WiFi.status()!=WL_CONNECTED){
   WiFi.disconnect(true);status_="Wi-Fi connection failed";return;
  }
  client_.setTimeout(1800);
  if(!client_.connect(pair_.address,38947,2200)){
   status_="Mac remote unavailable";return;
  }
  const String greeting=client_.readStringUntil('\n');
  const char*prefix="K132R1 CHALLENGE ";
  if(!greeting.startsWith(prefix) ||
     greeting.length()!=strlen(prefix)+32){
   disconnect();status_="Mac authentication handshake invalid";return;
  }
  challenge_=greeting.substring(strlen(prefix));
  for(unsigned i=0;i<challenge_.length();++i)
    if(suite::mac22::hexDigit(challenge_[i])<0){
      disconnect();status_="Invalid Mac challenge";return;
    }
  seq_=0;connected_=true;status_="Connected to authenticated Mac session";
 }
 bool send(const char*verb,const String&arg){
  if(!connected_ || !client_.connected())return false;
  if(seq_>=UINT32_MAX){disconnect();return false;}
  const String text=String("K132R1|")+challenge_+"|"+String(++seq_)+"|"+verb+"|"+arg;
  if(text.length()>400){status_="Remote message too long";return false;}
  unsigned char digest[32]={};
  const mbedtls_md_info_t*info=mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if(!info ||
     mbedtls_md_hmac(info,pair_.secret,32,
        reinterpret_cast<const unsigned char*>(text.c_str()),
        text.length(),digest)!=0){
    disconnect();status_="HMAC failure";return false;
  }
  static const char hex[]="0123456789abcdef";
  char signature[65]={};
  for(int i=0;i<32;++i){
   signature[2*i]=hex[digest[i]>>4];signature[2*i+1]=hex[digest[i]&15];
  }
  const String message=text+"|"+signature+"\n";
  if(client_.print(message)!=message.length()){
   disconnect();status_="Network write failed";return false;
  }
  return true;
 }
 suite::mac22::Config pair_;
 suite::net8::Config network_;
 WiFiClient client_;
 String challenge_,status_;
 uint32_t seq_=0;
 bool sd_=false,configured_=false,netConfigured_=false,connected_=false,
      connecting_=false,dirty_=true;
 int mode_=0;
};
