#pragma once
// P19: stock original K132 Game Boy (DMG) app; not a Game Boy Color emulator.
// MIT Peanut-GB by Mahyar Koshkouei; pinned copy in vendor/peanut_gb.h.
// No commercial ROMs, boot firmware, or firmware repartitioning.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include <cstring>
#include <cstdlib>
#include "core/GameBoy18Core.h"
#define ENABLE_SOUND 0
#define ENABLE_LCD 1
#include "vendor/peanut_gb.h"

class GameBoy19Collection {
  static constexpr const char* RomDir = "/SUITE/ROMS/GB";
  static constexpr const char* SaveDir = "/SUITE/SAVES/GB";
  static constexpr int MaxEntries=24;
  static constexpr int MaxScanned=128;
  static constexpr uint32_t MaxRam=32768;
  static constexpr uint32_t MaxStepsPerFrame=100000;
  struct Entry { char name[64]={}; uint32_t size=0; };
  Entry entries_[MaxEntries];
  int count_=0, chosen_=0;
  bool sdReady_=false, playing_=false, dirty_=true, frameReady_=false;
  bool ioFault_=false, cpuFault_=false, ramDirty_=false, saveWritable_=false;
  String status_;
  String currentName_, savePath_;
  uint32_t nextFrameAt_=0, frameCount_=0, ages_=0;
  uint8_t *cache_[2]={nullptr,nullptr}, *cartRam_=nullptr;
  int32_t cacheBank_[2]={-1,-1};
  uint32_t cacheAge_[2]={0,0};
  size_t cartBytes_=0;
  uint32_t romBytes_=0;
  File rom_;
  struct gb_s gb_={};
  // Direct 150x135 LCD output is 40.5 KiB; original K132 has no PSRAM.
  uint16_t screen_[suite::gb18::ScreenHeight*suite::gb18::ScreenWidth]={};
  const uint16_t palette_[4]={0xD6F5,0x9CB0,0x526A,0x1946};

