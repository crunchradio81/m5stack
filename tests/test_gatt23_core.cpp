#include "../src/core/Gatt23Core.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
using namespace suite::gatt23;
int main(){
 assert(uuid("180F")&&uuid("0000180f"));
 assert(uuid("12345678-1234-5678-aabb-112233445566"));
 for(const char*invalid:{"","180","180X","xxxxxxx", "12345678-1234-5678-aabb-11223344556Z",
   "../1234","12345678-1234-5678-aabb-11223344556"})assert(!uuid(invalid));
 char p[8]={};properties(true,false,true,false,p);assert(!strcmp(p,"RN"));
 properties(false,false,false,false,p);assert(!strcmp(p,"-"));
 Report r;assert(r.add("180F",'S',""));
 assert(r.add("2A19",'C',"RN"));
 assert(!r.add("1234",'W',"R"));
 assert(r.count()==2&&!strcmp(r.get(1)->uuid,"2A19"));
 for(int i=0;i<40;++i)r.add("1234",'C',"R");
 assert(r.count()==MaxResults && r.cropped());
 r.reset();assert(!r.cropped()&&r.count()==0&&r.get(1)==nullptr);
 puts("P23 BLE GATT: bounded UUID/service/characteristic metadata and permissions PASS");
}
