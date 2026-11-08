#pragma once
// Four stock Cardputer K132 utility apps, no SD, Wi-Fi, RTC or Grove required.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <stdio.h>
#include <string.h>
#include "core/PocketTools.h"

class PocketApps {
public:
    enum class Mode:uint8_t {Stopwatch,Countdown,Units,Programmer};
    void enter(Mode m) {
        mode_=m;alert_=false;status_="";
    }
    void poll(uint32_t now) {
        if(mode_==Mode::Countdown && countdown_.poll(now)) {
            alert_=true;status_="Timer completed!";
        }
    }
    void onKey(const Keyboard_Class::KeysState& k,uint32_t now) {
        switch(mode_) {
            case Mode::Stopwatch:
                if(letter(k,'s') || k.enter) {
                    if(stopwatch_.running())stopwatch_.pause(now);
                    else stopwatch_.start(now);
                } else if(letter(k,'l'))stopwatch_.lap(now);
                else if(letter(k,'r')) {
                    if(!stopwatch_.running())stopwatch_.reset();
                    else status_="Pause before reset";
                }
                break;
            case Mode::Countdown:
                timerInput(k,now);break;
            case Mode::Units:
                unitInput(k);break;
            case Mode::Programmer:
                programmerInput(k);break;
        }
    }
    void draw(uint32_t now) const {
        auto &d=M5Cardputer.Display;
        constexpr uint16_t bg=0x0841,panel=0x10c4,selected=0x2673;
        constexpr uint16_t white=0xffff,cyan=0x05ff,green=0x5fe9,muted=0xad55;
        d.fillScreen(bg);
        d.fillRect(0,0,240,23,panel);
        d.setTextColor(white,panel);d.setCursor(5,7);
        switch(mode_) {
            case Mode::Stopwatch:d.print("STOPWATCH");break;
            case Mode::Countdown:d.print("COUNTDOWN");break;
            case Mode::Units:d.print("UNIT CONVERTER");break;
            case Mode::Programmer:d.print("PROGRAMMER");break;
        }
        d.setTextColor(cyan,panel);d.setCursor(207,7);d.print("P3");
        d.fillRect(0,116,240,19,panel);
        d.setTextColor(muted,panel);d.setCursor(4,121);
        switch(mode_) {
            case Mode::Stopwatch:d.print("S start/pause L lap R reset");break;
            case Mode::Countdown:d.print("S run/pause R reset HHMMSS Enter");break;
            case Mode::Units:d.print("Up/down unit | Enter result | C clear");break;
            case Mode::Programmer:d.print("H base O op T field R clear");break;
        }
        auto line=[&d,bg](int x,int y,const String& value,uint16_t col=0xffff) {
            d.setTextColor(col,bg);d.setCursor(x,y);d.print(value.substring(0,39));
        };
        char tm[36];
        if(mode_==Mode::Stopwatch) {
            duration(stopwatch_.elapsed(now),tm,sizeof(tm));
            d.setTextSize(2);
            d.setTextColor(green,bg);d.setCursor(6,31);d.print(tm);
            d.setTextSize(1);
            line(6,57,stopwatch_.running()?"RUNNING":"PAUSED",cyan);
            line(6,69,"Laps (last eight):",muted);
            for(size_t i=0;i<stopwatch_.lapCount() && i<3;++i) {
                const size_t index=stopwatch_.lapCount()-i-1;
                duration(stopwatch_.lapAt(index),tm,sizeof(tm));
                line(6,81+static_cast<int>(i)*11,
                    String("#")+String(index+1)+"  "+tm,white);
            }
            if(status_.length())line(137,57,status_,muted);
            return;
        }
        if(mode_==Mode::Countdown) {
            duration(countdown_.left(now),tm,sizeof(tm));
            d.setTextSize(2);
            d.setTextColor(alert_?cyan:green,bg);
            d.setCursor(6,31);d.print(tm);
            d.setTextSize(1);
            line(6,56,countdown_.running()?"RUNNING":countdown_.finished()?
                        "FINISHED":"PAUSED",cyan);
            line(6,70,"New HHMMSS: "+String(inputTime_));
            line(6,85,"Examples 0130=1m30s",muted);
            line(6,99,status_,alert_?green:muted);
            return;
        }
        if(mode_==Mode::Units) {
            const auto unit=static_cast<suite::pocket::Units>(unitIndex_);
            const auto p=suite::pocket::pair(unit);
            line(6,27,String(unitIndex_+1)+"/16  "+p.title,cyan);
            line(6,45,String("Input (")+p.from+"): "+inputUnit_);
            double value=0,result=0;
            if(suite::pocket::parseNumber(inputUnit_,value)) {
                if(suite::pocket::convert(unit,value,result))
                    line(6,69,String("= ")+String(result,6)+" "+p.to,green);
                else line(6,69,"Conversion out of range",muted);
            } else line(6,69,"Type a number",muted);
            line(6,90,"Arrow keys change conversion",muted);
            line(6,102,status_,muted);
            return;
        }
        constexpr uint8_t bases[4]={2,8,10,16};
        const uint8_t base=bases[baseIndex_];
        line(6,26,String("Base ")+base+"   Op "+suite::pocket::bitName(op_),cyan);
        for(int i=0;i<2;++i) {
            const int y=41+i*16;
            const String value=String(i==field_?">":" ")+
                     (i==0?"A: ":"B: ")+operands_[i];
            if(i==field_)d.fillRect(3,y-1,233,14,selected);
            d.setTextColor(white,i==field_?selected:bg);
            d.setCursor(6,y);d.print(value.substring(0,37));
        }
        uint32_t a=0,b=0,result=0;
        if(suite::pocket::parseUnsigned(operands_[0],base,a) &&
           (op_==suite::pocket::BitOp::Not ||
            suite::pocket::parseUnsigned(operands_[1],base,b))) {
            if(suite::pocket::bitwise(op_,a,b,result)) {
                char hex[33],dec[33],bin[33];
                suite::pocket::formatUnsigned(result,16,hex,sizeof(hex));
                suite::pocket::formatUnsigned(result,10,dec,sizeof(dec));
                suite::pocket::formatUnsigned(result,2,bin,sizeof(bin));
                line(6,77,String("HEX ")+hex+" DEC "+dec,green);
                line(6,91,String("BIN ")+bin,white);
            } else line(6,77,"Shift count must be 0-31",muted);
        } else line(6,77,"Enter A and B, T switches field",muted);
        line(6,104,status_,muted);
    }

