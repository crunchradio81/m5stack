#include "../src/core/DevCore.h"
#include <cassert>
#include <initializer_list>
#include <cmath>
#include <cstdio>
#include <cstring>
using namespace suite::dev;
static void near(double a,double b){assert(std::fabs(a-b)<1e-8);}
int main(){
  Basic repl;
  auto a=repl.run("LET A=2+3*4");
  assert(a.ok);near(a.value,14);near(repl.variable('A'),14);
  a=repl.run("PRINT (A-2)/4");assert(a.ok);near(a.value,3);
  a=repl.run("? 2^3^2");assert(a.ok);near(a.value,512);
  a=repl.run("LET b = -2^2");assert(a.ok);near(repl.variable('B'),4);
  assert(!repl.run("LET A=1/0").ok);near(repl.variable('A'),14);
  assert(!repl.run("LET Q=(2+3").ok);
  assert(!repl.run("PRINT 1+").ok);
  assert(!repl.run("PRINT 1.2.3").ok);
  assert(!repl.run("PRINT 0^(-1)").ok);
  assert(!repl.run("IF X THEN 2").ok);
  a=repl.run("CLEAR");assert(a.ok);near(repl.variable('A'),0);
  for(const auto s:{Science::SquareRoot,Science::Square,Science::Reciprocal,
      Science::SinDeg,Science::CosDeg,Science::TanDeg,Science::NaturalLog,
      Science::Log10,Science::Exp,Science::Factorial}){
    assert(scienceName(s)[0]);}
  near(science(Science::SquareRoot,144).value,12);
  near(science(Science::SinDeg,30).value,.5);
  near(science(Science::CosDeg,60).value,.5);
  near(science(Science::Factorial,5).value,120);
  assert(!science(Science::SquareRoot,-1).ok);
  assert(!science(Science::TanDeg,90).ok);
  assert(!science(Science::Reciprocal,0).ok);
  assert(!science(Science::Factorial,21).ok);
  assert(!science(Science::NaturalLog,0).ok);
  uint8_t bytes[16]={};size_t n=0;
  assert(hexDecode("48656C6C6F",bytes,sizeof(bytes),n)&&n==5);
  assert(memcmp(bytes,"Hello",5)==0);
  char hex[33]={};assert(hexEncode(bytes,n,hex,sizeof(hex))&&strcmp(hex,"48656C6C6F")==0);
  assert(!hexDecode("123",bytes,sizeof(bytes),n));
  assert(!hexDecode("xz",bytes,sizeof(bytes),n));
  assert(!hexDecode("000102030405060708090A0B0C0D0E0F00",bytes,sizeof(bytes),n));
  assert(!hexEncode(bytes,16,hex,4));
  Chip8 chip;
  const uint8_t program[]={
    0x60,0x05, // LD V0,5
    0x70,0x02, // ADD V0,2
    0x61,0x07, // LD V1,7
    0x80,0x14, // ADD V0,V1 => 14
    0xA0,0x00, // LD I,0
    0xD0,0x15, // draw font 0 at x=14,y=7
    0xD0,0x15, // toggle pixels off, VF collision
    0x12,0x0E  // JMP to self (PC=0x20E)
  };
  assert(chip.load(program,sizeof(program)));
  for(int i=0;i<5;++i) assert(chip.step());
  assert(chip.reg(0)==14&&chip.reg(1)==7);
  assert(chip.step() && chip.frame(14,7));
  assert(chip.step() && !chip.frame(14,7) && chip.reg(15)==1);
  assert(chip.step() && chip.pc()==0x20e);
  assert(!chip.load(nullptr,1));
  assert(!chip.load(program,3585));
  const uint8_t bad[]={0xFF,0xFF};
  assert(chip.load(bad,sizeof(bad)));
  assert(!chip.step() && chip.halted());
  const uint8_t timer[]={0x60,0x03,0xF0,0x15,0xF0,0x18,0x12,0x06};
  assert(chip.load(timer,sizeof(timer)));
  assert(chip.step()&&chip.step()&&chip.step());
  assert(chip.delayTimer()==3&&chip.soundTimer()==3);
  chip.tickTimers();assert(chip.delayTimer()==2&&chip.soundTimer()==2);
  puts("Dev Core: BASIC REPL, science, strict hex, CHIP-8 instructions PASS");
}
