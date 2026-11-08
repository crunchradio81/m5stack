#pragma once
// Six read-only utility programs in one SD firmware image, no file writes.
// Hex Viewer, CSV Viewer, JSON Inspector (ASCII profile), File Search,
// ASCII Chart and bounded Storage Analyzer.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <SPI.h>
#include <SD.h>
#include <string.h>
#include "core/InspectorCore.h"
class InspectCollection {
public:
    void begin(){
        mode_=-1;cursor_=0;dirty_=true;view_=false;
        SPI.begin(40,39,14,12);
        sd_=SD.begin(12,SPI,10000000);
        status_=sd_?"Ready":"No FAT32 microSD";
    }
    void exit(){if(file_)file_.close();}
    void poll(){}
    bool dirty()const{return dirty_;}
    void input(const Keyboard_Class::KeysState& k) {
        if(mode_<0){
            if(k.up||letter(k,'w'))cursor_=(cursor_+5)%6;
            if(k.down||letter(k,'s'))cursor_=(cursor_+1)%6;
            if(k.enter)open(cursor_);
            dirty_=true;return;
        }
        if(mode_==4){
            if(k.up&&ascii_<120)ascii_+=8;
            if(k.down&&ascii_>=8)ascii_-=8;
            if(k.left&&ascii_>0)--ascii_;
            if(k.right&&ascii_<127)++ascii_;
            if(letter(k,'b'))mode_=-1;
            dirty_=true;return;
        }
        if(mode_==5){
            if(letter(k,'r'))analyze();
            if(letter(k,'b'))mode_=-1;
            dirty_=true;return;
        }
        if(mode_==3){
            if(letter(k,'b')&&!searching_){mode_=-1;dirty_=true;return;}
            if(k.enter){searching_=false;findAll();dirty_=true;return;}
            if(k.backspace){const size_t n=strlen(term_);if(n)term_[n-1]=0;}
            else if(!k.fn&&!k.alt&&!k.ctrl)
                for(char c:k.word) {
                    if(c<32||c>126)continue;
                    const size_t n=strlen(term_);
                    if(n<sizeof(term_)-1){term_[n]=c;term_[n+1]=0;}
                }
            if(k.up&&selected_>0)--selected_;
            if(k.down&&selected_+1<found_)++selected_;
            dirty_=true;return;
        }
        if(view_){
            if(letter(k,'b')||k.backspace){
                file_.close();view_=false;
                refreshDir();dirty_=true;return;
            }
            if(k.up) {
                if(offset_>=static_cast<uint32_t>(mode_==0?64:152))
                    offset_-=static_cast<uint32_t>(mode_==0?64:152);
                else offset_=0;
            }
            if(k.down){
                const uint32_t inc=mode_==0?64:152;
                if(file_&&offset_<file_.size() &&
                   file_.size()-offset_>inc)offset_+=inc;
            }
            dirty_=true;return;
        }
        if(letter(k,'b')||k.backspace){
            if(strcmp(directory_,"/SUITE")==0)mode_=-1;
            else {
                char* slash=strrchr(directory_,'/');
                if(slash&&slash!=directory_)*slash=0;
                else strcpy(directory_,"/SUITE");
                refreshDir();
            }
            dirty_=true;return;
        }
        if(k.up&&selected_>0)--selected_;
        if(k.down&&selected_+1<entries_)++selected_;
        if(letter(k,'r'))refreshDir();
        if(k.enter&&entries_>0)choose();
        dirty_=true;
    }
    void render(){
        dirty_=false;
        auto& d=M5Cardputer.Display;
        constexpr uint16_t bg=0x0841,head=0x10c4,white=0xffff,cyan=0x05ff,sel=0x2673,dim=0xad55,green=0x5fe9;
        d.fillScreen(bg);d.fillRect(0,0,240,23,head);
        d.setTextColor(white,head);d.setCursor(5,7);
        d.print(mode_<0?"SD INSPECTOR":label(mode_));
        d.setTextColor(cyan,head);d.setCursor(209,7);d.print("SD");
        d.fillRect(0,116,240,19,head);
        d.setTextColor(dim,head);d.setCursor(5,121);
        d.print(mode_<0?"Enter open | Fn arrows":mode_==3?"Type name, Enter search | B menu":
                mode_==4?"Arrows change code | B menu":mode_==5?"R recount | B menu":
                view_?"Arrows page | B files":"Enter open | B parent/menu");
        if(mode_<0){
            for(int i=0;i<6;++i){
                int y=27+i*13;if(cursor_==i)d.fillRect(3,y-1,233,12,sel);
                d.setTextColor(white,cursor_==i?sel:bg);
                d.setCursor(8,y);d.print(String(i+1)+". "+label(i));
            }
            d.setTextColor(dim,bg);d.setCursor(5,109);d.print(status_);
            return;
        }
        if(mode_==4) {
            d.setTextColor(cyan,bg);d.setCursor(6,29);
            d.print(String("ASCII ")+ascii_+" / HEX "+String(ascii_,HEX));
            d.setTextSize(3);d.setCursor(75,53);d.setTextColor(green,bg);
            if(ascii_>=32&&ascii_<127)d.print(static_cast<char>(ascii_));
            else d.print("CTL");
            d.setTextSize(1);d.setCursor(5,105);
            d.print("32..126 printable ASCII");
            return;
        }
        if(mode_==5) {
            d.setTextColor(cyan,bg);d.setCursor(6,29);
            d.print(String("Files: ")+scanCount_+"  Directories: "+dirs_);
            d.setTextColor(white,bg);d.setCursor(6,47);
            d.print(String("Bytes scanned: ")+String(static_cast<unsigned long>(bytes_)));
            d.setCursor(6,62);d.print(String("Limited to ")+MaxWalk+" entries");
            d.setCursor(6,76);d.print("Root: /SUITE (depth <= 3)");
            d.setTextColor(dim,bg);d.setCursor(6,94);
            d.print(status_);
            return;
        }
        if(mode_==3){
            d.setTextColor(cyan,bg);d.setCursor(5,27);
            d.print(String("Name contains: ")+term_);
            d.setTextColor(dim,bg);d.setCursor(5,39);
            d.print(String(found_)+" matches (max 12)");
            const int start=selected_>=6?selected_-5:0;
            for(int i=start;i<found_&&i<start+6;++i){
                const int y=51+(i-start)*10;
                d.setTextColor(white,bg);d.setCursor(5,y);
                d.print(String((i==selected_)?"> ":"  ")+results_[i]);
            }
            return;
        }
        if(!view_) {
            d.setTextColor(cyan,bg);d.setCursor(5,27);
            d.print(String(directory_).substring(0,37));
            const int top=selected_>=7?selected_-6:0;
            for(int i=top;i<entries_&&i<top+7;++i){
                int y=39+(i-top)*11;
                if(selected_==i)d.fillRect(3,y-1,233,11,sel);
                d.setTextColor(white,selected_==i?sel:bg);
                d.setCursor(5,y);
                d.print(String(dir_[i]?"[D] ":"    ")+names_[i]);
            }
            d.setTextColor(dim,bg);d.setCursor(5,109);
            d.print(status_.substring(0,37));
            return;
        }
        d.setTextColor(cyan,bg);d.setCursor(5,25);
        d.print(String(filename_).substring(0,38));
        if(mode_==0) {
            for(int row=0;row<8;++row){
                uint8_t bytes[8]={};
                const uint32_t at=offset_+static_cast<uint32_t>(row*8);
                if(!file_||at>=file_.size()||!file_.seek(at))break;
                const int n=file_.read(bytes,8);
                if(n<=0)break;
                char line[56]={};
                snprintf(line,sizeof(line),"%05lX ",static_cast<unsigned long>(at));
                for(int i=0;i<n;++i){
                    char hex[4];snprintf(hex,sizeof(hex),"%02X ",bytes[i]);
                    strncat(line,hex,sizeof(line)-strlen(line)-1);
                }
                d.setTextColor(white,bg);d.setCursor(5,37+row*9);
                d.print(line);
            }
        } else if(mode_==1) {
            if(!file_.seek(offset_))return;
            if(offset_)while(file_.available()) { if(file_.read()=='\n')break; }
            int row=0;
            while(file_.available()&&row<6){
                char raw[481]={};size_t count=0;
                while(file_.available()&&count<480){
                    const int ch=file_.read();
                    if(ch=='\n')break;
                    if(ch=='\r')continue;
                    raw[count++]=static_cast<char>(ch);
                }
                char fields[8][40]={{0}};unsigned n=0;
                if(suite::inspect::csvColumns(raw,count,fields,n)){
                    String text;
                    for(unsigned i=0;i<n;++i){
                        if(i)text+=" | ";
                        text+=fields[i];
                    }
                    d.setTextColor(white,bg);d.setCursor(5,37+row*12);
                    d.print(text.substring(0,38));
                } else {
                    d.setTextColor(dim,bg);d.setCursor(5,37+row*12);
                    d.print("(long/malformed CSV row)");
                }
                ++row;
            }
        } else {
            d.setTextColor(fileValid_?green:dim,bg);d.setCursor(5,37);
            d.print(fileValid_?"JSON: valid ASCII syntax":"JSON: invalid/too large");
            if(!file_.seek(offset_))return;
            for(int row=0;row<6;++row) {
                char line[39]={};size_t n=0;
                while(file_.available()&&n<38){
                    const int c=file_.read();
                    if(c<32&&c!='\n'&&c!='\t')continue;
                    if(c=='\n')break;
                    line[n++]=c=='\t'?' ':static_cast<char>(c);
                }
                if(!n&&!file_.available())break;
                d.setTextColor(white,bg);d.setCursor(5,49+row*10);
                d.print(line);
            }
        }
    }
private:
    static constexpr int MaxEntries=64,MaxWalk=256;
    static const char* label(int idx){
        static const char* const items[]={
        "Hex Viewer","CSV Viewer","JSON Inspector","File Search","ASCII Chart","Storage Analyzer"
    };
        return items[idx];
    }
    int mode_=-1,cursor_=0,selected_=0,entries_=0,found_=0,ascii_=65;
    uint32_t offset_=0;
    uint32_t scanCount_=0,dirs_=0;
    uint64_t bytes_=0;
    bool sd_=false,view_=false,dirty_=true,fileValid_=false,searching_=false;
    String status_;
    char directory_[96]="/SUITE";
    char filename_[96]={};
    char names_[MaxEntries][36]={};
    bool dir_[MaxEntries]={};
    char results_[12][65]={};
    char term_[25]={};
    File file_;
    static bool letter(const Keyboard_Class::KeysState& k,char c) {
        for(char v:k.word)if(v==c||v==(c-'a'+'A'))return true;
        return false;
    }
    void open(int index){
        mode_=index;selected_=entries_=0;dirty_=true;view_=false;
        if(index==4)return;
        if(!sd_){status_="No SD card";return;}
        if(index==5){analyze();return;}
        if(index==3){term_[0]=0;found_=0;return;}
        strcpy(directory_,"/SUITE");refreshDir();
    }
    void refreshDir(){
        entries_=0;selected_=0;
        if(!sd_)return;
        File folder=SD.open(directory_);
        if(!folder||!folder.isDirectory()){
            if(folder)folder.close();status_="Directory unavailable";return;
        }
        while(entries_<MaxEntries){
            File e=folder.openNextFile();
            if(!e)break;
            const char* base=strrchr(e.name(),'/');
            if(base)++base;
            else base=e.name();
            if(strlen(base)<sizeof(names_[0])&&strlen(base)>0){
                strcpy(names_[entries_],base);
                dir_[entries_]=e.isDirectory();++entries_;
            }
            e.close();
        }
        folder.close();status_=String(entries_)+" entries";
    }
    void choose(){
        if(selected_>=entries_)return;
        const String path=String(directory_)+"/"+names_[selected_];
        if(path.length()>=sizeof(directory_)){
            status_="Path too long";return;
        }
        if(dir_[selected_]) {
            strcpy(directory_,path.c_str());refreshDir();return;
        }
        file_.close();file_=SD.open(path.c_str(),FILE_READ);
        if(!file_||file_.isDirectory()) {
            if(file_)file_.close();status_="Cannot read file";return;
        }
        strncpy(filename_,names_[selected_],sizeof(filename_)-1);
        offset_=0;fileValid_=false;view_=true;
        if(mode_==2&&file_.size()<=4096){
            static char json[4097]={};
            if(file_.readBytes(json,file_.size())==file_.size())
                fileValid_=suite::inspect::Json::valid(json,file_.size());
        }
        file_.seek(0);
    }
    void walk(const String& path,int depth) {
        if(depth>3||scanCount_+dirs_>=MaxWalk)return;
        File dir=SD.open(path.c_str());
        if(!dir||!dir.isDirectory()){if(dir)dir.close();return;}
        while(scanCount_+dirs_<MaxWalk){
            File file=dir.openNextFile();
            if(!file)break;
            String name=file.name();
            const int pos=name.lastIndexOf('/');
            if(pos>=0)name=name.substring(pos+1);
            const String full=path+"/"+name;
            if(file.isDirectory()) {
                ++dirs_;file.close();
                if(depth<3)walk(full,depth+1);
            } else {
                ++scanCount_;bytes_+=file.size();
                if(mode_==3&&found_<12&&term_[0]&&
                   suite::inspect::asciiSearch(name.c_str(),term_)){
                    strncpy(results_[found_],full.c_str(),sizeof(results_[0])-1);
                    ++found_;
                }
                file.close();
            }
        }
        dir.close();
    }
    void analyze() {
        scanCount_=dirs_=0;bytes_=0;
        walk("/SUITE",0);status_="Bounded SD scan complete";
    }
    void findAll(){found_=selected_=0;scanCount_=dirs_=0;bytes_=0;walk("/SUITE",0);}
};
