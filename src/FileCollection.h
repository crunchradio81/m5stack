#pragma once
// Five SD-only file tools: restricted USER browser, backup, restore,
// filename search, and inventory. Never expose /SUITE/APPS or system dirs.
// Destructive delete/rename has explicit confirmation. No silent overwrites.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include "core/P7Core.h"
class FileCollection {
public:
 void begin(){
   mode_=-1;menu_=0;selection_=0;dirty_=true;entryMode_=0;confirmed_=false;
   SPI.begin(40,39,14,12);sd_=SD.begin(12,SPI,10000000);
   if(sd_){
     if(!SD.exists("/SUITE"))SD.mkdir("/SUITE");
     if(!SD.exists("/SUITE/USER"))SD.mkdir("/SUITE/USER");
     if(!SD.exists("/SUITE/BACKUP"))SD.mkdir("/SUITE/BACKUP");
   }
   status_=sd_?"Files restricted to /SUITE/USER":"Insert FAT32 SD";
 }
 void exit(){}
 void poll(){}
 bool dirty()const{return dirty_;}
 void input(const Keyboard_Class::KeysState&k){
   if(confirmed_){
     if(letter(k,'y'))act();
     else status_="Operation canceled";
     confirmed_=false;dirty_=true;return;
   }
   if(entryMode_) {
     if(k.backspace){size_t n=strlen(entry_);if(n)entry_[n-1]=0;}
     else if(k.enter){
       if(!suite::field::safeFile(entry_)){status_="Invalid file name";return;}
       if(entryMode_==1){action_='M';confirmed_=true;status_="Confirm rename? Y/N";}
       else if(entryMode_==2){action_='N';confirmed_=true;status_="Create empty text file? Y/N";}
       entryMode_=0;
     }else if(!k.fn&&!k.alt&&!k.ctrl)for(char c:k.word){
       if(c<32||c>126)continue;
       const size_t n=strlen(entry_);
       if(n+1<sizeof(entry_)){entry_[n]=c;entry_[n+1]=0;}
     }
     dirty_=true;return;
   }
   if(mode_<0) {
     if(k.up&&menu_>0)--menu_;
     if(k.down&&menu_<4)++menu_;
     if(k.enter){mode_=menu_;refresh();dirty_=true;}
     return;
   }
   if(letter(k,'b')){mode_=-1;dirty_=true;return;}
   if(mode_==3){
     if(k.backspace) {size_t n=strlen(query_);if(n)query_[n-1]=0;}
     else if(!k.fn&&!k.alt&&!k.ctrl)for(char c:k.word) {
       if(c<32||c>126)continue;
       const size_t n=strlen(query_);
       if(n+1<sizeof(query_)){query_[n]=c;query_[n+1]=0;}
     }
     if(letter(k,'r'))refresh();
     dirty_=true;return;
   }
   if(mode_==4){if(letter(k,'r'))inventory();dirty_=true;return;}
   if(k.up&&selection_>0)--selection_;
   if(k.down&&selection_+1<count_)++selection_;
   if(letter(k,'r'))refresh();
   if(mode_==0){
     if(letter(k,'m')&&count_) {
       entryMode_=1;entry_[0]=0;status_="Type new name, Enter";
     }
     if(letter(k,'n')) {
       entryMode_=2;entry_[0]=0;status_="Type new .TXT name";
     }
     if(letter(k,'d')&&count_){action_='D';confirmed_=true;status_="Permanently delete file? Y/N";}
     if(letter(k,'v')&&count_)preview();
   }
   if(mode_==1&&k.enter&&count_){action_='C';confirmed_=true;status_="Copy to BACKUP? Y/N";}
   if(mode_==2&&k.enter&&count_){action_='R';confirmed_=true;status_="Restore to USER? Y/N";}
   dirty_=true;
 }
 void render(){
   dirty_=false;auto&d=M5Cardputer.Display;
   constexpr uint16_t bg=0x0841,head=0x10c4,white=0xffff,
      cyan=0x05ff,sel=0x2673,dim=0xad55,green=0x5fe9;
   d.fillScreen(bg);d.fillRect(0,0,240,23,head);
   d.setTextColor(white,head);d.setCursor(5,7);
   d.print(mode_<0?"SD FILE TOOLS":names(mode_));
   d.setTextColor(cyan,head);d.setCursor(209,7);d.print("SD");
   d.fillRect(0,116,240,19,head);
   d.setTextColor(dim,head);d.setCursor(4,121);
   d.print(mode_<0?"Fn arrows / Enter":entryMode_?"Type name | Enter finish":
     confirmed_?"Y confirm / N cancel":
     mode_==0?"M rename D delete V view N new":
     mode_==1||mode_==2?"Enter copy | R rescan | B":
     mode_==3?"Type name substring | B menu":"R refresh | B menu");
   if(mode_<0){
     for(int i=0;i<5;++i){
       const int y=29+i*17;
       if(i==menu_)d.fillRect(3,y-1,233,14,sel);
       d.setTextColor(white,i==menu_?sel:bg);
       d.setCursor(7,y+2);d.print(String(i+1)+". "+names(i));
     }
   } else if(mode_==4){
     d.setTextColor(cyan,bg);d.setCursor(5,30);
     d.print("Restricted /SUITE/USER only");
     d.setTextColor(white,bg);d.setCursor(5,49);
     d.print(String("Files: ")+count_);
     d.setCursor(5,68);d.print(String("Bytes: ")+static_cast<unsigned long>(total_));
     d.setCursor(5,87);d.print("No destructive operations");
   } else {
     d.setTextColor(cyan,bg);d.setCursor(5,26);
     d.print(mode_==2?"/SUITE/BACKUP":"/SUITE/USER");
     if(mode_==3){
       d.setTextColor(green,bg);d.setCursor(5,39);
       d.print(String("Name contains: ")+query_);
     }
     const int start=selection_>=5?selection_-4:0;
     int row=0;
     for(int i=start;i<count_&&row<5;++i) {
       if(mode_==3&&query_[0]&&!matches(files_[i],query_))continue;
       const int y=mode_==3?54+row*11:41+row*13;
       if(selection_==i)d.fillRect(3,y-1,233,11,sel);
       d.setTextColor(white,selection_==i?sel:bg);
       d.setCursor(5,y);d.print(files_[i]);
       ++row;
     }
     if(mode_==0&&entryMode_){
       d.setTextColor(green,bg);d.setCursor(5,98);
       d.print(String(">")+entry_);
     }
   }
   d.setTextColor(dim,bg);d.setCursor(5,109);
   d.print(status_.substring(0,38));
 }
private:
 static const char*names(int i){
   static const char*const items[]={"Safe File Manager","Backup Files","Restore Files",
     "Filename Search","Storage Inventory"};
   return items[i];
 }
 int mode_=-1,menu_=0,selection_=0,count_=0,entryMode_=0;
 bool sd_=false,dirty_=true,confirmed_=false;
 char files_[40][32]={},entry_[32]={},query_[24]={},action_=0;
 uint64_t total_=0;
 String status_;
 static bool letter(const Keyboard_Class::KeysState&k,char c){
   for(char v:k.word)if(v==c||v==(c-'a'+'A'))return true;
   return false;
 }
 static bool matches(const char*a,const char*b){
   if(!*b)return true;
   for(const char*p=a;*p;++p){
     const char*x=p,*y=b;
     while(*x&&*y) {
       char v=*x,c=*y;
       if(v>='A'&&v<='Z')v=static_cast<char>(v+32);
       if(c>='A'&&c<='Z')c=static_cast<char>(c+32);
       if(v!=c)break;++x;++y;
     }
     if(!*y)return true;
   }
   return false;
 }
 const char*folder()const{return mode_==2?"/SUITE/BACKUP":"/SUITE/USER";}
 String filePath(const char*where,const char*name)const{
   return String(where)+"/"+name;
 }
 void refresh(){
   count_=selection_=0;total_=0;
   if(!sd_){status_="SD not mounted";return;}
   File dir=SD.open(folder());
   if(!dir||!dir.isDirectory()){if(dir)dir.close();status_="Cannot open directory";return;}
   while(count_<40){
     File f=dir.openNextFile();if(!f)break;
     String n=f.name();const int slash=n.lastIndexOf('/');
     if(slash>=0)n=n.substring(slash+1);
     if(!f.isDirectory()&&suite::field::safeFile(n.c_str())) {
       strncpy(files_[count_++],n.c_str(),31);total_+=f.size();
     }
     f.close();
   }
   dir.close();status_=String(count_)+" files listed (max 40)";
 }
 void inventory(){refresh();}
 bool copy(const String&src,const String&dst) {
   if(SD.exists(dst.c_str())||SD.exists((dst+".NEW").c_str())) {
     status_="Target exists; overwrite refused";return false;
   }
   File in=SD.open(src.c_str(),FILE_READ);
   if(!in||in.isDirectory()||in.size()>1024*1024u){
     if(in)in.close();status_="Missing/large source > 1MiB";return false;
   }
   const uint32_t size=in.size();
   File out=SD.open((dst+".NEW").c_str(),FILE_WRITE);
   if(!out){in.close();status_="Cannot create .NEW";return false;}
   static uint8_t block[1024]={};
   uint32_t total=0;bool valid=true;
   while(total<size) {
     const size_t request=(size-total)<sizeof(block)?(size-total):sizeof(block);
     const size_t n=in.read(block,request);
     if(n!=request||out.write(block,n)!=n){valid=false;break;}
     total+=n;yield();
   }
   out.flush();out.close();in.close();
   if(!valid||total!=size){status_="Copy interrupted; .NEW preserved";return false;}
   if(!SD.rename((dst+".NEW").c_str(),dst.c_str())){
     status_="Rename failed; .NEW preserved";return false;
   }
   status_=String("Copied ")+size+" bytes";return true;
 }
 void act(){
   if(!sd_){status_="No SD";return;}
   if(action_=='N'){
     if(!suite::field::safeLeaf(entry_,".TXT")){status_="Create only .TXT files";return;}
     const String p=filePath("/SUITE/USER",entry_);
     if(SD.exists(p.c_str())){status_="File already exists";return;}
     File f=SD.open(p.c_str(),FILE_WRITE);
     if(!f){status_="Create failed";return;}
     f.close();status_="Created empty text file";refresh();return;
   }
   if(selection_>=count_){status_="No selected file";return;}
   const char*name=files_[selection_];
   const String source=filePath(folder(),name);
   if(action_=='D'){
     if(SD.remove(source.c_str())){status_="File deleted";refresh();}
     else status_="Delete failed";
   }else if(action_=='M'){
     if(!suite::field::safeFile(entry_)){status_="Unsafe target";return;}
     const String dest=filePath("/SUITE/USER",entry_);
     if(SD.exists(dest.c_str())){status_="Target exists; denied";return;}
     if(SD.rename(source.c_str(),dest.c_str())){refresh();status_="Renamed";}
     else status_="Rename failed";
   }else if(action_=='C'){
     copy(source,filePath("/SUITE/BACKUP",name)); 
   }else if(action_=='R'){
     copy(source,filePath("/SUITE/USER",name));
   }
 }
 void preview(){
   if(selection_>=count_)return;
   const String path=filePath("/SUITE/USER",files_[selection_]);
   File f=SD.open(path.c_str(),FILE_READ);
   if(!f){status_="Cannot read";return;}
   char data[29]={};const size_t n=f.readBytes(data,28);
   f.close();
   for(size_t i=0;i<n;++i)if(static_cast<uint8_t>(data[i])<32||
     static_cast<uint8_t>(data[i])>126)data[i]='.';
   status_=String("Preview: ")+data;
 }
};
