#pragma once
// Host-testable pocket tools for original Cardputer K132.
// No Arduino, external hardware, SD, or wall-clock dependency.
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

namespace suite { namespace pocket {

class Stopwatch {
public:
    static constexpr size_t MaxLaps=8;
    void start(uint32_t now) {
        if(running_) return;
        anchor_=now; running_=true;
    }
    void pause(uint32_t now) {
        if(!running_) return;
        accumulated_+=static_cast<uint32_t>(now-anchor_);
        running_=false;
    }
    void reset() {
        running_=false;accumulated_=0;lapCount_=0;anchor_=0;
    }
    uint64_t elapsed(uint32_t now) const {
        return accumulated_+(running_?static_cast<uint32_t>(now-anchor_):0u);
    }
    bool lap(uint32_t now) {
        if(!running_) return false;
        if(lapCount_<MaxLaps) laps_[lapCount_++]=elapsed(now);
        else {
            for(size_t i=1;i<MaxLaps;++i) laps_[i-1]=laps_[i];
            laps_[MaxLaps-1]=elapsed(now);
        }
        return true;
    }
    uint64_t lapAt(size_t i) const {return i<lapCount_?laps_[i]:0;}
    size_t lapCount() const {return lapCount_;}
    bool running() const {return running_;}
private:
    uint32_t anchor_=0;
    uint64_t accumulated_=0,laps_[MaxLaps]={};
    size_t lapCount_=0;
    bool running_=false;
};

class Countdown {
public:
    static constexpr uint32_t MaximumSeconds=99u*3600u+59u*60u+59u;
    bool setSeconds(uint32_t secs) {
        if(!secs || secs>MaximumSeconds) return false;
        duration_=secs*1000u;
        remaining_=duration_;running_=finished_=false;
        return true;
    }
    void start(uint32_t now) {
        if(running_) return;
        if(!remaining_) remaining_=duration_;
        anchor_=now;running_=true;finished_=false;
    }
    void pause(uint32_t now) {
        if(!running_) return;
        remaining_=left(now);running_=false;
        if(!remaining_) finished_=true;
    }
    bool poll(uint32_t now) {
        if(!running_ || left(now)>0) return false;
        running_=false;remaining_=0;finished_=true;
        return true;
    }
    void reset() {
        running_=false;finished_=false;remaining_=duration_;
    }
    uint32_t left(uint32_t now) const {
        if(!running_) return remaining_;
        const uint32_t elapsed=static_cast<uint32_t>(now-anchor_);
        return elapsed>=remaining_?0:remaining_-elapsed;
    }
    bool running() const {return running_;}
    bool finished() const {return finished_;}
    uint32_t duration() const {return duration_;}
private:
    uint32_t duration_=300000,remaining_=300000,anchor_=0;
    bool running_=false,finished_=false;
};

// Strict 1-6 digit HHMMSS input: "90" -> 90 seconds invalid (SS>59),
// "0130" -> 1m30s, "010203" -> 1h2m3s.
inline bool parseHHMMSS(const char* value,uint32_t& secs) {
    if(!value) return false;
    const size_t n=strlen(value);
    if(n<1 || n>6) return false;
    uint32_t v=0;
    for(size_t i=0;i<n;++i) {
        if(value[i]<'0'||value[i]>'9') return false;
        v=v*10u+static_cast<uint32_t>(value[i]-'0');
    }
    const uint32_t seconds=v%100u;
    const uint32_t minutes=(v/100u)%100u;
    const uint32_t hours=v/10000u;
    if(seconds>=60 || minutes>=60 || hours>99) return false;
    secs=hours*3600u+minutes*60u+seconds;
    return secs!=0;
}

inline bool parseNumber(const char* s,double& out) {
    if(!s || !*s || strlen(s)>30) return false;
    unsigned decimal=0,digits=0;
    for(const char* p=s;*p;++p) {
        if(*p=='.') {if(++decimal>1)return false;}
        else if(*p=='-' && p==s) {}
        else if(*p>='0' && *p<='9')++digits;
        else return false;
    }
    if(!digits) return false;
    // Hand-rolled conversion, no locale or errno dependency.
    double integer=0,scale=0.1;
    bool fraction=false,negative=(*s=='-');
    for(const char* p=s+static_cast<int>(negative);*p;++p) {
        if(*p=='.') {fraction=true;continue;}
        const unsigned x=static_cast<unsigned>(*p-'0');
        if(fraction) {integer+=x*scale;scale*=0.1;}
        else integer=integer*10+x;
    }
    out=negative?-integer:integer;
    return isfinite(out);
}

enum class Units : uint8_t {
    KmToMi,MiToKm,MetresToFeet,FeetToMetres,CtoF,FtoC,
    KgToLb,LbToKg,LitresToGallons,GallonsToLitres,
    KmhToMph,MphToKmh,InchesToCm,CmToInches,
    CelsiusToKelvin,KelvinToCelsius,Count
};
struct UnitPair {
    const char* title;
    const char* from;
    const char* to;
};
inline UnitPair pair(Units type) {
    switch(type) {
        case Units::KmToMi:return {"Kilometres to miles","km","mi"};
        case Units::MiToKm:return {"Miles to kilometres","mi","km"};
        case Units::MetresToFeet:return {"Metres to feet","m","ft"};
        case Units::FeetToMetres:return {"Feet to metres","ft","m"};
        case Units::CtoF:return {"Celsius to Fahrenheit","C","F"};
        case Units::FtoC:return {"Fahrenheit to Celsius","F","C"};
        case Units::KgToLb:return {"Kilograms to pounds","kg","lb"};
        case Units::LbToKg:return {"Pounds to kilograms","lb","kg"};
        case Units::LitresToGallons:return {"Litres to US gallons","L","US gal"};
        case Units::GallonsToLitres:return {"US gallons to litres","US gal","L"};
        case Units::KmhToMph:return {"km/h to mph","km/h","mph"};
        case Units::MphToKmh:return {"mph to km/h","mph","km/h"};
        case Units::InchesToCm:return {"Inches to centimetres","in","cm"};
        case Units::CmToInches:return {"Centimetres to inches","cm","in"};
        case Units::CelsiusToKelvin:return {"Celsius to kelvin","C","K"};
        case Units::KelvinToCelsius:return {"Kelvin to Celsius","K","C"};
        default:return {"Unknown unit","",""};
    }
}
inline bool convert(Units type,double x,double& y) {
    if(!isfinite(x)) return false;
    switch(type) {
        case Units::KmToMi:y=x/1.609344;break;
        case Units::MiToKm:y=x*1.609344;break;
        case Units::MetresToFeet:y=x/0.3048;break;
        case Units::FeetToMetres:y=x*0.3048;break;
        case Units::CtoF:y=x*9.0/5.0+32;break;
        case Units::FtoC:y=(x-32)*5.0/9.0;break;
        case Units::KgToLb:y=x/0.45359237;break;
        case Units::LbToKg:y=x*0.45359237;break;
        case Units::LitresToGallons:y=x/3.785411784;break;
        case Units::GallonsToLitres:y=x*3.785411784;break;
        case Units::KmhToMph:y=x/1.609344;break;
        case Units::MphToKmh:y=x*1.609344;break;
        case Units::InchesToCm:y=x*2.54;break;
        case Units::CmToInches:y=x/2.54;break;
        case Units::CelsiusToKelvin:y=x+273.15;break;
        case Units::KelvinToCelsius:y=x-273.15;break;
        default:return false;
    }
    if((type==Units::CelsiusToKelvin && y<0) ||
       (type==Units::KelvinToCelsius && x<0)) return false;
    return isfinite(y);
}

inline bool parseUnsigned(const char* s,uint8_t base,uint32_t& out) {
    if(!s || !*s || !(base==2||base==8||base==10||base==16)) return false;
    uint32_t value=0;
    for(const char* p=s;*p;++p) {
        const char c=*p;
        uint8_t digit=255;
        if(c>='0'&&c<='9')digit=static_cast<uint8_t>(c-'0');
        else if(c>='a'&&c<='f')digit=static_cast<uint8_t>(c-'a'+10);
        else if(c>='A'&&c<='F')digit=static_cast<uint8_t>(c-'A'+10);
        if(digit>=base || value>(UINT32_MAX-digit)/base) return false;
        value=value*base+digit;
    }
    out=value;return true;
}
inline bool formatUnsigned(uint32_t value,uint8_t base,char* out,size_t cap) {
    if(!out || !cap || !(base==2||base==8||base==10||base==16)) return false;
    char reverse[33];size_t n=0;
    do {
        const uint32_t digit=value%base;
        reverse[n++]=digit<10?static_cast<char>('0'+digit):
                              static_cast<char>('A'+digit-10);
        value/=base;
    } while(value);
    if(n>=cap) return false;
    for(size_t i=0;i<n;++i)out[i]=reverse[n-i-1];
    out[n]=0;return true;
}
enum class BitOp:uint8_t {And,Or,Xor,Not,ShiftLeft,ShiftRight,Count};
inline const char* bitName(BitOp op) {
    switch(op) {
        case BitOp::And:return "AND";
        case BitOp::Or:return "OR";
        case BitOp::Xor:return "XOR";
        case BitOp::Not:return "NOT A";
        case BitOp::ShiftLeft:return "A << B";
        case BitOp::ShiftRight:return "A >> B";
        default:return "Invalid";
    }
}
inline bool bitwise(BitOp op,uint32_t a,uint32_t b,uint32_t& result) {
    switch(op) {
        case BitOp::And:result=a&b;break;
        case BitOp::Or:result=a|b;break;
        case BitOp::Xor:result=a^b;break;
        case BitOp::Not:result=~a;break;
        case BitOp::ShiftLeft:
            if(b>31)return false;
            result=a<<b;break;
        case BitOp::ShiftRight:
            if(b>31)return false;
            result=a>>b;break;
        default:return false;
    }
    return true;
}
}} // namespace suite::pocket
