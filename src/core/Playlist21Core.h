#pragma once
// P21 Cardputer ordered MIDI/KAR playlist. Portable, bounded, no Arduino/heap.
#include <stddef.h>
#include <stdint.h>
#include <string.h>
namespace suite { namespace playlist21 {
constexpr size_t MaxBytes=1664;
constexpr int MaxItems=24;
constexpr size_t MaxName=64;
inline char lower(char c){return c>='A'&&c<='Z'?static_cast<char>(c+32):c;}
inline bool same(const char*a,const char*b){
 if(!a||!b)return false;
 while(*a&&*b){if(lower(*a)!=lower(*b))return false;++a;++b;}
 return *a==*b;
}
inline bool fileName(const char*s){
 if(!s)return false;
 size_t n=0;
 for(;s[n]&&n<MaxName;++n){
  const char c=s[n];
  if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
        (c>='0'&&c<='9')||c=='_'||c=='-'||c==' '||c=='.'))return false;
 }
 if(n<5||n>=MaxName||s[0]==' '||s[0]=='.'||s[n-1]==' ')return false;
 for(size_t i=1;i<n;++i)if(s[i]=='.'&&s[i-1]=='.')return false;
 const char*dot=strrchr(s,'.');
 if(!dot)return false;
 return same(dot,".mid")||same(dot,".midi")||same(dot,".kar");
}
class Queue {
 char name_[MaxItems][MaxName]={};
 unsigned count_=0;
public:
 unsigned count()const{return count_;}
 const char*at(unsigned i)const{return i<count_?name_[i]:"";}
 void clear(){memset(name_,0,sizeof(name_));count_=0;}
 int indexOf(const char*name)const{
  if(!name)return -1;
  for(unsigned i=0;i<count_;++i)if(same(name_[i],name))return static_cast<int>(i);
  return -1;
 }
 bool add(const char*name){
  if(count_>=MaxItems||!fileName(name)||indexOf(name)>=0)return false;
  strcpy(name_[count_++],name);return true;
 }
 bool remove(unsigned i){
  if(i>=count_)return false;
  for(unsigned n=i;n+1<count_;++n)strcpy(name_[n],name_[n+1]);
  name_[--count_][0]=0;return true;
 }
 bool move(unsigned i,int delta){
  if(i>=count_|| (delta!=-1&&delta!=1))return false;
  const int to=static_cast<int>(i)+delta;
  if(to<0||to>=static_cast<int>(count_))return false;
  char temp[MaxName];strcpy(temp,name_[i]);
  strcpy(name_[i],name_[to]);strcpy(name_[to],temp);return true;
 }
 bool parse(const char*data,size_t len){
  static const char header[]="K132PLAY1\n";
  if(!data||len<sizeof(header)-1||len>MaxBytes||
     memcmp(data,header,sizeof(header)-1))return false;
  Queue result;
  size_t p=sizeof(header)-1;
  while(p<len){
   const size_t begin=p;
   while(p<len&&data[p]!='\n'){
    const unsigned char c=static_cast<unsigned char>(data[p]);
    if(c<32||c>126||p-begin>=MaxName)return false;
    ++p;
   }
   if(p==len||p==begin||p-begin>=MaxName)return false;
   char file[MaxName]={};
   memcpy(file,data+begin,p-begin);++p;
   if(!result.add(file))return false;
  }
  *this=result;return true;
 }
 size_t encode(char*dest,size_t cap)const{
  static const char head[]="K132PLAY1\n";
  if(!dest||cap<sizeof(head))return 0;
  size_t p=sizeof(head)-1;
  memcpy(dest,head,p);
  for(unsigned i=0;i<count_;++i){
   const size_t n=strlen(name_[i]);
   if(p+n+1>=cap)return 0;
   memcpy(dest+p,name_[i],n);p+=n;dest[p++]='\n';
  }
  dest[p]=0;return p;
 }
};
}} // namespace suite::playlist21
