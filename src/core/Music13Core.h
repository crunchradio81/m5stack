#pragma once
// P13 pocket music studio: 16-step, 4-track sequencer and chord theory.
// Fixed memory, no full SoundFont/sample banks. C++17 host-testable.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
namespace suite {namespace music13 {
struct Sequence {
  static constexpr unsigned Tracks=4,Steps=16,Encoded=88;
  uint8_t note[Tracks][Steps]={};
  uint8_t program[Tracks]={0,16,24,80};
  uint16_t bpm=120;
  bool muted[Tracks]={};
  void clear(){
    memset(note,0,sizeof(note));
    program[0]=0;program[1]=16;program[2]=24;program[3]=80;
    memset(muted,0,sizeof(muted));bpm=120;
  }
  bool set(unsigned track,unsigned step,unsigned pitch){
    if(track>=Tracks||step>=Steps||pitch>108||
     (pitch!=0&&pitch<24))return false;
    note[track][step]=static_cast<uint8_t>(pitch);return true;
  }
  uint8_t get(unsigned track,unsigned step)const{
    return track<Tracks&&step<Steps?note[track][step]:0;
  }
  bool tempo(unsigned value){if(value<40||value>240)return false;bpm=value;return true;}
  void drumPreset(){
    clear();program[0]=program[1]=program[2]=program[3]=0;
    for(unsigned step=0;step<Steps;++step){
      if(step%4==0)note[0][step]=36;
      if(step%8==4)note[1][step]=38;
      if(step%2==0)note[2][step]=42;
      if(step%8==7)note[3][step]=46;
    }
  }
  static uint32_t digest(const uint8_t*buf,size_t n){
    uint32_t h=2166136261u;
    for(size_t i=0;i<n;++i){h^=buf[i];h*=16777619u;}
    return h;
  }
  // Little-endian, strict version+length, FNV32 corruption guard.
  size_t encode(uint8_t*out,size_t cap)const{
    if(!out||cap<Encoded)return 0;
    memset(out,0,Encoded);
    memcpy(out,"K132SEQ1",8);
    out[8]=static_cast<uint8_t>(bpm);out[9]=static_cast<uint8_t>(bpm>>8);
    for(unsigned t=0;t<Tracks;++t) {
      out[10+t]=program[t];
      out[14+t]=muted[t]?1:0;
      for(unsigned st=0;st<Steps;++st)out[18+t*Steps+st]=note[t][st];
    }
    out[82]=1;out[83]=0;
    const uint32_t sum=digest(out,84);
    for(unsigned i=0;i<4;++i)out[84+i]=static_cast<uint8_t>(sum>>(i*8));
    return Encoded;
  }
  bool decode(const uint8_t*raw,size_t n){
    if(!raw||n!=Encoded||memcmp(raw,"K132SEQ1",8)||raw[82]!=1||raw[83]!=0)return false;
    const uint32_t expected=static_cast<uint32_t>(raw[84])|
      static_cast<uint32_t>(raw[85])<<8|
      static_cast<uint32_t>(raw[86])<<16|
      static_cast<uint32_t>(raw[87])<<24;
    if(digest(raw,84)!=expected)return false;
    Sequence next;next.clear();
    if(!next.tempo(static_cast<unsigned>(raw[8])|
       (static_cast<unsigned>(raw[9])<<8)))return false;
    for(unsigned t=0;t<Tracks;++t){
      if(raw[10+t]>127||raw[14+t]>1)return false;
      next.program[t]=raw[10+t];next.muted[t]=raw[14+t]!=0;
      for(unsigned st=0;st<Steps;++st){
        const uint8_t note=raw[18+t*Steps+st];
        if(note!=0&&(note<24||note>108))return false;
        next.note[t][st]=note;
      }
    }
    *this=next;return true;
  }
};
enum class Quality:uint8_t {Major,Minor,Dim,Aug,Seventh,Maj7,Min7,Sus4,Count};
inline const char*qualityName(Quality q){
  static const char*const names[]={"Major","Minor","Dim","Aug","7","Maj7","Min7","Sus4"};
  const unsigned n=static_cast<unsigned>(q);
  return n<static_cast<unsigned>(Quality::Count)?names[n]:"?";
}
inline const char*noteName(unsigned pitch){
  static const char*const names[]={"C","C#","D","D#","E","F",
      "F#","G","G#","A","A#","B"};
  return names[pitch%12];
}
inline bool chord(unsigned root,Quality q,uint8_t*out,unsigned&count){
  if(!out||root>11||static_cast<unsigned>(q)>=8)return false;
  static const int intervals[8][4]={
    {0,4,7,-1},{0,3,7,-1},{0,3,6,-1},{0,4,8,-1},
    {0,4,7,10},{0,4,7,11},{0,3,7,10},{0,5,7,-1}
  };
  const unsigned index=static_cast<unsigned>(q);
  count=0;
  for(int i=0;i<4;++i)if(intervals[index][i]>=0)
    out[count++]=static_cast<uint8_t>(60+root+intervals[index][i]);
  return true;
}
class Arpeggiator {
public:
  void reset(){index_=0;direction_=1;}
  uint8_t next(const uint8_t*notes,unsigned count,unsigned style){
    if(!notes||count==0||count>4)return 0;
    if(style==0){const uint8_t value=notes[index_%count];++index_;return value;}
    if(style==1){
      const uint8_t value=notes[(count-1)-(index_%count)];++index_;return value;
    }
    if(count==1)return notes[0];
    const unsigned period=2*(count-1);
    const unsigned p=index_++%period;
    return notes[p<count?p:period-p];
  }
private:unsigned index_=0,direction_=1;
};
class MorseQuiz {
public:
  void newQuestion(uint32_t seed){
    if(!seed)seed=1;
    for(int i=0;i<5;++i){
      seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;
      word_[i]=static_cast<char>('A'+seed%26);
    }
    word_[5]=0;
  }
  MorseQuiz(){newQuestion(17);}
  const char*word()const{return word_;}
private:char word_[6]={};
};
}} // suite::music13
