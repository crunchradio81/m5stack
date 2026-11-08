#pragma once
// P18 Game Boy cartridge preflight for stock original K132.
// Independent of Arduino, ROMs, Nintendo firmware and emulator libraries.
// Read-only metadata checks, NOT a Game Boy emulator or launch authorization.
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace suite { namespace gb18 {
constexpr size_t HeaderSize = 0x150;
constexpr unsigned ScreenWidth=150, ScreenHeight=135;
constexpr unsigned GBWidth=160, GBHeight=144;
enum class Error:uint8_t {Ok,ShortHeader,InvalidHeaderChecksum,UnknownSize,SizeMismatch};
struct Info {
  char title[17]={};
  uint8_t cartridgeType=0;
  uint8_t romSizeCode=0;
  uint8_t ramSizeCode=0;
  bool colorCapable=false,colorOnly=false;
  uint32_t expectedBytes=0;
};
inline bool romFilename(const char* s){
  if(!s)return false;
  size_t n=strlen(s);
  if(n<4||n>63||s[0]=='.')return false;
  for(size_t i=0;i<n;++i){
    const unsigned char c=static_cast<unsigned char>(s[i]);
    if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
         (c>='0'&&c<='9')||c==' '||c=='.'||c=='_'||c=='-'))return false;
    if(i && c=='.' && s[i-1]=='.')return false;
  }
  const char* ext=s+n-3;
  const bool gb=(ext[0]=='g'||ext[0]=='G')&&(ext[1]=='b'||ext[1]=='B');
  const bool gbc=gb&&(ext[2]=='c'||ext[2]=='C');
  const bool simple=ext[0]=='.'&&(ext[1]=='g'||ext[1]=='G')&&(ext[2]=='b'||ext[2]=='B');
  return simple || (n>=5 && s[n-4]=='.' && gbc);
}
inline uint32_t romBytes(uint8_t code){
  if(code<=8)return 32768u<<code;
  switch(code){
    case 0x52:return 72u*16384u;
    case 0x53:return 80u*16384u;
    case 0x54:return 96u*16384u;
    default:return 0;
  }
}
inline uint8_t headerChecksum(const uint8_t* data){
  uint8_t x=0;
  for(size_t i=0x134;i<=0x14c;++i)x=static_cast<uint8_t>(x-data[i]-1);
  return x;
}
inline Error inspect(const uint8_t* bytes,size_t available,
                     uint64_t fileBytes,Info& output){
  if(!bytes||available<HeaderSize)return Error::ShortHeader;
  if(headerChecksum(bytes)!=bytes[0x14d])return Error::InvalidHeaderChecksum;
  const uint32_t expected=romBytes(bytes[0x148]);
  if(!expected)return Error::UnknownSize;
  if(fileBytes!=expected)return Error::SizeMismatch;
  Info candidate;
  const uint8_t cgb=bytes[0x143];
  candidate.colorCapable=cgb==0x80||cgb==0xc0;
  candidate.colorOnly=cgb==0xc0;
  const size_t titleLength=candidate.colorCapable?15:16;
  size_t n=0;
  for(size_t i=0;i<titleLength && bytes[0x134+i];++i){
    const unsigned char c=bytes[0x134+i];
    if(c<32||c>126)break;
    candidate.title[n++]=static_cast<char>(c);
  }
  while(n && candidate.title[n-1]==' ')--n;
  candidate.title[n]=0;
  if(!n)strcpy(candidate.title,"UNTITLED");
  candidate.cartridgeType=bytes[0x147];
  candidate.romSizeCode=bytes[0x148];
  candidate.ramSizeCode=bytes[0x149];
  candidate.expectedBytes=expected;
  output=candidate;
  return Error::Ok;
}
inline unsigned srcX(unsigned destX){
  return destX<ScreenWidth?destX*GBWidth/ScreenWidth:GBWidth-1;
}
inline unsigned srcY(unsigned destY){
  return destY<ScreenHeight?destY*GBHeight/ScreenHeight:GBHeight-1;
}
// K132 keyboard plan for the eventual emulator; buttons are active-high bits.
enum Button:uint8_t {Up=1,Down=2,Left=4,Right=8,A=16,B=32,Start=64,Select=128};
inline uint8_t button(char c){
  switch(c){
    case 'e':case 'E':return Up;
    case 's':case 'S':return Down;
    case 'a':case 'A':return Left;
    case 'd':case 'D':return Right;
    case 'k':case 'K':return A;
    case 'l':case 'L':return B;
    case '1':return Start;
    case '2':return Select;
    default:return 0;
  }
}
}} // suite::gb18
