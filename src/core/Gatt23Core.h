#pragma once
// P23 BLE GATT: bounded, display-only service and characteristic metadata.
// Never issue characteristic writes, subscribe, read private values or pair.
#include <stddef.h>
#include <stdint.h>
#include <string.h>
namespace suite {namespace gatt23 {
constexpr size_t MaxUuid=39;
constexpr unsigned MaxResults=30;
constexpr unsigned MaxAdvertisers=16;
struct Result {
 char uuid[MaxUuid]={};
 char perms[8]={};
 char kind='S';
};
inline bool uuid(const char*s){
 if(!s)return false;
 const size_t n=strlen(s);
 if(n!=4 && n!=8 && n!=36)return false;
 for(size_t i=0;i<n;++i){
  if(n==36&&(i==8||i==13||i==18||i==23)){
   if(s[i]!='-')return false;
   continue;
  }
  const char c=s[i];
  if(!((c>='0'&&c<='9')||(c>='A'&&c<='F')||
       (c>='a'&&c<='f')))return false;
 }
 return true;
}
inline void properties(bool r,bool w,bool n,bool indicate,
                       char out[8]){
 if(!out)return;
 unsigned i=0;
 if(r)out[i++]='R';
 if(w)out[i++]='W';
 if(n)out[i++]='N';
 if(indicate)out[i++]='I';
 if(!i)out[i++]='-';
 out[i]=0;
}
class Report {
 Result rows_[MaxResults]={};
 unsigned count_=0;
 bool cropped_=false;
public:
 void reset(){for(auto& row:rows_)row=Result{};count_=0;cropped_=false;}
 unsigned count()const{return count_;}
 bool cropped()const{return cropped_;}
 const Result*get(unsigned index)const{return index<count_?&rows_[index]:nullptr;}
 bool add(const char*id,char kind,const char*perms){
  if(!uuid(id)||(kind!='S'&&kind!='C')||!perms||strlen(perms)>4)
   return false;
  if(count_>=MaxResults){cropped_=true;return false;}
  Result entry;
  strcpy(entry.uuid,id);strcpy(entry.perms,perms);entry.kind=kind;
  rows_[count_++]=entry;return true;
 }
};
}} // namespace suite::gatt23
