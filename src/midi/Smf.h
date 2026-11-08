#pragma once
// Bounded, streaming Standard MIDI File (SMF) parser.
// Host-testable: no Arduino, no heap allocation, no PSRAM.
// Type 0/1, PPQN timing, running status, meta tempo/time signature,
// MIDI channel events. Type 2 and SMPTE divisions deliberately rejected.
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace suite { namespace smf {

struct Source {
    virtual ~Source() {}
    virtual bool readAt(uint32_t offset, uint8_t* out, size_t n) = 0;
    virtual uint32_t length() const = 0;
};

enum class Kind : uint8_t {
    NoteOn, NoteOff, Control, Program, PitchBend, Tempo, TimeSignature,
    PolyPressure, ChannelPressure, Lyric, Text
};

struct Event {
    uint32_t tick=0;
    uint32_t tempo=500000; // microseconds per quarter note, only for Tempo
    uint8_t track=0, channel=0, a=0, b=0, numerator=4, denominatorPower=2;
    Kind kind=Kind::NoteOff;
    char text[48]={}; // truncated bounded ASCII lyric/text event
};

class Reader {
public:
    static constexpr uint8_t MaxTracks=16;
    static constexpr uint32_t MaxFile=4u*1024u*1024u;
    Reader() { reset(); }
    void reset() {
        source_=nullptr;tracksCount_=0;division_=0;format_=0;failed_=false;
        for(auto &track:tracks_) track=Track{};
    }
    bool open(Source& source) {
        reset();
        uint8_t h[14];
        const uint32_t length=source.length();
        if(length<14 || length>MaxFile || !source.readAt(0,h,sizeof(h))) return fail();
        if(memcmp(h,"MThd",4)!=0) return fail();
        const uint32_t chunk=be32(h+4);
        if(chunk<6 || chunk>length-8) return fail();
        format_=be16(h+8);
        const uint16_t num=be16(h+10);
        division_=be16(h+12);
        if(format_>1 || num<1 || num>MaxTracks ||
           (format_==0 && num!=1) || (division_&0x8000) ||
           !division_) return fail();
        source_=&source;tracksCount_=static_cast<uint8_t>(num);
        uint32_t pos=8+chunk;
        for(uint8_t i=0;i<tracksCount_;++i) {
            uint8_t hdr[8];
            if(pos>length || length-pos<8 ||
               !source_->readAt(pos,hdr,sizeof(hdr)) ||
               memcmp(hdr,"MTrk",4)!=0) return fail();
            const uint32_t size=be32(hdr+4);
            pos+=8;
            if(size>length-pos) return fail();
            Track &t=tracks_[i];
            t.pos=pos;t.end=pos+size;t.index=i;
            pos+=size;
        }
        for(uint8_t i=0;i<tracksCount_;++i)
            if(!prime(tracks_[i])) return fail();
        return !failed_;
    }
    bool next(Event& out) {
        if(failed_ || !source_) return false;
        int best=-1;
        for(uint8_t i=0;i<tracksCount_;++i) {
            if(!tracks_[i].ready) continue;
            if(best<0 || tracks_[i].next.tick<tracks_[best].next.tick)
                best=i;
        }
        if(best<0) return false;
        Track &t=tracks_[best];
        out=t.next;t.ready=false;
        if(!prime(t)) fail();
        return true;
    }
    bool failed() const { return failed_; }
    uint16_t division() const { return division_; }
    uint16_t format() const { return format_; }
    uint8_t trackCount() const { return tracksCount_; }

private:
    struct Track {
        uint32_t pos=0,end=0,tick=0;
        uint32_t cachedAt=0;
        uint8_t cache[96]={};
        uint8_t cached=0,index=0,running=0;
        bool done=false,ready=false;
        Event next;
    };
    Source* source_=nullptr;
    Track tracks_[MaxTracks];
    uint16_t division_=0,format_=0;
    uint8_t tracksCount_=0;
    bool failed_=false;

    bool fail() { failed_=true;return false; }
    static uint16_t be16(const uint8_t* p) {
        return static_cast<uint16_t>((p[0]<<8)|p[1]);
    }
    static uint32_t be32(const uint8_t* p) {
        return (static_cast<uint32_t>(p[0])<<24)|
               (static_cast<uint32_t>(p[1])<<16)|
               (static_cast<uint32_t>(p[2])<<8)|p[3];
    }
    bool get(Track& t,uint8_t& v) {
        if(t.pos>=t.end) return false;
        if(t.cached==0 || t.pos<t.cachedAt ||
           t.pos-t.cachedAt>=t.cached) {
            size_t n=t.end-t.pos;
            if(n>sizeof(t.cache)) n=sizeof(t.cache);
            if(!source_->readAt(t.pos,t.cache,n)) return false;
            t.cachedAt=t.pos;t.cached=static_cast<uint8_t>(n);
        }
        v=t.cache[t.pos-t.cachedAt];++t.pos;return true;
    }
    bool variable(Track& t,uint32_t& result) {
        result=0;
        for(int i=0;i<4;++i) {
            uint8_t b=0;
            if(!get(t,b)) return false;
            result=(result<<7)|(b&0x7f);
            if(!(b&0x80)) return true;
        }
        return false;
    }
    bool skip(Track& t,uint32_t n) {
        if(n>t.end-t.pos) return false;
        t.pos+=n;return true;
    }
    bool data(Track& t,uint8_t& v) {
        return get(t,v) && (v&0x80)==0;
    }
    bool prime(Track& t) {
        // Each call consumes ignored/meta events until one usable event.
        // Bounded by the track length: every event consumes at least a delta.
        while(t.pos<t.end && !t.done) {
            uint32_t delta=0;
            if(!variable(t,delta) || delta>UINT32_MAX-t.tick) return false;
            t.tick+=delta;
            uint8_t first=0;
            if(!get(t,first)) return false;
            if(first==0xff) {
                uint8_t meta=0;uint32_t len=0;
                if(!get(t,meta) || !variable(t,len) || len>t.end-t.pos)
                    return false;
                if(meta==0x2f) {
                    if(len!=0) return false;
                    t.done=true;t.pos=t.end;break;
                }
                if(meta==0x51 && len==3) {
                    uint8_t a,b,c;
                    if(!get(t,a)||!get(t,b)||!get(t,c)) return false;
                    const uint32_t tempo=(static_cast<uint32_t>(a)<<16)|
                                         (static_cast<uint32_t>(b)<<8)|c;
                    if(tempo==0) return false;
                    t.next=Event{};
                    t.next.tick=t.tick;t.next.track=t.index;
                    t.next.kind=Kind::Tempo;t.next.tempo=tempo;
                    t.running=0;t.ready=true;return true;
                }
                if(meta==0x58 && len==4) {
                    uint8_t a,b,c,d;
                    if(!get(t,a)||!get(t,b)||!get(t,c)||!get(t,d)) return false;
                    (void)c;(void)d;
                    t.next=Event{};
                    t.next.tick=t.tick;t.next.track=t.index;
                    t.next.kind=Kind::TimeSignature;
                    t.next.numerator=a;t.next.denominatorPower=b;
                    t.running=0;t.ready=true;return true;
                }
                if((meta==0x05 || meta==0x01) && len>0) {
                    t.next=Event{};
                    t.next.tick=t.tick;t.next.track=t.index;
                    t.next.kind=meta==0x05?Kind::Lyric:Kind::Text;
                    const uint32_t keep=len<sizeof(t.next.text)-1?
                        len:sizeof(t.next.text)-1;
                    for(uint32_t i=0;i<keep;++i){
                        uint8_t c=0;
                        if(!get(t,c))return false;
                        t.next.text[i]=(c>=32 && c<=126)?
                            static_cast<char>(c):' ';
                    }
                    t.next.text[keep]=0;
                    if(!skip(t,len-keep))return false;
                    t.running=0;t.ready=true;return true;
                }
                if(!skip(t,len)) return false;
                t.running=0;continue;
            }
            if(first==0xf0 || first==0xf7) {
                uint32_t len=0;
                if(!variable(t,len)||!skip(t,len)) return false;
                t.running=0;continue;
            }
            uint8_t status=first;
            bool usedFirst=false;uint8_t firstData=0;
            if(first<0x80) {
                if(t.running<0x80 || t.running>=0xf0) return false;
                status=t.running;usedFirst=true;firstData=first;
            } else {
                if(first>=0xf0) return false; // only SMF sysex/meta above
                t.running=first;
            }
            const uint8_t type=status&0xf0;
            const bool two=(type!=0xc0 && type!=0xd0);
            uint8_t a=0,b=0;
            if(usedFirst) a=firstData;
            else if(!data(t,a)) return false;
            if(two && !data(t,b)) return false;
            t.next=Event{};
            t.next.tick=t.tick;t.next.track=t.index;
            t.next.channel=status&0x0f;t.next.a=a;t.next.b=b;
            switch(type) {
                case 0x80:t.next.kind=Kind::NoteOff;break;
                case 0x90:t.next.kind=b?Kind::NoteOn:Kind::NoteOff;break;
                case 0xa0:t.next.kind=Kind::PolyPressure;break;
                case 0xb0:t.next.kind=Kind::Control;break;
                case 0xc0:t.next.kind=Kind::Program;break;
                case 0xd0:t.next.kind=Kind::ChannelPressure;break;
                case 0xe0:t.next.kind=Kind::PitchBend;break;
                default:return false;
            }
            t.ready=true;return true;
        }
        t.done=true;return true;
    }
};

}} // namespace suite::smf
