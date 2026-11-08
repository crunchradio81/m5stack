#pragma once
// P16 original K132 global appearance and three named settings profiles.
// Portable C++17; no Arduino/SD in the validation core. No credentials saved.
// The checksum detects accidental damage, not malicious modification.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
namespace suite {namespace skin16 {
enum class Theme:uint8_t {Classic,Slate,Terminal,Amber,Violet,HighContrast,Count};
struct Palette {
 uint16_t bg=0x0841,panel=0x10c4,text=0xffff,accent=0x05ff,
          selected=0x2673,muted=0xad55,good=0x5fe9,warning=0xf926;
};
inline const char*themeName(unsigned i){
 static const char*const names[]={"Classic Blue","Slate","Terminal Green",
     "Amber","Violet","High Contrast"};
 return i<static_cast<unsigned>(Theme::Count)?names[i]:"Invalid";
}
inline Palette palette(unsigned i){
 Palette p;
 switch(static_cast<Theme>(i)){
  case Theme::Classic:break;
  case Theme::Slate:
   p.bg=0x1082;p.panel=0x2945;p.accent=0xBDB7;
   p.selected=0x52AA;p.good=0x8F7E;break;
  case Theme::Terminal:
   p.bg=0x0000;p.panel=0x09A0;p.text=0xB7F0;p.accent=0x07E0;
   p.selected=0x0340;p.muted=0x7BCF;p.good=0x07E0;break;
  case Theme::Amber:
   p.bg=0x1000;p.panel=0x3100;p.accent=0xFCA0;
   p.selected=0x79E0;p.muted=0xCD0B;p.good=0xFDA0;break;
  case Theme::Violet:
   p.bg=0x100C;p.panel=0x2817;p.accent=0xF2BF;
   p.selected=0x483D;p.muted=0xB538;p.good=0xADB9;break;
  case Theme::HighContrast:
   p.bg=0xFFFF;p.panel=0xBDF7;p.text=0x0000;
   p.accent=0x001F;p.selected=0xB61F;p.muted=0x4208;
   p.good=0x0400;p.warning=0xA800;break;
  default:break;
 }
 return p;
}
struct Profile {
 char name[17]={};
 uint8_t theme=0,brightness=2,volume=60;
 bool autosave=true;
};
class Settings {
public:
 static constexpr size_t MaxBytes=384;
 static constexpr unsigned Profiles=3;
 Settings(){reset();}
 void reset(){
  active_=0;
  profiles_[0]=Profile{};strcpy(profiles_[0].name,"DEFAULT");
  profiles_[1]=Profile{};strcpy(profiles_[1].name,"NIGHT");
  profiles_[1].theme=1;profiles_[1].brightness=1;profiles_[1].volume=35;
  profiles_[2]=Profile{};strcpy(profiles_[2].name,"CONTRAST");
  profiles_[2].theme=5;profiles_[2].brightness=3;profiles_[2].volume=75;
 }
 unsigned activeIndex()const{return active_;}
 const Profile&active()const{return profiles_[active_];}
 const Profile&at(unsigned n)const{return profiles_[n<Profiles?n:active_];}
 bool choose(unsigned i){if(i>=Profiles)return false;active_=i;return true;}
 bool theme(unsigned i){if(i>=static_cast<unsigned>(Theme::Count))return false;
  profiles_[active_].theme=static_cast<uint8_t>(i);return true;}
 bool brightness(unsigned i){if(i>3)return false;
  profiles_[active_].brightness=static_cast<uint8_t>(i);return true;}
 bool volume(unsigned i){if(i>100)return false;
  profiles_[active_].volume=static_cast<uint8_t>(i);return true;}
 void autosave(bool enabled){profiles_[active_].autosave=enabled;}
 static bool validName(const char*s){
  if(!s)return false;
  const size_t n=strlen(s);if(n<1||n>16)return false;
  for(size_t i=0;i<n;++i){
   const char c=s[i];
   if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
        (c>='0'&&c<='9')||c==' '||c=='_'||c=='-'))return false;
  }
  return true;
 }
 bool rename(unsigned i,const char*s){
  if(i>=Profiles||!validName(s))return false;
  for(unsigned j=0;j<Profiles;++j)if(j!=i&&!strcmp(profiles_[j].name,s))
   return false;
  strcpy(profiles_[i].name,s);return true;
 }
 static uint32_t checksum(const char*data,size_t n){
  uint32_t v=2166136261u;
  for(size_t i=0;i<n;++i){
   v^=static_cast<uint8_t>(data[i]);v*=16777619u;
  }
  return v;
 }
 size_t encode(char*out,size_t cap)const{
  if(!out||cap<50||cap>2048)return 0;
  const int n=snprintf(out,cap,
   "K132PREF2\nACTIVE=%u\n"
   "P0=%s|%u|%u|%u|%u\n"
   "P1=%s|%u|%u|%u|%u\n"
   "P2=%s|%u|%u|%u|%u\n",
   active_,
   profiles_[0].name,profiles_[0].theme,profiles_[0].brightness,
   profiles_[0].volume,profiles_[0].autosave?1:0,
   profiles_[1].name,profiles_[1].theme,profiles_[1].brightness,
   profiles_[1].volume,profiles_[1].autosave?1:0,
   profiles_[2].name,profiles_[2].theme,profiles_[2].brightness,
   profiles_[2].volume,profiles_[2].autosave?1:0);
  if(n<=0||static_cast<size_t>(n)+14>cap||
     static_cast<size_t>(n)>MaxBytes-14)return 0;
  const uint32_t digest=checksum(out,static_cast<size_t>(n));
  const int tail=snprintf(out+n,cap-static_cast<size_t>(n),"HASH=%08lX\n",
      static_cast<unsigned long>(digest));
  if(tail!=14)return 0;
  return static_cast<size_t>(n+tail);
 }
 bool decode(const char*raw,size_t len){
  if(!raw||len<50||len>MaxBytes)return false;
  char copy[MaxBytes+1]={};
  for(size_t i=0;i<len;++i){
   const unsigned char c=static_cast<unsigned char>(raw[i]);
   if(!((c>=32&&c<=126)||c=='\n'))return false;
  }
  memcpy(copy,raw,len);
  if(copy[len-1]!='\n')return false;
  Settings parsed;
  char*lines[6]={};
  unsigned count=0;
  char*position=copy;
  while(*position){
   if(count>=6)return false;
   lines[count++]=position;
   char*end=strchr(position,'\n');
   if(!end)return false;
   *end=0;position=end+1;
  }
  if(count!=6||strcmp(lines[0],"K132PREF2")||
     strncmp(lines[1],"ACTIVE=",7)||strlen(lines[1])!=8||
     lines[1][7]<'0'||lines[1][7]>'2')return false;
  parsed.active_=static_cast<unsigned>(lines[1][7]-'0');
  if(strlen(lines[5])!=13||strncmp(lines[5],"HASH=",5))return false;
  uint32_t hash=0;
  for(unsigned i=5;i<13;++i){
   const char c=lines[5][i];
   const int v=c>='0'&&c<='9'?c-'0':
               c>='A'&&c<='F'?c-'A'+10:-1;
   if(v<0)return false;
   hash=hash*16+static_cast<unsigned>(v);
  }
  const size_t prefix=static_cast<size_t>(lines[5]-copy);
  // Restore the LF immediately before HASH to reconstruct hashed bytes.
  const char saved=copy[prefix-1];
  copy[prefix-1]='\n';
  const bool validHash=checksum(copy,prefix)==hash;
  copy[prefix-1]=saved;
  if(!validHash)return false;
  for(unsigned i=0;i<Profiles;++i){
   char*line=lines[2+i];
   if(line[0]!='P'||line[1]!=static_cast<char>('0'+i)||line[2]!='=')
    return false;
   char*values[5]={};
   unsigned n=0;
   char*start=line+3;
   values[n++]=start;
   for(char*p=start;*p;++p){
    if(*p=='|'){
     if(n>=5)return false;
     *p=0;values[n++]=p+1;
    }
   }
   if(n!=5||!validName(values[0]))return false;
   for(unsigned j=0;j<i;++j)
    if(!strcmp(values[0],parsed.profiles_[j].name))return false;
   strcpy(parsed.profiles_[i].name,values[0]);
   unsigned numbers[4]={};
   for(unsigned j=0;j<4;++j){
    const char*s=values[j+1];
    if(!*s||strlen(s)>3)return false;
    for(;*s;++s){
     if(*s<'0'||*s>'9')return false;
     numbers[j]=numbers[j]*10u+static_cast<unsigned>(*s-'0');
    }
   }
   if(numbers[0]>=static_cast<unsigned>(Theme::Count)||
      numbers[1]>3||numbers[2]>100||numbers[3]>1)return false;
   Profile&p=parsed.profiles_[i];
   p.theme=static_cast<uint8_t>(numbers[0]);
   p.brightness=static_cast<uint8_t>(numbers[1]);
   p.volume=static_cast<uint8_t>(numbers[2]);
   p.autosave=numbers[3]!=0;
  }
  *this=parsed;
  return true;
 }
private:
 Profile profiles_[Profiles]={};
 unsigned active_=0;
};
}} // namespace suite::skin16
