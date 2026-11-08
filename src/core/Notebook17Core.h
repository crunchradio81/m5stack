#pragma once
// P17 pocket notebook core; bounded, ASCII-only, no Arduino/SD/heap.
// User content always lives in /SUITE/NOTES17 or /SUITE/BACKUP/NOTES17.
// No unrestricted paths, implicit deletes, automatic remote sync or secrets.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
namespace suite {namespace notebook17 {
static constexpr unsigned MaxNotes=32,MaxBytes=1023,MaxStem=18;
static constexpr unsigned MaxName=24,MaxQuery=20;
inline char lower(char c){
 return c>='A'&&c<='Z'?static_cast<char>(c-'A'+'a'):c;
}
inline bool equalIC(const char*a,const char*b){
 if(!a||!b)return false;
 while(*a&&*b){
  if(lower(*a)!=lower(*b))return false;
  ++a;++b;
 }
 return *a==0&&*b==0;
}
inline bool contains(const char*body,size_t n,const char*query){
 if(!body||!query||!*query)return false;
 const size_t q=strlen(query);
 if(q>MaxQuery||q>n)return false;
 for(size_t start=0;start+q<=n;++start){
  size_t j=0;
  while(j<q&&lower(body[start+j])==lower(query[j]))++j;
  if(j==q)return true;
 }
 return false;
}
inline bool validStem(const char*s){
 if(!s)return false;
 const size_t n=strlen(s);
 if(n==0||n>MaxStem)return false;
 for(size_t i=0;i<n;++i){
  const char c=s[i];
  if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||
       (c>='0'&&c<='9')||c=='_'||c=='-'))return false;
 }
 static const char*const reserved[]={
  "CON","PRN","AUX","NUL","CLOCK$","COM1","COM2","COM3","COM4",
  "COM5","COM6","COM7","COM8","COM9","LPT1","LPT2","LPT3",
  "LPT4","LPT5","LPT6","LPT7","LPT8","LPT9"
 };
 for(const char*name:reserved)if(equalIC(s,name))return false;
 return true;
}
inline bool makeName(const char*stem,char*out,size_t cap){
 if(!validStem(stem)||!out)return false;
 const size_t n=strlen(stem);
 if(n+5>cap)return false;
 memcpy(out,stem,n);
 memcpy(out+n,".TXT",5);
 return true;
}
inline bool validName(const char*name){
 if(!name)return false;
 const size_t n=strlen(name);
 if(n<5||n>MaxStem+4)return false;
 if(!equalIC(name+n-4,".TXT"))return false;
 char stem[MaxStem+1]={};
 memcpy(stem,name,n-4);
 return validStem(stem);
}
inline bool restoredName(const char*source,unsigned slot,
                         char*out,size_t cap){
 if(!validName(source)||slot<1||slot>9||!out)return false;
 const size_t fullStem=strlen(source)-4;
 const size_t stemLen=fullStem>MaxStem-3?MaxStem-3:fullStem;
 const size_t need=3+stemLen+4;
 if(cap<=need)return false;
 out[0]='R';out[1]=static_cast<char>('0'+slot);out[2]='-';
 memcpy(out+3,source,stemLen);
 memcpy(out+3+stemLen,".TXT",5);
 return validName(out);
}
class Text {
public:
 static constexpr size_t Capacity=MaxBytes;
 Text(){clear();}
 void clear(){data_[0]=0;len_=pos_=0;}
 bool load(const char*data,size_t n){
  if(!data||n>Capacity)return false;
  for(size_t i=0;i<n;++i){
   const uint8_t c=static_cast<uint8_t>(data[i]);
   if(!((c>=32&&c<=126)||c=='\n'))return false;
  }
  memcpy(data_,data,n);data_[n]=0;
  len_=pos_=n;return true;
 }
 bool insert(char c){
  if(len_>=Capacity||!((c>=32&&c<=126)||c=='\n'))return false;
  memmove(data_+pos_+1,data_+pos_,len_-pos_+1);
  data_[pos_++]=c;++len_;return true;
 }
 bool backspace(){
  if(!pos_)return false;
  memmove(data_+pos_-1,data_+pos_,len_-pos_+1);
  --len_;--pos_;return true;
 }
 bool remove(){
  if(pos_>=len_)return false;
  memmove(data_+pos_,data_+pos_+1,len_-pos_);
  --len_;return true;
 }
 void left(){if(pos_)--pos_;}
 void right(){if(pos_<len_)++pos_;}
 void beginning(){pos_=0;}
 void end(){pos_=len_;}
 size_t size()const{return len_;}
 size_t cursor()const{return pos_;}
 const char*data()const{return data_;}
 bool search(const char*query)const{return contains(data_,len_,query);}
private:
 char data_[Capacity+1]={};
 size_t len_=0,pos_=0;
};
struct Entry {char name[MaxName]={};uint16_t bytes=0;};
class Index {
public:
 Index(){clear();}
 void clear(){count_=0;for(auto&e:items_)e=Entry{};}
 unsigned count()const{return count_;}
 const Entry&at(unsigned i)const{
  static const Entry empty{};
  return i<count_?items_[i]:empty;
 }
 int find(const char*name)const{
  if(!validName(name))return -1;
  for(unsigned i=0;i<count_;++i)
   if(equalIC(name,items_[i].name))return static_cast<int>(i);
  return -1;
 }
 bool add(const char*name,unsigned bytes){
  if(!validName(name)||bytes>MaxBytes||count_>=MaxNotes||find(name)>=0)
   return false;
  unsigned offset=0;
  while(offset<count_&&compare(items_[offset].name,name)<0)++offset;
  for(unsigned i=count_;i>offset;--i)items_[i]=items_[i-1];
  Entry e;
  memcpy(e.name,name,strlen(name)+1);
  e.bytes=static_cast<uint16_t>(bytes);
  items_[offset]=e;++count_;return true;
 }
 static int compare(const char*a,const char*b){
  for(;*a&&*b;++a,++b){
   if(lower(*a)<lower(*b))return -1;
   if(lower(*a)>lower(*b))return 1;
  }
  return (*a?1:0)-(*b?1:0);
 }
 int findNext(const char*query,unsigned start)const{
  for(unsigned i=start;i<count_;++i)
   if(contains(items_[i].name,strlen(items_[i].name),query))
    return static_cast<int>(i);
  return -1;
 }
private:
 Entry items_[MaxNotes]={};
 unsigned count_=0;
};
// FNV32 for human-readable inventory/diagnostics ONLY. Never a signature
// or substitute for byte-for-byte restoration verification.
inline uint32_t fingerprint(const uint8_t*bytes,size_t n){
 if(!bytes&&n)return 0;
 uint32_t h=2166136261u;
 for(size_t i=0;i<n;++i){h^=bytes[i];h*=16777619u;}
 return h;
}
inline bool same(const uint8_t*a,size_t an,const uint8_t*b,size_t bn){
 return a&&b&&an==bn&&memcmp(a,b,an)==0;
}
}} // namespace suite::notebook17
