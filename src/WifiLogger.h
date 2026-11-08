#pragma once
// Built-in 2.4GHz AP discovery only: passive scan, no injection or Wi-Fi join.
// SD is required; always persist individual observations to a bounded CSV.
#include <Arduino.h>
#include <WiFi.h>
#include <SD.h>
#include <esp_system.h>
#include <string.h>
#include "core/WifiCsv.h"

class WifiLogger {
public:
    static constexpr const char *Path="/SUITE/WIFI/WIFI.CSV";
    static constexpr uint32_t GapMs=5000;
    static constexpr uint32_t MaxBytes=32u*1024u*1024u;
    static constexpr int LiveRows=6;
    bool running=false, scanning=false;
    uint32_t scans=0, observations=0;
    int lastSeen=0, recentCount=0;
    suite::wifi_csv::Record recent[LiveRows];
    String status="Paused";

    bool start(bool sdReady) {
        if(running) return true;
        if(!sdReady) {status="Insert FAT32 SD to log";return false;}
        if(!ensureFile()) return false;
        if(!session_[0]) {
            snprintf(session_,sizeof(session_),"%08lX",
                     static_cast<unsigned long>(esp_random()));
        }
        running=true;scanning=false;nextScan_=0;
        status="Logging to SD";return true;
    }
    void pause() {
        if(scanning) {
            WiFi.scanDelete();WiFi.mode(WIFI_OFF);
        }
        running=false;scanning=false;
        if(status=="Logging to SD" || status=="Scanning") status="Paused";
    }
    void tick() {
        if(!running) return;
        if(!scanning) {
            if(nextScan_ && static_cast<int32_t>(millis()-nextScan_)<0) return;
            WiFi.mode(WIFI_STA);
            WiFi.disconnect();
            WiFi.scanDelete();
            const int result=WiFi.scanNetworks(true,true,true,250);
            if(result==WIFI_SCAN_FAILED) {
                stopWith("Wi-Fi scan failed");return;
            }
            scanning=true;status="Scanning";return;
        }
        const int n=WiFi.scanComplete();
        if(n==WIFI_SCAN_RUNNING) return;
        if(n<0) {stopWith("Wi-Fi scan failed");return;}
        if(!saveResults(n)) {stopWith(status);return;}
        WiFi.scanDelete();WiFi.mode(WIFI_OFF);
        scanning=false;nextScan_=millis()+GapMs;
        status="Logging to SD";
    }
    bool erase(bool sdReady) {
        pause();
        if(!sdReady) {status="No SD card";return false;}
        if(!SD.exists(Path)) {
            observations=scans=0;recentCount=lastSeen=0;
            status="Log already empty";return true;
        }
        if(!SD.remove(Path) || SD.exists(Path)) {
            status="Delete failed; SD unchanged";return false;
        }
        observations=scans=0;recentCount=lastSeen=0;
        session_[0]=0;
        status="Log erased";return true;
    }
    bool readFromEnd(uint32_t skip,suite::wifi_csv::Record &record,bool sdReady) const {
        if(!sdReady) return false;
        File file=SD.open(Path,FILE_READ);
        if(!file || file.isDirectory()) {if(file)file.close();return false;}
        int32_t end=static_cast<int32_t>(file.size());
        // A power-loss-truncated trailing row has no newline; ignore that row.
        if(end>0) {
            if(!file.seek(end-1)) {file.close();return false;}
            if(file.read()!='\n') {
                while(end>0) {
                    if(!file.seek(end-1)) break;
                    if(file.read()=='\n') break;
                    --end;
                }
            }
        }
        bool found=false;
        while(end>0) {
            while(end>0) {
                if(!file.seek(end-1)) {end=0;break;}
                int c=file.read();
                if(c!='\n' && c!='\r') break;
                --end;
            }
            if(end<=0) break;
            int32_t start=end;
            while(start>0) {
                if(!file.seek(start-1)) {start=0;break;}
                if(file.read()=='\n') break;
                --start;
            }
            const size_t len=static_cast<size_t>(end-start);
            if(len>0 && len<suite::wifi_csv::MaxLine && file.seek(start)) {
                char line[suite::wifi_csv::MaxLine]={};
                if(file.readBytes(line,len)==len) {
                    line[len]=0;
                    suite::wifi_csv::Record candidate;
                    if(suite::wifi_csv::decode(line,candidate)) {
                        if(skip==0) {record=candidate;found=true;break;}
                        --skip;
                    }
                }
            }
            if(start==0) break;
            end=start-1;
        }
        file.close();return found;
    }
private:
    char session_[9]={};
    uint32_t nextScan_=0;

