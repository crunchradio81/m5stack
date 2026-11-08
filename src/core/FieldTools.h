#pragma once
// Portable beat clock, musical keyboard layout and 2.4GHz channel analytics.
// No Arduino, heap, network, RTC or external hardware dependencies.
#include <stdint.h>
#include <stddef.h>
namespace suite { namespace fieldtools {
class BeatClock {
public:
    bool setBpm(uint16_t bpm) {if(bpm<30||bpm>260)return false;bpm_=bpm;return true;}
    bool setMeter(uint8_t beats) {if(beats<2||beats>12)return false;meter_=beats;return true;}
    void setSubdivision(uint8_t parts) {if(parts==1||parts==2||parts==4)subdivision_=parts;}
    void start(uint32_t nowUs) {
        running_=true; next_=nowUs; beat_=0;phase_=0;
    }
    void stop(){running_=false;}
    bool running()const{return running_;}
    uint16_t bpm()const{return bpm_;}
    uint8_t meter()const{return meter_;}
    uint8_t subdivision()const{return subdivision_;}
    uint8_t beat()const{return beat_;}
    bool due(uint32_t nowUs,bool& accented,bool& primary) {
        if(!running_ || static_cast<int32_t>(nowUs-next_)<0)return false;
        accented=(beat_==0&&phase_==0);
        primary=(phase_==0);
        const uint32_t interval=60000000u/
            (static_cast<uint32_t>(bpm_)*subdivision_);
        next_+=interval;
        if(static_cast<int32_t>(nowUs-next_)>static_cast<int32_t>(interval*6))
            next_=nowUs+interval; // missed frames: no runaway catch-up
        if(++phase_>=subdivision_) {
            phase_=0;
            beat_=static_cast<uint8_t>((beat_+1)%meter_);
        }
        return true;
    }
private:
    uint32_t next_=0;
    uint16_t bpm_=120;
    uint8_t meter_=4,subdivision_=1,beat_=0,phase_=0;
    bool running_=false;
};
inline int noteForKey(char c,int octaveOffset) {
    // White keys A..K; black keys W E T Y U. C4=60.
    if(octaveOffset< -2||octaveOffset>2)return -1;
    if(c>='A'&&c<='Z')c=static_cast<char>(c+('a'-'A'));
    const char *const keys="awsedftgyhujk";
    const uint8_t notes[]={60,61,62,63,64,65,66,67,68,69,70,71,72};
    for(size_t i=0;keys[i];++i)
        if(c==keys[i]) return static_cast<int>(notes[i])+12*octaveOffset;
    return -1;
}
inline double midiHz(int note) {
    if(note<0||note>127)return 0;
    // Valid MIDI notes; pow implementation lives in standard math library.
    return 440.0*__builtin_pow(2.0,(note-69)/12.0);
}
struct Channel {
    uint16_t count=0;
    int8_t peak=-127;
    uint32_t strength=0;
};
class Channels {
public:
    void clear(){for(auto& c:channels_)c=Channel{};}
    bool add(unsigned channel,int rssi) {
        if(channel<1||channel>14||rssi<-127||rssi>0)return false;
        Channel& c=channels_[channel-1];
        if(c.count<UINT16_MAX)++c.count;
        if(rssi>c.peak)c.peak=static_cast<int8_t>(rssi);
        // Heuristic: stronger AP contributes a greater overlapping-channel score.
        const uint32_t weight=static_cast<uint32_t>(rssi+128);
        if(c.strength<=UINT32_MAX-weight)c.strength+=weight;
        return true;
    }
    Channel at(unsigned channel) const {
        return channel>=1&&channel<=14?channels_[channel-1]:Channel{};
    }
    unsigned recommended()const {
        // Conservatively compare North American 2.4GHz channels 1-11 only.
        unsigned best=1;uint64_t score=UINT64_MAX;
        for(unsigned c=1;c<=11;++c) {
            uint64_t weight=0;
            for(unsigned observed=1;observed<=14;++observed) {
                const unsigned distance=c>observed?c-observed:observed-c;
                if(distance>2)continue;
                weight+=static_cast<uint64_t>(channels_[observed-1].strength)*
                        (3u-distance);
            }
            if(weight<score){score=weight;best=c;}
        }
        return best;
    }
private:
    Channel channels_[14]={};
};
}} // namespace suite::fieldtools
