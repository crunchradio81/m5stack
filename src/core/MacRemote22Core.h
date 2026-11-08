#pragma once
// Portable strict stock-K132 Mac remote config validator; no Arduino/heap.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
namespace suite {namespace mac22 {
struct Config {
 char address[16]={};
 uint8_t secret[32]={};
};
inline int hexDigit(char c){
 if(c>='0'&&c<='9')return c-'0';
 if(c>='a'&&c<='f')return c-'a'+10;
 if(c>='A'&&c<='F')return c-'A'+10;
 return -1;
}
inline bool decodeSecret(const char*s,uint8_t out[32]){
 if(!s||strlen(s)!=64)return false;
 uint8_t temp[32]={};
 for(int i=0;i<32;++i){
  const int hi=hexDigit(s[i*2]),lo=hexDigit(s[i*2+1]);
  if(hi<0||lo<0)return false;
  temp[i]=static_cast<uint8_t>((hi<<4)|lo);
 }
 memcpy(out,temp,32);return true;
}
inline bool address(const char*ip){
 if(!ip)return false;
 const size_t n=strlen(ip);
 if(n<7||n>15)return false;
 unsigned octets[4]={};
 const char*cursor=ip;
 for(int i=0;i<4;++i){
  if(!*cursor)return false;
  unsigned count=0,numeric=0;
  while(*cursor>='0'&&*cursor<='9'){
   if(++count>3)return false;
   numeric=numeric*10+static_cast<unsigned>(*cursor-'0');
   if(numeric>255)return false;
   ++cursor;
  }
  if(!count||(count>1&&cursor[-count]=='0'))return false;
  octets[i]=numeric;
  if(i==3){if(*cursor)return false;}
  else if(*cursor++!='.')return false;
 }
 const unsigned a=octets[0],b=octets[1],d=octets[3];
 if(d==0||d==255)return false;
 return a==10||(a==172&&b>=16&&b<=31)||(a==192&&b==168);
}
inline bool parse(const char*raw,size_t length,Config&out){
 if(!raw||length>160||length<100||memchr(raw,0,length))return false;
 static const char header[]="K132MAC1\n";
 if(length<sizeof(header)-1||memcmp(raw,header,sizeof(header)-1))return false;
 Config next;bool host=false,token=false;
 size_t pos=sizeof(header)-1;
 while(pos<length){
  const size_t start=pos;
  while(pos<length&&raw[pos]!='\n'){
   if(raw[pos]<33||raw[pos]>126)return false;
   ++pos;
  }
  if(pos==length)return false;
  const size_t n=pos-start;++pos;
  if(n==0)continue;
  if(n>=80)return false;
  char line[80]={};
  memcpy(line,raw+start,n);
  if(strncmp(line,"HOST=",5)==0){
   if(host||!address(line+5))return false;
   strcpy(next.address,line+5);host=true;
  }else if(strncmp(line,"TOKEN=",6)==0){
   if(token||!decodeSecret(line+6,next.secret))return false;
   token=true;
  }else return false;
 }
 if(!host||!token)return false;
 out=next;return true;
}
inline int macroIndex(char c){
 if(c>='1'&&c<='9')return c-'1'+1;
 if(c=='0')return 10;
 if(c=='-')return 11;
 if(c=='=')return 12;
 return 0;
}
inline bool move(int dx,int dy){return dx>=-40&&dx<=40&&dy>=-40&&dy<=40;}
inline bool utf8Ascii(char c){return c>=32&&c<=126&&c!='|';}
}} // suite::mac22
