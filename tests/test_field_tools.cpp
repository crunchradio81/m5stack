#include "../src/core/FieldTools.h"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace suite::fieldtools;
int main() {
    BeatClock beat;
    bool accented=false,primary=false;
    assert(!beat.setBpm(29)&&!beat.setBpm(261));
    assert(beat.setBpm(120) && beat.setMeter(3));
    assert(!beat.setMeter(1) && !beat.setMeter(13));
    beat.start(100);
    assert(beat.due(100,accented,primary)&&accented&&primary);
    assert(!beat.due(300,accented,primary));
    assert(beat.due(500100,accented,primary)&&!accented&&primary);
    assert(beat.due(1000100,accented,primary)&&!accented);
    assert(beat.due(1500100,accented,primary)&&accented);
    beat.stop();assert(!beat.due(50000000,accented,primary));
    assert(beat.setBpm(60));beat.setSubdivision(2);
    beat.start(UINT32_MAX-1000u);
    assert(beat.due(UINT32_MAX-1000u,accented,primary)&&primary);
    assert(!beat.due(UINT32_MAX-500u,accented,primary));
    // timer rollover (next = 499000us)
    assert(beat.due(499000u,accented,primary)&&!primary);
    assert(beat.beat()==1);
    // refuse impossible piano offsets
    assert(noteForKey('a',0)==60 && noteForKey('A',0)==60);
    assert(noteForKey('w',0)==61);
    assert(noteForKey('s',0)==62);
    assert(noteForKey('k',0)==72);
    assert(noteForKey('k',1)==84);
    assert(noteForKey('a',-2)==36);
    assert(noteForKey('q',0)==-1 && noteForKey('a',3)==-1);
    assert(std::fabs(midiHz(69)-440.0)<0.0001);
    assert(std::fabs(midiHz(60)-261.625565)<0.001);
    assert(midiHz(-1)==0 && midiHz(128)==0);
    Channels channels;
    channels.clear();
    assert(!channels.add(0,-40) && !channels.add(15,-50));
    assert(!channels.add(1,-128) && !channels.add(1,1));
    assert(channels.add(1,-55));
    assert(channels.add(6,-80));
    assert(channels.add(6,-30));
    assert(channels.add(11,-40));
    assert(channels.at(1).count==1);
    assert(channels.at(6).count==2 && channels.at(6).peak==-30);
    assert(channels.at(14).count==0);
    const unsigned suggested=channels.recommended();
    assert(suggested>=1 && suggested<=11);
    channels.clear();
    assert(channels.recommended()==1);
    for(int i=0;i<100;++i) assert(channels.add(1,-35));
    assert(channels.recommended()!=1);
    puts("Field Tools: beat clock, rollover, keyboard piano, channel histogram PASS");
}
