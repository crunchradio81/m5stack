#pragma once
// Bounded, deterministic SD app manifest parser for the original K132.
// This format is NOT a digital signature. SHA-256 checks accidental corruption;
// the owner must only install trusted original Cardputer binaries.
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
namespace suite { namespace appmanifest {
constexpr size_t MaxManifest=512;
constexpr uint32_t MaxBinary=0x300000u;
constexpr uint8_t MaxApps=32;
struct App {
    char name[30]={};
    char id[12]={};
    char image[48]={};
    char sha256[65]={};
    uint32_t size=0;
};
inline bool equals(const char*a,const char*b){return strcmp(a,b)==0;}
inline bool authorizedId(const char* id) {
    return equals(id,"METRONOME")||equals(id,"PIANO")||
           equals(id,"CHANNELS")||equals(id,"BLE")||
           equals(id,"ARCADE")||equals(id,"OFFICE")||
           equals(id,"INSPECT")||equals(id,"MUSICLAB");
}
inline bool safeFilename(const char *s) {
    const size_t n=strlen(s);
    if(n<5 || n>40) return false;
    if(strcmp(s+n-4,".BIN")!=0) return false;
    for(size_t i=0;i<n-4;++i) {
        const char c=s[i];
        if(!((c>='A'&&c<='Z')||(c>='0'&&c<='9')||
             c=='_'||c=='-')) return false;
    }
    return true;
}
inline bool hex(char c) {
    return (c>='0'&&c<='9')||(c>='a'&&c<='f');
}
inline bool parseSize(const char *s,uint32_t &out) {
    if(!*s) return false;
    uint32_t n=0;
    for(;*s;++s) {
        if(*s<'0'||*s>'9')return false;
        const uint32_t d=static_cast<uint32_t>(*s-'0');
        if(n>(UINT32_MAX-d)/10)return false;
        n=n*10+d;
    }
    if(n<64 || n>MaxBinary)return false;
    out=n;return true;
}
inline bool parse(const char* data,size_t length,App& output) {
    if(!data || length<12 || length>MaxManifest) return false;
    App candidate;
    unsigned mask=0;
    size_t pos=0;
    bool header=false;
    while(pos<length) {
        size_t start=pos;
        while(pos<length && data[pos]!='\n') {
            const unsigned char c=static_cast<unsigned char>(data[pos]);
            if(c<32||c>126)return false;
            ++pos;
        }
        const size_t len=pos-start;
        if(pos<length) ++pos;
        if(!len)return false;
        if(len>100)return false;
        char line[101]={};
        memcpy(line,data+start,len);
        if(!header) {
            if(!equals(line,"K132APP1"))return false;
            header=true;continue;
        }
        char *sep=strchr(line,'=');
        if(!sep)return false;
        *sep++=0;
        unsigned bit=0;
        if(equals(line,"NAME")) {
            bit=1;
            if(!*sep||strlen(sep)>=sizeof(candidate.name))return false;
            strcpy(candidate.name,sep);
        } else if(equals(line,"ID")) {
            bit=2;
            if(strlen(sep)>=sizeof(candidate.id))return false;
            strcpy(candidate.id,sep);
        } else if(equals(line,"IMAGE")) {
            bit=4;
            if(strlen(sep)>=sizeof(candidate.image))return false;
            strcpy(candidate.image,sep);
        } else if(equals(line,"SIZE")) {
            bit=8;
            if(!parseSize(sep,candidate.size))return false;
        } else if(equals(line,"SHA256")) {
            bit=16;
            if(strlen(sep)!=64)return false;
            for(const char*p=sep;*p;++p)if(!hex(*p))return false;
            strcpy(candidate.sha256,sep);
        } else return false;
        if(mask&bit)return false;
        mask|=bit;
    }
    if(!header||mask!=31||!authorizedId(candidate.id)||
       !safeFilename(candidate.image)) return false;
    // Lock the image name to the declared app ID: no aliases or paths.
    char expected[48]={};
    strcpy(expected,candidate.id);
    strcat(expected,".BIN");
    if(!equals(expected,candidate.image))return false;
    output=candidate;
    return true;
}
}} // namespace suite::appmanifest
