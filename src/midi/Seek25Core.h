#pragma once
// P25 original K132: fixed-memory tempo-aware MIDI seeking, no heap/Arduino.
#include <stdint.h>
#include <limits.h>
#include "Smf.h"
namespace suite { namespace midi25 {
constexpr uint64_t StepUs=10000000ULL;
constexpr uint64_t MaxTargetUs=86400000000ULL; // 24 hours of song time
constexpr uint32_t MaxScannedEvents=200000;
constexpr unsigned EventsPerPoll=128;
constexpr uint64_t InvalidTime=UINT64_MAX;
inline uint64_t target(uint64_t now,int direction) {
    if(direction<0)return now>StepUs?now-StepUs:0;
    if(direction>0)return now>=MaxTargetUs-StepUs?MaxTargetUs:now+StepUs;
    return now;
}
inline uint64_t eventTime(uint64_t previous,uint32_t fromTick,
                          uint32_t nextTick,uint32_t tempo,uint16_t division) {
    if(!division||!tempo||nextTick<fromTick)return InvalidTime;
    const uint64_t delta=static_cast<uint64_t>(nextTick-fromTick)*tempo/division;
    return delta>UINT64_MAX-previous?InvalidTime:previous+delta;
}
inline uint64_t playbackPosition(uint64_t last,uint64_t next,int64_t due,
                                  int64_t reference,unsigned speed) {
    if(next<last||!speed)return last;
    if(due<=reference)return next;
    const uint64_t future=static_cast<uint64_t>(due-reference);
    const uint64_t remaining=future>UINT64_MAX/speed?
        UINT64_MAX:future*speed/100;
    return remaining>=next-last?last:next-remaining;
}
inline int64_t delayAtSpeed(uint64_t us,unsigned speed) {
    if(!speed||us>MaxTargetUs)return -1;
    return static_cast<int64_t>(us*100/static_cast<uint64_t>(speed));
}
class Scanner {
public:
    enum class Result:uint8_t {Consume,Ready,Invalid};
    void begin(uint64_t goal){
        goal_=goal;position_=0;tick_=0;tempo_=500000;scanned_=0;
    }
    Result inspect(const smf::Event& e,uint16_t division,uint64_t& when){
        if(goal_>MaxTargetUs)return Result::Invalid;
        when=eventTime(position_,tick_,e.tick,tempo_,division);
        if(when==InvalidTime)return Result::Invalid;
        if(when>=goal_)return Result::Ready;
        if(scanned_>=MaxScannedEvents)return Result::Invalid;
        position_=when;tick_=e.tick;++scanned_;
        if(e.kind==smf::Kind::Tempo){
            if(!e.tempo)return Result::Invalid;
            tempo_=e.tempo;
        }
        return Result::Consume;
    }
    uint64_t position()const{return position_;}
    uint32_t tick()const{return tick_;}
    uint32_t tempo()const{return tempo_;}
    uint32_t scanned()const{return scanned_;}
    uint64_t goal()const{return goal_;}
private:
    uint64_t goal_=0,position_=0;
    uint32_t tick_=0,tempo_=500000,scanned_=0;
};
}} // namespace suite::midi25
