#pragma once
// P21 parse only bounded SSDP 200 responses; never follow remote LOCATION.
#include <stddef.h>
#include <stdint.h>
#include <string.h>
namespace suite {namespace ssdp21 {
constexpr size_t MaxResponse=1024;
struct Device {char type[49]={},server[49]={};};
inline char lower(char c){return c>='A'&&c<='Z'?static_cast<char>(c+32):c;}
inline bool starts(const char*head,size_t n,const char*want){
 const size_t len=strlen(want);
 if(n<len)return false;
 for(size_t i=0;i<len;++i)if(lower(head[i])!=lower(want[i]))return false;
 return true;
}
inline bool ascii(const char*p,size_t n){
 for(size_t i=0;i<n;++i)if(p[i]<32||p[i]>126)return false;
 return true;
}
inline bool parse(const char*response,size_t length,Device&device){
 if(!response||length<28||length>MaxResponse||memchr(response,0,length))return false;
 const char*end=response+length;
 const char*newline=static_cast<const char*>(memchr(response,'\n',length));
 if(!newline ||
    (!starts(response,static_cast<size_t>(newline-response),"HTTP/1.1 200") &&
     !starts(response,static_cast<size_t>(newline-response),"HTTP/1.0 200")))return false;
 Device next;
 bool terminated=false;
 const char*line=newline+1;
 while(line<end){
  const char*finish=static_cast<const char*>(memchr(line,'\n',static_cast<size_t>(end-line)));
  if(!finish)return false;
  size_t len=static_cast<size_t>(finish-line);
  if(len&&line[len-1]=='\r')--len;
  if(len==0){terminated=true;break;}
  if(len>220||!ascii(line,len))return false;
  const char*colon=static_cast<const char*>(memchr(line,':',len));
  if(colon){
   const size_t key=static_cast<size_t>(colon-line);
   const char*value=colon+1;
   const char*last=line+len;
   while(value<last && *value==' ')++value;
   if(key==2&&starts(line,key,"st")){
    const size_t n=static_cast<size_t>(last-value);
    if(!n||n>=sizeof(next.type))return false;
    memcpy(next.type,value,n);next.type[n]=0;
   }
   if(key==6&&starts(line,key,"server")){
    const size_t n=static_cast<size_t>(last-value);
    if(n>=sizeof(next.server))return false;
    memcpy(next.server,value,n);next.server[n]=0;
   }
  }
  line=finish+1;
 }
 if(!terminated||!next.type[0])return false;
 device=next;return true;
}
}} // suite::ssdp21
