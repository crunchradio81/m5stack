#include "../src/midi/Smf.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace suite::smf;
struct Buffer : Source {
    std::vector<uint8_t> bytes;
    bool readAt(uint32_t offset,uint8_t* out,size_t n) override {
        if(offset>bytes.size() || n>bytes.size()-offset) return false;
        memcpy(out,bytes.data()+offset,n);return true;
    }
    uint32_t length() const override {return static_cast<uint32_t>(bytes.size());}
};
static void append32(std::vector<uint8_t>& v,uint32_t n) {
    v.push_back(n>>24);v.push_back(n>>16);v.push_back(n>>8);v.push_back(n);
}
static void append16(std::vector<uint8_t>& v,uint16_t n) {
    v.push_back(n>>8);v.push_back(n);
}
static void track(Buffer& b,const std::vector<uint8_t>& bytes) {
    b.bytes.insert(b.bytes.end(),{'M','T','r','k'});
    append32(b.bytes,static_cast<uint32_t>(bytes.size()));
    b.bytes.insert(b.bytes.end(),bytes.begin(),bytes.end());
}
static Buffer valid() {
    Buffer b;
    b.bytes={'M','T','h','d'};
    append32(b.bytes,6);append16(b.bytes,1);append16(b.bytes,2);append16(b.bytes,480);
    track(b,{
        0x00,0xff,0x58,0x04,0x04,0x02,0x18,0x08,
        0x00,0xff,0x51,0x03,0x07,0xa1,0x20, // 500000
        0x83,0x60,0xff,0x51,0x03,0x0f,0x42,0x40, // delta 480, 1M
        0x00,0xff,0x2f,0x00
    });
    track(b,{
        0x00,0xc0,0x28, // program 40
        0x00,0x90,0x3c,0x64, // note 60
        0x81,0x70,0x40,0x50, // running-status note 64, tick240
        0x81,0x70,0x80,0x3c,0x00, // note-off 60 at 480
        0x00,0x90,0x40,0x00, // velocity zero note-off 64
        0x00,0xff,0x2f,0x00
    });
    return b;
}
int main() {
    Buffer b=valid();Reader p;Event e;
    assert(p.open(b) && p.format()==1 && p.division()==480 && p.trackCount()==2);
    assert(p.next(e) && e.kind==Kind::TimeSignature && e.numerator==4);
    assert(p.next(e) && e.kind==Kind::Tempo && e.tempo==500000 && e.tick==0);
    assert(p.next(e) && e.kind==Kind::Program && e.channel==0 && e.a==40);
    assert(p.next(e) && e.kind==Kind::NoteOn && e.a==60 && e.b==100 && e.tick==0);
    assert(p.next(e) && e.kind==Kind::NoteOn && e.a==64 && e.b==80 && e.tick==240);
    assert(p.next(e) && e.kind==Kind::Tempo && e.tempo==1000000 && e.tick==480);
    assert(p.next(e) && e.kind==Kind::NoteOff && e.a==60 && e.tick==480);
    assert(p.next(e) && e.kind==Kind::NoteOff && e.a==64 && e.tick==480);
    assert(!p.next(e) && !p.failed());
    Buffer bad=b;bad.bytes[12]=0xE7;assert(!p.open(bad)); // SMPTE
    bad=b;bad.bytes[9]=2;assert(!p.open(bad)); // format 2 unsupported
    bad=b;bad.bytes[13]=0;bad.bytes[12]=0;assert(!p.open(bad)); // PPQN 0
    bad=b;bad.bytes.resize(21);assert(!p.open(bad)); // missing tracks
    bad=b;bad.bytes[14]='X';assert(!p.open(bad)); // invalid track tag
    bad=b;bad.bytes[18]=0xff;assert(!p.open(bad)); // truncated track length
    // Running status without a previous channel status.
    Buffer noRunning;
    noRunning.bytes={'M','T','h','d'};append32(noRunning.bytes,6);
    append16(noRunning.bytes,0);append16(noRunning.bytes,1);append16(noRunning.bytes,96);
    track(noRunning,{0,60,100,0,0xff,0x2f,0});
    assert(!p.open(noRunning));
    // Oversized delta VLQ is rejected instead of spinning.
    Buffer overlong;
    overlong.bytes={'M','T','h','d'};append32(overlong.bytes,6);
    append16(overlong.bytes,0);append16(overlong.bytes,1);append16(overlong.bytes,96);
    track(overlong,{0x81,0x80,0x80,0x80,0x00,0x90,60,100});
    assert(!p.open(overlong));
    // Type 0 baseline fixture.
    Buffer single;
    single.bytes={'M','T','h','d'};append32(single.bytes,6);
    append16(single.bytes,0);append16(single.bytes,1);append16(single.bytes,96);
    track(single,{0,0x90,60,90,0x60,0x80,60,0,0,0xff,0x2f,0});
    assert(p.open(single));assert(p.next(e) && e.kind==Kind::NoteOn && e.tick==0);
    assert(p.next(e) && e.kind==Kind::NoteOff && e.tick==96);
    assert(!p.next(e) && !p.failed());
    puts("SMF streaming parser: PASS (format 0/1, tempo, time signature, running status, bounds)");
}
