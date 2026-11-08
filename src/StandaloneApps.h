#pragma once
#include "core/Theme16Runtime.h"
// Four separate SD-deployed binaries for original Cardputer K132.
// They use only built-in speaker, keyboard, Wi-Fi or BLE.
#include <Arduino.h>
#include <M5Cardputer.h>
#include "core/FieldTools.h"
#if defined(SUITE_APP_CHANNELS)
#include <WiFi.h>
#endif
#if defined(SUITE_APP_BLE)
#include <BLEDevice.h>
#include <BLEScan.h>
#endif

class StandaloneApp {
public:
    void begin(){
        SPI.begin(40,39,14,12);SD.begin(12,SPI,10000000);
#if defined(SUITE_APP_METRONOME)
        M5Cardputer.Speaker.begin();
        M5Cardputer.Speaker.setVolume(44);
        clock_.start(micros());
#elif defined(SUITE_APP_PIANO)
        M5Cardputer.Speaker.begin();
        M5Cardputer.Speaker.setVolume(54);
#elif defined(SUITE_APP_CHANNELS)
        WiFi.mode(WIFI_OFF);
        status_="Press S to scan";
#elif defined(SUITE_APP_BLE)
        BLEDevice::init("");
        scanner_=BLEDevice::getScan();
        scanner_->setActiveScan(false);
        scanner_->setInterval(100);
        scanner_->setWindow(80);
        status_="Press S to scan BLE";
#endif
    }
    void exit() {
#if defined(SUITE_APP_METRONOME) || defined(SUITE_APP_PIANO)
        M5Cardputer.Speaker.stop(0);
#endif
#if defined(SUITE_APP_CHANNELS)
        WiFi.scanDelete();WiFi.mode(WIFI_OFF);
#endif
#if defined(SUITE_APP_BLE)
        if(scanner_)scanner_->stop();
#endif
    }
    void poll() {
#if defined(SUITE_APP_METRONOME)
        bool accent=false,primary=false;
        if(clock_.due(micros(),accent,primary)) {
            if(primary)M5Cardputer.Speaker.tone(accent?1470:1050,40,0,true);
            else M5Cardputer.Speaker.tone(790,18,0,true);
            ++pulses_;dirty_=true;
        }
#elif defined(SUITE_APP_CHANNELS)
        if(scanning_) {
            const int n=WiFi.scanComplete();
            if(n!=WIFI_SCAN_RUNNING) {
                scanning_=false;channels_.clear();
                if(n>=0) {
                    seen_=n;
                    for(int i=0;i<n;++i)
                        channels_.add(static_cast<unsigned>(WiFi.channel(i)),
                                      WiFi.RSSI(i));
                    status_=String("Found ")+n+" access points";
                } else status_="Scan error";
                WiFi.scanDelete();WiFi.mode(WIFI_OFF);
                dirty_=true;
            }
        }
#endif
    }
    void input(const Keyboard_Class::KeysState& k) {
#if defined(SUITE_APP_METRONOME)
        if(has(k,'s')||k.enter) {
            if(clock_.running()) {
                clock_.stop();
                M5Cardputer.Speaker.stop(0);
            } else clock_.start(micros());
        }
        if(k.up || has(k,'+'))clock_.setBpm(clock_.bpm()+1);
        if(k.down || has(k,'-'))clock_.setBpm(clock_.bpm()-1);
        if(has(k,'m'))clock_.setMeter(clock_.meter()==12?2:clock_.meter()+1);
        if(has(k,'d')) {
            const uint8_t current=clock_.subdivision();
            clock_.setSubdivision(current==1?2:current==2?4:1);
        }
        if(has(k,'r')) {
            clock_.stop();clock_.start(micros());
        }
#elif defined(SUITE_APP_PIANO)
        if(k.up && octave_<2)++octave_;
        if(k.down && octave_>-2)--octave_;
        if(has(k,'+'))volume_=(volume_<150)?volume_+10:150;
        if(has(k,'-'))volume_=(volume_>20)?volume_-10:20;
        M5Cardputer.Speaker.setVolume(volume_);
        for(char key:k.word) {
            const int note=suite::fieldtools::noteForKey(key,octave_);
            if(note<0)continue;
            const double freq=suite::fieldtools::midiHz(note);
            if(freq>0) {
                lastNote_=note;
                M5Cardputer.Speaker.tone(static_cast<float>(freq),330,
                                         voiceCursor_,true);
                voiceCursor_=(voiceCursor_+1)%6; // bounded speaker channels
            }
        }
#elif defined(SUITE_APP_CHANNELS)
        if(has(k,'s')||has(k,'r')) startWifi();
        if(k.left && selected_>1)--selected_;
        if(k.right && selected_<14)++selected_;
#elif defined(SUITE_APP_BLE)
        if(has(k,'s')||has(k,'r')) scanBLE();
        if(k.up && selected_>0)--selected_;
        if(k.down && selected_+1<count_)++selected_;
        if(k.enter && count_)details_=!details_;
        if(k.backspace)details_=false;
#endif
        dirty_=true;
    }
    bool dirty() const {return dirty_;}
    void render() {
        dirty_=false;
        auto& d=M5Cardputer.Display;
        const auto &p=suite::skin16::colors();
        const uint16_t bg=p.bg,
        panel=p.panel,
        white=p.text,
        cyan=p.accent,
        green=p.good,
        muted=p.muted,
        sel=p.selected;
        d.fillScreen(bg);d.fillRect(0,0,240,23,panel);
        d.setTextColor(white,panel);d.setCursor(5,7);
#if defined(SUITE_APP_METRONOME)
        d.print("METRONOME");
#elif defined(SUITE_APP_PIANO)
        d.print("MUSIC KEYBOARD");
#elif defined(SUITE_APP_CHANNELS)
        d.print("WI-FI CHANNELS");
#elif defined(SUITE_APP_BLE)
        d.print("BLE EXPLORER");
#endif
        d.setTextColor(cyan,panel);d.setCursor(201,7);d.print("SD");
        d.fillRect(0,116,240,19,panel);
        d.setTextColor(muted,panel);d.setCursor(4,121);
#if defined(SUITE_APP_METRONOME)
        d.print("S run  Up/Down BPM  M meter D sub");
        d.setTextColor(cyan,bg);d.setTextSize(2);d.setCursor(7,32);
        d.print(clock_.bpm());d.print(" BPM");d.setTextSize(1);
        d.setCursor(7,61);d.setTextColor(white,bg);
        d.print(String(clock_.meter())+"/4  subdivision "+clock_.subdivision());
        d.setCursor(7,76);d.print(clock_.running()?"RUNNING":"STOPPED");
        for(int i=0;i<clock_.meter();++i) {
            const int x=6+i*19;
            d.fillRoundRect(x,95,13,11,3,(clock_.beat()==i)?green:sel);
        }
#elif defined(SUITE_APP_PIANO)
        d.print("A-K notes / W E T Y U sharps");
        d.setTextColor(green,bg);d.setCursor(5,32);
        d.print("C4 - C5 keyboard");
        d.setTextColor(white,bg);d.setCursor(5,48);
        d.print(String("Octave shift ")+octave_+"   Vol "+volume_);
        d.setCursor(5,65);d.print("A W S E D F T G Y H U J K");
        d.setCursor(5,79);d.print("C # D # E F # G # A # B C");
        d.setCursor(5,96);d.print(String("Last MIDI note: ")+lastNote_);
#elif defined(SUITE_APP_CHANNELS)
        d.print("S rescan  Left/Right detail");
        d.setTextColor(cyan,bg);d.setCursor(5,26);
        d.print(scanning_?"Scanning 2.4 GHz...":status_);
        d.setTextColor(green,bg);d.setCursor(5,39);
        d.print(String("Least busy estimate: ")+channels_.recommended());
        for(int i=1;i<=14;++i) {
            const auto c=channels_.at(i);
            const int bar=(c.count>16)?42:c.count*2+ (c.count?8:0);
            const int x=4+(i-1)*16;
            if(bar>0)d.fillRect(x,92-bar,12,bar,(selected_==i)?green:sel);
            d.setTextColor((selected_==i)?green:muted,bg);
            d.setCursor(x,97);d.print(i);
        }
        d.setTextColor(white,bg);d.setCursor(5,108);
        const auto c=channels_.at(selected_);
        d.print(String("Ch ")+selected_+"  APs "+c.count+"  Peak "+c.peak+"dBm");
#elif defined(SUITE_APP_BLE)
        d.print("S scan  Up/Down  Enter details");
        if(details_ && count_) {
            const auto &e=devices_[selected_];
            d.setTextColor(cyan,bg);d.setCursor(5,31);d.print(e.name);
            d.setTextColor(white,bg);d.setCursor(5,53);d.print(e.address);
            d.setCursor(5,69);d.print(String("RSSI: ")+e.rssi+" dBm");
            d.setCursor(5,89);d.print("Backspace = list");
        } else {
            d.setTextColor(cyan,bg);d.setCursor(5,26);
            d.print(status_.substring(0,36));
            const int start=selected_>=7?selected_-6:0;
            for(int i=start;i<count_&&i<start+7;++i) {
                const int y=37+(i-start)*11;
                if(selected_==i)d.fillRect(3,y-1,233,11,sel);
                d.setTextColor(white,selected_==i?sel:bg);
                d.setCursor(5,y);
                d.print(String(devices_[i].name).substring(0,25)+
                        " "+devices_[i].rssi);
            }
        }
#endif
    }
private:
    bool dirty_=true;
#if defined(SUITE_APP_METRONOME)
    suite::fieldtools::BeatClock clock_;
    uint32_t pulses_=0;
#elif defined(SUITE_APP_PIANO)
    int octave_=0,lastNote_=60;
    uint8_t volume_=54,voiceCursor_=0;
#elif defined(SUITE_APP_CHANNELS)
    suite::fieldtools::Channels channels_;
    bool scanning_=false;
    int seen_=0,selected_=1;
    String status_;
    void startWifi() {
        if(scanning_)return;
        channels_.clear();
        WiFi.mode(WIFI_STA);WiFi.disconnect();WiFi.scanDelete();
        const int result=WiFi.scanNetworks(true,true,true,250);
        if(result==WIFI_SCAN_FAILED) {
            scanning_=false;status_="Scan failed";
            WiFi.mode(WIFI_OFF);
        } else {
            scanning_=true;status_="Scanning...";
        }
    }
#elif defined(SUITE_APP_BLE)
    struct Device {
        char name[24]={},address[20]={};
        int rssi=-127;
    };
    BLEScan* scanner_=nullptr;
    Device devices_[24];
    int selected_=0,count_=0;
    bool details_=false;
    String status_;
    void scanBLE() {
        if(!scanner_){status_="BLE unavailable";return;}
        details_=false;selected_=count_=0;
        auto& d=M5Cardputer.Display;
        d.fillScreen(0x0841);
        d.setTextColor(0xffff,0x0841);
        d.setCursor(5,34);d.print("Scanning BLE 3 seconds...");
        BLEScanResults results=scanner_->start(3,false);
        const int found=results.getCount();
        const int limit=found<24?found:24;
        for(int i=0;i<limit;++i) {
            BLEAdvertisedDevice e=results.getDevice(i);
            const std::string name=e.getName();
            const std::string mac=e.getAddress().toString();
            strncpy(devices_[count_].name,
                    name.empty()?"(unnamed)":name.c_str(),23);
            strncpy(devices_[count_].address,mac.c_str(),19);
            devices_[count_].rssi=e.getRSSI();
            ++count_;
        }
        scanner_->clearResults();
        status_=String("Seen ")+found+" BLE advertisements";
    }
#endif
    static bool has(const Keyboard_Class::KeysState& k,char c) {
        for(char v:k.word)if(v==c||v==(c-'a'+'A'))return true;
        return false;
    }
};
