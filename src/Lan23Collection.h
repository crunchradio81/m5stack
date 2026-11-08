#pragma once
// P23 owner-initiated SD-to-SD file transfer. Original stock Cardputer K132.
// Private trusted LAN only: HMAC authenticates, but TCP is NOT encrypted.
// Receiver never browses arbitrary directories and never overwrites files.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <esp_system.h>
#include <mbedtls/md.h>
#include <mbedtls/sha256.h>
#include <string.h>
#include "core/Lan23Core.h"
#include "core/Field20Core.h"
#include "core/Net8Core.h"
#include "core/Theme16Runtime.h"

class Lan23Collection {
 static constexpr const char* OutDir="/SUITE/OUTBOX";
 static constexpr const char* InDir="/SUITE/INBOX";
 static constexpr const char* Temp="/SUITE/INBOX/UPLOAD.PART";
 static constexpr uint32_t ListenTime=35000;
 static constexpr uint32_t IdleTime=9000;
 enum class Stage:uint8_t {Off,Listen,Header,Body};
 struct Choice {
  char name[suite::safefiles17::MaxName]={};
  uint32_t bytes=0;
 };
 Choice choices_[suite::lan23::MaxFiles]={};
 int count_=0,selected_=0;
 bool sd_=false,configured_=false,wifiConnecting_=false,dirty_=true;
 uint32_t wifiAt_=0,listenAt_=0,activityAt_=0,received_=0;
 Stage stage_=Stage::Off;
 WiFiServer server_{suite::lan23::Port};
 WiFiClient peer_;
 File incoming_;
 char header_[suite::lan23::MaxHeader]={};
 size_t headerUsed_=0;
 suite::lan23::Header expectation_;
 suite::lan23::Config lan_;
 suite::net8::Config network_;
 String challenge_,status_;
 mbedtls_sha256_context rxHash_;
 bool hashStarted_=false;

