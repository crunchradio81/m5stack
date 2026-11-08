#pragma once
// P6 developer SD collection: BASIC expression REPL, scientific calculator,
// hex codec and bounded CHIP-8 emulator, original no-PSRAM Cardputer K132.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include "core/DevCore.h"
#include "core/PocketTools.h"
class DevCollection {
public:
  void begin(){
    mode_=-1;menu_=0;selected_=0;active_=false;dirty_=true;
    SPI.begin(40,39,14,12);
    sd_=SD.begin(12,SPI,10000000);
    if(sd_){
      if(!SD.exists("/SUITE"))SD.mkdir("/SUITE");
      if(!SD.exists("/SUITE/CHIP8"))SD.mkdir("/SUITE/CHIP8");
    }
    status_=sd_?"Ready": "microSD missing (CHIP-8 disabled)";
  }
  void exit(){active_=false;}
  void poll(){
    if(mode_!=3||!active_)return;
    const uint32_t now=millis();
    if(static_cast<uint32_t>(now-lastFrame_)<17)return;
    lastFrame_=now;
    // Keyboard pressed state is refreshed in the device's main loop.
    auto k=M5Cardputer.Keyboard.keysState();
    for(uint8_t v=0;v<16;++v)chip_.key(v,false);
    static const char keys[17]="1234qwerasdfzxcv";
    for(char c:k.word)for(uint8_t v=0;v<16;++v)
      if(c==keys[v]||c==(keys[v]-32))chip_.key(v,v>=0);
    for(int n=0;n<9;++n)if(!chip_.step()){
      active_=false;status_="ROM halted (invalid opcode/PC)";break;
    }
    chip_.tickTimers();dirty_=true;
  }
  bool dirty()const{return dirty_;}
  void input(const Keyboard_Class::KeysState& k){
    if(k.ctrl&&letter(k,'b')){mode_=-1;active_=false;dirty_=true;return;}
    if(mode_<0){
      if(k.up&&menu_>0)--menu_;
      if(k.down&&menu_<3)++menu_;
      if(k.enter)open(menu_);
      dirty_=true;return;
    }
    if(mode_==3){
      if(!active_) {
        if(k.up&&selected_>0)--selected_;
        if(k.down&&selected_+1<romCount_)++selected_;
        if(k.enter)loadRom();
        if(letter(k,'r'))refreshRoms();
      } else if(k.ctrl&&letter(k,'r')) {
        active_=false;loadRom();
      }
      dirty_=true;return;
    }
    if(mode_==1&&(k.up||k.down)) {
      int i=static_cast<int>(operation_);
      i+=k.up?-1:1;
      if(i<0)i=static_cast<int>(suite::dev::Science::Count)-1;
      if(i>=static_cast<int>(suite::dev::Science::Count))i=0;
      operation_=static_cast<suite::dev::Science>(i);
    }
    if(k.backspace){const size_t n=strlen(line_);if(n)line_[n-1]=0;}
    else if(k.enter)execute();
    else if(k.ctrl&&letter(k,'c'))line_[0]=0;
    else if(!k.fn&&!k.alt&&!k.ctrl) {
      for(char c:k.word){
        if(c<32||c>126)continue;
        const size_t n=strlen(line_);
        if(n+1<sizeof(line_)){line_[n]=c;line_[n+1]=0;}
      }
    }
    dirty_=true;
  }
  void render(){
    dirty_=false;
    auto& d=M5Cardputer.Display;
    constexpr uint16_t bg=0x0841,head=0x10c4,white=0xffff,
      cyan=0x05ff,green=0x5fe9,dim=0xad55,sel=0x2673;
    d.fillScreen(bg);d.fillRect(0,0,240,23,head);
    d.setTextColor(white,head);d.setCursor(5,7);
    d.print(mode_<0?"DEVELOPER TOOLS":names(mode_));
    d.setTextColor(cyan,head);d.setCursor(209,7);d.print("SD");
    d.fillRect(0,116,240,19,head);
    d.setTextColor(dim,head);d.setCursor(4,121);
    d.print(mode_<0?"Enter select":
      mode_==3?"Ctrl+B menu | R rescan ROMs":
      "Enter evaluate | Ctrl+C clear Ctrl+B");
    if(mode_<0){
      for(int i=0;i<4;++i){
        const int y=30+i*19;
        if(menu_==i)d.fillRect(3,y-2,233,15,sel);
        d.setTextColor(white,menu_==i?sel:bg);
        d.setCursor(8,y+1);d.print(String(i+1)+". "+names(i));
      }
      d.setTextColor(dim,bg);d.setCursor(5,108);d.print(status_.substring(0,38));
      return;
    }
    if(mode_==3){
      if(active_){
        for(int y=0;y<32;++y)for(int x=0;x<64;++x) {
          if(chip_.frame(x,y))
            d.fillRect(24+x*3,38+y*2,3,2,white);
        }
        d.setTextColor(cyan,bg);d.setCursor(5,26);
        d.print(String("CHIP8  PC ")+String(chip_.pc(),HEX)+"  Ctrl+B menu");
        d.setTextColor(dim,bg);d.setCursor(5,107);
        d.print("Keys 1234 QWER ASDF ZXCV");
      } else {
        d.setTextColor(dim,bg);d.setCursor(5,26);
        d.print(String(romCount_)+" files in /SUITE/CHIP8/");
        for(int i=0;i<romCount_&&i<6;++i) {
          const int y=41+i*11;
          if(selected_==i)d.fillRect(3,y-1,230,11,sel);
          d.setTextColor(white,selected_==i?sel:bg);
          d.setCursor(5,y);d.print(roms_[i]);
        }
        d.setTextColor(cyan,bg);d.setCursor(5,110);
        d.print(status_.substring(0,38));
      }
      return;
    }
    d.setTextColor(cyan,bg);d.setCursor(5,29);
    if(mode_==0)d.print("LET A=2+3  /  PRINT A*4  / CLEAR");
    if(mode_==1)d.print(suite::dev::scienceName(operation_));
    if(mode_==2)d.print("Hex ASCII codec (encode or decode)");
    d.setTextColor(white,bg);d.setCursor(5,48);
    d.print(String("> ")+String(line_).substring(0,35));
    d.setTextColor(green,bg);d.setCursor(5,71);
    d.print(result_.substring(0,39));
    d.setTextColor(dim,bg);d.setCursor(5,100);
    d.print(status_.substring(0,38));
  }
private:
  static const char* names(int i) {
    static const char* const all[]={"BASIC Mini REPL","Scientific Calculator",
                                   "Hex/ASCII Codec","CHIP-8 Emulator"};
    return all[i];
  }
  suite::dev::Basic basic_;
  suite::dev::Chip8 chip_;
  suite::dev::Science operation_=suite::dev::Science::SquareRoot;
  int mode_=-1,menu_=0,selected_=0,romCount_=0;
  uint32_t lastFrame_=0;
  bool sd_=false,active_=false,dirty_=true;
  char line_[97]={},roms_[16][36]={};
  String status_,result_;
  static bool letter(const Keyboard_Class::KeysState& k,char c){
    for(char v:k.word)if(v==c||v==(c-'a'+'A'))return true;
    return false;
  }
  void open(int index) {
    mode_=index;active_=false;line_[0]=0;dirty_=true;
    result_="";status_="";
    if(index==3)refreshRoms();
  }
  void execute() {
    if(mode_==0){
      const auto r=basic_.run(line_);
      if(r.ok) {result_=String(r.value,7);status_="OK";}else status_="BASIC syntax/division error";
    } else if(mode_==1){
      double number=0;
      if(!suite::pocket::parseNumber(line_,number)){status_="Invalid numeric input";return;}
      const auto r=suite::dev::science(operation_,number);
      if(r.ok){result_=String(r.value,7);status_="OK";}
      else status_="Domain/overflow error";
    } else if(mode_==2){
      static uint8_t data[48]={};
      size_t n=0;
      if(suite::dev::hexDecode(line_,data,sizeof(data),n)){
        result_="ASCII: ";
        for(size_t i=0;i<n;++i)
          result_+=data[i]>=32&&data[i]<=126?String(static_cast<char>(data[i])):".";
        status_="Hex decoded";
      } else if(suite::dev::hexEncode(
          reinterpret_cast<const uint8_t*>(line_),strlen(line_),hex_,sizeof(hex_))) {
        result_=String("HEX: ")+hex_;
        status_="Text encoded to hex";
      } else status_="Max 48 input chars";
    }
  }
  char hex_[193]={};
  void refreshRoms(){
    romCount_=selected_=0;
    if(!sd_){status_="Insert FAT32 microSD";return;}
    File dir=SD.open("/SUITE/CHIP8");
    if(!dir||!dir.isDirectory()){if(dir)dir.close();status_="ROM directory missing";return;}
    while(romCount_<16){
      File f=dir.openNextFile();if(!f)break;
      String n=f.name();const int slash=n.lastIndexOf('/');
      if(slash>=0)n=n.substring(slash+1);
      if(!f.isDirectory()&&n.endsWith(".CH8")&&n.length()<36&&
         f.size()>0&&f.size()<=suite::dev::Chip8::RomMax)
        strncpy(roms_[romCount_++],n.c_str(),35);
      f.close();
    }
    dir.close();status_=romCount_?"Select ROM and Enter":"Put .CH8 ROMs on SD";
  }
  void loadRom(){
    if(!sd_||romCount_==0||selected_>=romCount_)return;
    const String name=String("/SUITE/CHIP8/")+roms_[selected_];
    File f=SD.open(name.c_str(),FILE_READ);
    if(!f||f.isDirectory()||f.size()>suite::dev::Chip8::RomMax){
      if(f)f.close();status_="Cannot open ROM";return;
    }
    static uint8_t rom[suite::dev::Chip8::RomMax]={};
    const size_t n=f.size();
    const bool read=f.read(rom,n)==n;
    f.close();
    if(!read||!chip_.load(rom,n)){status_="ROM load failed";return;}
    active_=true;lastFrame_=millis();status_="Playing CHIP-8";
  }
};
