#pragma once
// P15 verified-CA HTTPS reader for original Cardputer K132. No setInsecure,
// arbitrary script/HTML execution or plaintext credential transport.
// Requests are explicit, <=3KiB response buffer, bounded 3-5s timeouts.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <time.h>
#include "core/Net8Core.h"
#include "core/Web15Core.h"
class Web15Collection {
public:
 void begin(){
   mode_=-1;menu_=0;dirty_=true;connecting_=false;
   SPI.begin(40,39,14,12);
   sd_=SD.begin(12,SPI,10000000);
   WiFi.persistent(false);WiFi.mode(WIFI_OFF);
   loadNetwork();loadWeb();loadCA();
 }
 void exit(){WiFi.disconnect(true);WiFi.mode(WIFI_OFF);}
 void poll(){
   if(!connecting_)return;
   if(WiFi.status()==WL_CONNECTED){
     connecting_=false;
     configTime(0,0,"pool.ntp.org","time.google.com");
     status_="Wi-Fi connected. Syncing NTP.";
     display_=WiFi.localIP().toString();dirty_=true;
   }else if(static_cast<uint32_t>(millis()-started_)>=12000){
     connecting_=false;WiFi.disconnect();
     status_="Wi-Fi timed out";dirty_=true;
   }
 }
 bool dirty()const{return dirty_;}
 void input(const Keyboard_Class::KeysState&k){
   if(mode_<0){
     if(k.up&&menu_>0)--menu_;
     if(k.down&&menu_<5)++menu_;
     if(k.enter){mode_=menu_;status_="Press S / Enter to run";dirty_=true;}
     return;
   }
   if(letter(k,'b')){mode_=-1;dirty_=true;return;}
   if(letter(k,'r')){loadNetwork();loadWeb();loadCA();}
   if(letter(k,'s')||k.enter){
     if(mode_==0)connect();
     else run();
   }
   if(mode_==0&&letter(k,'d')){
     WiFi.disconnect(true);WiFi.mode(WIFI_OFF);
     connecting_=false;status_="Wi-Fi disconnected";
   }
   dirty_=true;
 }
 void render(){
   dirty_=false;auto&d=M5Cardputer.Display;
   constexpr uint16_t bg=0x0841,head=0x10c4,white=0xffff,
     cyan=0x05ff,sel=0x2673,green=0x5fe9,dim=0xad55;
   d.fillScreen(bg);d.fillRect(0,0,240,23,head);
   d.setTextColor(white,head);d.setCursor(5,7);
   d.print(mode_<0?"HTTPS WEB TOOLS":names(mode_));
   d.setTextColor(cyan,head);d.setCursor(207,7);d.print("TLS");
   d.fillRect(0,116,240,19,head);
   d.setTextColor(dim,head);d.setCursor(5,121);
   d.print(mode_<0?"Fn arrows / Enter":
      mode_==0?"S connect D disconnect R reload":
      "S verified HTTPS GET R config B");
   if(mode_<0){
     for(int i=0;i<6;++i){
       const int y=29+i*14;
       if(i==menu_)d.fillRect(3,y-1,233,12,sel);
       d.setTextColor(white,i==menu_?sel:bg);
       d.setCursor(7,y+1);d.print(String(i+1)+". "+names(i));
     }
     return;
   }
   d.setTextColor(cyan,bg);d.setCursor(5,27);
   d.print(WiFi.status()==WL_CONNECTED?"Wi-Fi ONLINE":connecting_?
     "CONNECTING...":"Wi-Fi OFFLINE");
   if(mode_==0){
     d.setTextColor(white,bg);d.setCursor(5,43);
     d.print(netOk_?String("SSID: ")+net_.ssid:"No NET.CFG");
     d.setCursor(5,59);
     d.print(certOk_?"CA certificate loaded":"CA PEM missing/invalid");
     d.setCursor(5,75);
     d.print(webOk_?"Web settings loaded":"Web defaults/invalid WEB.CFG");
     d.setCursor(5,89);d.print("Password never displayed");
   }else{
     d.setTextColor(green,bg);d.setCursor(5,41);
     d.print(display_.substring(0,37));
     d.setTextColor(white,bg);d.setCursor(5,55);
     d.print(content_.substring(0,37));
     d.setCursor(5,66);
     d.print(content_.substring(37,74));
     d.setCursor(5,77);
     d.print(content_.substring(74,111));
     d.setCursor(5,88);
     d.print(content_.substring(111,148));
   }
   d.setTextColor(dim,bg);d.setCursor(5,107);
   d.print(status_.substring(0,37));
 }
private:
 static const char*names(int i){
   static const char* const labels[]={
     "Verified Wi-Fi & CA","HTTPS Text Browser","HTTPS RSS Reader",
     "Wikipedia Text","Weather (manual coords)","HTTPS API GET"};
   return labels[i];
 }
 static bool letter(const Keyboard_Class::KeysState&k,char c){
   for(char v:k.word)if(v==c||v==(c-'a'+'A'))return true;
   return false;
 }
 suite::net8::Config net_;
 suite::web15::Config web_;
 bool sd_=false,dirty_=true,connecting_=false,netOk_=false,
      webOk_=false,certOk_=false;
 int mode_=-1,menu_=0;
 uint32_t started_=0;
 char ca_[4097]={},raw_[3073]={};
 String status_,display_,content_;
 bool readSmall(const char*path,char*out,size_t cap,size_t&len){
   len=0;if(!sd_)return false;
   File f=SD.open(path,FILE_READ);
   if(!f||f.isDirectory()||f.size()>=cap){
     if(f)f.close();return false;
   }
   len=f.size();
   const bool ok=f.readBytes(out,len)==len;f.close();out[len]=0;
   return ok;
 }
 void loadNetwork(){
   netOk_=false;
   char text[1025]={};size_t n=0;
   if(!readSmall("/SUITE/NET/NET.CFG",text,sizeof(text),n)||
      !suite::net8::parse(text,n,net_)){
     status_="Need valid NET.CFG for Wi-Fi";return;
   }
   netOk_=true;status_="Wi-Fi profile loaded (hidden)";
 }
 void loadWeb(){
   webOk_=false;web_=suite::web15::Config{};
   if(!sd_){status_="SD required for HTTPS";return;}
   char text[1025]={};size_t n=0;
   if(!readSmall("/SUITE/WEB/WEB.CFG",text,sizeof(text),n)){
     status_="Create WEB.CFG for pages";return;
   }
   webOk_=suite::web15::parse(text,n,web_);
   if(!webOk_)status_="Invalid WEB.CFG; no requests";
 }
 void loadCA(){
   certOk_=false;memset(ca_,0,sizeof(ca_));
   size_t n=0;
   if(!readSmall("/SUITE/WEB/ROOT_CA.PEM",ca_,sizeof(ca_),n)||
      n<500||strstr(ca_,"-----BEGIN CERTIFICATE-----")==nullptr||
      strstr(ca_,"-----END CERTIFICATE-----")==nullptr||
      memchr(ca_,0,n)!=nullptr){
     status_="Missing/invalid ROOT_CA.PEM";return;
   }
   certOk_=true;
 }
 void connect(){
   if(!netOk_){status_="Valid NET.CFG required";return;}
   WiFi.mode(WIFI_STA);WiFi.begin(net_.ssid,net_.password);
   connecting_=true;started_=millis();status_="Connecting...";
 }
 bool ready(){
   if(WiFi.status()!=WL_CONNECTED){status_="Connect Wi-Fi first";return false;}
   if(!certOk_){status_="CA required; TLS will not bypass";return false;}
   if(!webOk_){status_="Valid WEB.CFG required";return false;}
   time_t current=time(nullptr);
   if(current<1704067200){ // 2024-01-01 UTC, required for TLS cert dates.
     struct tm now={};
     if(!getLocalTime(&now,3200)||now.tm_year+1900<2024){
       status_="NTP clock not synced; retry";return false;
     }
   }
   return true;
 }
 bool https(const String&host,const String&path){
   if(!ready())return false;
   if(!suite::web15::validHost(host.c_str())||
      !suite::web15::validPath(path.c_str())){
     status_="Endpoint rejected";return false;
   }
   display_=host+path;content_="";
   // CA buffer is an app member, valid for the entire secure connection.
   WiFiClientSecure client;
   client.setCACert(ca_);
   HTTPClient http;
   const String url=String("https://")+host+path;
   if(!http.begin(client,url)){status_="HTTPS begin failed";return false;}
   http.setTimeout(3000);http.setConnectTimeout(3000);
   http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
   http.addHeader("Accept-Encoding","identity");
   const int code=http.GET();
   if(code<200||code>=300){
     status_=String("HTTPS status/error ")+code;http.end();return false;
   }
   if(http.getSize()>3072){
     status_="HTTPS response exceeds 3KiB";http.end();return false;
   }
   WiFiClient*stream=http.getStreamPtr();
   size_t n=0;const uint32_t started=millis();
   while(n<sizeof(raw_)-1&&static_cast<uint32_t>(millis()-started)<3000&&
         (http.connected()||stream->available())){
     if(stream->available()){
       const int v=stream->read();
       if(v>=0)raw_[n++]=static_cast<char>(v);
     }else delay(1);
   }
   raw_[n]=0;
   http.end();
   if(n==0){status_="HTTPS body empty";return false;}
   if(n==sizeof(raw_)-1){
     status_="Truncated HTTPS response (3KiB)";
   }else status_=String("Verified HTTPS ")+code+", "+n+" bytes";
   return true;
 }
 void run(){
   if(mode_==1){
     if(!suite::web15::validHost(web_.webHost)){
       status_="Configure WEB_HOST";return;
     }
     if(https(web_.webHost,web_.webPath)){
       char out[205]={};
       if(suite::web15::htmlText(raw_,out,sizeof(out)))content_=out;
       else status_="No basic text found";
     }
   }else if(mode_==2){
     if(!suite::web15::validHost(web_.rssHost)){
       status_="Configure RSS_HOST";return;
     }
     if(https(web_.rssHost,web_.rssPath)){
       char title[160]={};
       if(suite::web15::firstRssTitle(raw_,title,sizeof(title)))
         content_=title;
       else status_="No RSS item/title in first 3KiB";
     }
   }else if(mode_==3){
     char path[210]={};
     if(!suite::web15::wikiPath(web_.wikiTitle,path,sizeof(path))){
       status_="Invalid Wikipedia title";return;
     }
     if(https("en.wikipedia.org",path)){
       char extract[210]={};
       if(suite::web15::jsonString(raw_,"extract",extract,sizeof(extract)))
         content_=extract;
       else status_="Wikipedia extract not in first 3KiB";
     }
   }else if(mode_==4){
     if(!web_.location){status_="Configure manual LATITUDE/LONGITUDE";return;}
     const String path=String("/v1/forecast?latitude=")+
       String(web_.latitude,4)+"&longitude="+String(web_.longitude,4)+
       "&current=temperature_2m,relative_humidity_2m,wind_speed_10m";
     if(https("api.open-meteo.com",path)){
       double temperature=0,humidity=0,wind=0;
       if(suite::web15::jsonNumber(raw_,"temperature_2m",temperature)&&
          suite::web15::jsonNumber(raw_,"relative_humidity_2m",humidity)&&
          suite::web15::jsonNumber(raw_,"wind_speed_10m",wind)) {
         content_=String("Temp ")+String(temperature,1)+" C; RH "+
           String(humidity,0)+"%; Wind "+String(wind,1)+" km/h";
       }else status_="Weather data not in response";
     }
   }else if(mode_==5){
     if(!suite::web15::validHost(web_.apiHost)){
       status_="Configure API_HOST";return;
     }
     if(https(web_.apiHost,web_.apiPath)){
       char out[205]={};
       suite::web15::asciiClean(raw_,strlen(raw_),out,sizeof(out));
       content_=out;
     }
   }
   dirty_=true;
 }
};
