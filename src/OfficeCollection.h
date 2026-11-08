#pragma once
// Five SD-backed pocket office applications: To-do, Contacts, Expenses,
// Journal, Manual Calendar. ASCII and bounded records; no clock/GPS claims.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include <string.h>
#include "core/OfficeCore.h"
class OfficeCollection {
public:
    void begin() {
        mode_=-1;cursor_=0;selected_=0;editing_=deleting_=false;
        SPI.begin(40,39,14,12);
        sd_=SD.begin(12,SPI,10000000);
        if(sd_) {
            if(!SD.exists("/SUITE"))SD.mkdir("/SUITE");
            if(!SD.exists("/SUITE/OFFICE"))SD.mkdir("/SUITE/OFFICE");
        }
        status_=sd_?"Ready":"Insert FAT32 microSD";
        dirty_=true;
    }
    void exit(){}
    void poll(){}
    bool dirty()const{return dirty_;}
    void input(const Keyboard_Class::KeysState& k) {
        if(editing_) {
            if(k.backspace) {
                const size_t n=strlen(entry_);
                if(n)entry_[n-1]=0;
            } else if(k.enter) {
                if(mode_==2) {
                    int32_t value=0;
                    if(!suite::office::parseCents(entry_,value)){
                        status_="Enter Label 12.34";dirty_=true;return;
                    }
                }
                if(!records_.add(entry_))status_="Empty/invalid/full entry";
                else if(!save())records_.erase(records_.count()-1);
                else {
                    editing_=false;entry_[0]=0;
                    selected_=records_.count()-1;status_="Saved to SD";
                }
            } else if(!k.ctrl&&!k.fn&&!k.alt) {
                for(char c:k.word) {
                    if(c<32||c>126)continue;
                    const size_t n=strlen(entry_);
                    if(n+1>=sizeof(entry_))break;
                    entry_[n]=c;entry_[n+1]=0;
                }
            }
            dirty_=true;return;
        }
        if(deleting_) {
            if(letter(k,'y')) {
                if(records_.erase(selected_)) {
                    if(!save())status_="SD delete-save failed; reload advised";
                    else status_="Entry deleted";
                }
                if(selected_>=records_.count()&&selected_>0)--selected_;
            } else if(letter(k,'n')||k.backspace)status_="Delete canceled";
            deleting_=false;dirty_=true;return;
        }
        if(mode_<0){
            if(k.up||letter(k,'w'))cursor_=(cursor_+4)%5;
            if(k.down||letter(k,'s'))cursor_=(cursor_+1)%5;
            if(k.enter)open(cursor_);
            dirty_=true;return;
        }
        if(letter(k,'b')){mode_=-1;dirty_=true;return;}
        if(mode_==4) {
            if(k.left||letter(k,'a'))changeMonth(-1);
            if(k.right||letter(k,'d'))changeMonth(1);
            if(k.up||letter(k,'w'))changeMonth(-12);
            if(k.down||letter(k,'s'))changeMonth(12);
            dirty_=true;return;
        }
        if(k.up&&selected_>0)--selected_;
        if(k.down&&selected_+1<records_.count())++selected_;
        if(letter(k,'r'))load();
        if(letter(k,'n')) {
            if(!sd_||readOnly_)status_="Cannot write SD";
            else {editing_=true;entry_[0]=0;status_="Type and press Enter";}
        }
        if(letter(k,'d')&&records_.count()>0)deleting_=true;
        if(mode_==0&&k.enter&&records_.count()>0&&!readOnly_) {
            records_.toggle(selected_);
            if(!save())status_="Save failed; reload advised";
        }
        dirty_=true;
    }
    void render(){
        dirty_=false;
        auto& d=M5Cardputer.Display;
        constexpr uint16_t bg=0x0841,head=0x10c4,white=0xffff,
                           green=0x5fe9,cyan=0x05ff,sel=0x2673,dim=0xad55;
        d.fillScreen(bg);d.fillRect(0,0,240,23,head);
        d.setTextColor(white,head);d.setCursor(5,7);
        d.print(mode_<0?"POCKET OFFICE":label(mode_));
        d.setTextColor(cyan,head);d.setCursor(208,7);d.print("SD");
        d.fillRect(0,116,240,19,head);
        d.setTextColor(dim,head);d.setCursor(4,121);
        d.print(mode_<0?"Enter open | Fn arrows":
               mode_==4?"Arrows month/year | B menu":
               editing_?"Enter save | Backspace edit":
               deleting_?"Y delete | N cancel":"N new | D delete | R reload | B");
        if(mode_<0) {
            for(int i=0;i<5;++i) {
                const int y=30+i*16;
                if(i==cursor_)d.fillRect(3,y-1,233,14,sel);
                d.setTextColor(white,i==cursor_?sel:bg);
                d.setCursor(8,y+2);d.print(String(i+1)+". "+label(i));
            }
            d.setTextColor(dim,bg);d.setCursor(5,109);d.print(status_);
            return;
        }
        if(mode_==4){
            const int weekday0=suite::office::weekday(year_,month_,1);
            const int days=suite::office::daysInMonth(year_,month_);
            const char* months[]={"","Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
            d.setTextColor(cyan,bg);d.setCursor(5,26);
            d.print(String(months[month_])+" "+year_+"  (manual date)");
            d.setTextColor(dim,bg);d.setCursor(5,38);
            d.print(" Su  Mo  Tu  We  Th  Fr  Sa");
            for(int day=1;day<=days;++day) {
                const int index=weekday0+day-1;
                const int x=7+(index%7)*33,y=51+(index/7)*11;
                d.setTextColor(white,bg);d.setCursor(x,y);d.print(day);
            }
            return;
        }
        d.setTextColor(cyan,bg);d.setCursor(5,26);
        d.print(String(records_.count())+" entries"+(readOnly_?" [READ-ONLY]":""));
        if(mode_==2) {
            int64_t total=0;
            for(int i=0;i<records_.count();++i) {
                int32_t value=0;
                if(suite::office::parseCents(records_.at(i),value))total+=value;
            }
            d.setTextColor(green,bg);d.setCursor(120,26);
            d.print(String("USD ")+String(static_cast<long>(total/100))+"."+String(static_cast<int>(total%100)));
        }
        const int top=selected_>=6?selected_-5:0;
        for(int i=top;i<records_.count()&&i<top+6;++i) {
            const int y=40+(i-top)*12;
            if(i==selected_)d.fillRect(3,y-1,234,11,sel);
            d.setTextColor(white,i==selected_?sel:bg);d.setCursor(5,y);
            const String row=String(mode_==0?(records_.done(i)?"[x] ":"[ ] "):"")+records_.at(i);
            d.print(row.substring(0,37));
        }
        d.setTextColor(dim,bg);d.setCursor(5,110);
        if(editing_)d.print(String(">")+entry_);
        else if(deleting_)d.print("Erase selected entry? Y / N");
        else d.print(status_.substring(0,38));
    }
private:
    static const char* label(int idx){
        static const char* const items[]={"To-do Manager","Contacts","Expense Tracker","Journal","Calendar"};
        return items[idx];
    }
    int mode_=-1,cursor_=0,selected_=0,year_=2026,month_=11;
    bool sd_=false,readOnly_=false,dirty_=true,editing_=false,deleting_=false;
    char entry_[suite::office::Records::Width]={};
    suite::office::Records records_;
    String status_;
    static bool letter(const Keyboard_Class::KeysState& k,char c) {
        for(char v:k.word)if(v==c||v==(c-'a'+'A'))return true;
        return false;
    }
    static const char* filename(int kind) {
        static const char* const files[4]={
            "/SUITE/OFFICE/TODO.DAT","/SUITE/OFFICE/CONTACTS.DAT",
            "/SUITE/OFFICE/EXPENSES.DAT","/SUITE/OFFICE/JOURNAL.DAT"
        };
        return kind>=0&&kind<4?files[kind]:"";
    }
    void open(int i){
        mode_=i;selected_=0;editing_=deleting_=false;dirty_=true;
        if(i<4)load();
    }
    bool load(){
        records_.clear();readOnly_=false;
        if(!sd_){status_="SD not mounted";readOnly_=true;return false;}
        const char* path=filename(mode_);
        if(!SD.exists(path)){
            const String backup=String(path)+".BAK";
            if(SD.exists(backup.c_str()))SD.rename(backup.c_str(),path);
        }
        if(!SD.exists(path)){status_="Empty list";return true;}
        File in=SD.open(path,FILE_READ);
        if(!in || in.isDirectory() || in.size()>4096) {
            if(in)in.close();
            status_="Large/invalid data protected";readOnly_=true;return false;
        }
        static char buffer[4097]={};
        const size_t len=in.size();
        const bool ok=in.readBytes(buffer,len)==len;
        in.close();
        if(!ok||!records_.parse(buffer,len)) {
            records_.clear();readOnly_=true;
            status_="Invalid file: protected";return false;
        }
        if(selected_>=records_.count())selected_=records_.count()?records_.count()-1:0;
        status_="Loaded SD list";return true;
    }
    bool save(){
        if(!sd_||readOnly_)return false;
        static char bytes[4096];
        const size_t len=records_.serialize(bytes,sizeof(bytes));
        if(!len&&records_.count())return false;
        const String path=filename(mode_);
        const String temp=path+".NEW",backup=path+".BAK";
        if(SD.exists(temp.c_str())&&!SD.remove(temp.c_str()))return false;
        File out=SD.open(temp.c_str(),FILE_WRITE);
        if(!out)return false;
        const bool ok=out.write(reinterpret_cast<const uint8_t*>(bytes),len)==len;
        out.flush();out.close();
        if(!ok)return false;
        const bool had=SD.exists(path);
        if(had){
            if(SD.exists(backup.c_str())&&!SD.remove(backup.c_str()))return false;
            if(!SD.rename(path,backup.c_str()))return false;
        }
        if(!SD.rename(temp.c_str(),path.c_str())){
            if(had)SD.rename(backup.c_str(),path.c_str());
            return false;
        }
        return true;
    }
    void changeMonth(int delta) {
        int index=year_*12+(month_-1)+delta;
        const int y=index/12,m=index%12+1;
        if(y>=1600&&y<=9999){year_=y;month_=m;}
    }
};
