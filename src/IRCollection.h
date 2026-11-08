#pragma once
// Original K132 infrared TX is GPIO44. There is NO IR receiver.
// The user supplies approved NEC command profiles on microSD.
#define DISABLE_CODE_FOR_RECEIVER
#define SEND_PWM_BY_TIMER
#define IR_TX_PIN 44
#include <IRremote.hpp>
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include "core/P7Core.h"
class IRCollection {
public:
 void begin(){
   mode_=-1;menu_=0;selected_=0;dirty_=true;loaded_=false;
   SPI.begin(40,39,14,12);sd_=SD.begin(12,SPI,10000000);
   if(sd_){if(!SD.exists("/SUITE"))SD.mkdir("/SUITE");
            if(!SD.exists("/SUITE/IR"))SD.mkdir("/SUITE/IR");}
   IrSender.begin(DISABLE_LED_FEEDBACK);
   IrSender.setSendPin(IR_TX_PIN);
   refresh();
 }
 void exit(){pending_=false;}
 void poll(){
   if(!pending_||static_cast<int32_t>(millis()-at_)<0)return;
   if(mode_!=4){pending_=false;return;}
   if(phase_==0){send(profile_.power);phase_=1;at_=millis()+500;}
   else if(phase_==1){send(profile_.play);phase_=2;at_=millis()+500;}
   else {send(profile_.mute);pending_=false;status_="Three-step macro sent";}
   dirty_=true;
 }
 bool dirty()const{return dirty_;}
 void input(const Keyboard_Class::KeysState&k){
   if(mode_<0){
     if(k.up&&menu_>0)--menu_;
     if(k.down&&menu_<4)++menu_;
     if(k.enter){mode_=menu_;dirty_=true;if(mode_==1)refresh();}
     return;
   }
   if(letter(k,'b')){mode_=-1;pending_=false;dirty_=true;return;}
   if(mode_==1){
     if(k.up&&selected_>0)--selected_;
     if(k.down&&selected_+1<count_)++selected_;
     if(k.enter)load();
     if(letter(k,'r'))refresh();
   }else if(mode_==4){
     if(letter(k,'s')){pending_=false;status_="Canceled";}
     if(k.enter && loaded_){
       phase_=0;pending_=true;at_=millis()+1000;
       status_="Macro starts in 1 second";
     }
   }else if(loaded_){
     if(mode_==3&&k.enter)send(profile_.play);
     else if(letter(k,'p'))send(profile_.power);
     else if(letter(k,'u')||k.up)send(profile_.up);
     else if(letter(k,'d')||k.down)send(profile_.down);
     else if(letter(k,'m'))send(profile_.mute);
     else if(letter(k,'l')||k.enter)send(profile_.play);
   } else status_="Load an .IRP profile first";
   dirty_=true;
 }
 void render(){
   dirty_=false;auto&d=M5Cardputer.Display;
   constexpr uint16_t bg=0x0841,head=0x10c4,white=0xffff,cyan=0x05ff,
      selected=0x2673,muted=0xad55;
   d.fillScreen(bg);d.fillRect(0,0,240,23,head);
   d.setTextColor(white,head);d.setCursor(5,7);
   d.print(mode_<0?"IR TRANSMITTER":names(mode_));
   d.setTextColor(cyan,head);d.setCursor(211,7);d.print("IR");
   d.fillRect(0,116,240,19,head);
   d.setTextColor(muted,head);d.setCursor(5,121);
   d.print(mode_<0?"Arrows / Enter":mode_==1?"Enter load | R rescan | B menu":
           mode_==4?"Enter macro | S cancel | B":
           "P power U/D volume M mute L play");
   if(mode_<0){
     for(int i=0;i<5;++i){
       const int y=28+i*17;
       if(menu_==i)d.fillRect(3,y-1,233,15,selected);
       d.setTextColor(white,menu_==i?selected:bg);
       d.setCursor(7,y+2);d.print(String(i+1)+". "+names(i));
     }
   }else if(mode_==1){
     d.setTextColor(cyan,bg);d.setCursor(5,27);
     d.print(String(count_)+" NEC profiles");
     for(int i=0;i<count_&&i<6;++i){
       const int y=40+i*11;
       if(i==selected_)d.fillRect(3,y-1,233,11,selected);
       d.setTextColor(white,i==selected_?selected:bg);
       d.setCursor(5,y);d.print(files_[i]);
     }
   }else{
     d.setTextColor(cyan,bg);d.setCursor(5,29);
     d.print(loaded_?String("Profile: ")+profile_.name:"NO PROFILE LOADED");
     d.setTextColor(white,bg);d.setCursor(5,48);
     if(mode_==0)d.print("Send owner-approved NEC codes");
     if(mode_==2)d.print("Play / mute / volume codes");
     if(mode_==3)d.print("User-profile shutter code");
     if(mode_==4)d.print("POWER, PLAY, MUTE with delays");
     d.setCursor(5,67);d.print("Built-in TX: GPIO44 / 38kHz");
     d.setCursor(5,81);d.print("No infrared reception/learning");
     d.setCursor(5,94);d.print("No universal code database");
   }
   d.setTextColor(muted,bg);d.setCursor(5,108);
   d.print(status_.substring(0,38));
 }
private:
 static const char*names(int mode){
   static const char* const list[]={"NEC Remote","Device Profiles","Media Controller","Camera Shutter","Manual IR Macro"};
   return list[mode];
 }
 int mode_=-1,menu_=0,selected_=0,count_=0,phase_=0;
 bool sd_=false,dirty_=true,loaded_=false,pending_=false;
 uint32_t at_=0;
 char files_[8][32]={};
 suite::field::NecProfile profile_;
 String status_;
 static bool letter(const Keyboard_Class::KeysState&k,char v){
   for(char c:k.word)if(c==v||c==(v-'a'+'A'))return true;
   return false;
 }
 void send(uint8_t command){
   if(!loaded_){status_="Load a profile";return;}
   IrSender.sendNEC(profile_.address,command,0);
   status_=String("Sent NEC command 0x")+String(command,HEX);
 }
 void refresh(){
   count_=selected_=0;
   if(!sd_){status_="Insert SD for IR profiles";return;}
   File dir=SD.open("/SUITE/IR");
   if(!dir||!dir.isDirectory()){if(dir)dir.close();status_="Missing IR directory";return;}
   while(count_<8){
     File f=dir.openNextFile();if(!f)break;
     String name=f.name();const int slash=name.lastIndexOf('/');
     if(slash>=0)name=name.substring(slash+1);
     if(!f.isDirectory()&&f.size()<120&&name.endsWith(".IRP")&&
        name.length()<32&&suite::field::safeFile(name.c_str()))
       strncpy(files_[count_++],name.c_str(),31);
     f.close();
   }
   dir.close();status_=count_?"Enter to load profile":"Copy owner .IRP files to /SUITE/IR";
 }
 void load(){
   if(selected_>=count_)return;
   const String path=String("/SUITE/IR/")+files_[selected_];
   File f=SD.open(path.c_str(),FILE_READ);
   if(!f||f.isDirectory()||f.size()>120){if(f)f.close();status_="Profile open failed";return;}
   char buf[121]={};const size_t n=f.size();
   const bool ok=f.readBytes(buf,n)==n;f.close();
   suite::field::NecProfile next;
   if(!ok||!suite::field::parseIrProfile(buf,n,next)){
     status_="Invalid .IRP profile";return;
   }
   profile_=next;loaded_=true;
   status_=String("Loaded ")+profile_.name;mode_=0;
 }
};