 static bool letter(const Keyboard_Class::KeysState& k,char c){
  for(char x:k.word)if(x==c || x==c-'a'+'A')return true;
  return false;
 }
 static bool privateIp(const IPAddress& ip){
  return suite::field20::privateLan(ip[0],ip[1],ip[2],ip[3]);
 }
 void freeHash(){
  if(hashStarted_){mbedtls_sha256_free(&rxHash_);hashStarted_=false;}
 }
 void endListen(const String&reason,bool failed){
  if(incoming_){incoming_.flush();incoming_.close();}
  freeHash();
  if(peer_){
   if(failed)peer_.print(suite::lan23::Failure);
   peer_.flush();peer_.stop();
  }
  server_.stop();
  stage_=Stage::Off;headerUsed_=0;received_=0;
  status_=reason;dirty_=true;
 }
 void ensureFolders(){
  if(!sd_)return;
  if(!SD.exists("/SUITE"))SD.mkdir("/SUITE");
  if(!SD.exists(OutDir))SD.mkdir(OutDir);
  if(!SD.exists(InDir))SD.mkdir(InDir);
 }
 void scan(){
  selected_=count_=0;
  if(!sd_){status_="Insert FAT32 SD";return;}
  File directory=SD.open(OutDir,FILE_READ);
  if(!directory||!directory.isDirectory()){
   if(directory)directory.close();status_="Create /SUITE/OUTBOX";return;
  }
  for(int checked=0;checked<128&&count_<suite::lan23::MaxFiles;++checked){
   File file=directory.openNextFile();
   if(!file)break;
   if(!file.isDirectory()){
    String name=file.name();
    const int cut=name.lastIndexOf('/');
    if(cut>=0)name=name.substring(cut+1);
    if(name.length()<suite::safefiles17::MaxName &&
       suite::safefiles17::safeName(name.c_str()) &&
       file.size()>0&&file.size()<=suite::lan23::MaxFile){
      name.toCharArray(choices_[count_].name,sizeof(choices_[count_].name));
      choices_[count_].bytes=static_cast<uint32_t>(file.size());
      ++count_;
    }
   }
   file.close();
  }
  directory.close();
  status_=String(count_)+" outgoing files (max 256 KiB)";
  dirty_=true;
 }
 void load(){
  configured_=false;
  memset(lan_.key,0,sizeof(lan_.key));
  if(!sd_){status_="SD required";return;}
  File f=SD.open("/SUITE/NET/NET.CFG",FILE_READ);
  if(!f||f.isDirectory()||f.size()>1024){if(f)f.close();status_="NET.CFG missing";return;}
  char netText[1025]={};
  const size_t netSize=f.size();
  bool okay=f.readBytes(netText,netSize)==netSize;f.close();
  suite::net8::Config next;
  if(!okay||!suite::net8::parse(netText,netSize,next)){
   status_="NET.CFG invalid";return;
  }
  f=SD.open("/SUITE/NET/LAN.CFG",FILE_READ);
  if(!f||f.isDirectory()||f.size()>160){if(f)f.close();status_="LAN.CFG missing";return;}
  char lanText[161]={};
  const size_t len=f.size();
  okay=f.readBytes(lanText,len)==len;f.close();
  suite::lan23::Config secret;
  if(!okay||!suite::lan23::parseConfig(lanText,len,secret)){
   status_="LAN.CFG invalid";return;
  }
  network_=next;lan_=secret;configured_=true;
  status_="Local LAN transfer configured (OFF)";
 }
 static bool shaFile(File&file,uint8_t digest[32]){
  mbedtls_sha256_context context;
  mbedtls_sha256_init(&context);
  bool success=mbedtls_sha256_starts_ret(&context,0)==0;
  uint8_t data[512];
  if(success && !file.seek(0))success=false;
  while(success && file.available()){
   const int n=file.read(data,sizeof(data));
   if(n<=0||mbedtls_sha256_update_ret(&context,data,n)!=0){
    success=false;break;
   }
   yield();
  }
  if(success)success=mbedtls_sha256_finish_ret(&context,digest)==0;
  mbedtls_sha256_free(&context);
  return success&&file.seek(0);
 }
 bool sign(const char*data,size_t count,uint8_t out[32]){
  const mbedtls_md_info_t*meta=mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  return meta && mbedtls_md_hmac(meta,lan_.key,32,
           reinterpret_cast<const unsigned char*>(data),count,out)==0;
 }
 bool verifyHeader(){
  suite::lan23::Header value;
  if(!suite::lan23::parseHeader(header_,headerUsed_,value))return false;
  char text[220]={};
  if(!suite::lan23::signatureBody(value,challenge_.c_str(),text,sizeof(text)))
   return false;
  uint8_t hmac[32]={};
  if(!sign(text,strlen(text),hmac) ||
     !suite::lan23::constantTimeHex(value.mac,hmac))return false;
  const String target=String(InDir)+"/"+value.filename;
  if(SD.exists(Temp)||SD.exists(target))return false;
  incoming_=SD.open(Temp,FILE_WRITE);
  if(!incoming_)return false;
  mbedtls_sha256_init(&rxHash_);hashStarted_=true;
  if(mbedtls_sha256_starts_ret(&rxHash_,0)!=0)return false;
  expectation_=value;received_=0;
  return true;
 }
 void finishReceived(){
  uint8_t digest[32]={};
  const bool complete=hashStarted_&&
    mbedtls_sha256_finish_ret(&rxHash_,digest)==0&&
    suite::lan23::constantTimeHex(expectation_.digest,digest);
  freeHash();
  if(incoming_){incoming_.flush();incoming_.close();}
  if(!complete){endListen("Hash mismatch; UPLOAD.PART retained",true);return;}
  File verify=SD.open(Temp,FILE_READ);
  const bool fileSize=verify && !verify.isDirectory() &&
     verify.size()==expectation_.bytes;
  if(verify)verify.close();
  if(!fileSize){endListen("SD size mismatch; .PART retained",true);return;}
  const String destination=String(InDir)+"/"+expectation_.filename;
  if(SD.exists(destination)||!SD.rename(Temp,destination.c_str())){
   endListen("Cannot commit; .PART retained",true);return;
  }
  if(peer_)peer_.print(suite::lan23::Success);
  endListen(String("Received ")+expectation_.filename+" ("+
            expectation_.bytes+" bytes)",false);
 }
 void listen(){
  if(!configured_||WiFi.status()!=WL_CONNECTED){
   status_="Configure and connect Wi-Fi first";return;
  }
  if(stage_!=Stage::Off){status_="Already receiving/listening";return;}
  if(SD.exists(Temp)){
   status_="UPLOAD.PART exists: recover manually";return;
  }
  server_.begin();stage_=Stage::Listen;listenAt_=millis();
  status_=String("One-shot RX :")+suite::lan23::Port+" (35s)";
 }
 void pollReceive(){
  const uint32_t now=millis();
  if(stage_==Stage::Listen){
   if(now-listenAt_>ListenTime){endListen("RX listen timed out",false);return;}
   WiFiClient next=server_.available();
   if(!next)return;
   if(!privateIp(next.remoteIP())){
    next.stop();status_="Rejected non-private LAN peer";return;
   }
   peer_=next;
   uint8_t nonce[16]={};esp_fill_random(nonce,sizeof(nonce));
   char hex[33]={};suite::lan23::toHex(nonce,sizeof(nonce),hex);
   challenge_=String(suite::lan23::ChallengeMagic)+hex;
   if(peer_.print(challenge_+"\n")!=challenge_.length()+1){
    endListen("Challenge send failed",true);return;
   }
   activityAt_=now;headerUsed_=0;
   stage_=Stage::Header;status_="Client connected: authenticating";
   dirty_=true;
  }
  if(stage_==Stage::Off||stage_==Stage::Listen)return;
  if(WiFi.status()!=WL_CONNECTED || !peer_.connected()){
   endListen("RX connection lost; PART may remain",true);return;
  }
  if(now-activityAt_>IdleTime){
   endListen("RX idle timeout; PART may remain",true);return;
  }
  if(stage_==Stage::Header){
   int bytes=0;
   while(peer_.available()&&bytes++<512){
    const int c=peer_.read();
    if(c<0)break;
    if(headerUsed_+1>=sizeof(header_)){
     endListen("Invalid header / length",true);return;
    }
    header_[headerUsed_++]=static_cast<char>(c);
    activityAt_=millis();
    if(c=='\n'){
     header_[headerUsed_]=0;
     if(!verifyHeader()){endListen("Auth, file name, size or collision rejected",true);return;}
     stage_=Stage::Body;
     peer_.print(suite::lan23::Ready);
     status_=String("Receiving ")+expectation_.filename;dirty_=true;
     break;
    }
   }
  }
  if(stage_!=Stage::Body)return;
  uint8_t buffer[512]={};
  for(int packets=0;packets<8 && peer_.available();++packets){
   const size_t remaining=expectation_.bytes-received_;
   const size_t cap=remaining<sizeof(buffer)?remaining:sizeof(buffer);
   if(!cap)break;
   const int n=peer_.read(buffer,cap);
   if(n<=0)break;
   if(!incoming_||incoming_.write(buffer,static_cast<size_t>(n))!=
      static_cast<size_t>(n) ||
      mbedtls_sha256_update_ret(&rxHash_,buffer,n)!=0){
    endListen("SD write failed; PART retained",true);return;
   }
   received_+=static_cast<uint32_t>(n);
   activityAt_=millis();
   if(received_==expectation_.bytes){finishReceived();return;}
  }
  dirty_=true;
 }
 void transmit(){
  if(stage_!=Stage::Off){status_="Stop receiver first";return;}
  if(!configured_||WiFi.status()!=WL_CONNECTED||!count_){
   status_="Connect Wi-Fi / select OUTBOX file first";return;
  }
  IPAddress destination;
  if(!destination.fromString(network_.tcpHost)||!privateIp(destination)){
   status_="TCP_HOST must be private IPv4";return;
  }
  const Choice&choice=choices_[selected_];
  if(!suite::safefiles17::safeName(choice.name))return;
  File file=SD.open((String(OutDir)+"/"+choice.name).c_str(),FILE_READ);
  if(!file || file.isDirectory()||file.size()!=choice.bytes||
     file.size()==0||file.size()>suite::lan23::MaxFile){
   if(file)file.close();status_="OUTBOX file changed; rescan";return;
  }
  uint8_t sha[32]={};
  if(!shaFile(file,sha)){
   file.close();status_="Cannot hash OUTBOX file";return;
  }
  WiFiClient sender;
  sender.setTimeout(2000);
  if(!sender.connect(destination,suite::lan23::Port,2000)){
   file.close();status_="Receiver not listening";return;
  }
  const String challenge=sender.readStringUntil('\n');
  if(!suite::lan23::challenge(challenge.c_str())){
   sender.stop();file.close();status_="Receiver challenge invalid";return;
  }
  suite::lan23::Header h;
  strcpy(h.filename,choice.name);
  h.bytes=choice.bytes;
  suite::lan23::toHex(sha,sizeof(sha),h.digest);
  char signedBody[220]={};
  if(!suite::lan23::signatureBody(h,challenge.c_str(),
       signedBody,sizeof(signedBody))){
   sender.stop();file.close();status_="Invalid transfer metadata";return;
  }
  uint8_t mac[32]={};
  if(!sign(signedBody,strlen(signedBody),mac)){
   sender.stop();file.close();status_="HMAC generation failed";return;
  }
  suite::lan23::toHex(mac,32,h.mac);
  const String header=String(suite::lan23::Magic)+"|"+h.filename+"|"+
       h.bytes+"|"+h.digest+"|"+h.mac+"\n";
  if(sender.print(header)!=header.length() ||
      sender.readStringUntil('\n')!="K132READY1"){
   sender.stop();file.close();status_="Receiver refused transfer";return;
  }
  const uint32_t start=millis();
  uint8_t buf[512];uint32_t sent=0;
  bool okay=true;
  while(sent<h.bytes && millis()-start<30000){
   const size_t cap=(h.bytes-sent)<sizeof(buf)?h.bytes-sent:sizeof(buf);
   const int n=file.read(buf,cap);
   if(n<=0||sender.write(buf,static_cast<size_t>(n))!=static_cast<size_t>(n)){
    okay=false;break;
   }
   sent+=static_cast<uint32_t>(n);yield();
  }
  file.close();
  if(!okay||sent!=h.bytes)status_="TX interrupted; receiver retains PART";
  else if(sender.readStringUntil('\n')!="K132OK1")
   status_="TX finished but receiver rejected/failed";
  else status_=String("Sent ")+h.filename+" ("+h.bytes+" bytes)";
  sender.stop();dirty_=true;
 }
public:
 void begin(){
  sd_=false;configured_=wifiConnecting_=false;dirty_=true;
  stage_=Stage::Off;count_=selected_=0;hashStarted_=false;
  WiFi.persistent(false);WiFi.mode(WIFI_OFF);
  SPI.begin(40,39,14,12);
  sd_=SD.begin(12,SPI,10000000);
  ensureFolders();load();scan();
 }
 void exit(){
  if(stage_!=Stage::Off)endListen("RX cancelled; PART preserved",true);
  WiFi.disconnect(true);WiFi.mode(WIFI_OFF);
  memset(lan_.key,0,sizeof(lan_.key));
 }
 bool dirty()const{return dirty_;}
 void input(const Keyboard_Class::KeysState&keys){
  if(letter(keys,'x')){if(stage_!=Stage::Off)endListen("RX stopped; PART preserved",true);}
  else if(letter(keys,'c'))connectWifi();
  else if(letter(keys,'l'))listen();
  else if(letter(keys,'t'))transmit();
  else if(letter(keys,'r')){load();scan();}
  else if(keys.up&&selected_>0)--selected_;
  else if(keys.down&&selected_+1<count_)++selected_;
  dirty_=true;
 }
 void poll(){
  const uint32_t now=millis();
  if(wifiConnecting_){
   if(WiFi.status()==WL_CONNECTED){
    wifiConnecting_=false;status_=String("Wi-Fi ")+WiFi.localIP().toString();
    dirty_=true;
   }else if(now-wifiAt_>12000){
    wifiConnecting_=false;WiFi.disconnect(true);
    status_="Wi-Fi connection timed out";dirty_=true;
   }
  }
  if(stage_!=Stage::Off)pollReceive();
 }
 void render(){
  dirty_=false;
  auto&d=M5Cardputer.Display;const auto&p=suite::skin16::colors();
  d.fillScreen(p.bg);d.fillRect(0,0,240,22,p.panel);
  d.setTextColor(p.text,p.panel);d.setCursor(5,7);d.print("LAN FILE TRANSFER");
  d.setTextColor(p.accent,p.panel);d.setCursor(208,7);d.print("P23");
  d.setTextColor(p.muted,p.bg);d.setCursor(4,26);
  d.print(WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():
          "OFFLINE (C connects)");
  if(stage_==Stage::Body){
   d.setTextColor(p.good,p.bg);d.setCursor(5,43);
   d.print(String("Receiving ")+expectation_.filename);
   d.setCursor(5,57);d.print(String(received_)+"/"+expectation_.bytes+" bytes");
  }else{
   d.setTextColor(p.text,p.bg);d.setCursor(5,41);
   d.print(String("OUTBOX: ")+count_+" files; INBOX: RX");
   const int first=selected_>=5?selected_-4:0;
   for(int i=first;i<count_&&i<first+5;++i){
    const int y=54+(i-first)*11;
    if(i==selected_)d.fillRect(3,y-1,232,11,p.selected);
    d.setTextColor(p.text,i==selected_?p.selected:p.bg);
    d.setCursor(5,y);d.print(choices_[i].name);
    d.setCursor(182,y);d.print(choices_[i].bytes);
   }
  }
  d.setTextColor(p.muted,p.bg);d.setCursor(4,105);
  d.print(status_.substring(0,37));
  d.fillRect(0,117,240,18,p.panel);d.setTextColor(p.text,p.panel);
  d.setCursor(3,122);d.print("C WiFi L receive T send X stop R");
 }
private:
 void connectWifi(){
  if(!configured_){status_="NET.CFG and LAN.CFG required";return;}
  if(WiFi.status()==WL_CONNECTED){status_="Wi-Fi already connected";return;}
  WiFi.mode(WIFI_STA);WiFi.begin(network_.ssid,network_.password);
  wifiConnecting_=true;wifiAt_=millis();status_="Connecting Wi-Fi (12s max)";
 }
};
