#pragma once
// Original Cardputer K132 networking data formats. No heap or Arduino required.
// Credentials and messages are *plain text* on SD / LAN; no implied encryption.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
namespace suite { namespace net8 {
struct Config {
  char ssid[33]={},password[64]={};
  char dnsHost[64]="example.com";
  char tcpHost[64]="example.com";
  uint16_t tcpPort=80;
  char httpHost[64]="";
  uint16_t httpPort=80;
  char httpPath[96]="/";
  char mqttHost[64]="";
  uint16_t mqttPort=1883;
  char mqttTopic[64]="cardputer/events";
  char chatName[22]="Cardputer";
};
inline bool valueAscii(const char*s, size_t len,bool allowSpaces) {
  if(!s||len==0)return false;
  for(size_t i=0;i<len;++i) {
    unsigned char c=static_cast<unsigned char>(s[i]);
    if(c<33||c>126){if(!(allowSpaces&&c==32))return false;}
  }
  return true;
}
inline bool host(const char*text) {
  if(!text)return false;
  const size_t n=strlen(text);
  if(n==0||n>63||text[0]=='.'||text[n-1]=='.')return false;
  for(size_t i=0;i<n;++i){
    const char c=text[i];
    if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
         (c>='0'&&c<='9')||c=='.'||c=='-'))return false;
  }
  return strstr(text,"..")==nullptr;
}
inline bool port(const char*s,uint16_t&output){
  if(!s||!*s)return false;
  unsigned v=0;
  for(;*s;++s){
    if(*s<'0'||*s>'9')return false;
    v=v*10+static_cast<unsigned>(*s-'0');
    if(v>65535)return false;
  }
  if(!v)return false;output=static_cast<uint16_t>(v);return true;
}
inline bool assign(char*dest,size_t cap,const char*value,bool spaces){
  const size_t n=strlen(value);
  if(n>=cap||!valueAscii(value,n,spaces))return false;
  memcpy(dest,value,n+1);return true;
}
inline bool parse(const char*raw,size_t size,Config&out){
  if(!raw||size<6||size>1024)return false;
  Config temp;
  unsigned mask=0;
  size_t pos=0;
  bool first=true;
  while(pos<size) {
    size_t start=pos;
    while(pos<size&&raw[pos]!='\n') {
      const unsigned char c=static_cast<unsigned char>(raw[pos]);
      if(c<32||c>126)return false;
      ++pos;
    }
    const size_t n=pos-start;
    if(pos==size||n>150)return false; // require newline
    ++pos;
    char line[151]={};memcpy(line,raw+start,n);
    if(first){if(strcmp(line,"K132NET1"))return false;first=false;continue;}
    if(!n)continue;
    char*sep=strchr(line,'=');
    if(!sep)return false;
    *sep++=0;unsigned bit=0;bool ok=false;
    if(!strcmp(line,"SSID")) {
      bit=1;ok=assign(temp.ssid,sizeof(temp.ssid),sep,true);
    } else if(!strcmp(line,"PASSWORD")) {
      bit=2;
      const size_t len=strlen(sep);
      ok=len<sizeof(temp.password)&&valueAscii(sep,len,true);
      if(ok)strcpy(temp.password,sep);
      // Password may also be empty for public Wi-Fi.
      if(len==0)ok=true;
    } else if(!strcmp(line,"DNS_HOST")) {
      bit=4;ok=assign(temp.dnsHost,sizeof(temp.dnsHost),sep,false)&&host(temp.dnsHost);
    } else if(!strcmp(line,"TCP_HOST")) {
      bit=8;ok=assign(temp.tcpHost,sizeof(temp.tcpHost),sep,false)&&host(temp.tcpHost);
    } else if(!strcmp(line,"TCP_PORT")) {
      bit=16;ok=port(sep,temp.tcpPort);
    } else if(!strcmp(line,"HTTP_HOST")) {
      bit=32;ok=assign(temp.httpHost,sizeof(temp.httpHost),sep,false)&&host(temp.httpHost);
    } else if(!strcmp(line,"HTTP_PORT")) {
      bit=64;ok=port(sep,temp.httpPort);
    } else if(!strcmp(line,"HTTP_PATH")) {
      bit=128;ok=assign(temp.httpPath,sizeof(temp.httpPath),sep,false)&&
        temp.httpPath[0]=='/'&&strstr(temp.httpPath,"..")==nullptr&&
        strchr(temp.httpPath,'#')==nullptr;
    } else if(!strcmp(line,"MQTT_HOST")) {
      bit=256;ok=assign(temp.mqttHost,sizeof(temp.mqttHost),sep,false)&&host(temp.mqttHost);
    } else if(!strcmp(line,"MQTT_PORT")) {
      bit=512;ok=port(sep,temp.mqttPort);
    } else if(!strcmp(line,"MQTT_TOPIC")) {
      bit=1024;ok=assign(temp.mqttTopic,sizeof(temp.mqttTopic),sep,false)&&
        strchr(temp.mqttTopic,'#')==nullptr&&strchr(temp.mqttTopic,'+')==nullptr;
    } else if(!strcmp(line,"CHAT_NAME")) {
      bit=2048;ok=assign(temp.chatName,sizeof(temp.chatName),sep,true);
    } else return false;
    if(!ok||(mask&bit))return false;
    mask|=bit;
  }
  if(!(mask&1))return false; // explicit owner configuration required
  out=temp;return true;
}
inline bool safeChat(const char*text,size_t n){
  if(!text||!n||n>72)return false;
  for(size_t i=0;i<n;++i) {
    unsigned char c=static_cast<unsigned char>(text[i]);
    if(c<32||c>126)return false;
  }
  return true;
}
inline bool parseChat(const uint8_t*data,size_t n,char sender[22],char message[73]){
  // "K132CHAT1|sender|message" ASCII: no arbitrary SD writes or commands.
  const char prefix[]="K132CHAT1|";
  if(!data||n<=sizeof(prefix)||n>108||memcmp(data,prefix,sizeof(prefix)-1))return false;
  const char*src=reinterpret_cast<const char*>(data);
  size_t sep=sizeof(prefix)-1;
  while(sep<n&&src[sep]!='|')++sep;
  const size_t nameLen=sep-(sizeof(prefix)-1);
  if(sep==n||nameLen==0||nameLen>21||!valueAscii(src+sizeof(prefix)-1,nameLen,true))return false;
  const size_t msgLen=n-sep-1;
  if(!safeChat(src+sep+1,msgLen)||memchr(src+sep+1,'|',msgLen))return false;
  memcpy(sender,src+sizeof(prefix)-1,nameLen);sender[nameLen]=0;
  memcpy(message,src+sep+1,msgLen);message[msgLen]=0;
  return true;
}
inline bool makeChat(const char*sender,const char*message,char*out,size_t cap){
  if(!sender||!message||!out||!valueAscii(sender,strlen(sender),true)||
     strlen(sender)>21||!safeChat(message,strlen(message))||
     strchr(sender,'|')||strchr(message,'|'))return false;
  const size_t total=10+strlen(sender)+1+strlen(message);
  if(total>=cap)return false;
  strcpy(out,"K132CHAT1|");strcat(out,sender);
  strcat(out,"|");strcat(out,message);return true;
}
inline bool safeResponseChar(char c) {
  return (c>=32&&c<=126)||c=='\n'||c=='\r'||c=='\t';
}
inline void safePrintLine(const char*raw,size_t n,char*out,size_t cap){
  if(!out||!cap)return;
  const size_t max=n<cap-1?n:cap-1;
  for(size_t i=0;i<max;++i)out[i]=safeResponseChar(raw[i])?raw[i]:'?';
  out[max]=0;
}
}} // suite::net8
