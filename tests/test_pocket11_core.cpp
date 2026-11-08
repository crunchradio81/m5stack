#include "../src/core/Pocket11Core.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace suite::pocket11;
int main(){
 Editor edit;
 assert(edit.insert('A')&&edit.insert('B'));
 assert(edit.size()==2&&strcmp(edit.text(),"AB")==0);
 edit.left();assert(edit.insert('x'));
 assert(strcmp(edit.text(),"AxB")==0);
 assert(edit.backspace()&&strcmp(edit.text(),"AB")==0);
 assert(edit.set("Line 1\nLine 2",13));
 assert(!edit.set("Invalid\x01",8));
 edit.clear();assert(!edit.backspace());
 for(int i=0;i<1023;++i)assert(edit.insert('a'));
 assert(!edit.insert('x')&&edit.size()==1023);
 Sheet sheet;
 assert(sheet.set(0,0,4)&&sheet.set(1,0,12));
 assert(sheet.sumRow(0)==16&&sheet.sumCol(1)==12);
 assert(!sheet.set(6,1,0)&&!sheet.set(1,1,999999));
 char output[600]={};
 size_t n=sheet.encode(output,sizeof(output));assert(n>20);
 Sheet loaded;
 assert(loaded.parse(output,n)&&loaded.at(1,0)==12);
 assert(!loaded.parse("SHEET1\n1,2\n",11));
 assert(!loaded.parse("SHEET1\n1,2,x,4,5,6\n",19));
 assert(loaded.at(0,0)==4);
 int32_t value=0;
 assert(Sheet::number("-123",4,value)&&value==-123);
 assert(!Sheet::number("100000",6,value));
 assert(!Sheet::number("1.5",3,value));
 Flashcards cards;
 const char*deck="Capital of France?|Paris\nBinary?|Two states\n";
 assert(cards.parse(deck,strlen(deck))&&cards.count()==2);
 assert(strcmp(cards.answer(0),"Paris")==0);
 int found=-1;assert(cards.search("FRANCE",found)&&found==0);
 assert(!cards.search("London",found));
 assert(!cards.parse("No separator\n",13)&&cards.count()==2);
 uint32_t state=1;
 char secret[40]={};
 auto rng=[&state]()->uint32_t {state=state*1664525u+1013904223u;return state;};
 assert(randomString(secret,sizeof(secret),16,rng));
 assert(strlen(secret)==16);
 assert(!randomString(secret,4,16,rng));
 assert(!randomString(secret,sizeof(secret),7,rng));
 puts("P11 Pocket Core: text editing, CSV spreadsheet, flashcards and bounded RNG PASS");
}
