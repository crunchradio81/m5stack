#pragma once
// P17 limited SD owner-data file library and collision-safe backup names.
// Portable C++17: no Arduino, no heap, no unrestricted filesystem paths.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
namespace suite {namespace safefiles17 {
static constexpr unsigned MaxItems=32;
static constexpr size_t MaxName=36;
static constexpr uint32_t MaxFileSize=256*1024;
inline char upper(char c){
 return c>='a'&&c<='z'?static_cast<char>(c-'a'+'A'):c;
}
inline bool equalIC(const char*a,const char*b){
 if(!a||!b)return false;
 while(*a&&*b){
  if(upper(*a++)!=upper(*b++))return false;
 }
 return *a==0&&*b==0;
}
inline bool safeName(const char*s){
 if(!s)return false;
 const size_t n=strlen(s);
 if(n<1||n>=MaxName||s[0]=='.'||s[n-1]=='.')return false;
 size_t dots=0;
 for(size_t i=0;i<n;++i){
  const char c=s[i];
  if(c=='.'){++dots;continue;}
  if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
       (c>='0'&&c<='9')||c=='-'||c=='_'))return false;
 }
 if(dots>1)return false;
 const char*dot=strchr(s,'.');
 if(dot&&(dot==s||dot[1]==0||strlen(dot+1)>8))return false;
 const size_t stem=dot?static_cast<size_t>(dot-s):n;
 if(stem>24)return false;
 static const char* const reserved[]={
   "CON","PRN","AUX","NUL","CLOCK$",
   "COM1","COM2","COM3","COM4","COM5","COM6","COM7","COM8","COM9",
   "LPT1","LPT2","LPT3","LPT4","LPT5","LPT6","LPT7","LPT8","LPT9"};
 char name[25]={};
 memcpy(name,s,stem);
 for(const char*x:reserved)if(equalIC(name,x))return false;
 return true;
}
inline bool restored(const char*original,unsigned slot,char*out,size_t capacity){
 if(!safeName(original)||slot<1||slot>9||!out)return false;
 const char*dot=strchr(original,'.');
 const size_t stem=dot?static_cast<size_t>(dot-original):strlen(original);
 const size_t trim=stem>21?21:stem;
 const size_t extension=dot?strlen(dot):0;
 if(capacity<3+trim+extension+1)return false;
 out[0]='R';out[1]=static_cast<char>('0'+slot);out[2]='-';
 memcpy(out+3,original,trim);
 if(dot)memcpy(out+3+trim,dot,extension);
 out[3+trim+extension]=0;
 return safeName(out);
}
struct Entry {char name[MaxName]={};uint32_t size=0;};
class Index {
public:
 Index(){clear();}
 void clear(){count_=0;for(auto &item:items_)item=Entry{};}
 unsigned count()const{return count_;}
 const Entry&at(unsigned i)const{
  static const Entry empty{};
  return i<count_?items_[i]:empty;
 }
 int find(const char*name)const{
  if(!safeName(name))return -1;
  for(unsigned i=0;i<count_;++i)if(equalIC(items_[i].name,name))
   return static_cast<int>(i);
  return -1;
 }
 bool add(const char*name,uint32_t size){
  if(!safeName(name)||size>MaxFileSize||count_>=MaxItems||find(name)>=0)
   return false;
  unsigned offset=0;
  for(;offset<count_;++offset){
   const char*existing=items_[offset].name;
   const char*p=existing,*q=name;
   int difference=0;
   while(*p&&*q){
    if(upper(*p)!=upper(*q)){difference=upper(*p)<upper(*q)?-1:1;break;}
    ++p;++q;
   }
   if(!difference&&*p!=*q)difference=*p?1:-1;
   if(difference>0)break;
  }
  for(unsigned i=count_;i>offset;--i)items_[i]=items_[i-1];
  Entry next;memcpy(next.name,name,strlen(name)+1);next.size=size;
  items_[offset]=next;++count_;return true;
 }
private:
 Entry items_[MaxItems]={};
 unsigned count_=0;
};
inline uint32_t digest(const uint8_t*data,size_t n){
 if(!data&&n)return 0;
 uint32_t value=2166136261u;
 for(size_t i=0;i<n;++i){value^=data[i];value*=16777619u;}
 return value;
}
inline bool equalBlock(const uint8_t*a,const uint8_t*b,size_t len){
 return a&&b&&memcmp(a,b,len)==0;
}
}} // suite::safefiles17
