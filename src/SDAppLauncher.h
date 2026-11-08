#pragma once
// Permanent factory launcher for suite-owned original Cardputer K132 SD apps.
// P12 categories, search, favorites, recent app list and explicit OTA install.
// Source/CI validation is not proof of safe original hardware recovery.
#include <Arduino.h>
#include <SD.h>
#include <Update.h>
#include <M5Cardputer.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <mbedtls/sha256.h>
#include "core/SDAppManifest.h"
#include "core/Launcher2Core.h"
#include "core/Theme16Runtime.h"

class SDAppLauncher {
public:
    static const char* directory(){return "/SUITE/APPS";}
    void enter(bool ready) {
        category_=suite::launch2::Category::All;
        searching_=confirming_=false;query_[0]=0;
        refresh(ready);
    }
    void refresh(bool ready) {
        count_=visibleCount_=selected_=0;confirming_=searching_=false;
        if(!ready){status_="Insert FAT32 SD card";return;}
        if(!SD.exists("/SUITE"))SD.mkdir("/SUITE");
        if(!SD.exists(directory()) && !SD.mkdir(directory())) {
            status_="Cannot create /SUITE/APPS";return;
        }
        File dir=SD.open(directory());
        if(!dir||!dir.isDirectory()) {
            if(dir)dir.close();status_="Cannot read apps folder";return;
        }
        while(count_<suite::appmanifest::MaxApps) {
            File file=dir.openNextFile();
            if(!file) break;
            if(!file.isDirectory() &&
               file.size()<=suite::appmanifest::MaxManifest) {
                String filename=file.name();
                const int slash=filename.lastIndexOf('/');
                if(slash>=0)filename=filename.substring(slash+1);
                if(filename.endsWith(".APP") && filename.length()<=45) {
                    char data[suite::appmanifest::MaxManifest+1]={};
                    const size_t size=file.size();
                    if(file.readBytes(data,size)==size) {
                        suite::appmanifest::App parsed;
                        if(suite::appmanifest::parse(data,size,parsed)) {
                            apps_[count_]=parsed;++count_;
                        }
                    }
                }
            }
            file.close();
        }
        dir.close();
        for(int i=0;i<count_;++i)
            for(int j=i+1;j<count_;++j)
                if(strcmp(apps_[j].id,apps_[i].id)<0) {
                    auto temp=apps_[i];apps_[i]=apps_[j];apps_[j]=temp;
                }
        loadPreferences();
        rebuild();
        if(writable_)
            status_=count_?"Enter install, C category":"No valid .APP manifests";
    }
    void onKey(const Keyboard_Class::KeysState&k,bool ready) {
        if(confirming_) {
            if(letter(k,'n')||k.backspace){confirming_=false;return;}
            if(letter(k,'y')) {
                confirming_=false;
                if(visibleCount_>0)install(apps_[visible_[selected_]]);
            }
            return;
        }
        if(searching_) {
            if(k.enter){searching_=false;return;}
            if(k.backspace){
                const size_t n=strlen(query_);
                if(n)query_[n-1]=0;
            }else if(!k.ctrl&&!k.alt&&!k.fn){
                for(char c:k.word){
                    if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
                         (c>='0'&&c<='9')||c==' '||c=='-'))continue;
                    const size_t n=strlen(query_);
                    if(n+1<sizeof(query_)){query_[n]=c;query_[n+1]=0;}
                }
            }
            rebuild();return;
        }
        if(k.up&&selected_>0)--selected_;
        if(k.down&&selected_+1<visibleCount_)++selected_;
        if(letter(k,'r')){refresh(ready);return;}
        if(letter(k,'c')){
            const auto next=static_cast<unsigned>(category_)+1;
            category_=static_cast<suite::launch2::Category>(
              next>=static_cast<unsigned>(suite::launch2::Category::Count)?0:next);
            rebuild();return;
        }
        if(letter(k,'v')){
            category_=suite::launch2::Category::Recent;
            rebuild();return;
        }
        if(letter(k,'s')){searching_=true;return;}
        if(k.backspace&&query_[0]){query_[0]=0;rebuild();return;}
        if(letter(k,'f')&&visibleCount_>0){
            if(!writable_){status_="Preferences invalid; protected";return;}
            const char* id=apps_[visible_[selected_]].id;
            auto before=prefs_;
            if(!prefs_.toggle(id)||!savePreferences()){
                prefs_=before;status_="Favorite save failed";return;
            }
            rebuild();
            status_="Favorite list saved";
            return;
        }
        if(k.enter&&visibleCount_>0){
            confirming_=true;
            status_="Install SD app into OTA_0? Y/N";
        }
    }
    void render()const {
        auto&d=M5Cardputer.Display;
        const auto&p=suite::skin16::colors();
        const uint16_t bg=p.bg,
          panel=p.panel,
          white=p.text,
          cyan=p.accent,
          muted=p.muted,
          selected=p.selected,
          green=p.good;
        d.fillScreen(bg);d.fillRect(0,0,240,23,panel);
        d.setTextColor(white,panel);d.setCursor(5,7);
        d.print("SD APP LIBRARY");
        d.setTextColor(cyan,panel);d.setCursor(207,7);d.print("L2");
        d.fillRect(0,116,240,19,panel);
        d.setTextColor(muted,panel);d.setCursor(4,121);
        d.print(confirming_?"Y install / N cancel":
                searching_?"Type search / Enter finish":
                "C category F fav S find V recent");
        d.setTextColor(cyan,bg);d.setCursor(5,26);
        d.print(String(suite::launch2::categoryName(category_))+" "+
                visibleCount_+"/"+count_+" apps");
        if(query_[0]){
            d.setTextColor(green,bg);d.setCursor(5,36);
            d.print(String("Search: ")+query_);
        }
        if(confirming_ && visibleCount_>0) {
            const auto&a=apps_[visible_[selected_]];
            d.setTextColor(white,bg);d.setCursor(5,50);
            d.print(String("Install ")+a.name+"?");
            d.setTextColor(muted,bg);d.setCursor(5,67);
            d.print("Factory stays in own partition.");
            d.setCursor(5,81);d.print("Boots selected app after write.");
        }else{
            const int first=selected_>=6?selected_-5:0;
            for(int i=first;i<visibleCount_&&i<first+6;++i){
                const auto&a=apps_[visible_[i]];
                const int y=44+(i-first)*10;
                if(i==selected_)d.fillRect(3,y-1,233,11,selected);
                d.setTextColor(white,i==selected_?selected:bg);
                d.setCursor(5,y);
                d.print(String(prefs_.favorite(a.id)?"* ":"  ")+
                        String(a.name).substring(0,35));
            }
        }
        d.setTextColor(muted,bg);d.setCursor(5,108);
        d.print(status_.substring(0,38));
    }
