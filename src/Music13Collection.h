#pragma once
#include "core/Theme16Runtime.h"
// P13 original K132 offline music production. Four-voice step sequencer,
// drum machine, chord dictionary, arpeggiator, MIDI-like preset audition,
// and a five-letter Morse trainer. No external MIDI/sound bank or PSRAM.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include <esp_system.h>
#include "core/Music13Core.h"
#include "core/MusicLabCore.h"
#include "midi/Synth.h"
class Music13Collection {
public:
 void begin(){
   mode_=-1;menu_=0;dirty_=true;running_=false;
   M5Cardputer.Mic.end();M5Cardputer.Speaker.end();
   SPI.begin(40,39,14,12);sd_=SD.begin(12,SPI,10000000);
   if(sd_){
     if(!SD.exists("/SUITE"))SD.mkdir("/SUITE");
     if(!SD.exists("/SUITE/MUSIC"))SD.mkdir("/SUITE/MUSIC");
   }
   status_=sd_?"Ready":"SD missing: playback only";
 }
 void exit(){stopAudio();M5Cardputer.Speaker.end();}
 void poll(){
   const uint32_t now=millis();
   if(mode_==4&&running_){
     if(static_cast<int32_t>(now-nextMs_)>=0)morseStep(now);
     return;
   }
   if(running_&&static_cast<int32_t>(now-nextMs_)>=0){
     if(mode_==0||mode_==1)playPatternStep();
     else if(mode_==2)playArpeggio();
     nextMs_=now+(mode_==0||mode_==1?
       15000u/sequence_.bpm:60000u/bpm_);
     dirty_=true;
   }
   if(auditionUntil_&&static_cast<int32_t>(now-auditionUntil_)>=0){
     auditionUntil_=0;
     if(!running_){stopAudio();dirty_=true;}
   }
   if(audioOn_&&(running_||auditionUntil_)){
     for(int i=0;i<2;++i){
       if(M5Cardputer.Speaker.isPlaying(0)>=2)break;
       int16_t*pcm=audio_[audioIndex_];
       synth_.render(pcm,256);
       if(!M5Cardputer.Speaker.playRaw(pcm,256,
               suite::midi::Synth::SampleRate,false,1,0,false))break;
       audioIndex_=(audioIndex_+1)%3;
     }
   }
 }
 bool dirty()const{return dirty_;}
 void input(const Keyboard_Class::KeysState&k){
   if(mode_<0){
     if(k.up&&menu_>0)--menu_;
     if(k.down&&menu_<5)++menu_;
     if(k.enter)open(menu_);
     dirty_=true;return;
   }
   if(letter(k,'b')){stopAudio();mode_=-1;dirty_=true;return;}
   if(mode_==0||mode_==1){
     if(k.left&&step_>0)--step_;
     if(k.right&&step_<15)++step_;
     if(k.up&&track_>0)--track_;
     if(k.down&&track_<3)++track_;
     if(k.enter){
       const unsigned prior=sequence_.get(track_,step_);
       sequence_.set(track_,step_,prior?0:
         mode_==1?drumNote(track_):60+track_*4);
     }
     if(letter(k,'z'))sequence_.set(track_,step_,0);
     if(mode_==0&&symbol(k,'+')){
       int value=sequence_.get(track_,step_);
       if(value<24)value=60;
       if(value<108)sequence_.set(track_,step_,value+1);
     }
     if(mode_==0&&symbol(k,'-')){
       const int value=sequence_.get(track_,step_);
       if(value>24)sequence_.set(track_,step_,value-1);
     }
     if(letter(k,'m'))sequence_.muted[track_]=!sequence_.muted[track_];
     if(letter(k,'v'))sequence_.program[track_]=
       static_cast<uint8_t>((sequence_.program[track_]+8)%128);
     if(letter(k,'p')){if(running_)stopAudio();else startAudio();}
     if(letter(k,'s'))saveSequence();
     if(letter(k,'l'))loadSequence();
   }else if(mode_==2||mode_==3){
     if(k.left&&root_>0)--root_;
     if(k.right&&root_<11)++root_;
     if(k.up&&quality_+1<static_cast<unsigned>(suite::music13::Quality::Count))
       ++quality_;
     if(k.down&&quality_>0)--quality_;
     if(mode_==2&&letter(k,'t'))style_=(style_+1)%3;
     if(mode_==2&&letter(k,'p')){
       if(running_)stopAudio();else startAudio();
     }
     if(mode_==3&&k.enter)auditionChord();
   }else if(mode_==4){
     if(letter(k,'n')){quiz_.newQuestion(esp_random());revealed_=false;}
     if(k.enter)revealed_=!revealed_;
     if(letter(k,'p'))playMorse();
   }else{
     if(k.up&&program_<127)++program_;
     if(k.down&&program_>0)--program_;
     if(k.left&&key_>36)--key_;
     if(k.right&&key_<96)++key_;
     if(k.enter)auditionPatch();
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
        sel=p.selected,
        green=p.good,
        dim=p.muted;
   d.fillScreen(bg);d.fillRect(0,0,240,23,head);
   d.setTextColor(white,head);d.setCursor(5,7);
   d.print(mode_<0?"MUSIC PRODUCTION":names(mode_));
   d.setTextColor(cyan,head);d.setCursor(207,7);d.print("SD");
   d.fillRect(0,116,240,19,head);
   d.setTextColor(dim,head);d.setCursor(4,121);
   d.print(mode_<0?"Fn arrows / Enter":
     mode_<=1?"P play S save L load M mute B":
     mode_==2?"P arp T style B menu":
     mode_==3?"Enter chord Arrows choose B":
     mode_==4?"N quiz P sound Enter reveal B":
     "Arrows patch/note Enter sound B");
   if(mode_<0){
     for(int i=0;i<6;++i){
       int y=29+i*14;
       if(i==menu_)d.fillRect(3,y-1,233,13,sel);
       d.setTextColor(white,i==menu_?sel:bg);
       d.setCursor(8,y+2);d.print(String(i+1)+". "+names(i));
     }
     d.setTextColor(dim,bg);d.setCursor(5,108);d.print(status_);
     return;
   }
   if(mode_<=1){
     d.setTextColor(cyan,bg);d.setCursor(5,27);
     d.print(String("BPM ")+sequence_.bpm+(running_?" PLAY":" STOP")+
             " Step "+(step_+1));
     for(int track=0;track<4;++track){
       const int y=45+track*15;
       d.setTextColor(white,bg);d.setCursor(4,y);
       d.print(String(track+1)+(sequence_.muted[track]?"x":" "));
       for(int step=0;step<16;++step){
         const int px=31+step*12;
         const bool active=sequence_.get(track,step)!=0;
         const uint16_t back=track==track_&&step==step_?cyan:
           running_&&step==playStep_?green:active?sel:0x2124;
         d.fillRect(px,y-2,10,11,back);
         d.setTextColor(white,back);d.setCursor(px+2,y);
         d.print(active?"#":".");
       }
     }
     d.setTextColor(dim,bg);d.setCursor(5,109);
     d.print(status_.substring(0,37));
   }else if(mode_==2||mode_==3){
     const auto q=static_cast<suite::music13::Quality>(quality_);
     uint8_t notes[4]={};unsigned n=0;
     suite::music13::chord(root_,q,notes,n);
     d.setTextColor(cyan,bg);d.setCursor(5,30);
     d.print(String(suite::music13::noteName(root_))+
           " "+suite::music13::qualityName(q));
     d.setTextColor(white,bg);d.setCursor(5,52);
     for(unsigned i=0;i<n;++i){
       d.print(String(suite::music13::noteName(notes[i]))+
         "("+notes[i]+") ");
     }
     d.setCursor(5,75);
     if(mode_==2)d.print(String("Pattern ")+
       (style_==0?"Up":style_==1?"Down":"UpDown")+
       "  "+(running_?"PLAYING":"STOP"));
     else d.print("Enter to audition chord");
     d.setCursor(5,101);d.print("Approximate waveform synth");
   }else if(mode_==4){
     d.setTextColor(cyan,bg);d.setCursor(5,29);
     d.print("Listen to the five-letter Morse");
     d.setTextColor(white,bg);d.setCursor(5,55);
     d.print(revealed_?quiz_.word():"? ? ? ? ?");
     d.setCursor(5,84);d.print(running_?"Sounding dots/dashes":"Press P to hear group");
   }else{
     d.setTextColor(cyan,bg);d.setCursor(5,29);
     d.print(String("Program ")+program_+" (GM family)");
     d.setTextColor(white,bg);d.setCursor(5,54);
     d.print(String("Pitch ")+key_+" "+suite::music13::noteName(key_));
     d.setCursor(5,78);d.print("No SoundFont samples required");
     d.setCursor(5,97);d.print("Enter to audition");
   }
 }
private:
 static const char*names(int i){
   static const char* const n[]={
     "4-Track Step Tracker","16-Step Drum Machine","Arpeggiator",
     "Chord Dictionary","Morse Quiz Trainer","Instrument Presets"};
   return n[i];
 }
 static uint8_t drumNote(unsigned track){
   static const uint8_t drum[]={36,38,42,46};return drum[track&3];
 }
 static bool letter(const Keyboard_Class::KeysState&k,char c){
   for(char v:k.word)if(v==c||v==(c-'a'+'A'))return true;return false;
 }
 static bool symbol(const Keyboard_Class::KeysState&k,char c){
   for(char v:k.word)if(v==c)return true;return false;
 }
 int mode_=-1,menu_=0,track_=0,step_=0,playStep_=0,program_=0,key_=60;
 unsigned root_=0,quality_=0,style_=0,bpm_=120;
 bool sd_=false,dirty_=true,running_=false,seenAudio_=false,revealed_=false,
      audioOn_=false;
 String status_;
 uint32_t nextMs_=0,auditionUntil_=0;
 int16_t audio_[3][256]={};int audioIndex_=0;
 uint8_t lastNote_[4]={};
 char morse_[128]={};size_t morsePos_=0;
 suite::music13::Sequence sequence_;
 suite::music13::Arpeggiator arp_;
 suite::music13::MorseQuiz quiz_;
 suite::midi::Synth synth_;
 void open(int m){
   stopAudio();mode_=m;dirty_=true;status_="";
   track_=step_=playStep_=0;root_=quality_=style_=0;program_=0;key_=60;
   if(m==0){sequence_.clear();loadSequence();}
   if(m==1){sequence_.drumPreset();loadSequence();}
   if(m==4){quiz_.newQuestion(esp_random());revealed_=false;}
 }
 void stopAudio(){
   running_=false;auditionUntil_=0;audioOn_=false;
   synth_.allSoundOff();
   M5Cardputer.Speaker.stop(0);M5Cardputer.Speaker.end();
 }
 bool readySpeaker(){
   if(audioOn_)return true;
   M5Cardputer.Mic.end();
   if(!M5Cardputer.Speaker.begin()){status_="Speaker init failed";return false;}
   M5Cardputer.Speaker.setVolume(suite::skin16::current().active().volume);
   synth_.reset();
   for(int i=0;i<4;++i)lastNote_[i]=0;
   audioOn_=true;audioIndex_=0;return true;
 }
 void startAudio(){
   if(!readySpeaker())return;
   running_=true;playStep_=0;nextMs_=millis();
   arp_.reset();status_="Playing";
 }
 static suite::smf::Event event(suite::smf::Kind kind,uint8_t ch,
                                 uint8_t note,uint8_t value,uint8_t track){
   suite::smf::Event e;e.kind=kind;e.channel=ch;
   e.a=note;e.b=value;e.track=track;return e;
 }
 void playPatternStep(){
   for(int tr=0;tr<4;++tr){
     const int ch=mode_==1?9:tr;
     if(lastNote_[tr]){
       synth_.event(event(suite::smf::Kind::NoteOff,ch,lastNote_[tr],0,tr));
       lastNote_[tr]=0;
     }
     const uint8_t note=sequence_.get(tr,playStep_);
     if(note&&!sequence_.muted[tr]){
       suite::smf::Event program=event(suite::smf::Kind::Program,ch,
                                       sequence_.program[tr],0,tr);
       synth_.event(program);
       synth_.event(event(suite::smf::Kind::NoteOn,ch,note,105,tr));
       lastNote_[tr]=note;
     }
   }
   playStep_=(playStep_+1)%16;
 }
 void playArpeggio(){
   uint8_t notes[4]={};unsigned n=0;
   if(!suite::music13::chord(root_,
       static_cast<suite::music13::Quality>(quality_),notes,n))return;
   if(lastNote_[0])
     synth_.event(event(suite::smf::Kind::NoteOff,0,lastNote_[0],0,0));
   const uint8_t p=arp_.next(notes,n,style_);
   synth_.event(event(suite::smf::Kind::NoteOn,0,p,110,0));
   lastNote_[0]=p;
 }
 void auditionChord(){
   if(!readySpeaker())return;
   uint8_t notes[4]={};unsigned n=0;
   if(!suite::music13::chord(root_,
     static_cast<suite::music13::Quality>(quality_),notes,n))return;
   synth_.allSoundOff();
   for(unsigned i=0;i<n;++i)
     synth_.event(event(suite::smf::Kind::NoteOn,0,notes[i],95,0));
   auditionUntil_=millis()+650;
 }
 void auditionPatch(){
   if(!readySpeaker())return;
   synth_.allSoundOff();
   synth_.event(event(suite::smf::Kind::Program,0,program_,0,0));
   synth_.event(event(suite::smf::Kind::NoteOn,0,key_,120,0));
   auditionUntil_=millis()+650;
 }
 void playMorse(){
   if(!readySpeaker())return;
   if(!suite::musiclab::encodeMorse(quiz_.word(),morse_,sizeof(morse_))){
     status_="Morse encoding failed";return;
   }
   M5Cardputer.Speaker.stop(0);
   morsePos_=0;nextMs_=millis();running_=true;
 }
 void morseStep(uint32_t now){
   const char c=morse_[morsePos_++];
   if(!c){
     running_=false;M5Cardputer.Speaker.stop(0);status_="Quiz sound ended";
     dirty_=true;return;
   }
   const unsigned len=c=='.'?100:c=='-'?300:c=='/'?400:150;
   if(c=='.'||c=='-')M5Cardputer.Speaker.tone(820,len,0,true);
   nextMs_=now+len+30;dirty_=true;
 }
 const char*filePath()const {
   return mode_==1?"/SUITE/MUSIC/DRUMS.SEQ":"/SUITE/MUSIC/TRACK.SEQ";
 }
 void loadSequence(){
   if(!sd_){status_="No SD; unsaved pattern";return;}
   File f=SD.open(filePath(),FILE_READ);
   if(!f){status_="Default pattern";return;}
   if(f.isDirectory()||f.size()!=suite::music13::Sequence::Encoded){
     f.close();status_="Invalid pattern; protected";return;
   }
   uint8_t buffer[suite::music13::Sequence::Encoded]={};
   const bool ok=f.read(buffer,sizeof(buffer))==sizeof(buffer);f.close();
   if(!ok||!sequence_.decode(buffer,sizeof(buffer))){
     status_="Invalid pattern; protected";return;
   }
   status_="SD pattern loaded";
 }
 void saveSequence(){
   if(!sd_){status_="Insert SD to save";return;}
   const char*path=filePath();
   if(SD.exists(path)){
     File input=SD.open(path,FILE_READ);
     uint8_t old[suite::music13::Sequence::Encoded]={};
     suite::music13::Sequence prior;
     const bool valid=input&&!input.isDirectory()&&
       input.size()==sizeof(old)&&input.read(old,sizeof(old))==sizeof(old)&&
       prior.decode(old,sizeof(old));
     if(input)input.close();
     if(!valid){status_="Invalid old pattern; no overwrite";return;}
   }
   uint8_t buffer[suite::music13::Sequence::Encoded]={};
   if(!sequence_.encode(buffer,sizeof(buffer)))return;
   const String tmp=String(path)+".NEW",bak=String(path)+".BAK";
   if(SD.exists(tmp.c_str())&&!SD.remove(tmp.c_str())){
     status_="Remove old .NEW manually";return;
   }
   File out=SD.open(tmp.c_str(),FILE_WRITE);
   if(!out){status_="SD create failed";return;}
   const bool written=out.write(buffer,sizeof(buffer))==sizeof(buffer);
   out.flush();out.close();
   if(!written){status_="SD short write";return;}
   const bool existed=SD.exists(path);
   if(existed){
     if(SD.exists(bak.c_str())&&!SD.remove(bak.c_str()))return;
     if(!SD.rename(path,bak.c_str()))return;
   }
   if(!SD.rename(tmp.c_str(),path)){
     if(existed)SD.rename(bak.c_str(),path);
     status_="Save rename failed";return;
   }
   status_="Pattern saved to SD";
 }
};
