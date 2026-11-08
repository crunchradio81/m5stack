#include "../src/core/PocketTools.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <stdint.h>

using namespace suite::pocket;
static void almost(double actual,double expected) {
    assert(std::fabs(actual-expected)<=1e-7*std::fmax(1.0,std::fabs(expected)));
}
int main() {
    Stopwatch sw;
    assert(sw.elapsed(123)==0 && !sw.running());
    sw.start(1000);sw.start(1500);
    assert(sw.elapsed(1800)==800);
    assert(sw.lap(2000) && sw.lapCount()==1 && sw.lapAt(0)==1000);
    sw.pause(2500);assert(sw.elapsed(9999)==1500);
    sw.start(10000);sw.pause(12500);assert(sw.elapsed(13000)==4000);
    sw.reset();assert(!sw.running() && sw.lapCount()==0 && sw.elapsed(13000)==0);
    // Unsigned millis() overflow: begin 16ms before wrap, 32ms later.
    sw.start(UINT32_MAX-15);
    assert(sw.elapsed(16)==32);
    sw.pause(16);assert(sw.elapsed(17)==32);
    sw.reset();sw.start(0);
    for(int i=1;i<=12;++i)assert(sw.lap(static_cast<uint32_t>(i*1000)));
    assert(sw.lapCount()==8 && sw.lapAt(0)==5000 && sw.lapAt(7)==12000);

    Countdown timer;
    assert(timer.duration()==300000);
    assert(!timer.setSeconds(0));
    assert(!timer.setSeconds(Countdown::MaximumSeconds+1));
    assert(timer.setSeconds(75));
    timer.start(1000);
    assert(timer.left(1500)==74500);
    timer.pause(2500);assert(timer.left(4500)==73500);
    timer.start(5000);assert(timer.left(6500)==72000);
    assert(!timer.poll(76000)); // 71000 elapsed since resumption
    assert(timer.poll(77000) && timer.finished() && !timer.running());
    assert(!timer.poll(78000)); // one completion edge
    timer.reset();assert(!timer.finished() && timer.left(0)==75000);
    assert(timer.setSeconds(2));
    timer.start(UINT32_MAX-1000);
    assert(timer.left(1000)==0);
    assert(timer.poll(1000) && timer.finished());
    uint32_t secs=0;
    assert(parseHHMMSS("000130",secs) && secs==90);
    assert(parseHHMMSS("010203",secs) && secs==3723);
    assert(parseHHMMSS("1",secs) && secs==1);
    assert(parseHHMMSS("235959",secs) && secs==86399);
    assert(!parseHHMMSS("0060",secs));
    assert(!parseHHMMSS("90",secs));
    assert(!parseHHMMSS("000000",secs));
    assert(!parseHHMMSS("1.23",secs));
    assert(!parseHHMMSS("1234567",secs));

    double x=0,y=0;
    assert(parseNumber("-12.5",x));almost(x,-12.5);
    assert(parseNumber(".25",x));almost(x,0.25);
    assert(parseNumber("0",x));almost(x,0);
    assert(!parseNumber("",x));
    assert(!parseNumber("nan",x));
    assert(!parseNumber("12.5.3",x));
    assert(!parseNumber("1e9",x));
    assert(convert(Units::KmToMi,1.609344,y));almost(y,1);
    assert(convert(Units::MiToKm,1,y));almost(y,1.609344);
    assert(convert(Units::MetresToFeet,0.3048,y));almost(y,1);
    assert(convert(Units::FeetToMetres,1,y));almost(y,0.3048);
    assert(convert(Units::CtoF,0,y));almost(y,32);
    assert(convert(Units::FtoC,212,y));almost(y,100);
    assert(convert(Units::KgToLb,0.45359237,y));almost(y,1);
    assert(convert(Units::LbToKg,1,y));almost(y,0.45359237);
    assert(convert(Units::LitresToGallons,3.785411784,y));almost(y,1);
    assert(convert(Units::GallonsToLitres,1,y));almost(y,3.785411784);
    assert(convert(Units::KmhToMph,1.609344,y));almost(y,1);
    assert(convert(Units::MphToKmh,1,y));almost(y,1.609344);
    assert(convert(Units::InchesToCm,1,y));almost(y,2.54);
    assert(convert(Units::CmToInches,2.54,y));almost(y,1);
    assert(convert(Units::CelsiusToKelvin,-273.15,y));almost(y,0);
    assert(convert(Units::KelvinToCelsius,273.15,y));almost(y,0);
    assert(!convert(Units::CelsiusToKelvin,-280,y));
    assert(!convert(Units::KelvinToCelsius,-1,y));
    assert(!convert(Units::Count,15,y));

    uint32_t n=0,v=0;
    assert(parseUnsigned("11111111",2,n) && n==255);
    assert(parseUnsigned("377",8,n) && n==255);
    assert(parseUnsigned("4294967295",10,n) && n==UINT32_MAX);
    assert(parseUnsigned("FFFFFFFF",16,n) && n==UINT32_MAX);
    assert(parseUnsigned("aBcD",16,n) && n==0xABCD);
    assert(!parseUnsigned("",10,n));
    assert(!parseUnsigned("2",2,n));
    assert(!parseUnsigned("8",8,n));
    assert(!parseUnsigned("4294967296",10,n));
    assert(!parseUnsigned("-1",10,n));
    assert(!parseUnsigned("ABC",10,n));
    char out[34]={};
    assert(formatUnsigned(UINT32_MAX,2,out,sizeof(out)) && strlen(out)==32);
    assert(formatUnsigned(255,8,out,sizeof(out)) && strcmp(out,"377")==0);
    assert(formatUnsigned(255,10,out,sizeof(out)) && strcmp(out,"255")==0);
    assert(formatUnsigned(255,16,out,sizeof(out)) && strcmp(out,"FF")==0);
    assert(formatUnsigned(0,16,out,sizeof(out)) && strcmp(out,"0")==0);
    char tiny[2];assert(!formatUnsigned(255,16,tiny,sizeof(tiny)));
    assert(bitwise(BitOp::And,0xF0,0xAA,v) && v==0xA0);
    assert(bitwise(BitOp::Or,0xF0,0x0F,v) && v==0xFF);
    assert(bitwise(BitOp::Xor,0xF0,0x0F,v) && v==0xFF);
    assert(bitwise(BitOp::Not,0,0,v) && v==UINT32_MAX);
    assert(bitwise(BitOp::ShiftLeft,1,31,v) && v==0x80000000u);
    assert(bitwise(BitOp::ShiftRight,0x80000000u,31,v) && v==1);
    assert(!bitwise(BitOp::ShiftLeft,1,32,v));
    assert(!bitwise(BitOp::ShiftRight,1,UINT32_MAX,v));
    assert(!bitwise(BitOp::Count,1,1,v));
    puts("Pocket Tools: stopwatch, countdown, conversion, 32-bit bitwise PASS");
}
