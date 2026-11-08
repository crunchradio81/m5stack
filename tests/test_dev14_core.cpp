#include "../src/core/Dev14Core.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace suite::dev14;
int main(){
 Program p;assert(p.count()==0);
 assert(p.put(10,"LET A=1"));
 assert(p.put(20,"PRINT A"));
 assert(p.put(30,"LET A=A+1"));
 assert(p.put(40,"IF A<5 THEN 20"));
 assert(p.put(50,"END"));
 assert(p.count()==5);
 auto run=p.execute(100);
 assert(run.status==Program::Status::OK&&run.count>=4);
 assert(strcmp(run.output[0],"1")==0&&
        strcmp(run.output[3],"4")==0);
 assert(!p.put(0,"PRINT 10")&&!p.put(10000,"PRINT 10"));
 char serialized[6102]={};
 const size_t n=p.encode(serialized,sizeof(serialized));
 assert(n>0);
 Program again;assert(again.parse(serialized,n)&&again.count()==5);
 assert(again.number(0)==10&&again.number(4)==50);
 assert(again.execute(100).status==Program::Status::OK);
 assert(!again.parse("10 PRINT 1",10));
 assert(!again.parse("10 FOO\x01\n",8));
 assert(again.count()==5); // parse is atomic
 assert(again.put(20,""));
 assert(again.count()==4);
 assert(again.put(20,"GOTO 99"));
 assert(again.execute(100).status==Program::Status::MissingTarget);
 Program infinite;
 assert(infinite.put(10,"GOTO 10"));
 assert(infinite.execute(40).status==Program::Status::StepLimit);
 Program invalid;assert(invalid.put(10,"INPUT A"));
 assert(invalid.execute().status==Program::Status::InvalidStatement);
 HexBuffer bytes;assert(bytes.append(0)&&bytes.append(255));
 assert(bytes.size()==2&&bytes.at(1)==255);
 assert(!bytes.set(3,40));assert(bytes.set(1,5)&&bytes.at(1)==5);
 uint8_t value=0;
 assert(HexBuffer::parseByte("Af",value)&&value==0xAF);
 assert(!HexBuffer::parseByte("ABC",value));
 assert(!HexBuffer::parseByte("FG",value));
 assert(contains("BASIC Studio","studio"));
 assert(!contains("BASIC Studio","lua"));
 puts("P14 Dev Studio: BASIC programs, line I/O, flow control, step limits and hex PASS");
}
