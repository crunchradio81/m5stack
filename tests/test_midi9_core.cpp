#include "../src/midi/Midi9Core.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace suite;
struct Memory : smf::Source {
  std::vector<uint8_t> buffer;
  uint32_t length()const override{return static_cast<uint32_t>(buffer.size());}
  bool readAt(uint32_t off,uint8_t*out,size_t n)override{
    if(off>buffer.size()||n>buffer.size()-off)return false;
    memcpy(out,buffer.data()+off,n);return true;
  }
};
static void be32(std::vector<uint8_t>&b,uint32_t n){
  b.push_back(n>>24);b.push_back(n>>16);b.push_back(n>>8);b.push_back(n);
}
int main(){
  const uint8_t notes[]={
    0x00,0xff,0x05,0x05,'H','e','l','l','o',
    0x00,0xff,0x01,0x04,'T','e','x','t',
    0x00,0xff,0x51,0x03,0x07,0xA1,0x20,
    0x00,0xC0,0x28,
    0x00,0x90,0x3C,0x64,
    0x60,0x80,0x3C,0x00,
    0x00,0xFF,0x2F,0x00
  };
  Memory mem;
  mem.buffer={'M','T','h','d'};be32(mem.buffer,6);
  mem.buffer.insert(mem.buffer.end(),{0,0,0,1,1,0xE0});
  mem.buffer.insert(mem.buffer.end(),{'M','T','r','k'});
  be32(mem.buffer,sizeof(notes));
  mem.buffer.insert(mem.buffer.end(),notes,notes+sizeof(notes));
  smf::Reader reader;smf::Event ev;midi9::Analysis analyzer;
  assert(reader.open(mem));
  assert(reader.next(ev)&&ev.kind==smf::Kind::Lyric &&
         strcmp(ev.text,"Hello")==0);
  analyzer.take(ev);
  assert(reader.next(ev)&&ev.kind==smf::Kind::Text&&strcmp(ev.text,"Text")==0);
  analyzer.take(ev);
  unsigned events=2;
  while(reader.next(ev)){analyzer.take(ev);++events;}
  assert(!reader.failed());
  assert(events==6&&analyzer.lyricCount()==2);
  assert(strcmp(analyzer.lyric(0),"Hello")==0);
  assert(strcmp(analyzer.lyric(1),"Text")==0);
  assert(analyzer.notes()==1&&analyzer.pitchClass(0)==1);
  assert(analyzer.program(0)==40&&analyzer.tempoCount()==1);
  assert(analyzer.tempoValue(0)==500000&&analyzer.noteWindow()==1);
  assert(analyzer.notePitch(0)==60);
  midi9::Favorites favorites;
  assert(favorites.add("SONG.MID")&&favorites.add("KARAOKE.KAR"));
  assert(!favorites.add("SONG.MID"));
  assert(!favorites.add("../HACK.MID")&&!favorites.add("BAD.MP3"));
  assert(!favorites.add("a/b.MID"));
  char text[2200]={};
  size_t len=favorites.write(text,sizeof(text));
  assert(len>0&&strstr(text,"SONG.MID\n"));
  midi9::Favorites loaded;
  assert(loaded.parse(text,len)&&loaded.count()==2);
  assert(loaded.contains("KARAOKE.KAR"));
  assert(loaded.remove("SONG.MID")&&!loaded.contains("SONG.MID"));
  assert(!loaded.parse("NOPE.MID",8)&&loaded.count()==1);
  assert(!loaded.parse("../BAD.MID\n",11));
  assert(!loaded.parse("A.MID\nA.MID\n",12));
  puts("P9 MIDI: lyric/text SMF streaming, tempo/note analysis, safe playlists PASS");
}
