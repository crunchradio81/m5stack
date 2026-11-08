#pragma once
// P11 five offline pocket apps on stock original K132, no PSRAM/Grove.
// Plain ASCII data and bounded fixed files on SD; no vault or cloud syncing.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include <esp_system.h>
#include <qrcode.h>
#include "core/Pocket11Core.h"
class Pocket11Collection {
public:
 void begin(){
   mode_=-1;menu_=0;dirty_=true;editing_=searching_=false;
   SPI.begin(40,39,14,12);
   sd_=SD.begin(12,SPI,10000000);
   if(sd_){
     if(!SD.exists("/SUITE"))SD.mkdir("/SUITE");
     if(!SD.exists("/SUITE/POCKET"))SD.mkdir("/SUITE/POCKET");
   }
   status_=sd_?"Pocket apps ready":"Insert FAT32 microSD";
 }
 void exit(){secret_[0]=0;}
 void poll(){}
 bool dirty()const{return dirty_;}
 void input(const Keyboard_Class::KeysState&k){
   if(mode_<0){
     if(k.up&&menu_>0)--menu_;
     if(k.down&&menu_<4)++menu_;
     if(k.enter)open(menu_);
     dirty_=true;return;
   }
   if(k.ctrl&&letter(k,'b')){
     secret_[0]=0;mode_=-1;dirty_=true;return;
   }
   if(mode_==0){
     if(k.ctrl&&letter(k,'s'))saveText();
     else if(k.left)editor_.left();
     else if(k.right)editor_.right();
     else if(k.backspace)editor_.backspace();
     else if(k.enter)editor_.insert('\n');
     else if(!k.ctrl&&!k.fn&&!k.alt)
       for(char c:k.word)editor_.insert(c);
   }else if(mode_==1){
     if(k.left&&x_>0)--x_;
     if(k.right&&x_<5)++x_;
     if(k.up&&y_>0)--y_;
     if(k.down&&y_<5)++y_;
     if(k.backspace){const size_t n=strlen(number_);if(n)number_[n-1]=0;}
     else if(k.enter){
       int32_t value=0;
       if(suite::pocket11::Sheet::number(number_,strlen(number_),value)
           &&sheet_.set(x_,y_,value)) {
         number_[0]=0;status_="Cell changed (Ctrl+S to save)";
       }else status_="Type integer -99999..99999";
     }else if(k.ctrl&&letter(k,'s'))saveSheet();
     else if(!k.ctrl&&!k.fn&&!k.alt){
       for(char c:k.word)if((c>='0'&&c<='9')||c=='-'){
         const size_t n=strlen(number_);
         if(n<7){number_[n]=c;number_[n+1]=0;}
       }
     }
   }else if(mode_==2){
     if(searching_){
       if(k.backspace){const size_t n=strlen(query_);if(n)query_[n-1]=0;}
       else if(k.enter){
         int hit=-1;
         if(cards_.search(query_,hit)){card_=hit;revealed_=false;status_="Found card";}
         else status_="No match";
         searching_=false;
       }else if(!k.ctrl&&!k.fn&&!k.alt)
         for(char c:k.word)if(c>=32&&c<=126){
           const size_t n=strlen(query_);
           if(n+1<sizeof(query_)){query_[n]=c;query_[n+1]=0;}
         }
     }else{
       if(k.up&&card_>0){--card_;revealed_=false;}
       if(k.down&&card_+1<cards_.count()){++card_;revealed_=false;}
       if(k.enter)revealed_=!revealed_;
       if(letter(k,'r'))loadCards();
       if(symbol(k,'/')){searching_=true;query_[0]=0;}
     }
   }else if(mode_==3){
     if(k.backspace){const size_t n=strlen(entry_);if(n)entry_[n-1]=0;qrReady_=false;}
     else if(k.enter||k.ctrl&&letter(k,'g'))makeQr();
     else if(!k.ctrl&&!k.fn&&!k.alt)
       for(char c:k.word)if(c>=32&&c<=126){
         const size_t n=strlen(entry_);
         if(n<42){entry_[n]=c;entry_[n+1]=0;qrReady_=false;}
       }
   }else if(mode_==4){
     if(k.up&&length_<32)++length_;
     if(k.down&&length_>8)--length_;
     if(letter(k,'g')||k.enter)makePassword();
   }
   dirty_=true;
 }
 void render(){
   dirty_=false;auto&d=M5Cardputer.Display;
   constexpr uint16_t bg=0x0841,head=0x10c4,white=0xffff,
     cyan=0x05ff,green=0x5fe9,sel=0x2673,dim=0xad55;
   d.fillScreen(bg);d.fillRect(0,0,240,23,head);
   d.setTextColor(white,head);d.setCursor(5,7);
   d.print(mode_<0?"POCKET PRODUCTIVITY":names(mode_));
   d.setTextColor(cyan,head);d.setCursor(209,7);d.print("SD");
   d.fillRect(0,116,240,19,head);
   d.setTextColor(dim,head);d.setCursor(4,121);
   d.print(mode_<0?"Arrows / Enter":
      mode_==0?"Ctrl+S save Ctrl+B menu":
      mode_==1?"Arrows cell Enter set Ctrl+S save":
      mode_==2?"Enter reveal / search R reload":
      mode_==3?"Type then Enter QR Ctrl+B menu":
      "G new Up/Down length Ctrl+B");
   if(mode_<0){
     for(int i=0;i<5;++i){
       const int y=29+i*17;
       if(i==menu_)d.fillRect(3,y-1,233,14,sel);
       d.setTextColor(white,i==menu_?sel:bg);
       d.setCursor(8,y+2);d.print(String(i+1)+". "+names(i));
     }
     d.setTextColor(dim,bg);d.setCursor(5,110);d.print(status_);
     return;
   }
   if(mode_==0){
     d.setTextColor(cyan,bg);d.setCursor(5,26);
     d.print(String("NOTE.TXT ")+editor_.size()+"/1023 chars");
     const size_t start=editor_.size()>260?editor_.size()-260:0;
     int x=5,y=40;
     for(size_t i=start;i<editor_.size()&&y<107;++i){
       const char c=editor_.text()[i];
       if(c=='\n'||x>225){x=5;y+=10;if(c=='\n')continue;}
       if(y>=107)break;
       d.setTextColor(white,bg);d.setCursor(x,y);d.print(c);x+=6;
     }
   }else if(mode_==1){
     d.setTextColor(cyan,bg);d.setCursor(5,26);
     d.print(String("6x6 values, row sum ")+
             static_cast<long>(sheet_.sumRow(y_)));
     for(int y=0;y<6;++y)for(int x=0;x<6;++x){
       const int px=4+x*39,py=40+y*11;
       if(x==x_&&y==y_)d.fillRect(px,py-1,37,11,sel);
       d.setTextColor(white,x==x_&&y==y_?sel:bg);
       d.setCursor(px,py);d.print(String(sheet_.at(x,y)).substring(0,6));
     }
     d.setTextColor(green,bg);d.setCursor(5,108);
     d.print(String("> ")+number_);
   }else if(mode_==2){
     d.setTextColor(cyan,bg);d.setCursor(5,27);
     d.print(String("Flashcards ")+(card_+1)+"/"+cards_.count());
     if(cards_.count()){
       d.setTextColor(white,bg);d.setCursor(5,51);
       d.print(String(cards_.question(card_)).substring(0,38));
       if(revealed_){
         d.setTextColor(green,bg);d.setCursor(5,74);
         d.print(String(cards_.answer(card_)).substring(0,38));
       }else{
         d.setTextColor(dim,bg);d.setCursor(5,74);
         d.print("Enter to reveal answer");
       }
     }
     if(searching_){
       d.setTextColor(cyan,bg);d.setCursor(5,101);
       d.print(String("Find: ")+query_);
     }
   }else if(mode_==3){
     if(qrReady_){
       d.fillRect(0,23,240,93,white);
       const int mod=3,ox=(240-29*mod)/2,oy=26;
       for(int y=0;y<qr_.size;++y)for(int x=0;x<qr_.size;++x)
         if(qrcode_getModule(&qr_,x,y))
           d.fillRect(ox+x*mod,oy+y*mod,mod,mod,0x0000);
     }else{
       d.setTextColor(cyan,bg);d.setCursor(5,31);
       d.print("QR Code, 42 ASCII chars max");
       d.setTextColor(white,bg);d.setCursor(5,57);
       d.print(String("> ")+entry_);
     }
   }else{
     d.setTextColor(cyan,bg);d.setCursor(5,28);
     d.print(String("Random string length: ")+length_);
     d.setTextColor(white,bg);d.setCursor(5,57);
     d.print(secret_[0]?secret_:"Press G / Enter");
     d.setTextColor(dim,bg);d.setCursor(5,85);
     d.print("Not a security-audited vault.");
     d.setCursor(5,99);d.print("Output is never written to SD.");
   }
   if(mode_!=3){
     d.setTextColor(dim,bg);d.setCursor(5,109);
     d.print(status_.substring(0,38));
   }
 }
private:
 static const char*names(int n){
   static const char*const list[]={
     "SD Text Editor","Spreadsheet Mini","Flashcard Trainer",
     "QR Code Generator","Random String Maker"
   };return list[n];
 }
 int mode_=-1,menu_=0,x_=0,y_=0,card_=0;
 unsigned length_=16;
 bool sd_=false,dirty_=true,revealed_=false,searching_=false,editing_=false,qrReady_=false;
 char number_[9]={},entry_[43]={},secret_[40]={},query_[33]={};
 String status_;
 suite::pocket11::Editor editor_;
 suite::pocket11::Sheet sheet_;
 suite::pocket11::Flashcards cards_;
 QRCode qr_={};
 uint8_t qrBuffer_[256]={};
 static bool letter(const Keyboard_Class::KeysState&k,char c){
   for(char v:k.word)if(v==c||v==(c-'a'+'A'))return true;
   return false;
 }
 static bool symbol(const Keyboard_Class::KeysState&k,char c){
   for(char v:k.word)if(v==c)return true;
   return false;
 }
 void open(int m){
   mode_=m;dirty_=true;
   status_="";revealed_=searching_=false;qrReady_=false;
   if(m==0)loadText();
   if(m==1)loadSheet();
   if(m==2)loadCards();
   if(m==3)entry_[0]=0;
   if(m==4)secret_[0]=0;
 }
 bool loadBlob(const char*path,char*out,size_t cap,size_t&len){
   len=0;if(!sd_)return false;
   File f=SD.open(path,FILE_READ);
   if(!f||f.isDirectory()||f.size()>=cap){
     if(f)f.close();return false;
   }
   len=f.size();
   const bool ok=f.readBytes(out,len)==len;f.close();
   out[len]=0;return ok;
 }
 bool saveBlob(const char*path,const uint8_t*bytes,size_t n){
   if(!sd_||!bytes||n>1500)return false;
   const String tmp=String(path)+".NEW",bak=String(path)+".BAK";
   if(SD.exists(tmp.c_str())&&!SD.remove(tmp.c_str()))return false;
   File f=SD.open(tmp.c_str(),FILE_WRITE);
   if(!f)return false;
   const bool ok=f.write(bytes,n)==n;f.flush();f.close();
   if(!ok)return false;
   const bool exists=SD.exists(path);
   if(exists){
     if(SD.exists(bak.c_str())&&!SD.remove(bak.c_str()))return false;
     if(!SD.rename(path,bak.c_str()))return false;
   }
   if(!SD.rename(tmp.c_str(),path)){
     if(exists)SD.rename(bak.c_str(),path);
     return false;
   }
   return true;
 }
 void loadText(){
   editor_.clear();readOnly_=false;
   const char*path="/SUITE/POCKET/NOTE.TXT";
   if(!sd_){status_="Insert SD to edit text";return;}
   if(!SD.exists(path)){status_="New note; Ctrl+S saves";return;}
   static char buf[1025]={};size_t n=0;
   if(!loadBlob(path,buf,sizeof(buf),n)||!editor_.set(buf,n)){
     status_="Invalid note, refusing overwrite";
     readOnly_=true;return;
   }
   readOnly_=false;status_="Loaded NOTE.TXT";
 }
 void saveText(){
   if(readOnly_||!sd_){status_="Cannot save protected note";return;}
   if(saveBlob("/SUITE/POCKET/NOTE.TXT",
          reinterpret_cast<const uint8_t*>(editor_.text()),editor_.size()))
     status_="Saved NOTE.TXT";
   else status_="SD save failed; .NEW may remain";
 }
 bool readOnly_=false;
 void loadSheet(){
   sheet_.clear();readOnly_=false;
   const char*path="/SUITE/POCKET/SHEET.CSV";
   if(!sd_){status_="Insert SD";return;}
   if(!SD.exists(path)){status_="New sheet; Ctrl+S saves";return;}
   static char data[513]={};size_t len=0;
   if(!loadBlob(path,data,sizeof(data),len)||!sheet_.parse(data,len)){
     status_="Corrupt sheet; refusing overwrite";readOnly_=true;return;
   }
   status_="Sheet loaded";
 }
 void saveSheet(){
   if(readOnly_||!sd_){status_="Read-only / no SD";return;}
   char text[512]={};
   const size_t n=sheet_.encode(text,sizeof(text));
   if(!n||!saveBlob("/SUITE/POCKET/SHEET.CSV",
        reinterpret_cast<const uint8_t*>(text),n))
     status_="Sheet save failed";
   else status_="Saved sheet";
 }
 void loadCards(){
   cards_.clear();card_=0;revealed_=false;
   if(!sd_){status_="No SD";return;}
   const char*path="/SUITE/POCKET/CARDS.TXT";
   if(!SD.exists(path)){status_="Put Q|A cards in CARDS.TXT";return;}
   static char data[1801]={};size_t len=0;
   if(!loadBlob(path,data,sizeof(data),len)||!cards_.parse(data,len)){
     status_="Invalid flashcard deck";return;
   }
   status_="Flashcards ready";
 }
 void makeQr(){
   const size_t n=strlen(entry_);
   if(!n||n>42){status_="Enter 1-42 ASCII chars";return;}
   memset(qrBuffer_,0,sizeof(qrBuffer_));
   const int rc=qrcode_initText(&qr_,qrBuffer_,3,ECC_LOW,entry_);
   qrReady_=rc==0;
   status_=qrReady_?"QR created":"QR data unsupported/long";
 }
 void makePassword(){
   secret_[0]=0;
   if(suite::pocket11::randomString(secret_,sizeof(secret_),length_,
        []()->uint32_t {return esp_random();}))
     status_="Random string (not stored)";
   else status_="Random generation failed";
 }
};
