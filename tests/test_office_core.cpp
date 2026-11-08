#include "../src/core/OfficeCore.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace suite::office;
int main(){
    Records a;
    assert(a.count()==0);
    assert(!a.add(""));
    assert(!a.add("x\ncorrupt"));
    assert(a.add("Pay rent")&&a.add("Call friend")&&a.count()==2);
    assert(a.toggle(0)&&a.done(0));
    char text[4096]={};
    const size_t n=a.serialize(text,sizeof(text));
    assert(n>0&&text[n]==0&&strstr(text,"1|Pay rent\n"));
    Records b;
    assert(b.parse(text,n)&&b.count()==2&&b.done(0)&&!b.done(1));
    assert(b.add("Later"));
    const int before=b.count();
    assert(!b.parse("0|not complete",14)&&b.count()==before);
    assert(!b.parse("5|invalid\n",10)&&b.count()==before);
    assert(!b.parse("0|x\n0|a\tb\n",12)&&b.count()==before);
    assert(!b.parse("1|z\n2|bad\n",10)&&b.count()==before);
    assert(b.erase(1)&&b.count()==2&&strcmp(b.at(1),"Later")==0);
    assert(!b.erase(42)&&!b.toggle(-1));
    assert(b.parse("",0)&&b.count()==0);
    for(int i=0;i<Records::Max;++i) assert(b.add("ok"));
    assert(!b.add("overflow"));
    assert(!b.serialize(text,5));
    assert(leap(2000)&&!leap(1900)&&leap(2024)&&!leap(2025));
    assert(daysInMonth(2024,2)==29&&daysInMonth(2026,2)==28);
    assert(weekday(2026,11,7)==6);
    assert(weekday(2024,2,29)==4);
    assert(weekday(2026,13,1)==-1&&weekday(2026,2,30)==-1);
    int32_t cents=0;
    assert(parseCents("Coffee 3.50",cents)&&cents==350);
    assert(parseCents("Car fuel 67.00",cents)&&cents==6700);
    assert(!parseCents("No number",cents));
    assert(!parseCents("Coffee -2.00",cents));
    assert(!parseCents("Coffee 2.5",cents));
    assert(!parseCents("Coffee 2.555",cents));
    assert(!parseCents("Coffee 99999999999.99",cents));
    puts("Office: bounded records, error preservation, calendar and cents PASS");
}
