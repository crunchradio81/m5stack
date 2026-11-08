// P19 source-owned synthetic Game Boy CPU/LCD smoke; no commercial ROM assets.
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define ENABLE_SOUND 0
#define ENABLE_LCD 1
#include "../src/vendor/peanut_gb.h"
static uint8_t rom[32768];
static unsigned lines, shaded;
static int failed;
static uint8_t read_rom(struct gb_s* gb,const uint_fast32_t addr){
  (void)gb;return addr<sizeof(rom)?rom[addr]:0xff;
}
static uint8_t read_ram(struct gb_s* gb,const uint_fast32_t addr){
  (void)gb;(void)addr;return 0xff;
}
static void write_ram(struct gb_s* gb,const uint_fast32_t addr,const uint8_t val){
  (void)gb;(void)addr;(void)val;
}
static void gb_fault(struct gb_s* gb,const enum gb_error_e err,const uint16_t addr){
  (void)gb;(void)err;(void)addr;failed=1;
}
static void draw_line(struct gb_s* gb,const uint8_t* pix,const uint_fast8_t line){
  (void)gb;
  if(line<144){++lines;if((pix[0]&3u)!=(pix[1]&3u))++shaded;}
}
static void synth_rom(void){
  memset(rom,0,sizeof(rom));
  const uint8_t program[]={
    0x3e,0x00,0xe0,0x40,0x21,0x00,0x80,0x06,0x08,
    0x3e,0xaa,0x22,0xaf,0x22,0x05,0x20,0xf8,
    0x3e,0xe4,0xe0,0x47,0x3e,0x91,0xe0,0x40,0x18,0xfe
  };
  rom[0x100]=0xc3;rom[0x101]=0x50;rom[0x102]=0x01;
  memcpy(rom+0x134,"SUITE DMG TEST",14);
  rom[0x143]=0;rom[0x147]=0;rom[0x148]=0;rom[0x149]=0;
  memcpy(rom+0x150,program,sizeof(program));
  uint8_t check=0;
  for(unsigned i=0x134;i<=0x14c;++i)check=(uint8_t)(check-rom[i]-1);
  rom[0x14d]=check;
}
int main(void){
  synth_rom();
  struct gb_s gb;
  memset(&gb,0,sizeof(gb));
  assert(gb_init(&gb,read_rom,read_ram,write_ram,gb_fault,0)==GB_INIT_NO_ERROR);
  gb_init_lcd(&gb,draw_line);
  for(unsigned frame=0;frame<4;++frame){
    gb.gb_frame=false;
    uint32_t step=0;
    for(;!gb.gb_frame&&!failed&&step<100000;++step)__gb_step_cpu(&gb);
    assert(!failed && gb.gb_frame && step<100000);
  }
  assert(lines>100 && shaded>20);
  printf("Peanut-GB P19 synthetic ROM: PASS (4 bounded CPU/LCD frames, %u scanlines)\n",lines);
  return 0;
}
