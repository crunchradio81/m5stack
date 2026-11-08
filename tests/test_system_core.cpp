#include "../src/core/SystemCore.h"
#include <cassert>
#include <cstdio>
using namespace suite::system;
int main(){
 BatteryTrend t;
 assert(!t.sample(1000)&&!t.sample(5100));
 assert(t.sample(3800)&&t.sample(3820));
 assert(t.average()==3810&&t.trend()==20);
 t.clear();assert(t.count()==0&&t.average()==0&&t.trend()==0);
 for(int i=0;i<20;++i)assert(t.sample(3500+i*10));
 assert(t.count()==16&&t.average()==3615&&t.trend()==150);
 ReadBenchmark b;
 assert(!b.complete(0,0,5000));
 assert(!b.complete(1024,1,1));
 assert(b.complete(1024,0,1000000)&&b.ready());
 assert(b.kibPerSecond()==1);
 assert(b.complete(2048,UINT32_MAX-499,500)); // wraps, 1000 us
 assert(b.kibPerSecond()==2000);
 assert(safeRelativeName("CHIP8.ROM") && safeRelativeName("name_01.BIN"));
 assert(!safeRelativeName("../escape")&&!safeRelativeName(".hidden"));
 assert(!safeRelativeName("x/y")&&!safeRelativeName(""));
 unsigned d,h,m,s;
 uptime(90061,d,h,m,s);
 assert(d==1&&h==1&&m==1&&s==1);
 puts("System Core: battery trend, read bandwidth, paths and uptime PASS");
}
