#pragma once
// P17 original K132 restricted, read-mostly SD file manager. USER files only.
// Never touches the SD root, bootloader, OTA binaries, Wi-Fi keys or system data.
// No deletion/move/rename: user-confirmed copies with verified source/dest bytes.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include "core/SafeFiles17Core.h"
#include "core/Theme16Runtime.h"

class SafeFiles17Collection {
public:
 void begin(){
  mode_=-1;menu_=selected_=0;confirm_=false;dirty_=true;
  SPI.begin(40,39,14,12);
  sd_=SD.begin(12,SPI,10000000);
  if(sd_){
   if(!SD.exists("/SUITE"))SD.mkdir("/SUITE");
   if(!SD.exists("/SUITE/USER"))SD.mkdir("/SUITE/USER");
   if(!SD.exists("/SUITE/BACKUP"))SD.mkdir("/SUITE/BACKUP");
   if(!SD.exists("/SUITE/BACKUP/USER"))
    SD.mkdir("/SUITE/BACKUP/USER");
  }
  refresh();
  status_=sd_?"Safe USER data only, no delete":"Insert FAT32 SD";
 }
 void exit(){}
 void poll(){}
 bool dirty()const{return dirty_;}
 void input(const Keyboard_Class::KeysState&k){
  if(confirm_){
   confirm_=false;
   if(letter(k,'y')){
    if(mode_==2)copySelected();
    else if(mode_==3)restoreSelected();
   }else status_="Cancelled, no changes";
   dirty_=true;return;
  }
  if(mode_<0){
   if(k.up&&menu_>0)--menu_;
   if(k.down&&menu_<4)++menu_;
   if(k.enter)open(menu_);
   dirty_=true;return;
  }
  if(letter(k,'b')||k.esc){mode_=-1;dirty_=true;return;}
  if(letter(k,'r')){refresh();dirty_=true;return;}
  const auto&listing=mode_==1||mode_==3?backups_:users_;
  if(k.up&&selected_>0)--selected_;
  if(k.down&&selected_+1<static_cast<int>(listing.count()))++selected_;
  if(k.enter&&selected_<static_cast<int>(listing.count())){
   if(mode_==2||mode_==3){
    confirm_=true;
    status_=mode_==2?"Copy into backup? Y/N":
                       "Restore as separate R#-file? Y/N";
   }else if(mode_==4)verifySelected();
   else{
    const auto&f=listing.at(selected_);
    status_=String(f.name)+": "+f.size+" bytes";
   }
  }
  dirty_=true;
 }
 void render(){
  dirty_=false;auto&d=M5Cardputer.Display;
  const auto&p=suite::skin16::colors();
  const uint16_t bg=p.bg,head=p.panel,white=p.text,
    accent=p.accent,sel=p.selected,dim=p.muted;
  d.fillScreen(bg);d.fillRect(0,0,240,23,head);
  d.setTextColor(white,head);d.setCursor(4,7);
  d.print(mode_<0?"SAFE SD FILE LIBRARY":names(mode_));
  d.setTextColor(accent,head);d.setCursor(209,7);d.print("F17");
  d.fillRect(0,116,240,19,head);
  d.setTextColor(dim,head);d.setCursor(4,121);
  d.print(mode_<0?"Fn arrows / Enter":
    confirm_?"Y confirm / any other cancels":
    mode_==0||mode_==1?"Enter inspect R refresh B menu":
    mode_==2?"Enter copy verified to backup":
    mode_==3?"Enter restore without overwrite":
    "Enter compare source and backup");
  if(mode_<0){
   for(int i=0;i<5;++i){
    const int y=29+i*16;
    if(i==menu_)d.fillRect(3,y-1,233,13,sel);
    d.setTextColor(white,i==menu_?sel:bg);
    d.setCursor(7,y+2);
    d.print(String(i+1)+". "+names(i));
   }
  }else{
   d.setTextColor(accent,bg);d.setCursor(5,28);
   const auto&listing=mode_==1||mode_==3?backups_:users_;
   d.print(mode_==1||mode_==3?"/SUITE/BACKUP/USER":"/SUITE/USER");
   d.setTextColor(dim,bg);d.setCursor(5,39);
   d.print(String(listing.count())+
          " files, <=256KiB each, no delete");
   const int first=selected_>=5?selected_-4:0;
   for(int i=first;i<static_cast<int>(listing.count())&&i<first+5;++i){
    const int y=53+(i-first)*11;
    if(i==selected_)d.fillRect(3,y-1,233,11,sel);
    const auto&f=listing.at(i);
    d.setTextColor(white,i==selected_?sel:bg);d.setCursor(6,y);
    d.print(String(f.name).substring(0,28)+" "+f.size);
   }
  }
  d.setTextColor(dim,bg);d.setCursor(5,108);
  d.print(status_.substring(0,38));
 }
private:
 static constexpr const char*UserDir="/SUITE/USER";
 static constexpr const char*BackupDir="/SUITE/BACKUP/USER";
 static const char*names(int mode){
  static const char*const names[]={
   "Browse USER Files","Browse Backups","Verified Backup",
   "Safe Restore Copy","Compare Source and Backup"
  };
  return names[mode];
 }
 static bool letter(const Keyboard_Class::KeysState&k,char c){
  for(char value:k.word)if(value==c||value==(c-'a'+'A'))return true;
  return false;
 }
 static String fullPath(const char*dir,const char*filename){
  return String(dir)+"/"+filename;
 }
 bool sd_=false,confirm_=false,dirty_=true;
 int mode_=-1,menu_=0,selected_=0;
 suite::safefiles17::Index users_,backups_;
 String status_;
 void open(int mode){
  mode_=mode;selected_=0;confirm_=false;
  refresh();dirty_=true;
 }
 void scan(const char*path,suite::safefiles17::Index&idx){
  idx.clear();if(!sd_)return;
  File dir=SD.open(path,FILE_READ);
  if(!dir||!dir.isDirectory()){if(dir)dir.close();return;}
  unsigned scanned=0;
  while(scanned<128&&idx.count()<suite::safefiles17::MaxItems){
   File f=dir.openNextFile();if(!f)break;
   ++scanned;
   String n=f.name();
   const int slash=n.lastIndexOf('/');
   if(slash>=0)n=n.substring(slash+1);
   if(!f.isDirectory()&&suite::safefiles17::safeName(n.c_str())&&
      f.size()<=suite::safefiles17::MaxFileSize)
    idx.add(n.c_str(),static_cast<uint32_t>(f.size()));
   f.close();
  }
  dir.close();
 }
 void refresh(){
  users_.clear();backups_.clear();
  if(!sd_){status_="SD offline";return;}
  scan(UserDir,users_);scan(BackupDir,backups_);
  status_=String(users_.count())+" user / "+backups_.count()+" backups";
 }
 bool compare(const String&src,const String&dst,uint32_t&size){
  size=0;
  File a=SD.open(src.c_str(),FILE_READ);
  File b=SD.open(dst.c_str(),FILE_READ);
  if(!a||!b||a.isDirectory()||b.isDirectory()||
     a.size()!=b.size()||a.size()>suite::safefiles17::MaxFileSize){
   if(a)a.close();if(b)b.close();return false;
  }
  size=static_cast<uint32_t>(a.size());
  uint8_t aa[512]={},bb[512]={};
  uint32_t remaining=size;
  bool match=true;
  while(remaining){
   const size_t n=remaining>sizeof(aa)?sizeof(aa):remaining;
   if(a.read(aa,n)!=static_cast<int>(n)||
      b.read(bb,n)!=static_cast<int>(n)||
      !suite::safefiles17::equalBlock(aa,bb,n)){
    match=false;break;
   }
   remaining-=static_cast<uint32_t>(n);
   yield();
  }
  a.close();b.close();return match&&remaining==0;
 }
 bool verifiedCopy(const String&src,const String&dst){
  if(!sd_||SD.exists(dst.c_str())){
   status_="Target exists or SD offline; refused";return false;
  }
  const String temporary=dst+".NEW";
  if(SD.exists(temporary.c_str())){
   status_="Previous .NEW present; repair on PC";return false;
  }
  File source=SD.open(src.c_str(),FILE_READ);
  if(!source||source.isDirectory()||
     source.size()>suite::safefiles17::MaxFileSize){
   if(source)source.close();
   status_="Unsafe/oversize/unreadable source";return false;
  }
  const uint32_t length=source.size();
  File output=SD.open(temporary.c_str(),FILE_WRITE);
  if(!output){source.close();status_="Cannot create .NEW";return false;}
  uint8_t block[512]={};
  uint32_t remaining=length;
  bool wrote=true;
  while(remaining){
   const size_t n=remaining>sizeof(block)?sizeof(block):remaining;
   if(source.read(block,n)!=static_cast<int>(n)||
      output.write(block,n)!=n){wrote=false;break;}
   remaining-=static_cast<uint32_t>(n);
   yield();
  }
  output.flush();
  const bool complete=output.size()==length;
  output.close();source.close();
  if(!wrote||remaining||!complete){
   status_="Copy short write; .NEW preserved";return false;
  }
  uint32_t verified=0;
  if(!compare(src,temporary,verified)||verified!=length){
   status_="Source/temp mismatch; .NEW preserved";return false;
  }
  if(SD.exists(dst.c_str())||!SD.rename(temporary.c_str(),dst.c_str())){
   status_="Destination appeared/rename failed";return false;
  }
  verified=0;
  if(!compare(src,dst,verified)||verified!=length){
   status_="Final copy verification failed";return false;
  }
  status_=String("Verified exact ")+length+"-byte copy";
  return true;
 }
 void copySelected(){
  if(selected_>=static_cast<int>(users_.count()))return;
  const char*n=users_.at(selected_).name;
  if(!suite::safefiles17::safeName(n))return;
  const bool ok=verifiedCopy(fullPath(UserDir,n),fullPath(BackupDir,n));
  if(ok){refresh();status_="Backup verified; source unchanged";}
 }
 void restoreSelected(){
  if(selected_>=static_cast<int>(backups_.count()))return;
  const char*n=backups_.at(selected_).name;
  if(!suite::safefiles17::safeName(n))return;
  char restored[suite::safefiles17::MaxName]={};
  for(unsigned i=1;i<=9;++i){
   if(!suite::safefiles17::restored(n,i,restored,sizeof(restored))){
    status_="Cannot form safe restore filename";return;
   }
   const String target=fullPath(UserDir,restored);
   if(!SD.exists(target.c_str())){
    if(verifiedCopy(fullPath(BackupDir,n),target)){
     refresh();status_="Verified restored copy; original kept";
    }
    return;
   }
  }
  status_="Nine restored versions exist; refused";
 }
 void verifySelected(){
  if(selected_>=static_cast<int>(users_.count()))return;
  const char*n=users_.at(selected_).name;
  uint32_t bytes=0;
  if(compare(fullPath(UserDir,n),fullPath(BackupDir,n),bytes))
   status_=String("MATCH: ")+bytes+" exact bytes";
  else status_="Missing backup or byte mismatch";
 }
};
