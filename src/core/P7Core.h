#pragma once
// P7 strict low-memory utilities: no Arduino, SD or device peripherals.
// Original Cardputer K132: 8MiB flash, no PSRAM.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
namespace suite { namespace field {
inline void le16(uint8_t*p,uint16_t v){p[0]=v&255;p[1]=(v>>8)&255;}
inline void le32(uint8_t*p,uint32_t v){
  for(int i=0;i<4;++i)p[i]=static_cast<uint8_t>(v>>(i*8));
}
inline uint16_t r16(const uint8_t*p){return static_cast<uint16_t>(p[0]|(p[1]<<8));}
inline uint32_t r32(const uint8_t*p){
  return static_cast<uint32_t>(p[0])|(static_cast<uint32_t>(p[1])<<8)|
         (static_cast<uint32_t>(p[2])<<16)|(static_cast<uint32_t>(p[3])<<24);
}
struct WavInfo {uint32_t rate=0,bytes=0;};
inline bool wavHeader(uint8_t out[44],uint32_t sampleRate,uint32_t dataBytes){
  if(!out||!(sampleRate==8000||sampleRate==16000)||
     dataBytes>4000000||dataBytes%2)return false;
  memset(out,0,44);
  memcpy(out,"RIFF",4);le32(out+4,dataBytes+36);
  memcpy(out+8,"WAVEfmt ",8);le32(out+16,16);
  le16(out+20,1);le16(out+22,1);
  le32(out+24,sampleRate);le32(out+28,sampleRate*2);
  le16(out+32,2);le16(out+34,16);
  memcpy(out+36,"data",4);le32(out+40,dataBytes);
  return true;
}
inline bool wavParse(const uint8_t*raw,size_t len,uint32_t fileSize,WavInfo& out){
  if(!raw||len<44||fileSize<44||memcmp(raw,"RIFF",4)||
     memcmp(raw+8,"WAVEfmt ",8)||memcmp(raw+36,"data",4)||
     r32(raw+16)!=16||r16(raw+20)!=1||r16(raw+22)!=1||
     r16(raw+32)!=2||r16(raw+34)!=16)return false;
  const uint32_t rate=r32(raw+24),data=r32(raw+40);
  if(!(rate==8000||rate==16000)||r32(raw+28)!=rate*2||
     data%2||data>4000000||r32(raw+4)!=data+36||
     static_cast<uint64_t>(data)+44u!=fileSize)return false;
  out.rate=rate;out.bytes=data;return true;
}
inline bool safeFile(const char*name){
  if(!name||!*name)return false;
  size_t n=0;
  for(;name[n];++n){
    const char c=name[n];
    if(n>=31||!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
                  (c>='0'&&c<='9')||c=='_'||c=='-'||c=='.'))return false;
  }
  if(n>31||name[0]=='.'||strstr(name,".."))return false;
  return true;
}
inline bool safeLeaf(const char*name,const char*extension){
  if(!safeFile(name)||!extension)return false;
  const size_t n=strlen(name),e=strlen(extension);
  return n>e&&strcmp(name+n-e,extension)==0;
}
inline bool validHex(const char*s,unsigned n,uint32_t& v){
  if(!s||strlen(s)!=n||n==0||n>8)return false;
  uint32_t a=0;
  for(unsigned i=0;i<n;++i) {
    const char c=s[i];unsigned digit=16;
    if(c>='0'&&c<='9')digit=c-'0';
    else if(c>='A'&&c<='F')digit=c-'A'+10;
    else if(c>='a'&&c<='f')digit=c-'a'+10;
    if(digit>15)return false;
    a=(a<<4)|digit;
  }
  v=a;return true;
}
struct NecProfile {
  char name[24]={};
  uint16_t address=0;
  uint8_t power=0,up=0,down=0,mute=0,play=0;
};
inline bool parseIrProfile(const char*text,size_t n,NecProfile&dest){
  // Strict one-line format: IRP1,NAME,AAAA,PP,UU,DD,MM,LL
  if(!text||n<24||n>120)return false;
  char temp[121]={};memcpy(temp,text,n);
  if(temp[n-1]=='\n')temp[n-1]=0;
  char*fields[8]={};unsigned fieldsCount=0;
  char*start=temp;
  for(char*cur=temp;;++cur) {
    if(*cur==','||*cur==0){
      if(fieldsCount==8)return false;
      const bool end=*cur==0;*cur=0;fields[fieldsCount++]=start;
      if(end)break;start=cur+1;
    }
  }
  if(fieldsCount!=8||strcmp(fields[0],"IRP1")||!fields[1][0]||
     strlen(fields[1])>=sizeof(dest.name))return false;
  for(const char*p=fields[1];*p;++p)
    if(*p<32||*p>126||*p==',')return false;
  NecProfile p;strcpy(p.name,fields[1]);uint32_t v=0;
  if(!validHex(fields[2],4,v))return false;p.address=static_cast<uint16_t>(v);
  uint8_t*outputs[]={&p.power,&p.up,&p.down,&p.mute,&p.play};
  for(int i=0;i<5;++i){
    if(!validHex(fields[i+3],2,v))return false;
    *outputs[i]=static_cast<uint8_t>(v);
  }
  dest=p;return true;
}
class SignalTrend {
public:
  bool sample(int rssi){
    if(rssi< -127||rssi>0)return false;
    history_[next_]=static_cast<int8_t>(rssi);
    next_=(next_+1)%16;if(count_<16)++count_;return true;
  }
  unsigned count()const{return count_;}
  int average()const {
    if(!count_)return -127;
    int sum=0;for(unsigned i=0;i<count_;++i)sum+=history_[i];
    return sum/static_cast<int>(count_);
  }
  int latest()const{return count_?history_[(next_+15)%16]:-127;}
  int at(unsigned i)const {
    if(i>=count_)return -127;
    const unsigned start=count_==16?next_:0;
    return history_[(start+i)%16];
  }
  void clear(){count_=next_=0;memset(history_,0,sizeof(history_));}
private:
  int8_t history_[16]={};unsigned next_=0,count_=0;
};
inline bool csvEscape(const char*in,char*out,size_t cap){
  if(!in||!out||cap<3)return false;
  size_t p=0;out[p++]='"';
  for(;*in;++in){
    const uint8_t c=static_cast<uint8_t>(*in);
    if(c=='\r'||c=='\n'||c<32)return false;
    if(c=='"'){
      if(p+3>=cap)return false;out[p++]='"';out[p++]='"';
    }else{
      if(p+2>=cap)return false;out[p++]=static_cast<char>(c);
    }
  }
  out[p++]='"';out[p]=0;return true;
}
}} // namespace suite::field