    static const char *authName(int auth) {
        switch(auth) {
            case WIFI_AUTH_OPEN:return "OPEN";
            case WIFI_AUTH_WEP:return "WEP";
            case WIFI_AUTH_WPA_PSK:return "WPA";
            case WIFI_AUTH_WPA2_PSK:return "WPA2";
            case WIFI_AUTH_WPA_WPA2_PSK:return "WPA/WPA2";
            case WIFI_AUTH_WPA2_ENTERPRISE:return "WPA2-ENT";
            case WIFI_AUTH_WPA3_PSK:return "WPA3";
            case WIFI_AUTH_WPA2_WPA3_PSK:return "WPA2/WPA3";
            default:return "OTHER";
        }
    }
    void stopWith(const String &reason) {
        pause();
        status=reason;
    }
    bool ensureFile() {
        if(!SD.exists("/SUITE")) SD.mkdir("/SUITE");
        if(!SD.exists("/SUITE/WIFI") && !SD.mkdir("/SUITE/WIFI")) {
            status="Cannot create WIFI folder";return false;
        }
        if(SD.exists(Path)) {
            File input=SD.open(Path,FILE_READ);
            if(!input || input.isDirectory()) {
                if(input)input.close();
                status="Cannot open existing log";return false;
            }
            char header[88]={};
            const size_t n=input.readBytesUntil('\n',header,sizeof(header)-1);
            const bool valid= n+1==strlen(suite::wifi_csv::Header) &&
                              strncmp(header,suite::wifi_csv::Header,n)==0;
            const uint32_t size=input.size();
            int last='\n';
            if(size>0 && input.seek(size-1)) last=input.read();
            input.close();
            if(!valid) {status="Unknown log format; not overwriting";return false;}
            if(size>=MaxBytes) {status="Log 32MB cap; paused";return false;}
            if(last!='\n') {
                File repair=SD.open(Path,FILE_APPEND);
                if(!repair || repair.write(static_cast<uint8_t>('\n'))!=1) {
                    if(repair)repair.close();
                    status="Cannot recover partial row";return false;
                }
                repair.flush();repair.close();
            }
            return true;
        }
        File output=SD.open(Path,FILE_WRITE);
        if(!output) {status="Cannot create WIFI.CSV";return false;}
        const size_t len=strlen(suite::wifi_csv::Header);
        const bool ok=output.write(reinterpret_cast<const uint8_t*>(suite::wifi_csv::Header),len)==len;
        output.flush();output.close();
        if(!ok) {status="Cannot write CSV header";return false;}
        return true;
    }
    void makeRecord(int index,suite::wifi_csv::Record &r) const {
        strncpy(r.session,session_,sizeof(r.session)-1);
        r.uptimeMs=millis();r.scan=scans;
        String mac=WiFi.BSSIDstr(index);
        strncpy(r.bssid,mac.c_str(),sizeof(r.bssid)-1);
        String name=WiFi.SSID(index);
        const size_t len=name.length()>32?32:static_cast<size_t>(name.length());
        memcpy(r.ssid,name.c_str(),len);r.ssid[len]=0;
        r.hidden=name.length()==0;
        r.rssi=WiFi.RSSI(index);
        r.channel=WiFi.channel(index);
        strncpy(r.security,authName(static_cast<int>(WiFi.encryptionType(index))),
                sizeof(r.security)-1);
    }
    bool saveResults(int n) {
        File output=SD.open(Path,FILE_APPEND);
        if(!output) {status="SD missing; paused";return false;}
        ++scans;
        lastSeen=n;recentCount=0;
        bool ok=true;
        for(int i=0;i<n;++i) {
            suite::wifi_csv::Record r;
            makeRecord(i,r);
            if(recentCount<LiveRows) recent[recentCount++]=r;
            char line[suite::wifi_csv::MaxLine]={};
            const size_t len=suite::wifi_csv::encode(r,line,sizeof(line));
            if(!len || output.size()+len>MaxBytes ||
               output.write(reinterpret_cast<const uint8_t*>(line),len)!=len) {
                status=output.size()+len>MaxBytes?"Log 32MB cap; paused":"SD write error; paused";
                ok=false;break;
            }
            ++observations;
        }
        output.flush();output.close();
        return ok;
    }
};
