#pragma once
// P20: bounded portable parsing and metrics; no hardware or networking needed.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
namespace suite {namespace field20 {
struct Survey {
 uint32_t uptime=0; char bssid[18]={}; int rssi=0; unsigned channel=0;
};
inline bool hex(char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F');}
inline bool mac(const char* s){
 if(!s||strlen(s)!=17)return false;
 for(unsigned i=0;i<17;++i)if((i%3==2)?s[i]!=':':!hex(s[i]))return false;
 return true;
}
inline bool decimal(const char* s,uint32_t& out){
 if(!s||!*s)return false;
 uint64_t value=0;
 for(const char* p=s;*p;++p){
  if(*p<'0'||*p>'9')return false;
  value=value*10+static_cast<unsigned>(*p-'0');
  if(value>UINT32_MAX)return false;
 }
 out=static_cast<uint32_t>(value);return true;
}
inline bool survey(const char*line,Survey&out){
 if(!line||strlen(line)>220)return false;
 // P7 WirelessCollection CSV: uptime_seconds,site,bssid,rssi,channel,quoted_ssid
 // The SSID field is ignored; it may contain embedded commas/escaped quotes.
 char prefix[160]={};
 const char* cursor=line;size_t n=0;int commas=0;
 while(*cursor && commas<5 && n+1<sizeof(prefix)){
  if(*cursor==',')++commas;
  prefix[n++]=*cursor++;
 }
 if(commas!=5)return false;
 prefix[n-1]=0;
 char* col[5]={};char* p=prefix;
 for(int i=0;i<5;++i){
  col[i]=p;
  char* comma=strchr(p,',');
  if(i<4){if(!comma)return false;*comma=0;p=comma+1;}
  else if(comma)return false;
 }
 uint32_t time=0,ch=0;
 if(!decimal(col[0],time)||!mac(col[2])||!decimal(col[4],ch)||ch==0||ch>14)return false;
 if(!*col[3])return false;
 const char* s=col[3];
 if(*s=='-')++s;
 uint32_t magnitude=0;
 if(!decimal(s,magnitude) || magnitude>127 || col[3][0]!='-')return false;
 const int rssi=-static_cast<int>(magnitude);
 if(rssi>0||rssi<-127)return false;
 if(*cursor!='"')return false;
 ++cursor;
 while(*cursor){
  if(*cursor=='"'){
   ++cursor;
   if(*cursor=='"'){++cursor;continue;}
   if(*cursor=='\r')++cursor;
   if(*cursor=='\n')++cursor;
   if(*cursor)return false;
   break;
  }
  const unsigned char ch=static_cast<unsigned char>(*cursor++);
  if(ch<32||ch>126)return false;
 }
 if(cursor[-1]!='"' && cursor[-1]!='\n' && cursor[-1]!='\r')return false;
 Survey record;
 record.uptime=time;record.rssi=rssi;record.channel=static_cast<unsigned>(ch);
 strcpy(record.bssid,col[2]);out=record;return true;
}
struct History {
 char bssid[18]={};unsigned samples=0;int min=0,max=0;
 int64_t sum=0;uint32_t first=0,last=0;
 int avg()const{return samples?static_cast<int>(sum/static_cast<int64_t>(samples)):0;}
};
inline bool accumulate(History*items,size_t count,const Survey&s){
 if(!items||!count)return false;
 for(size_t i=0;i<count;++i){
  History&h=items[i];
  if(h.samples && strcmp(h.bssid,s.bssid))continue;
  if(!h.samples){
   strcpy(h.bssid,s.bssid);h.min=h.max=s.rssi;h.first=s.uptime;
  }
  if(h.samples>=1000000)return false;
  if(s.rssi<h.min)h.min=s.rssi;if(s.rssi>h.max)h.max=s.rssi;
  h.last=s.uptime;++h.samples;h.sum+=s.rssi;
  return true;
 }
 return false;
}
inline bool privateLan(uint8_t a,uint8_t b,uint8_t c,uint8_t d){
 // RFC1918 only, exclude network and broadcast host addresses.
 if(d==0||d==255)return false;
 if(a==10)return true;
 if(a==172&&b>=16&&b<=31)return true;
 if(a==192&&b==168)return true;
 return false;
}
inline uint32_t kilobitsPerSecond(uint32_t bytes,uint32_t elapsedMs){
 if(!elapsedMs)return 0;
 const uint64_t rate=static_cast<uint64_t>(bytes)*8u*1000u/(static_cast<uint64_t>(elapsedMs)*1000u);
 return rate>UINT32_MAX?UINT32_MAX:static_cast<uint32_t>(rate);
}
inline int offsetHours(int choice){
 const int zones[]={0,-7,-5,0,1,9,10};
 return choice>=0&&choice<7?zones[choice]:0;
}
inline const char*zoneName(int choice){
 const char* zones[]={"UTC","Phoenix MST","NY (EST fixed)","London UTC","Berlin CET","Tokyo JST","Sydney AEST"};
 return choice>=0&&choice<7?zones[choice]:"UTC";
}
}} // namespace suite::field20
