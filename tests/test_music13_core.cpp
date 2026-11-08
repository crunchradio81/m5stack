#include "../src/core/Music13Core.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace suite::music13;
int main(){
 Sequence s;
 assert(s.tempo(180)&&!s.tempo(241)&&!s.tempo(39));
 assert(s.set(0,0,60)&&s.set(2,15,71));
 assert(!s.set(4,0,60)&&!s.set(0,16,60)&&!s.set(0,0,200));
 uint8_t buf[128]={};
 assert(s.encode(buf,sizeof(buf))==Sequence::Encoded);
 Sequence copy;
 assert(copy.decode(buf,Sequence::Encoded));
 assert(copy.bpm==180&&copy.get(0,0)==60&&copy.get(2,15)==71);
 assert(!copy.decode(buf,87));
 buf[19]^=1;
 assert(!copy.decode(buf,Sequence::Encoded));
 assert(copy.get(0,0)==60);
 s.drumPreset();assert(s.get(0,0)==36&&s.get(1,4)==38);
 assert(s.get(2,2)==42&&s.get(3,7)==46);
 assert(s.encode(buf,sizeof(buf))==Sequence::Encoded);
 assert(copy.decode(buf,Sequence::Encoded)&&copy.get(0,0)==36);
 uint8_t notes[4]={};unsigned count=0;
 assert(chord(0,Quality::Major,notes,count)&&count==3);
 assert(notes[0]==60&&notes[1]==64&&notes[2]==67);
 assert(chord(9,Quality::Min7,notes,count)&&count==4);
 assert(notes[0]==69&&notes[3]==79);
 assert(!chord(12,Quality::Major,notes,count));
 Arpeggiator arp;uint8_t triad[4]={60,64,67};
 assert(arp.next(triad,3,0)==60);
 assert(arp.next(triad,3,0)==64);
 arp.reset();
 assert(arp.next(triad,3,2)==60);
 assert(arp.next(triad,3,2)==64);
 assert(arp.next(triad,3,2)==67);
 assert(arp.next(triad,3,2)==64);
 MorseQuiz q;q.newQuestion(3);
 assert(strlen(q.word())==5);
 for(const char*p=q.word();*p;++p)assert(*p>='A'&&*p<='Z');
 puts("P13 Music Studio: tracker patterns, drum gates, chords, arpeggio and quiz PASS");
}
