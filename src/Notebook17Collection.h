#pragma once
// P17 original K132: multiple independent searchable notes and guarded backups.
// Built-in keyboard/display, 8MiB flash, no PSRAM/Grove/network.
// NOTEBOOK.BIN runs only in the suite OTA_0 partition, not directly from SD.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include "core/Notebook17Core.h"
#include "core/Theme16Runtime.h"

class Notebook17Collection {
public:
 void begin(){
   mode_=-1;menu_=selected_=hitSelected_=0;
   editing_=naming_=dirtyDoc_=false;
   confirm_=Action::None;dirty_=true;
   query_[0]=stem_[0]=current_[0]=0;
   SPI.begin(40,39,14,12);
   sd_=SD.begin(12,SPI,10000000);
   if(sd_){
     if(!SD.exists("/SUITE"))SD.mkdir("/SUITE");
     if(!SD.exists("/SUITE/NOTES17"))SD.mkdir("/SUITE/NOTES17");
     if(!SD.exists("/SUITE/BACKUP"))SD.mkdir("/SUITE/BACKUP");
     if(!SD.exists("/SUITE/BACKUP/NOTES17"))
       SD.mkdir("/SUITE/BACKUP/NOTES17");
   }
   refresh();
   status_=sd_?"Notes local to SD; no syncing":"FAT32 microSD not mounted";
 }
 void exit(){draft_.clear();}
 void poll(){}
 bool dirty()const{return dirty_;}
 void input(const Keyboard_Class::KeysState&k){
   if(confirm_!=Action::None){
     if(letter(k,'y'))confirmAction();
     else status_="Cancelled; files unchanged";
     confirm_=Action::None;dirty_=true;return;
   }
   if(naming_){nameInput(k);dirty_=true;return;}
   if(editing_){editorInput(k);dirty_=true;return;}
   if(mode_<0){
     if(k.up&&menu_>0)--menu_;
     if(k.down&&menu_<4)++menu_;
     if(k.enter)open(menu_);
     dirty_=true;return;
   }
   if(letter(k,'b')){mode_=-1;dirty_=true;return;}
   if(mode_==1){
     if(k.enter){search();dirty_=true;return;}
     if(k.ctrl&&letter(k,'e')){
       if(hitSelected_<hits_){
         const auto index=results_[hitSelected_];
         loadNote(notes_.at(index).name);
       }
       dirty_=true;return;
     }
     if(k.up&&hitSelected_>0)--hitSelected_;
     if(k.down&&hitSelected_+1<hits_)++hitSelected_;
     if(k.backspace){
       const size_t n=strlen(query_);if(n)query_[n-1]=0;
     }else if(!k.fn&&!k.alt&&!k.ctrl)
       for(char c:k.word)append(query_,sizeof(query_),c);
     dirty_=true;return;
   }
   if(letter(k,'r')){refresh();dirty_=true;return;}
   const auto&list=mode_==3?backups_:notes_;
   if(k.up&&selected_>0)--selected_;
   if(k.down&&selected_+1<list.count())++selected_;
   if(mode_==0){
     if(letter(k,'n')){stem_[0]=0;naming_=true;status_="Name new note (18 chars max)";}
     else if(k.enter&&selected_<notes_.count())
       loadNote(notes_.at(selected_).name);
   }else if(mode_==2&&k.enter&&selected_<notes_.count()){
     confirm_=Action::Backup;status_="Copy note into backup? Y/N";
   }else if(mode_==3&&k.enter&&selected_<backups_.count()){
     confirm_=Action::Restore;
     status_="Restore as NEW R#-name? Y/N";
   }else if(mode_==4&&k.enter&&selected_<notes_.count()){
     char raw[suite::notebook17::MaxBytes+1]={};size_t n=0;
     if(readBody(path(NoteDir,notes_.at(selected_).name),raw,n)){
       char hash[30]={};
       const uint32_t v=suite::notebook17::fingerprint(
         reinterpret_cast<const uint8_t*>(raw),n);
       snprintf(hash,sizeof(hash),"%08lX",static_cast<unsigned long>(v));
       status_=String("FNV32 ")+hash+" / "+n+" B";
     }else status_="Invalid/unreadable source";
   }
   dirty_=true;
 }
 void render(){
   dirty_=false;auto&d=M5Cardputer.Display;
   const auto&p=suite::skin16::colors();
   const uint16_t bg=p.bg,head=p.panel,white=p.text,
     cyan=p.accent,sel=p.selected,dim=p.muted,green=p.good;
   d.fillScreen(bg);d.fillRect(0,0,240,23,head);
   d.setTextColor(white,head);d.setCursor(5,7);
   d.print(editing_?"NOTE EDITOR":naming_?"NEW NOTE":
     mode_<0?"NOTEBOOK & BACKUPS":names(mode_));
   d.setTextColor(cyan,head);d.setCursor(211,7);d.print("N17");
   d.fillRect(0,116,240,19,head);
   d.setTextColor(dim,head);d.setCursor(4,121);
   d.print(editing_?"Ctrl+S save Ctrl+B leave":
     naming_?"Type stem / Enter / Ctrl+B":
     confirm_!=Action::None?"Y confirm / N cancel":
     mode_<0?"Fn arrows / Enter":
     mode_==0?"N new Enter edit R rescan B":
     mode_==1?"Type Enter search Ctrl+E edit":
     mode_==2?"Enter backup R rescan B":
     mode_==3?"Enter restore copy R rescan B":
     "Enter checksum R rescan B");
   if(mode_<0&&!editing_&&!naming_){
     for(int i=0;i<5;++i){
       const int y=29+i*17;
       if(i==menu_)d.fillRect(3,y-1,233,14,sel);
       d.setTextColor(white,i==menu_?sel:bg);
       d.setCursor(7,y+2);d.print(String(i+1)+". "+names(i));
     }
     d.setTextColor(dim,bg);d.setCursor(5,109);
     d.print(status_.substring(0,37));return;
   }
   if(naming_){
     d.setTextColor(cyan,bg);d.setCursor(5,29);
     d.print("Use A-Z, 0-9, underscore, dash");
     d.setTextColor(green,bg);d.setCursor(5,57);
     d.print(String("> ")+stem_);
     d.setTextColor(dim,bg);d.setCursor(5,87);
     d.print("Saved as /SUITE/NOTES17/*.TXT");
   }else if(editing_){
     d.setTextColor(cyan,bg);d.setCursor(5,27);
     d.print(String(current_).substring(0,23)+" "+
       draft_.cursor()+"/"+draft_.size()+(dirtyDoc_?" *":""));
     const size_t position=draft_.cursor();
     const size_t start=position>200?position-200:0;
     int x=5,y=41;
     const char*bytes=draft_.data();
     for(size_t i=start;i<draft_.size()&&y<=99;++i){
       const char c=bytes[i];
       if(c=='\n'||x>226){x=5;y+=10;if(c=='\n')continue;}
       if(y>99)break;
       d.setTextColor(white,bg);d.setCursor(x,y);
       d.print(c);x+=6;
     }
     d.setTextColor(dim,bg);d.setCursor(5,108);
     d.print(status_.substring(0,37));
   }else if(mode_==1){
     d.setTextColor(cyan,bg);d.setCursor(5,27);
     d.print(String("Query: ")+query_);
     d.setTextColor(dim,bg);d.setCursor(5,40);
     d.print(String(hits_)+" matches (max 32 notes)");
     const int start=hitSelected_>=5?hitSelected_-4:0;
     for(int i=start;i<hits_&&i<start+5;++i){
       const int y=53+(i-start)*11;
       if(i==hitSelected_)d.fillRect(3,y-1,234,11,sel);
       d.setTextColor(white,i==hitSelected_?sel:bg);d.setCursor(6,y);
       d.print(notes_.at(results_[i]).name);
     }
   }else{
     const auto&list=mode_==3?backups_:notes_;
     d.setTextColor(cyan,bg);d.setCursor(5,26);
     d.print(mode_==3?"/BACKUP/NOTES17":"/SUITE/NOTES17");
     d.setTextColor(dim,bg);d.setCursor(5,36);
     d.print(String(list.count())+" files; each <=1023B");
     const int start=selected_>=5?selected_-4:0;
     for(int i=start;i<static_cast<int>(list.count())&&i<start+5;++i){
       const int y=49+(i-start)*11;
       if(i==selected_)d.fillRect(3,y-1,233,11,sel);
       d.setTextColor(white,i==selected_?sel:bg);d.setCursor(6,y);
       d.print(String(list.at(i).name)+" "+list.at(i).bytes);
     }
     d.setTextColor(dim,bg);d.setCursor(5,108);
     d.print(status_.substring(0,37));
   }
 }
private:
 static constexpr const char*NoteDir="/SUITE/NOTES17";
 static constexpr const char*BackupDir="/SUITE/BACKUP/NOTES17";
 enum class Action:uint8_t {None,Backup,Restore,Discard};
 static const char*names(int index){
   static const char*const n[]={
     "Multi-file Notes","Content Search","Backup Note",
     "Restore Safe Copy","Note Integrity"};
   return n[index];
 }
 static bool letter(const Keyboard_Class::KeysState&k,char c){
   for(char v:k.word)
     if(v==c||v==(c-'a'+'A'))return true;
   return false;
 }
 static void append(char*str,size_t cap,char c){
   if(c<32||c>126||!cap)return;
   const size_t n=strlen(str);
   if(n+1<cap){str[n]=c;str[n+1]=0;}
 }
 static String path(const char*base,const char*name){
   return String(base)+"/"+name;
 }
 bool sd_=false,dirty_=true,editing_=false,naming_=false,dirtyDoc_=false;
 int mode_=-1,menu_=0,selected_=0,hitSelected_=0,hits_=0;
 char stem_[suite::notebook17::MaxStem+1]={},
   current_[suite::notebook17::MaxName]={},
   query_[suite::notebook17::MaxQuery+1]={};
 uint8_t results_[suite::notebook17::MaxNotes]={};
 Action confirm_=Action::None;
 suite::notebook17::Text draft_;
 suite::notebook17::Index notes_,backups_;
 String status_;
 void open(int mode){
   mode_=mode;selected_=hitSelected_=hits_=0;
   query_[0]=0;editing_=naming_=false;confirm_=Action::None;
   refresh();dirty_=true;
 }
 void scan(const char*where,suite::notebook17::Index&index){
   index.clear();if(!sd_)return;
   File dir=SD.open(where,FILE_READ);
   if(!dir||!dir.isDirectory()){if(dir)dir.close();return;}
   unsigned scanned=0;
   while(scanned<128 && index.count()<suite::notebook17::MaxNotes){
     File f=dir.openNextFile();if(!f)break;
     ++scanned;
     String name=f.name();
     const int slash=name.lastIndexOf('/');
     if(slash>=0)name=name.substring(slash+1);
     if(!f.isDirectory()&&suite::notebook17::validName(name.c_str())&&
        f.size()<=suite::notebook17::MaxBytes)
       index.add(name.c_str(),f.size());
     f.close();
   }
   dir.close();
 }
 void refresh(){
   notes_.clear();backups_.clear();
   if(!sd_){status_="SD unavailable";return;}
   scan(NoteDir,notes_);scan(BackupDir,backups_);
   if(selected_>=static_cast<int>(notes_.count()))selected_=
     notes_.count()?notes_.count()-1:0;
   status_=String(notes_.count())+" notes, "+backups_.count()+" backups";
 }
 bool readBody(const String&filename,char*out,size_t&size){
   size=0;if(!sd_||!out)return false;
   File f=SD.open(filename.c_str(),FILE_READ);
   if(!f||f.isDirectory()||f.size()>suite::notebook17::MaxBytes){
     if(f)f.close();return false;
   }
   const size_t n=f.size();
   const bool read=f.readBytes(out,n)==n;
   f.close();
   if(!read)return false;
   out[n]=0;
   suite::notebook17::Text checked;
   if(!checked.load(out,n))return false;
   size=n;return true;
 }
 bool writeTmp(const String&temp,const char*data,size_t n){
   if(n>suite::notebook17::MaxBytes||SD.exists(temp.c_str()))
     return false;
   File out=SD.open(temp.c_str(),FILE_WRITE);
   if(!out)return false;
   const bool ok=out.write(reinterpret_cast<const uint8_t*>(data),n)==n;
   out.flush();
   const bool sized=out.size()==n;
   out.close();
   if(!ok||!sized)return false;
   char check[suite::notebook17::MaxBytes+1]={};size_t read=0;
   return readBody(temp,check,read)&&
     suite::notebook17::same(
       reinterpret_cast<const uint8_t*>(data),n,
       reinterpret_cast<const uint8_t*>(check),read);
 }
 void loadNote(const char*filename){
   if(!suite::notebook17::validName(filename))return;
   char raw[suite::notebook17::MaxBytes+1]={};size_t n=0;
   if(!readBody(path(NoteDir,filename),raw,n)||!draft_.load(raw,n)){
     status_="Unreadable note; refusing edit";return;
   }
   strcpy(current_,filename);dirtyDoc_=false;editing_=true;
   status_="Loaded; Ctrl+S explicitly saves";dirty_=true;
 }
 void nameInput(const Keyboard_Class::KeysState&k){
   if(k.ctrl&&letter(k,'b')){naming_=false;return;}
   if(k.backspace){const size_t n=strlen(stem_);if(n)stem_[n-1]=0;}
   else if(k.enter){
     char filename[suite::notebook17::MaxName]={};
     if(!suite::notebook17::makeName(stem_,filename,sizeof(filename))){
       status_="Invalid filename";return;
     }
     if(notes_.count()>=suite::notebook17::MaxNotes){
       status_="32-note limit; preserve existing notes";return;
     }
     if(notes_.find(filename)>=0||
        SD.exists(path(NoteDir,filename).c_str())){
       status_="Name already exists";return;
     }
     strcpy(current_,filename);draft_.clear();
     dirtyDoc_=true;editing_=true;naming_=false;
     status_="New unsaved note; Ctrl+S creates";
   }else if(!k.ctrl&&!k.alt&&!k.fn)
     for(char c:k.word)append(stem_,sizeof(stem_),c);
 }
 void editorInput(const Keyboard_Class::KeysState&k){
   if(k.ctrl&&letter(k,'b')){
     if(dirtyDoc_){confirm_=Action::Discard;status_="Discard unsaved edits? Y/N";}
     else editing_=false;
     return;
   }
   if(k.ctrl&&letter(k,'s')){saveNote();return;}
   if(k.left)draft_.left();
   else if(k.right)draft_.right();
   else if(k.backspace){if(draft_.backspace())dirtyDoc_=true;}
   else if(k.enter){if(draft_.insert('\n'))dirtyDoc_=true;}
   else if(!k.ctrl&&!k.alt&&!k.fn){
     for(char c:k.word)if(draft_.insert(c))dirtyDoc_=true;
   }
 }
 void saveNote(){
   if(!sd_||!suite::notebook17::validName(current_)){
     status_="SD missing/invalid name";return;
   }
   const String target=path(NoteDir,current_);
   const String temp=target+".NEW";
   const String backup=target+".BAK";
   const bool existed=SD.exists(target.c_str());
   if(existed){
     char previous[suite::notebook17::MaxBytes+1]={};size_t n=0;
     if(!readBody(target,previous,n)){
       status_="Malformed original protected";return;
     }
   }
   if(!writeTmp(temp,draft_.data(),draft_.size())){
     status_="Save verify failed; .NEW kept";return;
   }
   if(existed){
     // Latest previous-version .BAK is replaced only after .NEW is
     // successfully written and read back. A manual backup in
     // /SUITE/BACKUP/NOTES17 is never touched.
     if(SD.exists(backup.c_str())&&!SD.remove(backup.c_str())){
       status_="Cannot rotate .BAK; source kept";return;
     }
     if(!SD.rename(target.c_str(),backup.c_str())){
       status_="Cannot preserve original; .NEW kept";return;
     }
   }
   if(!SD.rename(temp.c_str(),target.c_str())){
     if(existed)SD.rename(backup.c_str(),target.c_str());
     status_="Save commit failed; verify SD";return;
   }
   char finalBytes[suite::notebook17::MaxBytes+1]={};
   size_t finalLength=0;
   if(!readBody(target,finalBytes,finalLength)||
      !suite::notebook17::same(
         reinterpret_cast<const uint8_t*>(draft_.data()),draft_.size(),
         reinterpret_cast<const uint8_t*>(finalBytes),finalLength)){
     status_="Committed note mismatch; check .BAK";return;
   }
   dirtyDoc_=false;
   refresh();
   status_=existed?"Verified save; previous in .BAK":
                   "Verified new note saved";
 }
 bool copyVerified(const String&from,const String&to){
   if(!sd_||SD.exists(to.c_str())||SD.exists((to+".NEW").c_str())){
     status_="Destination exists; no overwrite";return false;
   }
   char body[suite::notebook17::MaxBytes+1]={};size_t n=0;
   if(!readBody(from,body,n)){
     status_="Source damaged or unreadable";return false;
   }
   const String tmp=to+".NEW";
   if(!writeTmp(tmp,body,n)){
     status_="Copy mismatch; .NEW kept";return false;
   }
   if(SD.exists(to.c_str())||!SD.rename(tmp.c_str(),to.c_str())){
     status_="Final rename failed; .NEW kept";return false;
   }
   char finalBytes[suite::notebook17::MaxBytes+1]={};
   size_t finalLength=0;
   if(!readBody(to,finalBytes,finalLength)||
      !suite::notebook17::same(
         reinterpret_cast<const uint8_t*>(body),n,
         reinterpret_cast<const uint8_t*>(finalBytes),finalLength)){
     status_="Restored/copy destination mismatch";return false;
   }
   status_=String("Verified copy, ")+n+" bytes";
   return true;
 }
 void confirmAction(){
   if(confirm_==Action::Discard){
     editing_=false;dirtyDoc_=false;draft_.clear();
     status_="Unsaved buffer discarded";return;
   }
   if(confirm_==Action::Backup){
     if(selected_<static_cast<int>(notes_.count())){
       const char*name=notes_.at(selected_).name;
       if(copyVerified(path(NoteDir,name),path(BackupDir,name)))refresh();
     }
     return;
   }
   if(confirm_==Action::Restore){
     if(selected_>=static_cast<int>(backups_.count()))return;
     const char*name=backups_.at(selected_).name;
     char out[suite::notebook17::MaxName]={};
     for(unsigned version=1;version<=9;++version){
       if(!suite::notebook17::restoredName(name,version,out,sizeof(out))){
         status_="Unsafe restored filename";return;
       }
       const String target=path(NoteDir,out);
       if(!SD.exists(target.c_str())){
         if(copyVerified(path(BackupDir,name),target))refresh();
         return;
       }
     }
     status_="9 restored copies exist; no overwrite";
   }
 }
 void search(){
   hits_=hitSelected_=0;
   if(!query_[0]){status_="Type a search term first";return;}
   unsigned bad=0;
   for(unsigned i=0;i<notes_.count();++i){
     const auto&entry=notes_.at(i);
     const bool nameHit=suite::notebook17::contains(
       entry.name,strlen(entry.name),query_);
     bool bodyHit=false;
     if(!nameHit){
       char raw[suite::notebook17::MaxBytes+1]={};size_t n=0;
       if(!readBody(path(NoteDir,entry.name),raw,n))++bad;
       else bodyHit=suite::notebook17::contains(raw,n,query_);
     }
     if(nameHit||bodyHit)results_[hits_++]=static_cast<uint8_t>(i);
     yield();
   }
   status_=String(hits_)+" matches, "+bad+" unreadable notes";
 }
};
