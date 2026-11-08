#pragma once
// Five SD-stored music-practice utilities: Morse Player, Tone Lab,
// Drum Sequencer, Interval Ear Trainer and Tap Tempo.
// Only internal speaker and keyboard, no external MIDI/Grove module.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <esp_system.h>
#include "core/MusicLabCore.h"
#include "core/FieldTools.h"
class MusicCollection {
public:
    void begin(){
        M5Cardputer.Speaker.begin();M5Cardputer.Speaker.setVolume(42);
        mode_=-1;menu_=0;status_="Pick a music tool";dirty_=true;
    }
    void exit(){M5Cardputer.Speaker.stop(0);running_=false;}
    bool dirty()const{return dirty_;}
    void poll(){
        if(!running_)return;
        const uint32_t now=millis();
        if(static_cast<int32_t>(now-next_)<0)return;
        if(mode_==0) {
            const char c=sequence_[seqPos_];
            if(!c) {running_=false;status_="Morse finished";dirty_=true;return;}
            if(c=='.'||c=='-'){
                const unsigned duration=c=='.'?100:300;
                M5Cardputer.Speaker.tone(820,duration,0,true);
                next_=now+duration+100;
            } else next_=now+(c=='/'?550:200);
            ++seqPos_;dirty_=true;
        } else if(mode_==1) {
            M5Cardputer.Speaker.tone(freq_,330,0,true);
            next_=now+350;
        } else if(mode_==2) {
            const uint8_t mask=steps_[beatStep_];
            if(mask&1)M5Cardputer.Speaker.tone(105,60,0,true);
            if(mask&2)M5Cardputer.Speaker.tone(215,60,1,true);
            if(mask&4)M5Cardputer.Speaker.tone(840,28,2,true);
            beatStep_=(beatStep_+1)%16;
            next_=now+60000u/(static_cast<uint32_t>(bpm_)*4u);
            dirty_=true;
        } else if(mode_==3) {
            if(quizPhase_==0) {
                M5Cardputer.Speaker.tone(
                    static_cast<float>(suite::fieldtools::midiHz(first_)),350,0,true);
                quizPhase_=1;next_=now+520;
            } else {
                M5Cardputer.Speaker.tone(
                    static_cast<float>(suite::fieldtools::midiHz(second_)),350,0,true);
                quizPhase_=2;running_=false;dirty_=true;
            }
        }
    }
    void input(const Keyboard_Class::KeysState& k) {
        if(mode_==0 && typing_){
            if(k.backspace) {
                const size_t n=strlen(message_);
                if(n)message_[n-1]=0;
            } else if(k.enter){
                if(!suite::musiclab::encodeMorse(message_,sequence_,sizeof(sequence_))){
                    status_="Message too long/invalid";
                } else {
                    seqPos_=0;typing_=false;running_=true;
                    next_=millis();status_="Playing Morse";
                }
            }else if(!k.ctrl&&!k.fn&&!k.alt) {
                for(char c:k.word){
                    if(!suite::musiclab::morse(c)&&c!=' ')continue;
                    const size_t n=strlen(message_);
                    if(n+1<sizeof(message_)){message_[n]=c;message_[n+1]=0;}
                }
            }
            dirty_=true;return;
        }
        if(mode_<0){
            if(k.up||letter(k,'w'))menu_=(menu_+4)%5;
            if(k.down||letter(k,'s'))menu_=(menu_+1)%5;
            if(k.enter)open(menu_);
            dirty_=true;return;
        }
        if(letter(k,'b')) {
            running_=false;M5Cardputer.Speaker.stop(0);
            mode_=-1;dirty_=true;return;
        }
        if(mode_==0) {
            if(letter(k,'n')){
                running_=false;typing_=true;message_[0]=0;
                M5Cardputer.Speaker.stop(0);
            } else if(letter(k,'r')&&sequence_[0]) {
                seqPos_=0;running_=true;next_=millis();
            }
        } else if(mode_==1) {
            if(k.up&&freq_<2980)freq_+=20;
            if(k.down&&freq_>120)freq_-=20;
            if(k.left&&freq_>110)freq_-=10;
            if(k.right&&freq_<2990)freq_+=10;
            if(k.enter||letter(k,'s')) {
                running_=!running_;
                if(running_)next_=millis();
                else M5Cardputer.Speaker.stop(0);
            }
        } else if(mode_==2) {
            if(k.up&&lane_>0)--lane_;
            if(k.down&&lane_<2)++lane_;
            if(k.left)step_=(step_+15)%16;
            if(k.right)step_=(step_+1)%16;
            if(k.enter)steps_[step_]^=static_cast<uint8_t>(1u<<lane_);
            if(letter(k,'s')) {
                running_=!running_;
                if(running_)next_=millis();
                else M5Cardputer.Speaker.stop(0);
            }
            if(letter(k,'r'))patternReset();
            if(letter(k,'+')){if(bpm_<220)bpm_+=5;}
            if(letter(k,'-')){if(bpm_>40)bpm_-=5;}
        } else if(mode_==3) {
            if(letter(k,'n'))newQuiz();
            if(k.enter) {
                quizPhase_=0;running_=true;next_=millis();
            }
            if(quizPhase_==2&&(letter(k,'h')||letter(k,'l'))) {
                const bool higher=letter(k,'h');
                if(higher==(second_>first_))++correct_;
                ++asked_;newQuiz();
            }
        } else if(mode_==4) {
            if(k.enter||letter(k,'t')||space(k))tap_.tap(millis());
            if(letter(k,'r'))tap_.clear();
        }
        dirty_=true;
    }
    void render(){
        dirty_=false;
        auto& d=M5Cardputer.Display;
        constexpr uint16_t bg=0x0841,head=0x10c4,sel=0x2673,
                           white=0xffff,cyan=0x05ff,green=0x5fe9,dim=0xad55;
        d.fillScreen(bg);d.fillRect(0,0,240,23,head);
        d.setTextColor(white,head);d.setCursor(5,7);
        d.print(mode_<0?"MUSIC PRACTICE":label(mode_));
        d.setTextColor(cyan,head);d.setCursor(209,7);d.print("SD");
        d.fillRect(0,116,240,19,head);
        d.setTextColor(dim,head);d.setCursor(4,121);
        d.print(mode_<0?"Enter open | arrows":mode_==0?"N type R replay | B menu":
                mode_==1?"S tone | arrows frequency | B":
                mode_==2?"S run Enter toggle R clear | B":
                mode_==3?"H higher L lower N new | B":"Tap with Enter/T | R clear | B");
        if(mode_<0){
            for(int i=0;i<5;++i) {
                const int y=29+i*16;
                if(menu_==i)d.fillRect(3,y-1,234,13,sel);
                d.setTextColor(white,menu_==i?sel:bg);
                d.setCursor(8,y+2);d.print(String(i+1)+". "+label(i));
            }
            return;
        }
        d.setTextColor(cyan,bg);d.setCursor(5,28);
        if(mode_==0) {
            d.print(typing_?"Type a message then Enter":running_?"Playing":"Stopped");
            d.setTextColor(white,bg);d.setCursor(5,47);
            d.print(String("Text: ")+message_);
            d.setCursor(5,67);d.print(String("Morse: ")+sequence_);
            d.setTextColor(dim,bg);d.setCursor(5,102);d.print(status_);
        } else if(mode_==1) {
            d.setTextSize(2);d.print(String(freq_)+" Hz");d.setTextSize(1);
            d.setCursor(6,67);d.setTextColor(white,bg);
            d.print(running_?"Tone ON":"Tone OFF");
            d.setCursor(6,90);d.print("Range 100 .. 3000 Hz");
        } else if(mode_==2) {
            d.print(String(bpm_)+" BPM  Step "+(beatStep_+1)+" / 16");
            const char* parts[]={"Kick","Snare","Hi-hat"};
            for(int lane=0;lane<3;++lane){
                d.setTextColor(lane_==lane?green:white,bg);
                d.setCursor(5,43+lane*19);d.print(parts[lane]);
                for(int st=0;st<16;++st) {
                    const int x=52+st*11;
                    const int y=42+lane*19;
                    const bool active=steps_[st]&(1u<<lane);
                    d.fillRect(x,y,9,11,step_==st&&lane_==lane?cyan:
                               active?sel:0x18c5);
                }
            }
            d.setTextColor(dim,bg);d.setCursor(5,106);
            d.print(running_?"Running; +/- adjusts tempo":"Stopped; Fn arrows edit");
        } else if(mode_==3) {
            d.print("Is the second note higher?");
            d.setTextColor(white,bg);d.setCursor(5,49);
            d.print("Enter replay, H = up, L = down");
            d.setCursor(5,71);
            d.print(String("Correct ")+correct_+" / "+asked_);
            d.setTextColor(dim,bg);d.setCursor(5,95);
            d.print(quizPhase_==2?"Ready for your answer":"Listen to the notes");
        } else {
            d.print("Tap regularly to estimate tempo");
            d.setTextSize(2);d.setTextColor(green,bg);
            d.setCursor(5,53);
            d.print(tap_.bpm()?String(tap_.bpm())+" BPM":"-- BPM");
            d.setTextSize(1);d.setTextColor(dim,bg);
            d.setCursor(5,92);
            d.print(String("Samples: ")+tap_.samples()+" (last 8)");
        }
    }
private:
    static const char* label(int idx){
        static const char* const items[]={
        "Morse Player","Tone Generator","Drum Sequencer","Ear Trainer","Tap Tempo"
    };
        return items[idx];
    }
    int mode_=-1,menu_=0,step_=0,lane_=0,beatStep_=0;
    unsigned freq_=440,bpm_=110,first_=60,second_=64,quizPhase_=0;
    unsigned correct_=0,asked_=0;
    bool running_=false,typing_=false,dirty_=true;
    uint32_t next_=0;
    uint8_t steps_[16]={};
    size_t seqPos_=0;
    char message_[48]="SOS",sequence_[256]={};
    suite::musiclab::TapTempo tap_;
    String status_;
    static bool letter(const Keyboard_Class::KeysState& k,char c){
        for(char v:k.word)if(v==c||v==(c-'a'+'A'))return true;
        return false;
    }
    static bool space(const Keyboard_Class::KeysState& k){
        for(char v:k.word)if(v==' ')return true;
        return false;
    }
    void patternReset(){
        memset(steps_,0,sizeof(steps_));
        for(int s=0;s<16;++s){
            if(s%4==0)steps_[s]|=1;
            if(s==4||s==12)steps_[s]|=2;
            if(s%2==0)steps_[s]|=4;
        }
    }
    void newQuiz(){
        first_=48+esp_random()%25u;
        const int delta=1+static_cast<int>(esp_random()%7u);
        second_=static_cast<unsigned>(static_cast<int>(first_)+
                                     ((esp_random()&1u)?delta:-delta));
        quizPhase_=0;next_=millis();running_=true;
    }
    void open(int mode){
        mode_=mode;dirty_=true;running_=false;typing_=false;
        M5Cardputer.Speaker.stop(0);
        if(mode==0) {
            sequence_[0]=0;seqPos_=0;typing_=true;message_[0]=0;
            status_="Morse letters/digits";
        } else if(mode==1)freq_=440;
        else if(mode==2){patternReset();beatStep_=step_=lane_=0;}
        else if(mode==3){correct_=asked_=0;newQuiz();}
        else if(mode==4)tap_.clear();
    }
};
