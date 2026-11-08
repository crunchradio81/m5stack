#pragma once
// Bounded, host-testable CSV codec for the stock Cardputer's Wi-Fi AP log.
// No Arduino, heap allocations, GPS or PSRAM dependency.
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace suite { namespace wifi_csv {
constexpr const char *Header="session,uptime_ms,scan,bssid,ssid,rssi_dbm,channel,security,hidden\n";
constexpr size_t MaxLine=256;
struct Record {
    char session[9]={};
    uint32_t uptimeMs=0, scan=0;
    char bssid[18]={}, ssid[40]={};
    int rssi=0;
    unsigned channel=0;
    char security[20]={};
    bool hidden=false;
};
inline bool append(char *out,size_t cap,size_t &at,char c) {
    if(at+1>=cap) return false;
    out[at++]=c;out[at]=0;return true;
}
inline size_t encode(const Record &r,char *out,size_t cap) {
    if(!out || cap<2) return 0;
    out[0]=0;
    if(strlen(r.session)!=8 || strlen(r.bssid)!=17 || r.channel>14 ||
       r.rssi<-127 || r.rssi>0 || strlen(r.ssid)>32) return 0;
    const int n=snprintf(out,cap,"%s,%lu,%lu,%s,\"",r.session,
                         static_cast<unsigned long>(r.uptimeMs),
                         static_cast<unsigned long>(r.scan),r.bssid);
    if(n<0 || static_cast<size_t>(n)>=cap) return 0;
    size_t at=static_cast<size_t>(n);
    const char *p=r.ssid;
    // Spreadsheet formula injection prevention for exported network names.
    if(*p=='=' || *p=='+' || *p=='-' || *p=='@') {
        if(!append(out,cap,at,'\'')) return 0;
    }
    while(*p) {
        const unsigned char ch=static_cast<unsigned char>(*p++);
        const char c=(ch<32 || ch==127) ? '_' : static_cast<char>(ch);
        if(c=='"' && !append(out,cap,at,'"')) return 0;
        if(!append(out,cap,at,c)) return 0;
    }
    const int suffix=snprintf(out+at,cap-at,"\",%d,%u,%s,%u\n",
                              r.rssi,r.channel,r.security,r.hidden?1u:0u);
    if(suffix<0 || static_cast<size_t>(suffix)>=cap-at) return 0;
    return at+static_cast<size_t>(suffix);
}
inline bool field(const char *&s,char *out,size_t cap,bool last) {
    if(!s || !out || !cap) return false;
    size_t n=0;
    if(*s=='"') {
        ++s;
        bool closed=false;
        while(*s) {
            if(*s=='"') {
                ++s;
                if(*s!='"') { closed=true;break; }
            }
            if(n+1>=cap) return false;
            out[n++]=*s++;
        }
        if(!closed) return false;
    } else {
        while(*s && *s!=',' && *s!='\r' && *s!='\n') {
            if(n+1>=cap || *s=='"') return false;
            out[n++]=*s++;
        }
    }
    out[n]=0;
    if(last) {
        if(*s=='\r') ++s;
        return *s==0;
    }
    if(*s!=',') return false;
    ++s;return true;
}
inline bool unsignedNumber(const char *s,uint32_t &out) {
    if(!s || !*s) return false;
    for(const char *p=s;*p;++p) if(*p<'0' || *p>'9') return false;
    char *end=nullptr;
    const unsigned long n=strtoul(s,&end,10);
    if(!end || *end || n>UINT32_MAX) return false;
    out=static_cast<uint32_t>(n);return true;
}
inline bool signedNumber(const char *s,int &out) {
    if(!s || !*s) return false;
    char *end=nullptr;
    const long n=strtol(s,&end,10);
    if(!end || *end || n < -127 || n>0) return false;
    out=static_cast<int>(n);return true;
}
inline bool decode(const char *line,Record &out) {
    if(!line || strlen(line)>=MaxLine) return false;
    char fields[9][64]={{0}};
    const char *s=line;
    for(int i=0;i<9;++i) if(!field(s,fields[i],sizeof(fields[i]),i==8)) return false;
    if(strlen(fields[0])!=8 || strlen(fields[3])!=17 ||
       strlen(fields[4])>=sizeof(out.ssid) ||
       strlen(fields[7])>=sizeof(out.security)) return false;
    uint32_t time=0,scan=0,channel=0,hidden=0;
    int rssi=0;
    if(!unsignedNumber(fields[1],time) ||
       !unsignedNumber(fields[2],scan) ||
       !signedNumber(fields[5],rssi) ||
       !unsignedNumber(fields[6],channel) || channel>14 ||
       !unsignedNumber(fields[8],hidden) || hidden>1) return false;
    Record r;
    strcpy(r.session,fields[0]);
    r.uptimeMs=time;r.scan=scan;
    strcpy(r.bssid,fields[3]);strcpy(r.ssid,fields[4]);
    r.rssi=rssi;r.channel=static_cast<unsigned>(channel);
    strcpy(r.security,fields[7]);r.hidden=hidden!=0;
    out=r;return true;
}
}} // namespace suite::wifi_csv
