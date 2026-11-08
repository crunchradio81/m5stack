#pragma once
// P16 original Cardputer K132 Theme Editor and Settings Profiles.
// One SD firmware with six screens; no PSRAM, network, Grove or credentials.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include "core/Theme16Runtime.h"
class Theme16Collection {
public:
 void begin(){
  menu_=-1;item_=0;target_=0;field_=0;
  dirty_=true;confirm_=false;name_[0]=0;
  SPI.begin(40,39,14,12);
  sd_=SD.begin(12,SPI,10000000);
  const bool read=suite::skin16::reload();
  working_=suite::skin16::current();
  target_=working_.activeIndex();
  if(!sd_)status_="Insert FAT32 microSD to save";
  else if(!read)status_="Corrupt profiles; recovery only";
  else status_=suite::skin16::validFile()?
    "Saved profiles loaded":"Default profiles; choose to save";
 }
 void exit(){}
 void poll(){}
 bool dirty()const{return dirty_;}
 void input(const Keyboard_Class::KeysState&k){
  if(menu_<0){
   if(k.up&&item_>0)--item_;
   if(k.down&&item_<5)++item_;
   if(k.enter)open(item_);
   dirty_=true;return;
  }
  if(k.ctrl&&letter(k,'b')){menu_=-1;dirty_=true;return;}
  if(menu_!=2&&(letter(k,'b')||k.esc)){
   menu_=-1;dirty_=true;return;
  }
  if(menu_==0){
   int next=working_.active().theme;
   if(k.left)--next;
   if(k.right)++next;
   if(next<0)next=static_cast<int>(suite::skin16::Theme::Count)-1;
   if(next>=static_cast<int>(suite::skin16::Theme::Count))next=0;
   working_.theme(static_cast<unsigned>(next));
   if(k.enter)commit();
  }else if(menu_==1){
   if(k.up&&target_>0)--target_;
   if(k.down&&target_<2)++target_;
   if(k.enter){
    working_.choose(target_);commit();
   }
  }else if(menu_==2){
   if(k.backspace){
    const size_t n=strlen(name_);if(n)name_[n-1]=0;
   }else if(k.enter){
    if(working_.rename(target_,name_))commit();
    else status_="1..16 letters/digits/spaces only; unique";
   }else if(!k.ctrl&&!k.fn&&!k.alt)
    for(char ch:k.word){
     if(!((ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||
          (ch>='0'&&ch<='9')||ch==' '||ch=='_'||ch=='-'))continue;
     const size_t n=strlen(name_);
     if(n+1<sizeof(name_)){name_[n]=ch;name_[n+1]=0;}
    }
  }else if(menu_==3){
   if(k.up||k.down)field_=1-field_;
   int delta=0;
   if(k.left)delta=-1;
   if(k.right)delta=1;
   if(delta){
    if(field_==0){
     int value=working_.active().brightness+delta;
     if(value<0)value=0;
     if(value>3)value=3;
     working_.brightness(static_cast<unsigned>(value));
    }else{
     int value=working_.active().volume+delta*5;
     if(value<0)value=0;
     if(value>100)value=100;
     working_.volume(static_cast<unsigned>(value));
    }
   }
   if(k.enter)commit();
  }else if(menu_==4){
   if(k.enter||k.left||k.right){
    working_.autosave(!working_.active().autosave);
    commit();
   }
  }else if(menu_==5){
   if(letter(k,'r')){
    confirm_=true;status_="Restore verified .BAK? Y/N";
   }else if(confirm_&&letter(k,'n')){
    confirm_=false;status_="Recovery cancelled";
   }else if(confirm_&&letter(k,'y')){
    confirm_=false;
    const bool ok=sd_&&suite::skin16::restoreBackup();
    if(ok){working_=suite::skin16::current();status_="Verified backup restored";}
    else status_="Recovery refused or unavailable";
   }
  }
  dirty_=true;
 }
 void render(){
  dirty_=false;auto&d=M5Cardputer.Display;
  const auto p=suite::skin16::palette(working_.active().theme);
  const uint16_t bg=p.bg,head=p.panel,white=p.text,cyan=p.accent,
    sel=p.selected,dim=p.muted,green=p.good;
  d.fillScreen(bg);d.fillRect(0,0,240,23,head);
  d.setTextColor(white,head);d.setCursor(5,7);
  d.print(menu_<0?"THEMES & PROFILES":labels(menu_));
  d.setTextColor(cyan,head);d.setCursor(210,7);d.print("P16");
  d.fillRect(0,116,240,19,head);
  d.setTextColor(dim,head);d.setCursor(4,121);
  d.print(menu_<0?"Arrows / Enter":
   menu_==0?"Left/right preview Enter save B back":
   menu_==1?"Up/down select Enter activate":
   menu_==2?"Type Enter rename Ctrl+B back":
   menu_==3?"Up/down field left/right Enter save":
   menu_==4?"Enter toggles note autosave":
   "R request verified backup, Y/N");
  if(menu_<0){
   for(int i=0;i<6;++i){
    const int y=28+i*14;
    if(i==item_)d.fillRect(3,y-1,233,12,sel);
    d.setTextColor(white,i==item_?sel:bg);
    d.setCursor(7,y+2);d.print(String(i+1)+". "+labels(i));
   }
  }else if(menu_==0){
   d.setTextColor(cyan,bg);d.setCursor(5,30);
   d.print(String(suite::skin16::themeName(working_.active().theme)));
   d.fillRect(6,52,227,40,p.panel);
   d.setTextColor(p.text,p.panel);d.setCursor(14,57);
   d.print("Live color preview");
   d.fillRoundRect(14,72,97,13,2,p.selected);
   d.setTextColor(p.text,p.selected);d.setCursor(18,75);
   d.print("Selected item");
   d.setTextColor(p.accent,p.panel);d.setCursor(125,75);
   d.print("Accent");
   d.fillRect(10,99,24,9,p.good);d.fillRect(39,99,24,9,p.warning);
   d.fillRect(68,99,24,9,p.accent);
  }else if(menu_==1){
   for(int i=0;i<3;++i){
    const int y=35+i*21;
    if(i==target_)d.fillRect(3,y-2,233,17,sel);
    d.setTextColor(white,i==target_?sel:bg);
    d.setCursor(8,y);
    d.print(String(i==static_cast<int>(working_.activeIndex())?"* ":"  ")+
      working_.at(i).name);
    d.setCursor(137,y);d.print(suite::skin16::themeName(working_.at(i).theme));
   }
  }else if(menu_==2){
   d.setTextColor(cyan,bg);d.setCursor(5,31);
   d.print(String("Profile ")+(target_+1));
   d.setTextColor(white,bg);d.setCursor(5,49);
   d.print(String("Current: ")+working_.at(target_).name);
   d.setTextColor(green,bg);d.setCursor(5,75);
   d.print(String("> ")+name_);
   d.setTextColor(dim,bg);d.setCursor(5,94);
   d.print("No passwords or network data");
  }else if(menu_==3){
   const uint8_t level=working_.active().brightness;
   d.setTextColor(cyan,bg);d.setCursor(5,31);
   d.print(String("Active: ")+working_.active().name);
   for(int i=0;i<2;++i){
    const int y=54+i*23;
    if(i==field_)d.fillRect(3,y-2,230,18,sel);
    d.setTextColor(white,i==field_?sel:bg);d.setCursor(9,y+2);
    if(i==0)d.print(String("Brightness: ")+(level+1)+"/4");
    else d.print(String("Volume preference: ")+working_.active().volume+"%");
   }
  }else if(menu_==4){
   d.setTextColor(cyan,bg);d.setCursor(5,39);
   d.print(String("Note autosave: ")+
     (working_.active().autosave?"ON":"OFF"));
   d.setTextColor(white,bg);d.setCursor(5,68);
   d.print("Applies to resident Notes app.");
  }else{
   d.setTextColor(white,bg);d.setCursor(5,32);
   d.print(suite::skin16::validFile()?
     "Current settings file verified.":"No verified current settings.");
   d.setCursor(5,51);d.print("Backup is not overwritten");
   d.setCursor(5,68);d.print("unless saving new settings.");
   d.setTextColor(cyan,bg);d.setCursor(5,93);
   d.print("R: restore only if invalid");
  }
  if(menu_>=0){
   d.setTextColor(dim,bg);d.setCursor(5,107);
   d.print(status_.substring(0,37));
  }
 }
private:
 static const char*labels(int i){
  static const char*const names[]={"Theme Editor","Switch Profile",
    "Rename Profile","Brightness & Volume","Note Autosave","Backup Recovery"};
  return names[i];
 }
 static bool letter(const Keyboard_Class::KeysState&k,char c){
  for(char v:k.word)if(v==c||v==(c-'a'+'A'))return true;
  return false;
 }
 void open(int index){
  menu_=index;
  working_=suite::skin16::current();
  target_=working_.activeIndex();
  field_=0;confirm_=false;name_[0]=0;dirty_=true;
  status_=sd_?"Enter saves your changes":"microSD required to save";
 }
 void commit(){
  if(!sd_){status_="No SD; changes not persisted";return;}
  if(!suite::skin16::writable()){
   status_="Invalid primary; use backup recovery";return;
  }
  if(!suite::skin16::save(working_)){
   status_="Save failed; original kept";return;
  }
  working_=suite::skin16::current();
  status_="Saved; applies to next launched app";
 }
 suite::skin16::Settings working_;
 bool sd_=false,dirty_=true,confirm_=false;
 int menu_=-1,item_=0,target_=0,field_=0;
 char name_[17]={};
 String status_;
};
