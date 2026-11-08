#pragma once
// P14 original Cardputer K132 developer SD collection: BASIC Studio,
// syntax-colored code editor, guarded hex editor, USB CDC console,
// and bounded SD user-content search. No arbitrary system calls or PSRAM.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include "core/Dev14Core.h"
#include "core/Pocket11Core.h"
class Dev14Collection {
public:
 void begin(){
   mode_=-1;menu_=0;dirty_=true;
   SPI.begin(40,39,14,12);
   sd_=SD.begin(12,SPI,10000000);
   if(sd_){
     if(!SD.exists("/SUITE"))SD.mkdir("/SUITE");
     if(!SD.exists("/SUITE/DEV"))SD.mkdir("/SUITE/DEV");
     if(!SD.exists("/SUITE/USER"))SD.mkdir("/SUITE/USER");
   }
   status_=sd_?"Ready: files stay on SD":"SD missing";
 }
 void exit(){};
 void poll(){
   if(mode_!=3)return;
   int n=0;
   while(Serial.available()&&n++<96){
     const int v=Serial.read();
     if(v<0)break;
     const char c=static_cast<char>(v);
     if(c=='\r')continue;
     if(c=='\n'){pushLine(serialIncoming_);serialIncoming_[0]=0;}
     else if(c>=32&&c<=126){
       const size_t len=strlen(serialIncoming_);
       if(len<36){serialIncoming_[len]=c;serialIncoming_[len+1]=0;}
     }
     dirty_=true;
   }
 }
 bool dirty()const{return dirty_;}
 void input(const Keyboard_Class::KeysState&k){
   if(mode_<0){
     if(k.up&&menu_>0)--menu_;
     if(k.down&&menu_<4)++menu_;
     if(k.enter)open(menu_);
     dirty_=true;return;
   }
   if(k.ctrl&&letter(k,'b')){mode_=-1;dirty_=true;return;}
   if(mode_==0){
     if(k.backspace)backspace(entry_);
     else if(k.enter)basicLine();
     else if(k.up&&listScroll_>0)--listScroll_;
     else if(k.down&&listScroll_+1<basic_.count())++listScroll_;
     else if(!k.ctrl&&!k.fn&&!k.alt)append(k,entry_,sizeof(entry_));
   }else if(mode_==1){
     if(k.ctrl&&letter(k,'s'))saveCode();
     else if(k.backspace)code_.backspace();
     else if(k.left)code_.left();
     else if(k.right)code_.right();
     else if(k.enter)code_.insert('\n');
     else if(!k.ctrl&&!k.fn&&!k.alt)
       for(char c:k.word)code_.insert(c);
   }else if(mode_==2){
     if(k.left&&hexCursor_>0)--hexCursor_;
     if(k.right&&hexCursor_+1<hex_.size())++hexCursor_;
     if(k.up&&hexCursor_>=16)hexCursor_-=16;
     if(k.down&&hexCursor_+16<hex_.size())hexCursor_+=16;
     if(k.ctrl&&letter(k,'s'))saveHex();
     else if(letter(k,'a')){
       if(hex_.append(0))hexCursor_=hex_.size()-1;
     }else if(k.enter){
       uint8_t value=0;
       if(suite::dev14::HexBuffer::parseByte(hexEntry_,value)&&
          hex_.set(hexCursor_,value)){
         hexEntry_[0]=0;status_="Byte updated, Ctrl+S to save";
       }else status_="Enter two hex digits (00-FF)";
     }else if(k.backspace)backspace(hexEntry_);
     else if(!k.fn&&!k.alt&&!k.ctrl)
       for(char c:k.word){
         const bool valid=(c>='0'&&c<='9')||(c>='a'&&c<='f')||
                          (c>='A'&&c<='F');
         if(valid&&strlen(hexEntry_)<2){
           const size_t n=strlen(hexEntry_);
           hexEntry_[n]=c;hexEntry_[n+1]=0;
         }
       }
   }else if(mode_==3){
     if(k.backspace)backspace(entry_);
     else if(k.enter){
       if(entry_[0]){
         Serial.println(entry_);
         pushLine(String(">")+entry_);
         entry_[0]=0;
       }
     }else if(!k.ctrl&&!k.fn&&!k.alt)append(k,entry_,sizeof(entry_));
   }else{
     if(k.backspace)backspace(entry_);
     else if(k.enter)searchFiles();
     else if(!k.ctrl&&!k.fn&&!k.alt)append(k,entry_,21);
   }
   dirty_=true;
 }
 void render(){
   dirty_=false;auto&d=M5Cardputer.Display;
   constexpr uint16_t bg=0x0841,head=0x10c4,white=0xffff,
      cyan=0x05ff,green=0x5fe9,sel=0x2673,dim=0xad55;
   d.fillScreen(bg);d.fillRect(0,0,240,23,head);
   d.setTextColor(white,head);d.setCursor(5,7);
   d.print(mode_<0?"DEVELOPER STUDIO":names(mode_));
   d.setTextColor(cyan,head);d.setCursor(207,7);d.print("SD");
   d.fillRect(0,116,240,19,head);
   d.setTextColor(dim,head);d.setCursor(5,121);
   d.print(mode_<0?"Fn arrows / Enter":
     mode_==0?"10 PRINT 5 / RUN / SAVE Ctrl+B":
     mode_==1?"Ctrl+S save Ctrl+B menu":
     mode_==2?"Enter hex A append Ctrl+S":
     mode_==3?"Enter send USB Ctrl+B menu":
     "Type substring Enter search Ctrl+B");
   if(mode_<0){
     for(int i=0;i<5;++i){
       const int y=29+i*17;
       if(menu_==i)d.fillRect(3,y-1,233,14,sel);
       d.setTextColor(white,menu_==i?sel:bg);
       d.setCursor(7,y+2);d.print(String(i+1)+". "+names(i));
     }
     return;
   }
   if(mode_==0){
     d.setTextColor(cyan,bg);d.setCursor(5,27);
     d.print(String("BASIC lines ")+basic_.count()+" / 64");
     for(unsigned i=listScroll_;i<basic_.count()&&i<listScroll_+4;++i){
       const int y=40+(i-listScroll_)*12;
       d.setTextColor(white,bg);d.setCursor(5,y);
       d.print(String(basic_.number(i))+" "+
               String(basic_.statement(i)).substring(0,31));
     }
     d.setTextColor(green,bg);d.setCursor(5,91);
     d.print(String("> ")+entry_);
     d.setTextColor(dim,bg);d.setCursor(5,108);
     d.print(status_.substring(0,38));
   }else if(mode_==1){
     d.setTextColor(cyan,bg);d.setCursor(5,27);
     d.print(String("CODE.TXT: ")+code_.size()+" / 1023");
     const size_t start=code_.size()>260?code_.size()-260:0;
     int x=5,y=39;
     for(size_t i=start;i<code_.size()&&y<109;++i){
       const char c=code_.text()[i];
       if(c=='\n'||x>=228){
         x=5;y+=10;if(c=='\n')continue;
       }
       if(y>=109)break;
       const bool keyword=(c>='A'&&c<='Z');
       const bool digit=(c>='0'&&c<='9');
       d.setTextColor(keyword?cyan:digit?green:white,bg);
       d.setCursor(x,y);d.print(c);x+=6;
     }
   }else if(mode_==2){
     d.setTextColor(cyan,bg);d.setCursor(5,28);
     d.print(String("DATA.BIN ")+hex_.size()+" bytes");
     const int start=static_cast<int>((hexCursor_/64)*64);
     for(int r=0;r<4;++r){
       d.setTextColor(dim,bg);d.setCursor(5,44+r*13);
       d.print(String(start+r*16,HEX));
       for(int c=0;c<16;++c){
         const int index=start+r*16+c;
         if(index>=static_cast<int>(hex_.size()))break;
         const int px=34+c*12;
         if(index==static_cast<int>(hexCursor_))d.fillRect(px-1,43+r*13,13,12,sel);
         d.setTextColor(white,index==static_cast<int>(hexCursor_)?sel:bg);
         d.setCursor(px,44+r*13);
         if(hex_.at(index)<16)d.print('0');
         d.print(String(hex_.at(index),HEX));
       }
     }
     d.setTextColor(green,bg);d.setCursor(5,108);
     d.print(String("New byte: ")+hexEntry_);
   }else if(mode_==3){
     d.setTextColor(cyan,bg);d.setCursor(5,27);
     d.print("USB serial 115200 / local CDC");
     for(int i=0;i<5;++i){
       d.setTextColor(white,bg);d.setCursor(5,42+i*12);
       d.print(serialLines_[i]);
     }
     d.setTextColor(green,bg);d.setCursor(5,108);
     d.print(String(">")+entry_);
   }else{
     d.setTextColor(cyan,bg);d.setCursor(5,27);
     d.print("Search up to 20 USER files");
     d.setTextColor(white,bg);d.setCursor(5,45);
     d.print(String(">")+entry_);
     for(int i=0;i<hits_&&i<4;++i){
       d.setTextColor(green,bg);d.setCursor(5,63+i*11);
       d.print(hitNames_[i]);
     }
     d.setTextColor(dim,bg);d.setCursor(5,108);
     d.print(status_.substring(0,38));
   }
 }
private:
 static const char*names(int i){
   static const char*const labels[]={
     "BASIC Program Studio","Syntax Color Editor","Bounded Hex Editor",
     "USB Serial Console","SD Content Search"};
   return labels[i];
 }
 static bool letter(const Keyboard_Class::KeysState&k,char c){
   for(char x:k.word)if(x==c||x==(c-'a'+'A'))return true;
   return false;
 }
 static void backspace(char*text){
   const size_t n=strlen(text);if(n)text[n-1]=0;
 }
 static void append(const Keyboard_Class::KeysState&k,char*text,size_t cap){
   for(char c:k.word){
     if(c<32||c>126)continue;
     const size_t n=strlen(text);
     if(n+1<cap){text[n]=c;text[n+1]=0;}
   }
 }
 suite::dev14::Program basic_;
 suite::pocket11::Editor code_;
 suite::dev14::HexBuffer hex_;
 int mode_=-1,menu_=0,hits_=0;
 unsigned listScroll_=0;
 size_t hexCursor_=0;
 bool dirty_=true,sd_=false,readonly_=false;
 char entry_[96]={},hexEntry_[3]={};
 char hitNames_[4][36]={},serialLines_[5][38]={},serialIncoming_[38]={};
 String status_;
 static const char*basicPath(){return "/SUITE/DEV/PROGRAM.BAS";}
 static const char*codePath(){return "/SUITE/DEV/CODE.TXT";}
 static const char*hexPath(){return "/SUITE/DEV/DATA.BIN";}
 void open(int mode){
   mode_=mode;dirty_=true;status_="";entry_[0]=hexEntry_[0]=0;
   listScroll_=0;hits_=0;readonly_=false;hexCursor_=0;
   if(mode==0)loadBasic();
   if(mode==1)loadCode();
   if(mode==2)loadHex();
   if(mode==3)Serial.println("K132 developer USB serial ready");
 }
 bool readFile(const char*path,uint8_t*out,size_t cap,size_t&n) {
   n=0;if(!sd_)return false;
   File f=SD.open(path,FILE_READ);
   if(!f||f.isDirectory()||f.size()>cap){
     if(f)f.close();return false;
   }
   n=f.size();
   const bool ok=f.read(out,n)==static_cast<int>(n);
   f.close();return ok;
 }
 bool saveFile(const char*path,const uint8_t*bytes,size_t n){
   if(!sd_||readonly_||n>suite::dev14::Program::MaxText)return false;
   const String tmp=String(path)+".NEW",bak=String(path)+".BAK";
   if(SD.exists(tmp)&&!SD.remove(tmp))return false;
   File f=SD.open(tmp,FILE_WRITE);
   if(!f)return false;
   const bool ok=f.write(bytes,n)==n;f.flush();f.close();
   if(!ok)return false;
   const bool had=SD.exists(path);
   if(had){
     if(SD.exists(bak)&&!SD.remove(bak))return false;
     if(!SD.rename(path,bak))return false;
   }
   if(!SD.rename(tmp,path)){
     if(had)SD.rename(bak,path);
     return false;
   }
   return true;
 }
 void loadBasic(){
   basic_.clear();
   if(!sd_){status_="SD required to save";return;}
   if(!SD.exists(basicPath())){status_="New BASIC program";return;}
   static char text[suite::dev14::Program::MaxText+1]={};
   size_t n=0;
   if(!readFile(basicPath(),reinterpret_cast<uint8_t*>(text),
        sizeof(text)-1,n)){
     readonly_=true;status_="Invalid BASIC file protected";return;
   }
   text[n]=0;
   if(!basic_.parse(text,n)){
     readonly_=true;status_="Malformed BASIC file protected";return;
   }
   status_="BASIC loaded";
 }
 void saveBasic(){
   if(readonly_){status_="Protected old program";return;}
   static char text[suite::dev14::Program::MaxText+1]={};
   const size_t n=basic_.encode(text,sizeof(text));
   if(n&&saveFile(basicPath(),reinterpret_cast<const uint8_t*>(text),n))
     status_="Saved PROGRAM.BAS";
   else status_="Save failed";
 }
 void basicLine(){
   if(!entry_[0])return;
   if(!strcasecmp(entry_,"RUN")){
     const auto r=basic_.execute();
     if(r.status==suite::dev14::Program::Status::OK){
       status_=r.count?String("Output: ")+r.output[r.count-1]:"Run complete";
     }else status_=String("BASIC error ")+static_cast<int>(r.status)+
       " after "+r.steps+" steps";
   }else if(!strcasecmp(entry_,"SAVE"))saveBasic();
   else if(!strcasecmp(entry_,"LOAD"))loadBasic();
   else if(!strcasecmp(entry_,"NEW")){
     basic_.clear();readonly_=false;status_="New unsaved program";
   }else{
     const char*ptr=entry_;unsigned line=0;
     while(*ptr>='0'&&*ptr<='9'){
       line=line*10+static_cast<unsigned>(*ptr-'0');++ptr;
       if(line>9999)break;
     }
     if(*ptr==' '&&line>0&&line<=9999&&basic_.put(line,ptr+1))
       status_=String("Stored line ")+line;
     else status_="Use: 10 LET A=1 or RUN";
   }
   entry_[0]=0;
 }
 void loadCode(){
   code_.clear();
   if(!sd_||!SD.exists(codePath())){status_="New code note";return;}
   static char data[1025]={};size_t n=0;
   if(!readFile(codePath(),reinterpret_cast<uint8_t*>(data),1024,n)||
      !code_.set(data,n)){
     readonly_=true;status_="Code invalid: protected";return;
   }
   status_="Code loaded";
 }
 void saveCode(){
   if(saveFile(codePath(),reinterpret_cast<const uint8_t*>(code_.text()),
       code_.size()))status_="Saved CODE.TXT";
   else status_="Code save failed/protected";
 }
 void loadHex(){
   hex_.clear();hexCursor_=0;
   if(!sd_){status_="SD unavailable";return;}
   if(!SD.exists(hexPath())){status_="Empty DATA.BIN; A append";return;}
   uint8_t bytes[256]={};size_t n=0;
   if(!readFile(hexPath(),bytes,sizeof(bytes),n)||!hex_.load(bytes,n)){
     readonly_=true;status_="Oversized hex file protected";return;
   }
   status_="Hex data loaded";
 }
 void saveHex(){
   if(saveFile(hexPath(),hex_.bytes(),hex_.size()))status_="Saved DATA.BIN";
   else status_="Hex save failed/protected";
 }
 void pushLine(const String&s){
   for(int i=0;i<4;++i)strcpy(serialLines_[i],serialLines_[i+1]);
   String clean="";
   for(int i=0;i<s.length()&&i<37;++i){
     const char c=s[i];clean+=(c>=32&&c<=126?c:'?');
   }
   strncpy(serialLines_[4],clean.c_str(),37);serialLines_[4][37]=0;
 }
 void searchFiles(){
   hits_=0;if(!sd_||!entry_[0]){status_="Enter query and insert SD";return;}
   File dir=SD.open("/SUITE/USER",FILE_READ);
   if(!dir||!dir.isDirectory()){if(dir)dir.close();status_="Cannot scan USER";return;}
   int visited=0;
   while(visited<20&&hits_<4){
     File f=dir.openNextFile();if(!f)break;
     if(!f.isDirectory()&&f.size()<=4096){
       ++visited;
       String path=f.name();
       if(suite::dev14::contains(path.c_str(),entry_)){
         copyHit(path);
       }else{
         const size_t needle=strlen(entry_);
         char window[64]={};size_t position=0,read=0;
         while(f.available()&&read<4096&&hits_<4){
           const int v=f.read();++read;
           if(v<0)break;
           if(v<32||v>126){position=0;continue;}
           if(position<sizeof(window)-1)window[position++]=static_cast<char>(v);
           else{
             memmove(window,window+1,sizeof(window)-2);
             window[sizeof(window)-2]=static_cast<char>(v);
             position=sizeof(window)-1;
           }
           window[position]=0;
           if(position>=needle&&suite::dev14::contains(window,entry_))
             {copyHit(path);break;}
         }
       }
     }
     f.close();
     yield();
   }
   dir.close();
   status_=String(hits_)+" matches (20 files / 4KiB cap)";
 }
 void copyHit(const String&path){
   String name=path.substring(path.lastIndexOf('/')+1);
   strncpy(hitNames_[hits_],name.c_str(),35);hitNames_[hits_][35]=0;
   ++hits_;
 }
};