  static GameBoy19Collection* owner(struct gb_s* gb){
    return static_cast<GameBoy19Collection*>(gb->direct.priv);
  }
  static uint8_t readRom(struct gb_s* gb,const uint_fast32_t address){
    auto* p=owner(gb);
    if(!p || p->ioFault_)return 0xff;
    if(address>=p->romBytes_)return 0xff;
    const int32_t bank=static_cast<int32_t>(address/16384u);
    const unsigned offset=static_cast<unsigned>(address%16384u);
    for(int slot=0;slot<2;++slot)if(p->cacheBank_[slot]==bank){
      p->cacheAge_[slot]=++p->ages_;
      return p->cache_[slot][offset];
    }
    const int slot=p->cacheAge_[0]<=p->cacheAge_[1]?0:1;
    const uint32_t start=static_cast<uint32_t>(bank)*16384u;
    const size_t length=p->romBytes_-start<16384u?p->romBytes_-start:16384u;
    if(!p->cache_[slot]||!p->rom_||!p->rom_.seek(start)||
       p->rom_.read(p->cache_[slot],length)!=static_cast<int>(length)){
      p->ioFault_=true;
      return 0xff;
    }
    p->cacheBank_[slot]=bank;
    p->cacheAge_[slot]=++p->ages_;
    return p->cache_[slot][offset];
  }
  static uint8_t readRam(struct gb_s* gb,const uint_fast32_t address){
    const auto* p=owner(gb);
    if(!p||!p->cartRam_||address>=p->cartBytes_)return 0xff;
    return p->cartRam_[address];
  }
  static void writeRam(struct gb_s* gb,const uint_fast32_t address,const uint8_t value){
    auto* p=owner(gb);
    if(!p||!p->cartRam_||address>=p->cartBytes_)return;
    if(p->cartRam_[address]!=value){
      p->cartRam_[address]=value;
      p->ramDirty_=true;
    }
  }
  static void coreError(struct gb_s* gb,const enum gb_error_e, const uint16_t){
    if(auto* p=owner(gb))p->cpuFault_=true;
  }
  static void scanline(struct gb_s* gb,const uint8_t* pixels,const uint_fast8_t line){
    auto* p=owner(gb);
    if(!p||!pixels||line>=suite::gb18::GBHeight)return;
    // Inverse nearest-neighbour mapping keeps 160:144 proportions on 150:135.
    const unsigned y=(static_cast<unsigned>(line)*135u+143u)/144u;
    if(y>=135 || suite::gb18::srcY(y)!=line)return;
    uint16_t* row=p->screen_+y*150;
    for(unsigned x=0;x<150;++x)
      row[x]=p->palette_[pixels[suite::gb18::srcX(x)]&3u];
  }
  static bool letter(const Keyboard_Class::KeysState& k,char c){
    for(char x:k.word)if(x==c||x==c-'a'+'A')return true;
    return false;
  }
  void release(){
    if(rom_)rom_.close();
    for(int i=0;i<2;++i){
      if(cache_[i])free(cache_[i]);
      cache_[i]=nullptr;cacheBank_[i]=-1;cacheAge_[i]=0;
    }
    if(cartRam_)free(cartRam_);
    cartRam_=nullptr;cartBytes_=0;romBytes_=0;ages_=0;
    frameReady_=playing_=ramDirty_=false;
    currentName_="";savePath_="";
  }
  void scan(){
    count_=chosen_=0;
    if(!sdReady_)return;
    File dir=SD.open(RomDir,FILE_READ);
    if(!dir||!dir.isDirectory()){
      if(dir)dir.close();
      status_="Create /SUITE/ROMS/GB";return;
    }
    for(int examined=0;examined<MaxScanned && count_<MaxEntries;++examined){
      File f=dir.openNextFile();if(!f)break;
      if(!f.isDirectory()){
        String name=f.name();
        const int last=name.lastIndexOf('/');
        if(last>=0)name=name.substring(last+1);
        if(name.length()<64 && suite::gb18::romFilename(name.c_str()) &&
           f.size()>=suite::gb18::HeaderSize && f.size()<=8u*1024u*1024u){
          name.toCharArray(entries_[count_].name,sizeof(entries_[count_].name));
          entries_[count_].size=static_cast<uint32_t>(f.size());
          ++count_;
        }
      }
      f.close();
    }
    dir.close();
    status_=count_?"Enter load DMG game":"No .GB homebrew ROMs found";
  }
  static uint8_t controls(const Keyboard_Class::KeysState& keys){
    uint8_t active=0;
    for(char c:keys.word)active|=suite::gb18::button(c);
    uint8_t result=0xff;
    if(active&suite::gb18::Up)result&=~JOYPAD_UP;
    if(active&suite::gb18::Down)result&=~JOYPAD_DOWN;
    if(active&suite::gb18::Left)result&=~JOYPAD_LEFT;
    if(active&suite::gb18::Right)result&=~JOYPAD_RIGHT;
    if(active&suite::gb18::A)result&=~JOYPAD_A;
    if(active&suite::gb18::B)result&=~JOYPAD_B;
    if(active&suite::gb18::Start)result&=~JOYPAD_START;
    if(active&suite::gb18::Select)result&=~JOYPAD_SELECT;
    return result;
  }
  static bool batteryType(uint8_t type){
    switch(type){
      case 0x03:case 0x06:case 0x09:case 0x0f:case 0x10:
      case 0x13:case 0x1b:case 0x1e:return true;
      default:return false;
    }
  }
  bool verifyFile(const String& path)const{
    File f=SD.open(path.c_str(),FILE_READ);
    if(!f || f.isDirectory() || f.size()!=cartBytes_){if(f)f.close();return false;}
    uint8_t bytes[256];
    for(size_t i=0;i<cartBytes_;i+=sizeof(bytes)){
      const size_t n=cartBytes_-i<sizeof(bytes)?cartBytes_-i:sizeof(bytes);
      if(f.read(bytes,n)!=static_cast<int>(n) ||
         memcmp(bytes,cartRam_+i,n)!=0){f.close();return false;}
    }
    f.close();return true;
  }
  bool saveRam(){
    if(!saveWritable_||!cartRam_||!ramDirty_)return true;
    const String temp=savePath_+".NEW",backup=savePath_+".BAK";
    // Never overwrite interrupted saves or existing recovery snapshots.
    if(SD.exists(temp) || (SD.exists(savePath_) && SD.exists(backup))){
      status_="Save protected; check .NEW/.BAK";
      Serial.println(status_);return false;
    }
    File f=SD.open(temp.c_str(),FILE_WRITE);
    if(!f){status_="Could not create save temp";return false;}
    const bool complete=f.write(cartRam_,cartBytes_)==cartBytes_;
    f.flush();f.close();
    if(!complete||!verifyFile(temp)){status_="Save verify failed";return false;}
    const bool old=SD.exists(savePath_);
    if(old && !SD.rename(savePath_.c_str(),backup.c_str())){
      status_="Cannot preserve original save";return false;
    }
    if(!SD.rename(temp.c_str(),savePath_.c_str())){
      if(old)SD.rename(backup.c_str(),savePath_.c_str());
      status_="Save commit failed";return false;
    }
    if(!verifyFile(savePath_)){
      status_="Final save verify failed";return false;
    }
    ramDirty_=false;return true;
  }
  void startSelected(){
    if(!sdReady_||chosen_<0||chosen_>=count_)return;
    release();
    ioFault_=cpuFault_=false;saveWritable_=false;
    currentName_=entries_[chosen_].name;
    const String path=String(RomDir)+"/"+currentName_;
    rom_=SD.open(path.c_str(),FILE_READ);
    if(!rom_ || rom_.isDirectory() || rom_.size()!=entries_[chosen_].size){
      status_="ROM missing or changed";release();return;
    }
    uint8_t header[suite::gb18::HeaderSize]={};
    if(rom_.read(header,sizeof(header))!=static_cast<int>(sizeof(header))){
      status_="ROM header read failed";release();return;
    }
    suite::gb18::Info info;
    const auto checked=suite::gb18::inspect(header,sizeof(header),rom_.size(),info);
    if(checked!=suite::gb18::Error::Ok || info.colorOnly ||
       info.romSizeCode>8 || info.ramSizeCode>5){
      status_=info.colorOnly?"GBC-only; use future CGB app":"ROM header/size unsupported";
      release();return;
    }
    romBytes_=info.expectedBytes;
    for(int i=0;i<2;++i){
      cache_[i]=static_cast<uint8_t*>(malloc(16384));
      if(!cache_[i]){
        status_="Insufficient RAM for ROM banks";release();return;
      }
    }
    const enum gb_init_error_e err=gb_init(&gb_,readRom,readRam,writeRam,coreError,this);
    if(err!=GB_INIT_NO_ERROR || ioFault_){
      status_="Game Boy mapper or ROM rejected";release();return;
    }
    size_t ramNeeded=0;
    if(gb_get_save_size_s(&gb_,&ramNeeded)!=0 || ramNeeded>MaxRam){
      status_="Cartridge RAM unsupported";release();return;
    }
    if(ramNeeded){
      cartRam_=static_cast<uint8_t*>(malloc(ramNeeded));
      if(!cartRam_){
        status_="Insufficient cartridge RAM";release();return;
      }
      cartBytes_=ramNeeded;
      memset(cartRam_,0xff,cartBytes_);
    }
    // Only battery-backed SRAM is persisted. Volatile RAM still works in-game.
    if(cartBytes_ && batteryType(info.cartridgeType)){
      if(!SD.exists("/SUITE/SAVES"))SD.mkdir("/SUITE/SAVES");
      if(!SD.exists(SaveDir))SD.mkdir(SaveDir);
      if(!SD.exists(SaveDir)){
        status_="Save folder unavailable";release();return;
      }
      savePath_=String(SaveDir)+"/"+currentName_+".SAV";
      if(SD.exists(savePath_)){
        File f=SD.open(savePath_.c_str(),FILE_READ);
        bool ok=f && !f.isDirectory() && f.size()==cartBytes_;
        if(ok)ok=f.read(cartRam_,cartBytes_)==static_cast<int>(cartBytes_);
        if(f)f.close();
        if(!ok){
          status_="Invalid existing .SAV protected";release();return;
        }
      }
      // An old interrupted .NEW cannot be overwritten automatically.
      saveWritable_=!SD.exists(savePath_+".NEW");
      if(!saveWritable_){
        status_="Interrupted save .NEW needs recovery";release();return;
      }
    }
    gb_init_lcd(&gb_,scanline);
    gb_.direct.joypad=0xff;
    memset(screen_,0,sizeof(screen_));
    playing_=true;frameReady_=false;dirty_=true;frameCount_=0;
    nextFrameAt_=millis();
    status_=String("Playing ")+info.title;
    Serial.println(status_);
  }
  void stopWithError(const char* error){
    Serial.println(error);
    release();
    status_=error;
    dirty_=true;
  }
public:
  void begin(){
    release();
    ioFault_=cpuFault_=false;
    auto& display=M5Cardputer.Display;
    display.setRotation(1);
    SPI.begin(40,39,14,12);
    sdReady_=SD.begin(12,SPI,10000000);
    if(sdReady_){
      if(!SD.exists("/SUITE"))SD.mkdir("/SUITE");
      if(!SD.exists("/SUITE/ROMS"))SD.mkdir("/SUITE/ROMS");
      if(!SD.exists(RomDir))SD.mkdir(RomDir);
    }
    scan();
    dirty_=true;
  }
  void exit(){
    if(playing_){
      if(!saveRam()){
        // The main suite reset handler is outside this app. Always preserve
        // the .NEW and old .SAV for manual recovery if committing failed.
        Serial.println("Game Boy: save not committed; recovery files preserved");
      }
    }
    release();
  }
  bool dirty()const{return dirty_;}
  void input(const Keyboard_Class::KeysState& keys){
    if(playing_)return; // Hold-state input is sampled in poll().
    if(keys.up&&chosen_>0)--chosen_;
    if(keys.down&&chosen_+1<count_)++chosen_;
    if(letter(keys,'r'))scan();
    if(keys.enter && count_>0)startSelected();
    dirty_=true;
  }
  void poll(){
    if(!playing_)return;
    const uint32_t now=millis();
    if(static_cast<int32_t>(now-nextFrameAt_)<0)return;
    nextFrameAt_=now+17;
    gb_.direct.joypad=controls(M5Cardputer.Keyboard.keysState());
    gb_.gb_frame=false;
    const uint32_t start=millis();
    unsigned steps=0;
    while(!gb_.gb_frame&&!ioFault_&&!cpuFault_ && steps<MaxStepsPerFrame){
      __gb_step_cpu(&gb_);++steps;
      if((steps&511u)==0){
        yield();
        if(millis()-start>250)break;
      }
    }
    if(!gb_.gb_frame||ioFault_||cpuFault_){
      stopWithError(ioFault_?"SD cartridge read failed":
                    cpuFault_?"Game Boy CPU fault":"Game Boy frame budget exhausted");
      return;
    }
    frameReady_=true;dirty_=true;++frameCount_;
  }
  void render(){
    if(playing_){
      if(!frameReady_)return;
      M5Cardputer.Display.pushImage(45,0,150,135,screen_);
      frameReady_=false;dirty_=false;return;
    }
    auto& d=M5Cardputer.Display;
    d.fillScreen(0x1062);d.fillRect(0,0,240,21,0x18c3);
    d.setTextColor(0xffff,0x18c3);d.setCursor(5,7);
    d.print("GAME BOY / DMG");
    d.setTextColor(0xad55,0x1062);
    d.setCursor(5,25);d.print("SD /SUITE/ROMS/GB");
    if(!sdReady_){
      d.setCursor(5,49);d.print("Insert FAT32 microSD");
    }else if(!count_){
      d.setCursor(5,49);d.print("No games found; add .GB files");
    }else{
      const int first=chosen_>=6?chosen_-5:0;
      for(int i=first;i<count_&&i<first+6;++i){
        const int y=43+(i-first)*11;
        if(i==chosen_)d.fillRect(2,y-1,235,11,0x2945);
        d.setTextColor(0xffff,i==chosen_?0x2945:0x1062);
        d.setCursor(5,y);d.print(entries_[i].name);
      }
    }
    d.fillRect(0,116,240,19,0x18c3);
    d.setTextColor(0xc638,0x18c3);d.setCursor(4,121);
    d.print("Enter play  R rescan  Esc home");
    dirty_=false;
    Serial.println(status_);
  }
};
