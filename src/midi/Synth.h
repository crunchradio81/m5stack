#pragma once
// Fixed-memory 12-voice mono synthesizer for original Cardputer speaker.
// GM instrument program numbers are mapped to lightweight waveform families;
// THIS IS NOT a General MIDI sample bank / SoundFont renderer.
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include <string.h>
#include "Smf.h"

namespace suite { namespace midi {
class Synth {
public:
    static constexpr size_t Voices=12;
    static constexpr uint32_t SampleRate=22050;
    Synth() {
        for(size_t i=0;i<256;++i)
            sine_[i]=sinf(6.2831853071795864769f*static_cast<float>(i)/256.f);
        reset();
    }
    void reset() {
        memset(voices_,0,sizeof(voices_));
        for(size_t i=0;i<16;++i) {
            program_[i]=0; volume_[i]=100;expression_[i]=127;
            bend_[i]=8192;sustain_[i]=false;
        }
        memset(muted_,0,sizeof(muted_));
        totalStarts_=0;
    }
    void allSoundOff() { memset(voices_,0,sizeof(voices_)); }
    void muteTrack(uint8_t track,bool muted) {
        if(track>=16) return;
        muted_[track]=muted;
        if(muted)
            for(auto &v:voices_) if(v.active && v.track==track) v.active=false;
    }
    bool trackMuted(uint8_t t) const {return t<16 && muted_[t];}
    uint8_t program(uint8_t ch) const {return program_[ch&15];}
    size_t activeVoices() const {
        size_t n=0;for(const auto &v:voices_) if(v.active) ++n;return n;
    }
    void event(const smf::Event& e) {
        const uint8_t ch=e.channel&15;
        switch(e.kind) {
            case smf::Kind::NoteOn:
                noteOn(e.track,ch,e.a,e.b);break;
            case smf::Kind::NoteOff:
                noteOff(ch,e.a,e.track);break;
            case smf::Kind::Program:
                program_[ch]=e.a;break;
            case smf::Kind::Control:
                control(ch,e.a,e.b);break;
            case smf::Kind::PitchBend:
                bend_[ch]=static_cast<uint16_t>(e.a|(e.b<<7));
                for(auto &v:voices_) if(v.active && v.channel==ch)
                    v.step=stepFor(v.note,ch);
                break;
            default:break;
        }
    }
    void render(int16_t* out,size_t count) {
        if(!out) return;
        for(size_t i=0;i<count;++i) {
            float sum=0.f;
            for(auto &v:voices_) {
                if(!v.active) continue;
                // One-pole envelope: quick onset, subdued sustain, short release.
                if(v.release) {
                    v.env*=0.99915f;
                    if(v.env<0.002f) {v.active=false;continue;}
                } else if(v.env<0.70f) {
                    v.env+=0.006f;
                    if(v.env>0.70f) v.env=0.70f;
                }
                const uint32_t phase=v.phase;
                v.phase+=v.step;
                float wave=0.f;
                switch(v.shape) {
                    case 0:wave=sine_[phase>>24];break;
                    case 1:wave=static_cast<float>(static_cast<int32_t>(phase))/2147483648.f;break;
                    case 2:wave=(phase&0x80000000u)?-0.72f:0.72f;break;
                    case 3:wave=(phase&0x80000000u)?
                        (1.f-static_cast<float>(phase>>16)/32768.f):
                        (static_cast<float>(phase>>16)/32768.f);
                        break;
                    case 4:wave=0.75f*sine_[phase>>24]+0.25f*sine_[(phase>>23)&255];break;
                    default:
                        noise_=noise_*1664525u+1013904223u;
                        wave=(static_cast<int32_t>(noise_)/2147483648.f)*0.65f;
                        break;
                }
                const float gain=(v.velocity/127.f)*(volume_[v.channel]/127.f)*
                                 (expression_[v.channel]/127.f);
                sum+=wave*v.env*gain;
            }
            const float scaled=sum*0.16f;
            const float clipped=scaled>0.95f?0.95f:scaled< -0.95f? -0.95f:scaled;
            out[i]=static_cast<int16_t>(clipped*32767.f);
        }
    }
private:
    struct Voice {
        uint32_t phase=0,step=0;
        uint8_t note=0,channel=0,track=0,velocity=0,shape=0;
        float env=0;
        bool active=false,release=false,held=false;
        uint32_t age=0;
    };
    Voice voices_[Voices];
    float sine_[256];
    uint8_t program_[16],volume_[16],expression_[16];
    uint16_t bend_[16];
    bool sustain_[16],muted_[16];
    uint32_t noise_=0xBADC0FFEu, totalStarts_=0;

    uint32_t stepFor(uint8_t note,uint8_t channel) const {
        const float pitch=(static_cast<float>(note)-69.f)+
            (static_cast<int>(bend_[channel])-8192)*(2.f/8192.f);
        const float frequency=440.f*powf(2.f,pitch/12.f);
        const double step=(frequency*4294967296.0)/SampleRate;
        if(step<1) return 1;
        if(step>2147483647.0) return 2147483647u;
        return static_cast<uint32_t>(step);
    }
    static uint8_t shapeFor(uint8_t program,uint8_t channel) {
        if(channel==9) return 5; // Approximate unpitched drums
        const uint8_t family=program/8;
        if(family==0 || family==1 || family==9) return 4; // piano/chromatic/pipe
        if(family==2 || family==10) return 2; // organ, synth lead
        if(family==3 || family==6 || family==7 || family==11) return 1;
        if(family==4 || family==8 || family==12) return 3;
        return 0;
    }
    void noteOn(uint8_t track,uint8_t channel,uint8_t note,uint8_t velocity) {
        if(!velocity) {noteOff(channel,note,track);return;}
        if(track>=16 || muted_[track]) return;
        Voice *slot=nullptr;
        for(auto &v:voices_) if(!v.active) {slot=&v;break;}
        if(!slot) {
            slot=&voices_[0];
            for(auto &v:voices_)
                if(v.release || v.env<slot->env || v.age<slot->age) slot=&v;
        }
        Voice v;
        v.active=true;v.channel=channel;v.note=note;
        v.track=track;v.velocity=velocity;v.step=stepFor(note,channel);
        v.shape=shapeFor(program_[channel],channel);
        v.age=++totalStarts_;
        *slot=v;
    }
    void noteOff(uint8_t channel,uint8_t note,uint8_t track) {
        for(auto &v:voices_) if(v.active && v.channel==channel &&
            v.note==note && v.track==track && !v.release) {
            if(sustain_[channel]) v.held=true;
            else v.release=true;
        }
    }
    void control(uint8_t channel,uint8_t cc,uint8_t value) {
        switch(cc) {
            case 7:volume_[channel]=value;break;
            case 11:expression_[channel]=value;break;
            case 64:
                sustain_[channel]=value>=64;
                if(!sustain_[channel])
                    for(auto &v:voices_) if(v.active && v.channel==channel && v.held) {
                        v.held=false;v.release=true;
                    }
                break;
            case 120: // All sound off
                for(auto &v:voices_) if(v.channel==channel) v.active=false;
                break;
            case 121: // Reset controllers
                volume_[channel]=100;expression_[channel]=127;
                bend_[channel]=8192;sustain_[channel]=false;
                break;
            case 123: // All notes off
                for(auto &v:voices_) if(v.channel==channel && v.active) v.release=true;
                break;
        }
    }
};
}} // namespace suite::midi
