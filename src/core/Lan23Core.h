#pragma once
// Portable P23 authenticated, one-shot LAN file-transfer framing.
// Transport is PLAINTEXT TCP: trust only a private owner-controlled LAN.
// No paths, overwrite, arbitrary commands, or large transfers.
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include "SafeFiles17Core.h"
namespace suite {namespace lan23 {
constexpr uint16_t Port=38948;
constexpr uint32_t MaxFile=256u*1024u;
constexpr size_t MaxHeader=250;
constexpr int MaxFiles=24;
constexpr char Magic[]="K132FILE1";
constexpr char ChallengeMagic[]="K132CHAL1 ";
constexpr char Ready[]="K132READY1\n";
constexpr char Success[]="K132OK1\n";
constexpr char Failure[]="K132ERR1\n";
struct Config {uint8_t key[32]={};};
struct Header {
 char filename[safefiles17::MaxName]={};
 uint32_t bytes=0;
 char digest[65]={};
 char mac[65]={};
};
inline int fromHex(char c) {
 if(c>='0'&&c<='9')return c-'0';
 if(c>='a'&&c<='f')return c-'a'+10;
 if(c>='A'&&c<='F')return c-'A'+10;
 return -1;
}
inline bool hex(const char*s,size_t len){
 if(!s)return false;
 for(size_t i=0;i<len;++i)if(fromHex(s[i])<0)return false;
 return true;
}
inline bool toBytes(const char*s,uint8_t*out,size_t count){
 if(!s||!out||strlen(s)!=2*count||!hex(s,2*count))return false;
 for(size_t i=0;i<count;++i)
  out[i]=static_cast<uint8_t>((fromHex(s[i*2])<<4)|fromHex(s[i*2+1]));
 return true;
}
inline void toHex(const uint8_t*bytes,size_t count,char*out){
 static constexpr char table[]="0123456789abcdef";
 for(size_t i=0;i<count;++i){
  out[i*2]=table[bytes[i]>>4];out[i*2+1]=table[bytes[i]&15];
 }
 out[2*count]=0;
}
inline bool number(const char*s,uint32_t&out){
 if(!s||!*s)return false;
 if(s[0]=='0'&&s[1])return false;
 uint32_t n=0;
 for(;*s;++s){
  if(*s<'0'||*s>'9')return false;
  const uint32_t d=static_cast<uint32_t>(*s-'0');
  if(n>(UINT32_MAX-d)/10)return false;
  n=n*10+d;
 }
 if(n==0||n>MaxFile)return false;
 out=n;return true;
}
inline bool parseConfig(const char*data,size_t len,Config&out){
 static constexpr char beginning[]="K132LAN1\nKEY=";
 if(!data||len!=sizeof(beginning)-1+64+1||
    memcmp(data,beginning,sizeof(beginning)-1)||
    data[len-1]!='\n')return false;
 char digits[65]={};
 memcpy(digits,data+sizeof(beginning)-1,64);
 Config temp;
 if(!toBytes(digits,temp.key,32))return false;
 out=temp;return true;
}
inline bool challenge(const char*s){
 static constexpr size_t Prefix=sizeof(ChallengeMagic)-1;
 return s&&strlen(s)==Prefix+32&&!memcmp(s,ChallengeMagic,Prefix)&&
   hex(s+Prefix,32);
}
inline bool parseHeader(const char*data,size_t len,Header&out){
 if(!data||len<145||len>=MaxHeader||data[len-1]!='\n')return false;
 for(size_t i=0;i<len-1;++i){
  const unsigned char c=static_cast<unsigned char>(data[i]);
  if(c<32||c>126)return false;
 }
 char fields[MaxHeader]={};memcpy(fields,data,len-1);
 char*chunks[5]={};chunks[0]=fields;
 int n=1;
 for(size_t i=0;i<len-1;++i)if(fields[i]=='|'){
  if(n>=5)return false;
  fields[i]=0;chunks[n++]=fields+i+1;
 }
 if(n!=5||strcmp(chunks[0],Magic)!=0)return false;
 if(!safefiles17::safeName(chunks[1]))return false;
 Header candidate;strcpy(candidate.filename,chunks[1]);
 if(!number(chunks[2],candidate.bytes))return false;
 if(strlen(chunks[3])!=64||!hex(chunks[3],64)||
    strlen(chunks[4])!=64||!hex(chunks[4],64))return false;
 strcpy(candidate.digest,chunks[3]);
 strcpy(candidate.mac,chunks[4]);
 out=candidate;return true;
}
// The signed HMAC input includes the receiver's per-session random challenge.
// Canonical format makes re-signing alternate file/size/hash impossible.
inline bool signatureBody(const Header&h,const char*challengeText,
                          char*out,size_t cap){
 if(!challenge(challengeText)||!out||cap<200||
    !safefiles17::safeName(h.filename)||
    h.bytes==0||h.bytes>MaxFile||
    strlen(h.digest)!=64||!hex(h.digest,64))return false;
 const int n=snprintf(out,cap,"%s|%s|%lu|%s|%s",
   Magic,h.filename,static_cast<unsigned long>(h.bytes),
   h.digest,challengeText+sizeof(ChallengeMagic)-1);
 return n>0&&static_cast<size_t>(n)<cap;
}
inline bool constantTimeHex(const char*received,const uint8_t*expected){
 if(!received||strlen(received)!=64||!hex(received,64))return false;
 uint8_t difference=0;
 for(size_t i=0;i<32;++i){
  const uint8_t given=static_cast<uint8_t>(
    (fromHex(received[i*2])<<4)|fromHex(received[i*2+1]));
  difference|=static_cast<uint8_t>(given^expected[i]);
 }
 return difference==0;
}
}} // suite::lan23
