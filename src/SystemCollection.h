#pragma once
// Seven diagnostic apps for the original K132: battery, heap, firmware,
// SD information, read benchmark, SHA256 digest and uptime. No destructive I/O.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_timer.h>
#include <mbedtls/sha256.h>
#include "core/SystemCore.h"
class SystemCollection {
public:
 void begin() {
   mode_=-1;menu_=0;dirty_=true;selected_=0;
   SPI.begin(40,39,14,12);
   sd_=SD.begin(12,SPI,10000000);
   status_=sd_?"Ready":"SD unavailable";
   lastSample_=0;trend_.clear();
 }
 void exit(){}
 void poll(){
   if(mode_!=0)return;
   const uint32_t now=millis();
   if(static_cast<uint32_t>(now-lastSample_)>=3000u||lastSample_==0){
     lastSample_=now;
     int milli=M5Cardputer.Power.getBatteryVoltage();
     if(trend_.sample(milli))status_="Voltage sample captured";
     else status_="Battery voltage unavailable";
     dirty_=true;
   }
 }
 bool dirty()const{return dirty_;}
 void input(const Keyboard_Class::KeysState& k){
   if(mode_<0){
     if(k.up&&menu_>0)--menu_;
     if(k.down&&menu_<6)++menu_;
     if(k.enter){mode_=menu_;status_="";dirty_=true;if(mode_==5)refreshFiles();}
     dirty_=true;return;
   }
   if(letter(k,'b')){mode_=-1;dirty_=true;return;}
   if(mode_==0&&letter(k,'s')){lastSample_=0;poll();}
   if(mode_==4&&letter(k,'s'))benchmark();
   if(mode_==5){
     if(k.up&&selected_>0)--selected_;
     if(k.down&&selected_+1<files_)++selected_;
     if(letter(k,'r'))refreshFiles();
     if(k.enter)digest();
   }
   dirty_=true;
 }
 void render(){
   dirty_=false;
   auto& d=M5Cardputer.Display;
   constexpr uint16_t bg=0x0841,head=0x10c4,white=0xffff,
     cyan=0x05ff,green=0x5fe9,sel=0x2673,dim=0xad55;
   d.fillScreen(bg);d.fillRect(0,0,240,23,head);
   d.setTextColor(white,head);d.setCursor(5,7);
   d.print(mode_<0?"SYSTEM TOOLBOX":label(mode_));
   d.setTextColor(cyan,head);d.setCursor(209,7);d.print("SD");
   d.fillRect(0,116,240,19,head);
   d.setTextColor(dim,head);d.setCursor(4,121);
   d.print(mode_<0?"Fn arrows / Enter":
     mode_==4?"S read test | B menu":
     mode_==5?"Enter hash | R list | B menu":
     "B menu | S refresh battery");
   if(mode_<0){
     for(int i=0;i<7;++i){
       const int y=27+i*12;
       if(i==menu_)d.fillRect(3,y-1,233,12,sel);
       d.setTextColor(white,i==menu_?sel:bg);
       d.setCursor(6,y);d.print(String(i+1)+". "+label(i));
     }
     d.setTextColor(dim,bg);d.setCursor(5,111);d.print(status_.substring(0,38));
     return;
   }
   auto line=[&d,bg](int y,const String& s,uint16_t col=0xffff){
     d.setTextColor(col,bg);d.setCursor(6,y);
     d.print(s.substring(0,38));
   };
   if(mode_==0){
     const int pct=M5Cardputer.Power.getBatteryLevel();
     const int voltage=M5Cardputer.Power.getBatteryVoltage();
     line(29,String("Estimated battery: ")+pct+"%",green);
     line(44,String("Voltage: ")+voltage+" mV",cyan);
     line(59,String("Average: ")+trend_.average()+" mV");
     line(74,String("Trend samples: ")+trend_.count());
     line(89,String("Delta: ")+trend_.trend()+" mV");
     line(103,"Charge current/status unsupported",dim);
   } else if(mode_==1){
     line(29,String("Free heap: ")+ESP.getFreeHeap(),green);
     line(45,String("Min free heap: ")+ESP.getMinFreeHeap());
     line(61,String("Largest block: ")+ESP.getMaxAllocHeap());
     line(77,String("Total heap: ")+ESP.getHeapSize());
     line(93,String("CPU: ")+ESP.getCpuFreqMHz()+" MHz",cyan);
   } else if(mode_==2){
     const auto* partition=esp_ota_get_running_partition();
     line(29,String("MCU: ")+ESP.getChipModel(),cyan);
     line(45,String("SDK: ")+ESP.getSdkVersion());
     line(61,String("Flash: ")+ESP.getFlashChipSize()/1024+" KiB");
     line(77,String("Sketch: ")+ESP.getSketchSize()+" B");
     if(partition) {
       line(93,String("Partition: ")+partition->label);
       line(106,String("Offset 0x")+String(partition->address,HEX));
     }
   } else if(mode_==3){
     if(!sd_){line(29,"No FAT32 SD mounted");return;}
     line(29,String("Card: ")+String(static_cast<unsigned long>(SD.cardSize()/1024/1024))+" MiB",cyan);
     line(45,String("Filesystem total: ")+String(static_cast<unsigned long>(SD.totalBytes()/1024/1024))+" MiB");
     line(61,String("Filesystem used: ")+String(static_cast<unsigned long>(SD.usedBytes()/1024/1024))+" MiB");
     line(77,"Application library: /SUITE/APPS");
     line(93,"No formatting or writes.",dim);
   } else if(mode_==4){
     line(29,"Read-only test of packaged app",cyan);
     line(45,"Source: /SUITE/APPS/*.BIN");
     line(61,String("Bytes read: ")+String(static_cast<unsigned long>(bench_.bytes())));
     line(77,String("Time: ")+bench_.microseconds()+" us");
     line(93,String("Rate: ")+bench_.kibPerSecond()+" KiB/s",green);
     line(106,status_,dim);
   } else if(mode_==5){
     line(26,"SHA256 of selected SD app",cyan);
     for(int i=0;i<files_&&i<4;++i){
       const int y=39+i*11;
       if(i==selected_)d.fillRect(3,y-1,233,11,sel);
       d.setTextColor(white,i==selected_?sel:bg);
       d.setCursor(5,y);d.print(names_[i]);
     }
     line(87,digest_.substring(0,32),green);
     line(98,digest_.substring(32,64),green);
     line(109,status_,dim);
   } else {
     unsigned days,hours,minutes,seconds;
     suite::system::uptime(static_cast<uint64_t>(esp_timer_get_time()/1000000),
                           days,hours,minutes,seconds);
     line(29,String("Uptime days: ")+days,green);
     line(45,String("Hours: ")+hours);
     line(61,String("Minutes: ")+minutes);
     line(77,String("Seconds: ")+seconds);
     line(96,"This is NOT a wall clock.",dim);
   }
 }
private:
 static const char* label(int i){
   static const char* const items[]={
     "Battery Monitor","Heap & CPU","Firmware Info","SD Card Info",
     "SD Read Benchmark","SHA-256 File Digest","Uptime Clock"
   };return items[i];
 }
 int mode_=-1,menu_=0,selected_=0,files_=0;
 bool dirty_=true,sd_=false;
 uint32_t lastSample_=0;
 suite::system::BatteryTrend trend_;
 suite::system::ReadBenchmark bench_;
 char names_[4][36]={};
 String status_,digest_;
 static bool letter(const Keyboard_Class::KeysState& k,char c){
   for(char v:k.word)if(v==c||v==(c-'a'+'A'))return true;
   return false;
 }
 String appPath(int index)const{
   return String("/SUITE/APPS/")+names_[index];
 }
 void refreshFiles(){
   files_=selected_=0;digest_="";
   if(!sd_){status_="SD unavailable";return;}
   File dir=SD.open("/SUITE/APPS");
   if(!dir||!dir.isDirectory()){if(dir)dir.close();status_="No app library";return;}
   while(files_<4){
     File f=dir.openNextFile();if(!f)break;
     String name=f.name();const int slash=name.lastIndexOf('/');
     if(slash>=0)name=name.substring(slash+1);
     if(!f.isDirectory()&&name.endsWith(".BIN")&&
        name.length()<36&&name.length()>4){
       strncpy(names_[files_++],name.c_str(),35);
     }
     f.close();
   }
   dir.close();status_=files_?"Choose file to hash":"No .BIN files";
 }
 void benchmark(){
   bench_.reset();
   if(!sd_){status_="Insert SD";return;}
   // Use an app image as a read-only reference. Never create or overwrite a file.
   File file;
   for(int i=0;i<files_;++i){
     file=SD.open(appPath(i).c_str(),FILE_READ);
     if(file)break;
   }
   if(!file) {
     refreshFiles();
     for(int i=0;i<files_;++i){
       file=SD.open(appPath(i).c_str(),FILE_READ);
       if(file)break;
     }
   }
   if(!file){status_="No app binary for benchmark";return;}
   static uint8_t buffer[1024]={};
   uint64_t count=0;
   const uint32_t beginUs=micros();
   while(file.available()&&count<1024u*1024u){
     const size_t n=file.read(buffer,sizeof(buffer));
     if(n==0)break;
     count+=n;yield();
   }
   const uint32_t endUs=micros();
   file.close();
   status_=bench_.complete(count,beginUs,endUs)?"Read benchmark complete":"Read benchmark failed";
 }
 void digest(){
   digest_="";
   if(!sd_||files_==0||selected_>=files_)return;
   File f=SD.open(appPath(selected_).c_str(),FILE_READ);
   if(!f||f.isDirectory()){if(f)f.close();status_="Open failed";return;}
   mbedtls_sha256_context hash;mbedtls_sha256_init(&hash);
   int err=mbedtls_sha256_starts_ret(&hash,0);
   static uint8_t bytes[1024]={};
   while(!err&&f.available()){
     const size_t n=f.read(bytes,sizeof(bytes));
     if(!n){err=-1;break;}
     err=mbedtls_sha256_update_ret(&hash,bytes,n);yield();
   }
   uint8_t raw[32]={};
   if(!err)err=mbedtls_sha256_finish_ret(&hash,raw);
   mbedtls_sha256_free(&hash);f.close();
   if(err){status_="Read/hash failed";return;}
   static const char digits[]="0123456789abcdef";
   char display[65]={};
   for(int i=0;i<32;++i){
     display[2*i]=digits[raw[i]>>4];
     display[2*i+1]=digits[raw[i]&15];
   }
   digest_=display;status_="SHA256 calculated";
 }
};
