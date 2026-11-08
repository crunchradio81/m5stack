#pragma once
// Permanent factory-partition launcher for suite-managed Cardputer K132
// application binaries stored on FAT32 SD. Installs only the app OTA slot.
// Before release, physical board boot/recovery acceptance is REQUIRED.
#include <Arduino.h>
#include <SD.h>
#include <Update.h>
#include <M5Cardputer.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <mbedtls/sha256.h>
#include "core/SDAppManifest.h"

class SDAppLauncher {
public:
    static const char* directory(){return "/SUITE/APPS";}
    void enter(bool ready) {
        confirming_=false;selected_=0;count_=0;
        refresh(ready);
    }
    void refresh(bool ready) {
        count_=0;selected_=0;confirming_=false;
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
            if(!file.isDirectory() && file.size()<=suite::appmanifest::MaxManifest) {
                String filename=file.name();
                const int slash=filename.lastIndexOf('/');
                if(slash>=0)filename=filename.substring(slash+1);
                if(filename.endsWith(".APP") && filename.length()<=45) {
                    char data[suite::appmanifest::MaxManifest+1]={};
                    const size_t size=file.size();
                    if(file.readBytes(data,size)==size) {
                        suite::appmanifest::App parsed;
                        if(suite::appmanifest::parse(data,size,parsed)) {
                            apps_[count_]=parsed;
                            ++count_;
                        }
                    }
                }
            }
            file.close();
        }
        dir.close();
        // Deterministic menu ordering, independent of FAT directory order.
        for(int i=0;i<count_;++i)
            for(int j=i+1;j<count_;++j)
                if(strcmp(apps_[j].id,apps_[i].id)<0) {
                    auto temp=apps_[i];apps_[i]=apps_[j];apps_[j]=temp;
                }
        status_=count_?"Select app to install":"No valid .APP manifests found";
    }
    void onKey(const Keyboard_Class::KeysState& k,bool ready) {
        if(confirming_) {
            if(letter(k,'n')||k.backspace){confirming_=false;return;}
            if(letter(k,'y')) {
                confirming_=false;
                if(count_>0) install(apps_[selected_]);
            }
            return;
        }
        if(k.up && selected_>0)--selected_;
        if(k.down && selected_+1<count_)++selected_;
        if(letter(k,'r'))refresh(ready);
        if(k.enter && count_) {
            confirming_=true;status_="Install to app flash? Y/N";
        }
    }
    void render()const {
        auto &d=M5Cardputer.Display;
        constexpr uint16_t bg=0x0841,panel=0x10c4,white=0xffff,cyan=0x05ff,
                           muted=0xad55,selected=0x2673;
        d.fillScreen(bg);d.fillRect(0,0,240,23,panel);
        d.setTextColor(white,panel);d.setCursor(5,7);d.print("SD APPLICATIONS");
        d.setTextColor(cyan,panel);d.setCursor(207,7);d.print("P4");
        d.fillRect(0,116,240,19,panel);
        d.setTextColor(muted,panel);d.setCursor(4,121);
        d.print(confirming_?"Y install / N cancel":"Enter install | R rescan");
        d.setTextColor(cyan,bg);d.setCursor(6,27);
        d.print(String(count_)+" stored SD programs");
        if(confirming_ && count_) {
            d.setTextColor(white,bg);d.setCursor(5,48);
            d.print(String("Flash ")+apps_[selected_].name+" ?");
            d.setTextColor(muted,bg);d.setCursor(5,67);
            d.print("Factory launcher is retained.");
            d.setCursor(5,80);d.print("Requires a reboot.");
        } else {
            const int top=selected_>=6?selected_-5:0;
            for(int i=top;i<count_ && i<top+6;++i) {
                const int y=40+(i-top)*11;
                if(i==selected_)d.fillRect(3,y-1,233,11,selected);
                d.setTextColor(white,i==selected_?selected:bg);
                d.setCursor(6,y);d.print(String(apps_[i].name).substring(0,35));
            }
        }
        d.setTextColor(muted,bg);d.setCursor(5,106);
        d.print(status_.substring(0,38));
    }
private:
    suite::appmanifest::App apps_[suite::appmanifest::MaxApps];
    int count_=0,selected_=0;
    bool confirming_=false;
    String status_;
    static bool symbol(const Keyboard_Class::KeysState& k,char c) {
        for(char v:k.word)if(v==c)return true;
        return false;
    }
    static bool letter(const Keyboard_Class::KeysState& k,char c) {
        return symbol(k,c)||symbol(k,static_cast<char>(c-'a'+'A'));
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
