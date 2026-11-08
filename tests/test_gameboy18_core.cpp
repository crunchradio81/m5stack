#include "../src/core/GameBoy18Core.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
using namespace suite::gb18;
int main(){
  assert(romFilename("TETRIS.GB"));
  assert(romFilename("my game.gbc"));
  assert(romFilename("HELLO.gb"));
  assert(!romFilename("../TETRIS.GB"));
  assert(!romFilename("/TETRIS.GB"));
  assert(!romFilename("TETRIS.zip"));
  assert(!romFilename(".gb"));
  assert(!romFilename("abc..gb"));
  assert(romBytes(0)==32768);
  assert(romBytes(7)==4u*1024u*1024u);
  assert(romBytes(0x52)==72u*16384u);
  assert(romBytes(0xff)==0);
  uint8_t header[HeaderSize]={};
  memcpy(header+0x134,"PUBLIC TEST",11);
  header[0x143]=0x80; // GBC-compatible; 15-byte title
  header[0x147]=0x01; // MBC1
  header[0x148]=0x00; // 32KiB
  header[0x149]=0x02; // 8KiB cartridge RAM
  header[0x14d]=headerChecksum(header);
  Info info;
  assert(inspect(header,sizeof(header),32768,info)==Error::Ok);
  assert(!strcmp(info.title,"PUBLIC TEST"));
  assert(info.colorCapable && !info.colorOnly);
  assert(info.cartridgeType==1 && info.expectedBytes==32768);
  assert(inspect(header,sizeof(header)-1,32768,info)==Error::ShortHeader);
  assert(inspect(header,sizeof(header),32769,info)==Error::SizeMismatch);
  header[0x14d]^=1;
  assert(inspect(header,sizeof(header),32768,info)==Error::InvalidHeaderChecksum);
  header[0x14d]^=1;
  header[0x143]=0xc0;header[0x14d]=headerChecksum(header);
  assert(inspect(header,sizeof(header),32768,info)==Error::Ok && info.colorOnly);
  header[0x148]=0x55;header[0x14d]=headerChecksum(header);
  assert(inspect(header,sizeof(header),32768,info)==Error::UnknownSize);
  assert(srcX(0)==0&&srcX(149)==158&&srcX(999)==159);
  assert(srcY(0)==0&&srcY(134)==142&&srcY(999)==143);
  assert(button('e')==Up&&button('D')==Right);
  assert(button('k')==A&&button('L')==B);
  assert(button('1')==Start&&button('2')==Select);
  assert(button('q')==0);
  std::cout<<"Game Boy P18 preflight: PASS (synthetic headers, bounds, controls)\n";
}