private:
    suite::appmanifest::App apps_[suite::appmanifest::MaxApps];
    uint8_t visible_[suite::appmanifest::MaxApps]={};
    int count_=0,visibleCount_=0,selected_=0;
    suite::launch2::Preferences prefs_;
    suite::launch2::Category category_=suite::launch2::Category::All;
    char query_[17]={};
    bool confirming_=false,searching_=false,writable_=true;
    String status_;
    static bool symbol(const Keyboard_Class::KeysState&k,char c){
        for(char v:k.word)if(v==c)return true;
        return false;
    }
    static bool letter(const Keyboard_Class::KeysState&k,char c){
        return symbol(k,c)||symbol(k,static_cast<char>(c-'a'+'A'));
    }
    void rebuild(){
        selected_=0;
        visibleCount_=suite::launch2::filter(apps_,count_,category_,
               prefs_,query_,visible_,suite::appmanifest::MaxApps);
    }
    void loadPreferences(){
        writable_=true;prefs_.clear();
        const char*path="/SUITE/APPS/LAUNCHER.CFG";
        if(!SD.exists(path))return;
        File f=SD.open(path,FILE_READ);
        if(!f||f.isDirectory()||f.size()>suite::launch2::Preferences::MaxBytes){
            if(f)f.close();
            writable_=false;status_="Preferences invalid: read-only";return;
        }
        char text[suite::launch2::Preferences::MaxBytes+1]={};
        const size_t n=f.size();
        const bool ok=f.readBytes(text,n)==n;f.close();
        if(!ok||!prefs_.parse(text,n)){
            writable_=false;status_="Corrupt favorites protected";
        }
    }
    bool savePreferences(){
        if(!writable_)return false;
        char data[suite::launch2::Preferences::MaxBytes+1]={};
        const size_t n=prefs_.encode(data,sizeof(data));
        if(!n)return false;
        const char*path="/SUITE/APPS/LAUNCHER.CFG";
        const char*temp="/SUITE/APPS/LAUNCHER.NEW";
        const char*backup="/SUITE/APPS/LAUNCHER.BAK";
        if(SD.exists(temp)&&!SD.remove(temp))return false;
        File f=SD.open(temp,FILE_WRITE);
        if(!f)return false;
        const bool ok=f.write(reinterpret_cast<const uint8_t*>(data),n)==n;
        f.flush();f.close();if(!ok)return false;
        const bool had=SD.exists(path);
        if(had){
            if(SD.exists(backup)&&!SD.remove(backup))return false;
            if(!SD.rename(path,backup))return false;
        }
        if(!SD.rename(temp,path)){
            if(had)SD.rename(backup,path);
            return false;
        }
        return true;
    }
    static bool verifySha(File& input,const char* expected,uint32_t length) {
        if(!input.seek(0))return false;
        mbedtls_sha256_context ctx;
        mbedtls_sha256_init(&ctx);
        int error=mbedtls_sha256_starts_ret(&ctx,0);
        uint8_t chunk[1024];
        uint32_t left=length;
        while(!error && left) {
            const size_t count=left<sizeof(chunk)?left:sizeof(chunk);
            const size_t read=input.read(chunk,count);
            if(read!=count){error=-1;break;}
            error=mbedtls_sha256_update_ret(&ctx,chunk,count);
            left-=count;
            yield();
        }
        uint8_t digest[32]={};
        if(!error)error=mbedtls_sha256_finish_ret(&ctx,digest);
        mbedtls_sha256_free(&ctx);
        if(error)return false;
        char actual[65]={};
        const char* digits="0123456789abcdef";
        for(int i=0;i<32;++i) {
            actual[i*2]=digits[digest[i]>>4];
            actual[i*2+1]=digits[digest[i]&15];
        }
        return strcmp(actual,expected)==0;
    }
    void feedback(const char* message) {
        status_=message;
        auto& d=M5Cardputer.Display;
        d.fillScreen(0x0841);
        d.setTextColor(0xffff,0x0841);
        d.setCursor(6,22);d.print("SD APP INSTALL");
        d.setCursor(6,48);d.print(message);
        Serial.println(message);
    }
    void install(const suite::appmanifest::App& app) {
        const esp_partition_t* running=esp_ota_get_running_partition();
        const esp_partition_t* factory=esp_partition_find_first(
            ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_FACTORY,nullptr);
        const esp_partition_t* target=esp_ota_get_next_update_partition(nullptr);
        if(!running||!factory||running->address!=factory->address||!target||
           target->subtype!=ESP_PARTITION_SUBTYPE_APP_OTA_0 ||
           target->size<app.size) {
            status_="Wrong partitions; unsafe to flash";
            return;
        }
        const String path=String(directory())+"/"+app.image;
        File input=SD.open(path,FILE_READ);
        if(!input||input.isDirectory()) {
            if(input)input.close();status_="Missing app .BIN";return;
        }
        if(input.size()!=app.size) {
            input.close();status_="BIN size mismatch";return;
        }
        const int magic=input.read();
        if(magic!=0xE9) {
            input.close();status_="Not an ESP32 app image";return;
        }
        feedback("Checking SHA-256...");
        if(!verifySha(input,app.sha256,app.size)) {
            input.close();status_="Checksum differs: canceled";return;
        }
        if(!input.seek(0)) {
            input.close();status_="SD seek error";return;
        }
        feedback("Installing in OTA slot...");
        if(!Update.begin(app.size,U_FLASH)) {
            input.close();status_="OTA begin error";return;
        }
        const size_t written=Update.writeStream(input);
        input.close();
        if(written!=app.size||!Update.end(false)||Update.hasError()) {
            Update.abort();
            status_="OTA failed; factory remains";
            return;
        }
        // Arduino Update.end() already sets next boot slot. A suite SD app
        // resets the boot target to factory immediately upon boot.
        // Only record a recent app after OTA reports a completed write.
        // Preference-save failure must not turn a valid install into a
        // half-written firmware; the app boot remains owner-requested.
        if(writable_){prefs_.remember(app.id);savePreferences();}
        feedback("App installed. Rebooting...");
        delay(250);
        ESP.restart();
    }
};

// App image boot guard: MUST execute before starting radio/UI peripherals.
// This makes the factory launcher the next boot target; pressing Home inside
// a suite-managed SD app calls ESP.restart() to return to factory.
inline bool suiteSdAppPrepareReturnToLauncher() {
    const esp_partition_t* factory=esp_partition_find_first(
        ESP_PARTITION_TYPE_APP,ESP_PARTITION_SUBTYPE_APP_FACTORY,nullptr);
    const esp_partition_t* running=esp_ota_get_running_partition();
    if(!factory||!running||running==factory ||
       running->subtype!=ESP_PARTITION_SUBTYPE_APP_OTA_0)return false;
    return esp_ota_set_boot_partition(factory)==ESP_OK;
}
