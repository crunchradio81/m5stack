#include "../src/midi/Seek25Core.h"
#include <cassert>
#include <cstdio>
using namespace suite::midi25;
using suite::smf::Event;
using suite::smf::Kind;
int main(){
    assert(target(0,-1)==0 && target(5000000,-1)==0);
    assert(target(15000000,-1)==5000000);
    assert(target(0,1)==10000000);
    assert(target(MaxTargetUs-1,1)==MaxTargetUs);
    assert(target(7,0)==7);
    assert(eventTime(0,0,480,500000,480)==500000);
    assert(eventTime(500000,480,960,1000000,480)==1500000);
    assert(eventTime(0,100,99,500000,480)==InvalidTime);
    assert(eventTime(0,0,1,500000,0)==InvalidTime);
    assert(eventTime(UINT64_MAX-2,0,100,500000,1)==InvalidTime);
    assert(playbackPosition(0,10000000,10000000,0,100)==0);
    assert(playbackPosition(0,10000000,10000000,5000000,100)==5000000);
    assert(playbackPosition(0,10000000,10000000,5000000,200)==0);
    assert(playbackPosition(0,10000000,10000000,20000000,100)==10000000);
    assert(delayAtSpeed(10000000,50)==20000000);
    assert(delayAtSpeed(10000000,200)==5000000);
    assert(delayAtSpeed(MaxTargetUs+1,100)==-1);
    assert(delayAtSpeed(10,0)==-1);
    Scanner s;s.begin(600000);uint64_t at=0;
    Event e;e.tick=0;e.kind=Kind::Program;e.a=40;
    assert(s.inspect(e,480,at)==Scanner::Result::Consume && at==0);
    e.tick=480;e.kind=Kind::Tempo;e.tempo=1000000;
    assert(s.inspect(e,480,at)==Scanner::Result::Consume && at==500000);
    assert(s.tempo()==1000000 && s.tick()==480 && s.scanned()==2);
    e.tick=960;e.kind=Kind::NoteOn;
    assert(s.inspect(e,480,at)==Scanner::Result::Ready && at==1500000);
    assert(s.scanned()==2 && s.position()==500000);
    s.begin(0);
    assert(s.inspect(e,480,at)==Scanner::Result::Ready);
    assert(s.position()==0 && s.tempo()==500000);
    s.begin(600000);
    e.tick=2;e.kind=Kind::Tempo;e.tempo=0;
    assert(s.inspect(e,480,at)==Scanner::Result::Invalid);
    s.begin(MaxTargetUs+1);
    assert(s.inspect(e,480,at)==Scanner::Result::Invalid);
    puts("P25 MIDI seek: tempo-aware scan, playback position and bounds PASS");
}
