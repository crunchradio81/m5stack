#pragma once
#include "core/Theme16Runtime.h"
// P9 MIDI analysis suite: five SD-stored tools, no PSRAM or external MIDI.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include <string.h>
#include <strings.h>
#include "midi/Smf.h"
#include "midi/Midi9Core.h"
class MidiExtrasCollection {
public:
 void begin(){
   mode_=-1;menu_=0;selected_=0;view_=false;dirty_=true;
   SPI.begin(40,39,14,12);sd_=SD.begin(12,SPI,10000000);
   if(sd_){
     if(!SD.exists("/SUITE"))SD.mkdir("/SUITE");
     if(!SD.exists("/SUITE/MIDI"))SD.mkdir("/SUITE/MIDI");
   }
   refresh();loadFavorites();
 }
 void exit(){source_.close();}
 void poll(){}
 bool dirty()const{return dirty_;}
 void input(const Keyboard_Class::KeysState& k){
   if(mode_<0){
     if(k.up&&menu_>0)--menu_;
     if(k.down&&menu_<4)++menu_;
     if(k.enter){mode_=menu_;selected_=0;view_=false;dirty_=true;}
     return;
   }
   if(letter(k,'b')||k.backspace) {
     if(view_){view_=false;source_.close();}
     else mode_=-1;
     dirty_=true;return;
   }
   if(view_){
     if(k.up&&scroll_>0)--scroll_;
     if(k.down&&scroll_<30)++scroll_;
   }else{
     if(k.up&&selected_>0)--selected_;
     if(k.down&&selected_+1<files_)++selected_;
     if(letter(k,'r'))refresh();
     if(mode_==3&&letter(k,'f'))toggleFavorite();
     else if(k.enter&&selected_<files_)analyze();
   }
   dirty_=true;
 }
 void render(){
   dirty_=false;auto&d=M5Cardputer.Display;
   const auto &p=suite::skin16::colors();
        const uint16_t bg=p.bg,
        head=p.panel,
        white=p.text,
        cyan=p.accent,
        green=p.good,
        sel=p.selected,
        dim=p.muted;
   d.fillScreen(bg);d.fillRect(0,0,240,23,head);
   d.setTextColor(white,head);d.setCursor(5,7);
   d.print(mode_<0?"MIDI DEEP TOOLS":names(mode_));
   d.setTextColor(cyan,head);d.setCursor(207,7);d.print("SD");
   d.fillRect(0,116,240,19,head);
   d.setTextColor(dim,head);d.setCursor(5,121);
   d.print(mode_<0?"Fn arrows | Enter":view_?"Fn arrows scroll | B files":
         mode_==3?"F favorite | Enter analyze | B":
         "Enter inspect | R refresh | B");
   if(mode_<0) {
     for(int i=0;i<5;++i) {
       const int y=30+i*16;
       if(i==menu_)d.fillRect(3,y-1,233,13,sel);
       d.setTextColor(white,i==menu_?sel:bg);
       d.setCursor(8,y+1);d.print(String(i+1)+". "+names(i));
     }
     d.setTextColor(dim,bg);d.setCursor(5,109);d.print(status_);
     return;
   }
   if(!view_) {
     d.setTextColor(cyan,bg);d.setCursor(5,26);
     d.print(String(files_)+" MIDI/KAR files (max 48)");
     const int first=selected_>=7?selected_-6:0;
     for(int i=first;i<files_&&i<first+7;++i){
       const int y=37+(i-first)*11;
       if(i==selected_)d.fillRect(3,y-1,233,11,sel);
       d.setTextColor(white,i==selected_?sel:bg);
       d.setCursor(5,y);
       d.print(String(favorites_.contains(fileNames_[i])?"* ":"  ")+fileNames_[i]);
     }
     d.setTextColor(dim,bg);d.setCursor(5,109);d.print(status_.substring(0,38));
     return;
   }
   d.setTextColor(cyan,bg);d.setCursor(5,26);
   d.print(String(fileNames_[selected_]).substring(0,37));
   if(mode_==0) {
     info(d,white,bg,40,String("Notes ")+stats_.notes()+" Events "+stats_.events());
     info(d,white,bg,54,String("Tracks ")+tracks_+" Format "+format_);
     info(d,white,bg,68,String("PPQN ")+division_+" MaxTick "+stats_.lastTick());
     info(d,white,bg,82,String("Ch1 notes ")+stats_.channelNotes(0)+
                             " patch "+stats_.program(0));
     info(d,green,bg,97,partial_?"Partial scan (time/event cap)":"Scan complete");
   } else if(mode_==1) {
     info(d,white,bg,39,"Recent 40 note-on events");
     const unsigned n=stats_.noteWindow();
     for(unsigned i=0;i<n;++i){
       const int note=stats_.notePitch(i);
       const int height=note<24?0:note>96?55:(note-24)*55/72;
       const int x=6+static_cast<int>(i)*5;
       if(height)d.fillRect(x,103-height,4,2,green);
     }
     info(d,dim,bg,108,"Y represents MIDI pitch, not time");
   } else if(mode_==2) {
     info(d,white,bg,39,String(stats_.lyricCount())+" text/lyric events (first 12)");
     for(unsigned i=scroll_;i<stats_.lyricCount()&&i<scroll_+6;++i){
       info(d,green,bg,53+static_cast<int>(i-scroll_)*10,stats_.lyric(i));
     }
   } else if(mode_==3) {
     info(d,white,bg,39,String("Favorite: ")+
       (favorites_.contains(fileNames_[selected_])?"YES":"NO"));
     info(d,white,bg,58,String("Saved favorites: ")+favorites_.count());
     info(d,dim,bg,77,"Back to library; F toggles.");
     info(d,dim,bg,96,"No playback queue editing yet.");
   } else {
     info(d,white,bg,39,String(stats_.tempoCount())+" tempo events (first 16)");
     for(unsigned i=scroll_;i<stats_.tempoCount()&&i<scroll_+6;++i){
       const uint32_t us=stats_.tempoValue(i);
       info(d,green,bg,52+static_cast<int>(i-scroll_)*10,
            String("Tick ")+stats_.tempoTick(i)+" : "+
            (us?60000000u/us:0)+" BPM");
     }
   }
   info(d,dim,bg,109,status_.substring(0,38));
 }
private:
 static void info(M5GFX&d,uint16_t color,uint16_t bg,int y,const String&s){
   d.setTextColor(color,bg);d.setCursor(5,y);d.print(s.substring(0,38));
 }
 static const char*names(int n){
   static const char* const t[]={
     "MIDI Track Analyzer","Piano Roll","Lyrics Catalog",
     "Favorite Playlists","Tempo Map"
   };return t[n];
 }
 class SdSource:public suite::smf::Source {
 public:
   bool open(const String&path){close();file_=SD.open(path.c_str(),FILE_READ);return file_&&!file_.isDirectory();}
   void close(){if(file_)file_.close();}
   uint32_t length()const override{return file_?static_cast<uint32_t>(file_.size()):0;}
   bool readAt(uint32_t offset,uint8_t*out,size_t n)override{
     return file_&&file_.seek(offset)&&file_.read(out,n)==static_cast<int>(n);
   }
 private: File file_;
 };
 int mode_=-1,menu_=0,selected_=0,files_=0,scroll_=0;
 int tracks_=0,format_=0,division_=0;
 bool sd_=false,dirty_=true,view_=false,partial_=false,readOnly_=false;
 char fileNames_[48][41]={};
 String status_;
 SdSource source_;
 suite::smf::Reader reader_;
 suite::smf::Event next_;
 suite::midi9::Favorites favorites_;
 suite::midi9::Analysis stats_;
 static bool letter(const Keyboard_Class::KeysState&k,char c){
   for(char v:k.word)if(v==c||v==(c-'a'+'A'))return true;
   return false;
 }
 void refresh(){
   files_=selected_=0;
   if(!sd_){status_="Insert SD for MIDI";return;}
   File dir=SD.open("/SUITE/MIDI");
   if(!dir||!dir.isDirectory()){if(dir)dir.close();status_="MIDI dir unavailable";return;}
   while(files_<48){
     File f=dir.openNextFile();if(!f)break;
     String n=f.name();const int slash=n.lastIndexOf('/');
     if(slash>=0)n=n.substring(slash+1);
     if(!f.isDirectory()&&suite::midi9::Favorites::safe(n.c_str())&&
        f.size()>=14&&f.size()<=suite::smf::Reader::MaxFile)
       strcpy(fileNames_[files_++],n.c_str());
     f.close();
   }
   dir.close();status_=files_?"Select MIDI":"Copy .MID/.KAR to /SUITE/MIDI";
 }
 void analyze(){
   if(selected_>=files_)return;
   source_.close();stats_.clear();scroll_=0;partial_=false;
   const String path=String("/SUITE/MIDI/")+fileNames_[selected_];
   if(!source_.open(path)||!reader_.open(source_)){
     source_.close();status_="Invalid MIDI file";return;
   }
   tracks_=reader_.trackCount();format_=reader_.format();
   division_=reader_.division();
   auto&d=M5Cardputer.Display;
   d.fillScreen(0x0841);d.setTextColor(0xffff,0x0841);
   d.setCursor(5,40);d.print("Reading MIDI event stream...");
   const uint32_t start=millis();
   unsigned count=0;
   while(count<30000&&static_cast<uint32_t>(millis()-start)<2000) {
     if(!reader_.next(next_))break;
     stats_.take(next_);++count;
     if(count%512==0)yield();
   }
   partial_=count==30000||static_cast<uint32_t>(millis()-start)>=2000;
   if(reader_.failed()){status_="Damaged MIDI stream";source_.close();return;}
   source_.close();
   status_=partial_?"Partial analysis (bounded)":"MIDI scan complete";
   view_=true;
 }
 void loadFavorites(){
   favorites_.clear();readOnly_=false;
   if(!sd_)return;
   if(!SD.exists("/SUITE/MIDI/FAVORITES.TXT"))return;
   File file=SD.open("/SUITE/MIDI/FAVORITES.TXT",FILE_READ);
   if(!file||file.isDirectory()||file.size()>2100){
     if(file)file.close();readOnly_=true;status_="Favorites damaged; protected";return;
   }
   static char text[2101]={};const size_t len=file.size();
   const bool ok=file.readBytes(text,len)==len;file.close();
   if(!ok||!favorites_.parse(text,len)){
     favorites_.clear();readOnly_=true;
     status_="Invalid favorites: read-only";
   }
 }
 bool saveFavorites(){
   if(!sd_||readOnly_)return false;
   static char text[2101]={};
   const size_t n=favorites_.write(text,sizeof(text));
   if(!n&&favorites_.count())return false;
   const char*path="/SUITE/MIDI/FAVORITES.TXT";
   const char*temp="/SUITE/MIDI/FAVORITES.NEW";
   const char*backup="/SUITE/MIDI/FAVORITES.BAK";
   if(SD.exists(temp)&&!SD.remove(temp))return false;
   File file=SD.open(temp,FILE_WRITE);if(!file)return false;
   const bool written=file.write(reinterpret_cast<const uint8_t*>(text),n)==n;
   file.flush();file.close();if(!written)return false;
   const bool exists=SD.exists(path);
   if(exists) {
     if(SD.exists(backup)&&!SD.remove(backup))return false;
     if(!SD.rename(path,backup))return false;
   }
   if(!SD.rename(temp,path)) {
     if(exists)SD.rename(backup,path);
     return false;
   }
   return true;
 }
 void toggleFavorite(){
   if(selected_>=files_||readOnly_){status_="Read-only favorites";return;}
   const char*name=fileNames_[selected_];
   suite::midi9::Favorites prior=favorites_;
   if(favorites_.contains(name))favorites_.remove(name);
   else if(!favorites_.add(name)){status_="Favorites full";return;}
   if(!saveFavorites()){
     favorites_=prior;status_="SD favorite save failed";
   }else status_="Favorites updated";
 }
};
