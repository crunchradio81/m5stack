#pragma once
// K132 mic and speaker share GPIO43: NEVER operate them concurrently.
// Five AUDIO SD tools: bounded WAV recorder, mono WAV player, level meter,
// rough pitch tuner, 8-band spectrum. Built-in mic/speaker only.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include <math.h>
#include "core/P7Core.h"
class AudioCollection {
public:
 void begin(){
   mode_=-1;menu_=0;selected_=0;dirty_=true;active_=false;
   SPI.begin(40,39,14,12);sd_=SD.begin(12,SPI,10000000);
   if(sd_){if(!SD.exists("/SUITE"))SD.mkdir("/SUITE");
            if(!SD.exists("/SUITE/AUDIO"))SD.mkdir("/SUITE/AUDIO");}
   M5Cardputer.Mic.end();M5Cardputer.Speaker.end();
   status_=sd_?"Ready":"Insert FAT32 SD";
 }
 void exit(){stop();}
 bool dirty()const{return dirty_;}
 void poll(){
   if(!active_)return;
   if(mode_==1){feedPlayback();return;}
   if(!(mode_==0||mode_>=2))return;
   if(!M5Cardputer.Mic.isEnabled())return;
   if(!M5Cardputer.Mic.record(samples_,512,16000))return;
   uint64_t magnitude=0;
   for(int i=0;i<512;++i)magnitude+=static_cast<uint32_t>(
      samples_[i]<0?-static_cast<int32_t>(samples_[i]):samples_[i]);
   level_=static_cast<uint32_t>(magnitude/512);
   if(mode_==0){
     if(!file_||recorded_+sizeof(samples_)>4000000){
       stopRecord();status_="Saved / 4MB safety limit";return;
     }
     const size_t n=file_.write(reinterpret_cast<const uint8_t*>(samples_),
                                 sizeof(samples_));
     if(n!=sizeof(samples_)){stopRecord();status_="SD write failure";return;}
     recorded_+=static_cast<uint32_t>(n);
     if(static_cast<uint32_t>(millis()-lastFlush_)>1000){
       lastFlush_=millis();file_.flush();
     }
   }else if(mode_==3){
     pitch_=estimatePitch();
   }else if(mode_==4){
     spectrum();
   }
   dirty_=true;
 }
 void input(const Keyboard_Class::KeysState&k){
   if(mode_<0){
     if(k.up&&menu_>0)--menu_;
     if(k.down&&menu_<4)++menu_;
     if(k.enter)openMode(menu_);
     dirty_=true;return;
   }
   if(letter(k,'b')){stop();mode_=-1;dirty_=true;return;}
   if(mode_==0){
     if(letter(k,'s')||k.enter){
       if(active_)stopRecord();else startRecord();
     }
   }else if(mode_==1){
     if(k.up&&selected_>0)--selected_;
     if(k.down&&selected_+1<clips_)++selected_;
     if(letter(k,'r'))refreshClips();
     if(letter(k,'s')||k.enter){
       if(active_)stop();else startPlayback();
     }
   }else if(mode_>=2){
     if(letter(k,'s')||k.enter){
       if(active_)stop();else startMic();
     }
   }
   dirty_=true;
 }
 void render(){
   dirty_=false;auto&d=M5Cardputer.Display;
   constexpr uint16_t bg=0x0841,head=0x10c4,white=0xffff,
       cyan=0x05ff,green=0x5fe9,sel=0x2673,dim=0xad55;
   d.fillScreen(bg);d.fillRect(0,0,240,23,head);
   d.setTextColor(white,head);d.setCursor(5,7);
   d.print(mode_<0?"AUDIO LAB":names(mode_));
   d.setTextColor(cyan,head);d.setCursor(209,7);d.print("SD");
   d.fillRect(0,116,240,19,head);
   d.setTextColor(dim,head);d.setCursor(4,121);
   d.print(mode_<0?"Fn arrows / Enter":
      mode_==1?"S play/stop | R files | B menu":"S start/stop | B menu");
   if(mode_<0){
     for(int i=0;i<5;++i){
       const int y=29+i*16;
       if(i==menu_)d.fillRect(3,y-1,233,13,sel);
       d.setTextColor(white,i==menu_?sel:bg);
       d.setCursor(8,y+2);d.print(String(i+1)+". "+names(i));
     }
   }else if(mode_==0){
     d.setTextColor(cyan,bg);d.setCursor(5,29);
     d.print(active_?"RECORDING 16k mono PCM":"Recorder stopped");
     d.setTextColor(white,bg);d.setCursor(5,51);
     d.print(String("Captured: ")+recorded_/32000+" seconds");
     d.setCursor(5,69);d.print(String("Bytes: ")+recorded_);
     d.setCursor(5,87);d.print(String("Level: ")+level_);
     d.setCursor(5,102);d.print("Max 4 MB per clip");
   }else if(mode_==1){
     d.setTextColor(cyan,bg);d.setCursor(5,27);
     d.print(String(clips_)+" supported WAV files");
     for(int i=0;i<clips_&&i<5;++i){
       const int y=42+i*13;
       if(i==selected_)d.fillRect(3,y-1,233,11,sel);
       d.setTextColor(white,i==selected_?sel:bg);
       d.setCursor(5,y);d.print(clipsName_[i]);
     }
     d.setTextColor(green,bg);d.setCursor(5,109);
     d.print(active_?"PLAYING":"Stopped");
   }else{
     d.setTextColor(cyan,bg);d.setCursor(5,29);
     d.print(active_?"MIC ACTIVE":"MIC OFF");
     d.setTextColor(white,bg);d.setCursor(5,45);
     d.print(String("Amplitude: ")+level_);
     if(mode_==3){
       d.setCursor(5,65);d.print(String("Estimated frequency: ")+pitch_+" Hz");
       d.setCursor(5,87);d.print("Approximate; noisy input varies");
     }else if(mode_==2){
       const int len=level_>10000?220:level_*220/10000;
       d.fillRoundRect(7,72,len>0?len:1,16,3,green);
     }else if(mode_==4){
       for(int i=0;i<8;++i){
         const int h=bands_[i]>45?45:bands_[i];
         if(h)d.fillRect(8+i*28,105-h,23,h,sel);
       }
       d.setCursor(5,109);d.print("8 coarse spectral bands");
     }
   }
   d.setTextColor(dim,bg);d.setCursor(5,109);
   if(mode_!=1&&mode_!=4)d.print(status_.substring(0,38));
 }
private:
 static const char*names(int i){
   static const char*const labels[]={"Voice Recorder","Mono WAV Player",
     "Audio Level Meter","Guitar Pitch Estimator","Spectrum Viewer"};
   return labels[i];
 }
 int mode_=-1,menu_=0,selected_=0,clips_=0;
 bool active_=false,dirty_=true,sd_=false;
 int16_t samples_[512]={},playData_[3][512]={};
 int playIndex_=0;
 uint32_t recorded_=0,played_=0,lastFlush_=0,remaining_=0,rate_=16000;
 uint32_t level_=0;
 int pitch_=0,bands_[8]={};
 File file_;
 String status_;
 char clipsName_[16][32]={};
 static bool letter(const Keyboard_Class::KeysState&k,char c){
   for(char v:k.word)if(v==c||v==(c-'a'+'A'))return true;
   return false;
 }
 void stop(){
   if(mode_==0&&active_)stopRecord();
   else if(mode_==1){
     M5Cardputer.Speaker.stop(0);M5Cardputer.Speaker.end();
     if(file_)file_.close();
     active_=false;
   }else{
     M5Cardputer.Mic.end();active_=false;
   }
 }
 void openMode(int m){
   stop();mode_=m;dirty_=true;recorded_=played_=level_=0;
   pitch_=0;status_="";
   if(mode_==1)refreshClips();
 }
 void startMic(){
   if(!M5Cardputer.Speaker.isEnabled()){} // speaker always stopped first
   M5Cardputer.Speaker.end();
   if(!M5Cardputer.Mic.begin()){status_="Mic initialization failed";return;}
   active_=true;status_="Microphone enabled";
 }
 void startRecord(){
   if(!sd_){status_="Insert FAT32 SD";return;}
   char dest[44]={};int num=0;
   for(;num<1000;++num){
     snprintf(dest,sizeof(dest),"/SUITE/AUDIO/REC%03d.WAV",num);
     if(!SD.exists(dest))break;
   }
   if(num==1000){status_="999 recordings limit";return;}
   finalName_=dest;pendingName_=String(dest)+".NEW";
   if(SD.exists(pendingName_.c_str())){status_="Interrupted .NEW file exists";return;}
   file_=SD.open(pendingName_.c_str(),FILE_WRITE);
   if(!file_){status_="Cannot create WAV";return;}
   uint8_t header[44]={};
   suite::field::wavHeader(header,16000,0);
   if(file_.write(header,44)!=44){
     file_.close();status_="WAV header write failure";return;
   }
   recorded_=0;lastFlush_=millis();
   startMic();
   if(!active_){file_.close();return;}
   status_="Recording started";
 }
 void stopRecord(){
   M5Cardputer.Mic.end();
   active_=false;
   if(!file_){status_="No recording file";return;}
   uint8_t hdr[44]={};
   const bool hdrOk=suite::field::wavHeader(hdr,16000,recorded_);
   const bool seek=file_.seek(0);
   const bool wrote=hdrOk&&seek&&file_.write(hdr,44)==44;
   file_.flush();file_.close();
   if(!wrote){status_="Header failure; .NEW preserved";return;}
   if(SD.rename(pendingName_.c_str(),finalName_.c_str()))
     status_=String("Saved: ")+finalName_;
   else status_="Rename error; .NEW preserved";
 }
 String finalName_,pendingName_;
 void refreshClips(){
   clips_=selected_=0;
   if(!sd_){status_="No SD";return;}
   File dir=SD.open("/SUITE/AUDIO");
   if(!dir||!dir.isDirectory()){if(dir)dir.close();status_="No audio folder";return;}
   while(clips_<16){
     File f=dir.openNextFile();if(!f)break;
     String name=f.name();int slash=name.lastIndexOf('/');
     if(slash>=0)name=name.substring(slash+1);
     if(!f.isDirectory()&&suite::field::safeLeaf(name.c_str(),".WAV")&&
        name.length()<32&&f.size()>=44&&f.size()<=4000044u)
       strncpy(clipsName_[clips_++],name.c_str(),31);
     f.close();
   }
   dir.close();
   status_=clips_?"Select file, press S":"No WAV files";
 }
 void startPlayback(){
   if(!sd_||selected_>=clips_)return;
   const String path=String("/SUITE/AUDIO/")+clipsName_[selected_];
   file_=SD.open(path.c_str(),FILE_READ);
   if(!file_){status_="Open failed";return;}
   uint8_t hdr[44]={};
   if(file_.read(hdr,44)!=44){file_.close();status_="Short WAV";return;}
   suite::field::WavInfo info;
   if(!suite::field::wavParse(hdr,44,file_.size(),info)){
     file_.close();status_="Needs 8/16kHz mono PCM16";return;
   }
   M5Cardputer.Mic.end();M5Cardputer.Speaker.begin();
   M5Cardputer.Speaker.setVolume(65);
   M5Cardputer.Speaker.stop(0);
   rate_=info.rate;remaining_=info.bytes;playIndex_=0;active_=true;
   status_="Playing";
 }
 void feedPlayback(){
   if(M5Cardputer.Speaker.isPlaying(0)>=2)return;
   if(remaining_==0) {
     if(!M5Cardputer.Speaker.isPlaying(0)){stop();status_="Playback finished";dirty_=true;}
     return;
   }
   const size_t bytes=remaining_>sizeof(playData_[0])?
     sizeof(playData_[0]):remaining_;
   const size_t n=file_.read(reinterpret_cast<uint8_t*>(playData_[playIndex_]),bytes);
   if(n!=bytes){stop();status_="SD read error";dirty_=true;return;}
   if(!M5Cardputer.Speaker.playRaw(playData_[playIndex_],n/2,rate_,false,1,0)){
     // Not consumed; retry on the next poll.
     file_.seek(file_.position()-n);
     return;
   }
   remaining_-=n;played_+=n;playIndex_=(playIndex_+1)%3;dirty_=true;
 }
 int estimatePitch() {
   // AC correlation over overlapping 512-sample windows, 125–1000 Hz.
   int bestLag=0;
   double best=-1;
   for(int lag=16;lag<=128;++lag){
     int64_t dot=0,a=0,b=0;
     for(int n=0;n<512-lag;++n){
       const int32_t x=samples_[n]>>4,y=samples_[n+lag]>>4;
       dot+=static_cast<int64_t>(x)*y;
       a+=static_cast<int64_t>(x)*x;
       b+=static_cast<int64_t>(y)*y;
     }
     if(a<100000||b<100000)continue;
     const double score=static_cast<double>(dot)/sqrt(
       static_cast<double>(a)*static_cast<double>(b));
     if(score>best){best=score;bestLag=lag;}
   }
   return best>.78&&bestLag?16000/bestLag:0;
 }
 void spectrum(){
   static const unsigned freq[]={125,250,375,500,750,1000,1500,2500};
   for(int k=0;k<8;++k){
     const double omega=6.283185307179586*freq[k]/16000.0;
     const double coeff=2.0*cos(omega);
     double a=0,b=0;
     for(int i=0;i<512;++i){
       const double v=static_cast<double>(samples_[i]>>6)+coeff*a-b;
       b=a;a=v;
     }
     const double energy=sqrt(a*a+b*b-coeff*a*b)/512.0;
     bands_[k]=energy>450?45:static_cast<int>(energy/10);
   }
 }
};
