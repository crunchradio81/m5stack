#include "../src/core/MusicLabCore.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace suite::musiclab;
int main() {
    assert(strcmp(morse('S'),"...")==0);
    assert(strcmp(morse('o'),"---")==0);
    assert(strcmp(morse('5'),".....")==0);
    assert(morse('?')==nullptr);
    char code[256]={};
    assert(encodeMorse("SOS",code,sizeof(code)));
    assert(strcmp(code,"... --- ...")==0);
    assert(encodeMorse("HI 7",code,sizeof(code)));
    assert(strcmp(code,".... .. / --...")==0);
    char small[4];
    assert(!encodeMorse("SOS",small,sizeof(small)));
    assert(!encodeMorse("",code,sizeof(code)));
    assert(!encodeMorse("HELLO?",code,sizeof(code)));
    TapTempo tap;
    assert(tap.bpm()==0 && tap.samples()==0);
    tap.tap(1000);tap.tap(1500);tap.tap(2000);
    assert(tap.bpm()==120 && tap.samples()==2);
    tap.tap(2500);assert(tap.bpm()==120);
    tap.tap(10000);assert(tap.bpm()==0);
    tap.tap(10750);assert(tap.bpm()==80);
    tap.clear();assert(tap.samples()==0);
    tap.tap(UINT32_MAX-249);tap.tap(250); // 500 ms across wrap
    assert(tap.bpm()==120);
    puts("Music Lab: Morse coding and tapping/rollover PASS");
}
