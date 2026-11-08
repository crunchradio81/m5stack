#include "../src/midi/Synth.h"
#include <cassert>
#include <cstdio>
using namespace suite;
static smf::Event note(smf::Kind kind,uint8_t ch,uint8_t key,uint8_t v,uint8_t track=0){
    smf::Event e;e.kind=kind;e.channel=ch;e.a=key;e.b=v;e.track=track;return e;
}
int main(){
    midi::Synth s;
    int16_t samples[1024]={};
    assert(s.activeVoices()==0);
    s.event(note(smf::Kind::NoteOn,0,60,110));
    assert(s.activeVoices()==1);
    s.render(samples,1024);
    bool nonZero=false;
    for(int16_t x:samples) if(x!=0){nonZero=true;break;}
    assert(nonZero);
    s.event(note(smf::Kind::NoteOff,0,60,0));
    for(int i=0;i<400;++i) s.render(samples,1024);
    assert(s.activeVoices()==0);
    for(int i=0;i<50;++i)
        s.event(note(smf::Kind::NoteOn,1,static_cast<uint8_t>(30+i),80));
    assert(s.activeVoices()<=midi::Synth::Voices);
    s.muteTrack(0,true);
    assert(s.activeVoices()==0);
    s.event(note(smf::Kind::NoteOn,1,64,100,0));
    assert(s.activeVoices()==0);
    s.muteTrack(0,false);
    s.event(note(smf::Kind::NoteOn,0,64,100,0));
    assert(s.activeVoices()==1);
    smf::Event cc=note(smf::Kind::Control,0,64,127);s.event(cc);
    s.event(note(smf::Kind::NoteOff,0,64,0,0));
    s.render(samples,1024);assert(s.activeVoices()==1); // sustain holds note
    cc.b=0;s.event(cc);
    for(int i=0;i<400;++i) s.render(samples,1024);
    assert(s.activeVoices()==0);
    smf::Event bend=note(smf::Kind::PitchBend,0,0,127);s.event(bend);
    s.event(note(smf::Kind::NoteOn,9,36,120));
    assert(s.activeVoices()==1);
    s.event(note(smf::Kind::Control,9,120,0));
    assert(s.activeVoices()==0);
    puts("Fixed-memory synthesizer: PASS (polyphony, mute, sustain, bend, drums)");
}
