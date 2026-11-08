#include "../src/core/Playlist21Core.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
using suite::playlist21::Queue;
int main(){
 Queue q;
 assert(q.count()==0);
 assert(q.add("01 Prelude.MID"));
 assert(q.add("02 Rag.KAR"));
 assert(!q.add("01 prelude.mid"));
 assert(!q.add("../something.mid"));
 assert(!q.add("Bad\\Slash.mid"));
 assert(!q.add("BAD.TXT"));
 assert(!q.add(".hidden.mid"));
 assert(!q.add("bad..mid"));
 assert(!q.add("track.mid "));
 assert(q.move(1,-1));
 assert(std::strcmp(q.at(0),"02 Rag.KAR")==0);
 char buf[suite::playlist21::MaxBytes+1]={};
 const size_t n=q.encode(buf,sizeof(buf));
 assert(n>0);
 Queue restored;
 assert(restored.parse(buf,n));
 assert(restored.count()==2);
 assert(restored.indexOf("02 rag.kar")==0);
 assert(restored.move(0,1));
 assert(restored.remove(0)&&restored.count()==1);
 assert(!restored.remove(9));
 const std::string original(buf,n);
 const char*invalid[]={
  "WRONGPLAY\nA.mid\n","K132PLAY1\n../foo.mid\n",
  "K132PLAY1\nA.mid\nA.MID\n","K132PLAY1\nA.mid",
  "K132PLAY1\nA.mid\r\n","K132PLAY1\nbad\\name.mid\n",
  "K132PLAY1\nA.mid\n\n","K132PLAY1\nB.txt\n"
 };
 for(const char*caseText:invalid){
  Queue old=restored;
  assert(!restored.parse(caseText,std::strlen(caseText)));
  assert(restored.count()==old.count());
  assert(std::strcmp(restored.at(0),old.at(0))==0);
 }
 assert(!restored.parse(original.c_str(),suite::playlist21::MaxBytes+1));
 assert(q.encode(buf,4)==0);
 assert(!q.move(0,-1));
 assert(q.move(1,-1));
 assert(q.add("Another Song.midi"));
 assert(q.count()==3);
 puts("P21 MIDI playlists: strict filenames, order, duplicate/overflow protection and atomic parse PASS");
}
