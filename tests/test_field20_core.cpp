#include "../src/core/Field20Core.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace suite::field20;
int main(){
 Survey s{};
 assert(survey("123,OFFICE,AC:DE:48:00:11:22,-63,11,\"AP, guest\"",s));
 assert(s.uptime==123&&s.rssi==-63&&s.channel==11);
 assert(!survey("123,OFFICE,AC:DE:48:00:11:22,-63,99,\"bad\"",s));
 assert(!survey("123,OFFICE,../files,-63,11,\"bad\"",s));
 assert(!survey("123,OFFICE,AC:DE:48:00:11:22,-300,11,\"bad\"",s));
 assert(!survey("123,OFFICE,AC:DE:48:00:11:22,-63,11,\"bad\"\nWORSE",s));
 History h[2]={};
 assert(accumulate(h,2,s));
 assert(survey("124,OFFICE,AC:DE:48:00:11:22,-43,11,\"AP\"",s));
 assert(accumulate(h,2,s)&&h[0].samples==2);
 assert(h[0].avg()==-53&&h[0].min==-63&&h[0].max==-43);
 assert(privateLan(10,0,0,1)&&privateLan(172,16,1,2)&&privateLan(192,168,0,5));
 assert(!privateLan(8,8,8,8)&&!privateLan(192,168,1,255));
 assert(kilobitsPerSecond(125000,1000)==1000);
 assert(kilobitsPerSecond(1000,0)==0);
 assert(offsetHours(1)==-7&&offsetHours(5)==9);
 puts("P20 Field Suite: CSV history, malformed logs, private LAN, timing and fixed zones PASS");
}
