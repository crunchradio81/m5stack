#pragma once
// Bounded host-testable MIDI analysis and SD playlist primitives.
// Original K132, no PSRAM; never loads an entire song into memory.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "Smf.h"
namespace suite { namespace midi9 {
class Analysis {
public:
  static constexpr unsigned MaxLyrics=12,MaxNotes=40,MaxTempo=16;
  void clear() {
    notes_=events_=lyrics_=tempos_=lastTick_=0;
    memset(program_,0,sizeof(program_));
    memset(channels_,0,sizeof(channels_));
    memset(pitches_,0,sizeof(pitches_));
    memset(noteTick_,0,sizeof(noteTick_));
    memset(notePitch_,0,sizeof(notePitch_));
    memset(lyric_,0,sizeof(lyric_));
    memset(tempoTick_,0,sizeof(tempoTick_));
    memset(tempoValue_,0,sizeof(tempoValue_));
  }
  Analysis(){clear();}
  void take(const smf::Event&e){
    ++events_;if(e.tick>lastTick_)lastTick_=e.tick;
    if(e.kind==smf::Kind::NoteOn){
      ++notes_;++channels_[e.channel&15];++pitches_[e.a%12];
      const unsigned index=notes_<=MaxNotes?notes_-1:(notes_-1)%MaxNotes;
      noteTick_[index]=e.tick;notePitch_[index]=e.a;
    }else if(e.kind==smf::Kind::Program)program_[e.channel&15]=e.a;
    else if(e.kind==smf::Kind::Tempo){
      if(tempos_<MaxTempo){
        tempoTick_[tempos_]=e.tick;tempoValue_[tempos_]=e.tempo;
        ++tempos_;
      }
    }else if((e.kind==smf::Kind::Lyric||e.kind==smf::Kind::Text)&&
             e.text[0]&&lyrics_<MaxLyrics){
      strncpy(lyric_[lyrics_],e.text,sizeof(lyric_[lyrics_])-1);
      ++lyrics_;
    }
  }
  uint32_t events()const{return events_;}
  uint32_t notes()const{return notes_;}
  uint32_t lastTick()const{return lastTick_;}
  uint32_t channelNotes(unsigned c)const{return c<16?channels_[c]:0;}
  uint32_t pitchClass(unsigned c)const{return c<12?pitches_[c]:0;}
  uint8_t program(unsigned c)const{return c<16?program_[c]:0;}
  unsigned lyricCount()const{return lyrics_;}
  const char* lyric(unsigned i)const{return i<lyrics_?lyric_[i]:"";}
  unsigned tempoCount()const{return tempos_;}
  uint32_t tempoTick(unsigned i)const{return i<tempos_?tempoTick_[i]:0;}
  uint32_t tempoValue(unsigned i)const{return i<tempos_?tempoValue_[i]:0;}
  unsigned noteWindow()const{return notes_<MaxNotes?notes_:MaxNotes;}
  uint8_t notePitch(unsigned index)const{
    if(index>=noteWindow())return 0;
    const unsigned oldest=notes_>MaxNotes?notes_%MaxNotes:0;
    return notePitch_[(oldest+index)%MaxNotes];
  }
  uint32_t noteTick(unsigned index)const{
    if(index>=noteWindow())return 0;
    const unsigned oldest=notes_>MaxNotes?notes_%MaxNotes:0;
    return noteTick_[(oldest+index)%MaxNotes];
  }
private:
  uint32_t notes_=0,events_=0,lastTick_=0;
  uint32_t channels_[16]={},pitches_[12]={};
  uint8_t program_[16]={};
  uint32_t noteTick_[MaxNotes]={};
  uint8_t notePitch_[MaxNotes]={};
  uint32_t tempoTick_[MaxTempo]={},tempoValue_[MaxTempo]={};
  char lyric_[MaxLyrics][48]={};
  unsigned lyrics_=0,tempos_=0;
};
// Exactly one filename per line; no paths or traversal; fixed 48 entries.
class Favorites {
public:
  static constexpr unsigned Max=48;
  void clear(){count_=0;memset(names_,0,sizeof(names_));}
  Favorites(){clear();}
  static bool safe(const char*name){
    if(!name)return false;
    const size_t n=strlen(name);
    if(n<5||n>40||name[0]=='.'||strstr(name,".."))return false;
    const bool extension=(!strcmp(name+n-4,".MID")||
                          !strcmp(name+n-4,".KAR"));
    if(!extension)return false;
    for(size_t i=0;i<n;++i) {
      const char c=name[i];
      if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
           (c>='0'&&c<='9')||c=='.'||c=='_'||c=='-'||c==' '))return false;
    }
    return true;
  }
  bool contains(const char*name)const{
    for(unsigned i=0;i<count_;++i)if(!strcmp(names_[i],name))return true;
    return false;
  }
  bool add(const char*name){
    if(!safe(name)||count_>=Max||contains(name))return false;
    strcpy(names_[count_++],name);return true;
  }
  bool remove(const char*name){
    for(unsigned i=0;i<count_;++i)if(!strcmp(names_[i],name)){
      for(unsigned j=i;j+1<count_;++j)strcpy(names_[j],names_[j+1]);
      --count_;names_[count_][0]=0;return true;
    }
    return false;
  }
  unsigned count()const{return count_;}
  const char*at(unsigned i)const{return i<count_?names_[i]:"";}
  bool parse(const char*text,size_t n){
    if(!text||n>2100)return false;
    Favorites next;
    size_t pos=0;
    while(pos<n){
      size_t start=pos;
      while(pos<n&&text[pos]!='\n'){
        if(static_cast<unsigned char>(text[pos])<32)return false;
        ++pos;
      }
      if(pos==n)return false;
      const size_t len=pos-start;++pos;
      if(!len||len>40)return false;
      char name[41]={};memcpy(name,text+start,len);
      if(!next.add(name))return false;
    }
    *this=next;return true;
  }
  size_t write(char*out,size_t cap)const{
    if(!out||!cap)return 0;
    size_t p=0;
    for(unsigned i=0;i<count_;++i){
      const size_t n=strlen(names_[i]);
      if(p+n+1>=cap)return 0;
      memcpy(out+p,names_[i],n);p+=n;out[p++]='\n';
    }
    out[p]=0;return p;
  }
private:
  char names_[Max][41]={};
  unsigned count_=0;
};
}} // suite::midi9
