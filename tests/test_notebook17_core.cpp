#include "../src/core/Notebook17Core.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace suite::notebook17;
int main(){
 char name[MaxName]={};
 assert(validStem("MUSIC_2026"));
 assert(!validStem("../APPS")&&!validStem(".SECRET"));
 assert(!validStem("CON")&&!validStem("nul"));
 assert(!validStem("COM1")&&!validStem("LPT9"));
 assert(!validStem("TOO_LONG_NAME_FOR_K132"));
 assert(makeName("Journal_08",name,sizeof(name)));
 assert(!strcmp(name,"Journal_08.TXT")&&validName(name));
 assert(validName("notes.txt")&&!validName("X.NEW"));
 assert(!validName("../X.TXT")&&!validName("X/T.TXT"));
 assert(!makeName("THIS_TOO_LONG_FILENAME",name,sizeof(name)));
 assert(restoredName("Journal_08.TXT",1,name,sizeof(name)));
 assert(!strcmp(name,"R1-Journal_08.TXT"));
 assert(restoredName("ABCDEFGHIJKLMNOPQR.TXT",9,name,sizeof(name)));
 assert(validName(name)&&strlen(name)<=MaxStem+4);
 assert(!restoredName("bad.PEM",1,name,sizeof(name)));
 assert(!restoredName("Hello.TXT",0,name,sizeof(name)));
 assert(!restoredName("Hello.TXT",10,name,sizeof(name)));
 assert(!restoredName("Hello.TXT",1,name,5));
 Index index;
 assert(index.add("ZETA.TXT",17));
 assert(index.add("alpha.txt",4));
 assert(index.add("Middle.TXT",2));
 assert(index.count()==3);
 assert(!strcmp(index.at(0).name,"alpha.txt"));
 assert(!strcmp(index.at(1).name,"Middle.TXT"));
 assert(index.find("ALPHA.TXT")==0);
 assert(!index.add("AlPhA.TxT",2));
 assert(!index.add("OUTSIDE.TXT",1024));
 assert(index.findNext("ET",0)==2);
 assert(index.findNext("doesnotexist",0)<0);
 assert(contains("My Piano Practice\n",18,"PIANO"));
 assert(contains("AB\nCdEF",7,"CDEF"));
 assert(!contains("short",5,"a longer query"));
 assert(!contains("A",1,""));
 Text text;
 assert(text.load("First line\nsecond",17));
 assert(text.size()==17&&text.cursor()==17);
 text.left();text.left();
 assert(text.insert('!')&&text.backspace());
 assert(text.size()==17);
 assert(text.search("FIRST")&&text.search("line\nSEC"));
 assert(!text.load("bad\x01",4));
 assert(text.size()==17); // malformed read never destroys existing content
 assert(!text.insert('\t'));
 text.beginning();assert(!text.backspace());
 text.end();assert(!text.remove());
 for(unsigned i=text.size();i<MaxBytes;++i)assert(text.insert('x'));
 assert(text.size()==MaxBytes&&!text.insert('y'));
 uint8_t a[]={0,1,2,3,254,255};
 uint8_t b[]={0,1,2,3,254,255};
 assert(same(a,sizeof(a),b,sizeof(b)));
 b[3]=4;assert(!same(a,sizeof(a),b,sizeof(b)));
 assert(fingerprint(a,sizeof(a))!=fingerprint(b,sizeof(b)));
 assert(!same(a,sizeof(a),b,sizeof(b)-1));
 puts("P17 Notebook: strict names, safe restored names, sorted index, text editor/search and exact backup comparison PASS");
}
