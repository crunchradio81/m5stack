#pragma once
// Bounded on-device diagnostics; no battery current/charging inference.
#include <stdint.h>
#include <stddef.h>
namespace suite { namespace system {
class BatteryTrend {
public:
  void clear(){count_=cursor_=0;for(auto &x:levels_)x=0;}
  BatteryTrend(){clear();}
  bool sample(int millivolts) {
    if(millivolts<2500||millivolts>5000)return false;
    levels_[cursor_]=static_cast<uint16_t>(millivolts);
    cursor_=(cursor_+1)%16;
    if(count_<16)++count_;
    return true;
  }
  unsigned count()const{return count_;}
  unsigned average()const{
    if(!count_)return 0;
    uint32_t total=0;
    for(unsigned i=0;i<count_;++i)total+=levels_[i];
    return total/count_;
  }
  int trend()const{
    if(count_<2)return 0;
    const int latest=static_cast<int>((cursor_+15)%16);
    const int oldest=count_==16?static_cast<int>(cursor_):0;
    return static_cast<int>(levels_[latest])-levels_[oldest];
  }
private:
  uint16_t levels_[16]={};
  unsigned cursor_=0,count_=0;
};
class ReadBenchmark {
public:
  void reset(){bytes_=0;elapsed_=0;ready_=false;}
  bool complete(uint64_t readBytes,uint32_t startUs,uint32_t endUs){
    if(readBytes==0)return false;
    const uint32_t delta=static_cast<uint32_t>(endUs-startUs);
    if(delta==0)return false;
    bytes_=readBytes;elapsed_=delta;ready_=true;return true;
  }
  bool ready()const{return ready_;}
  uint64_t bytes()const{return bytes_;}
  uint32_t microseconds()const{return elapsed_;}
  uint32_t kibPerSecond()const{
    if(!ready_)return 0;
    return static_cast<uint32_t>((bytes_*1000000ull)/(static_cast<uint64_t>(elapsed_)*1024ull));
  }
private:
  uint64_t bytes_=0;uint32_t elapsed_=0;
  bool ready_=false;
};
inline bool safeRelativeName(const char* name){
  if(!name||!*name)return false;
  size_t count=0;
  for(const char*p=name;*p;++p){
    const char c=*p;
    if(++count>40)return false;
    if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
         (c>='0'&&c<='9')||c=='.'||c=='_'||c=='-'))return false;
  }
  if(name[0]=='.')return false; // do not allow dot/dotdot or hidden files
  return true;
}
inline void uptime(uint64_t seconds,unsigned& days,unsigned& hours,
                   unsigned& minutes,unsigned& secs) {
  days=static_cast<unsigned>(seconds/86400ull);
  hours=static_cast<unsigned>((seconds/3600ull)%24ull);
  minutes=static_cast<unsigned>((seconds/60ull)%60ull);
  secs=static_cast<unsigned>(seconds%60ull);
}
}}