    Mode mode() const {return mode_;}
private:
    Mode mode_=Mode::Stopwatch;
    suite::pocket::Stopwatch stopwatch_;
    suite::pocket::Countdown countdown_;
    bool alert_=false;
    char inputTime_[7]={},inputUnit_[32]="1",operands_[2][34]={{0},{0}};
    uint8_t unitIndex_=0,baseIndex_=2,field_=0;
    suite::pocket::BitOp op_=suite::pocket::BitOp::And;
    String status_;
    static bool symbol(const Keyboard_Class::KeysState& k,char c) {
        for(char v:k.word)if(v==c)return true;
        return false;
    }
    static bool letter(const Keyboard_Class::KeysState& k,char c) {
        return symbol(k,c)||symbol(k,static_cast<char>(c-'a'+'A'));
    }
    static bool add(char* out,size_t cap,char c) {
        const size_t len=strlen(out);
        if(len+1>=cap)return false;
        out[len]=c;out[len+1]=0;return true;
    }
    static void backspace(char* out) {
        const size_t len=strlen(out);
        if(len)out[len-1]=0;
    }
    static void duration(uint64_t milliseconds,char* out,size_t cap) {
        const uint64_t centiseconds=milliseconds/10;
        const uint32_t h=static_cast<uint32_t>(centiseconds/360000);
        const uint32_t m=static_cast<uint32_t>((centiseconds/6000)%60);
        const uint32_t s=static_cast<uint32_t>((centiseconds/100)%60);
        const uint32_t cs=static_cast<uint32_t>(centiseconds%100);
        snprintf(out,cap,"%02lu:%02lu:%02lu.%02lu",
                 static_cast<unsigned long>(h),
                 static_cast<unsigned long>(m),
                 static_cast<unsigned long>(s),
                 static_cast<unsigned long>(cs));
    }
    void timerInput(const Keyboard_Class::KeysState& k,uint32_t now) {
        if(letter(k,'s')) {
            if(countdown_.running())countdown_.pause(now);
            else countdown_.start(now);
            alert_=false;status_="";
        } else if(letter(k,'r')) {
            countdown_.reset();alert_=false;status_="";
        } else if(k.backspace)backspace(inputTime_);
        else if(k.enter) {
            uint32_t seconds=0;
            if(!suite::pocket::parseHHMMSS(inputTime_,seconds) ||
               !countdown_.setSeconds(seconds))status_="Use HHMMSS (valid seconds/minutes)";
            else {inputTime_[0]=0;status_="Timer set";alert_=false;}
        } else if(!k.ctrl && !k.fn && !k.alt)
            for(char c:k.word)if(c>='0'&&c<='9')add(inputTime_,sizeof(inputTime_),c);
    }
    void unitInput(const Keyboard_Class::KeysState& k) {
        if(k.up) {
            unitIndex_=(unitIndex_+15)%16;
            status_="";
        } else if(k.down) {
            unitIndex_=(unitIndex_+1)%16;
            status_="";
        } else if(k.backspace)backspace(inputUnit_);
        else if(letter(k,'c'))inputUnit_[0]=0;
        else if(k.enter)status_="Calculated above";
        else if(!k.ctrl && !k.fn && !k.alt) {
            for(char c:k.word)if((c>='0'&&c<='9')||c=='.'||c=='-')
                add(inputUnit_,sizeof(inputUnit_),c);
        }
    }
    void programmerInput(const Keyboard_Class::KeysState& k) {
        constexpr uint8_t bases[4]={2,8,10,16};
        if(letter(k,'h')) {
            char converted[2][34]={};
            const uint8_t previous=bases[baseIndex_];
            const uint8_t target=bases[(baseIndex_+1)%4];
            for(int i=0;i<2;++i) {
                if(!operands_[i][0])continue;
                uint32_t value=0;
                if(!suite::pocket::parseUnsigned(operands_[i],previous,value) ||
                   !suite::pocket::formatUnsigned(value,target,converted[i],34)) {
                    status_="Invalid number, base not changed";
                    return;
                }
            }
            baseIndex_=(baseIndex_+1)%4;
            for(int i=0;i<2;++i)strcpy(operands_[i],converted[i]);
            status_="";
        } else if(letter(k,'o')) {
            op_=static_cast<suite::pocket::BitOp>(
                (static_cast<int>(op_)+1)%
                static_cast<int>(suite::pocket::BitOp::Count));
            status_="";
        } else if(letter(k,'t'))field_=1-field_;
        else if(letter(k,'r')) {
            operands_[0][0]=operands_[1][0]=0;status_="";
        } else if(k.backspace)backspace(operands_[field_]);
        else if(k.enter)status_="Result displayed";
        else if(!k.ctrl && !k.fn && !k.alt) {
            for(char c:k.word) {
                uint32_t parsed=0;
                char digit[2]={c,0};
                if(suite::pocket::parseUnsigned(digit,bases[baseIndex_],parsed))
                    add(operands_[field_],sizeof(operands_[field_]),c);
            }
        }
    }
};
