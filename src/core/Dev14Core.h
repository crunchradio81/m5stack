#pragma once
// P14 fixed-memory line-numbered BASIC Studio, text search and hex editing.
// Original K132, no Lua claim, no file/network commands or arbitrary code exec.
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "DevCore.h"
namespace suite {namespace dev14 {
inline bool matchWord(const char*text,const char*keyword){
  if(!text||!keyword)return false;
  while(*keyword){
    char c=*text++,k=*keyword++;
    if(c>='a'&&c<='z')c=static_cast<char>(c-32);
    if(c!=k)return false;
  }
  return *text==' '||*text==0||*text=='\t';
}
inline bool contains(const char*text,const char*needle){
  if(!text||!needle||!*needle)return false;
  for(const char*p=text;*p;++p){
    const char*a=p,*b=needle;
    while(*a&&*b){
      char x=*a,y=*b;
      if(x>='a'&&x<='z')x=static_cast<char>(x-32);
      if(y>='a'&&y<='z')y=static_cast<char>(y-32);
      if(x!=y)break;++a;++b;
    }
    if(!*b)return true;
  }
  return false;
}
class HexBuffer {
public:
 static constexpr size_t Max=256;
 void clear(){len_=0;memset(data_,0,sizeof(data_));}
 HexBuffer(){clear();}
 bool load(const uint8_t*src,size_t n){
   if(!src||n>Max)return false;
   clear();memcpy(data_,src,n);len_=n;return true;
 }
 bool set(size_t offset,uint8_t value){
   if(offset>=len_)return false;data_[offset]=value;return true;
 }
 bool append(uint8_t value){
   if(len_>=Max)return false;data_[len_++]=value;return true;
 }
 uint8_t at(size_t i)const{return i<len_?data_[i]:0;}
 size_t size()const{return len_;}
 const uint8_t*bytes()const{return data_;}
 static bool parseByte(const char*hex,uint8_t&value){
   if(!hex||!hex[0]||!hex[1]||hex[2])return false;
   auto digit=[](char c)->int {
     if(c>='0'&&c<='9')return c-'0';
     if(c>='a'&&c<='f')return c-'a'+10;
     if(c>='A'&&c<='F')return c-'A'+10;
     return -1;
   };
   const int a=digit(hex[0]),b=digit(hex[1]);
   if(a<0||b<0)return false;
   value=static_cast<uint8_t>(a*16+b);return true;
 }
private:
 uint8_t data_[Max]={};
 size_t len_=0;
};
class Program {
public:
 static constexpr int MaxLines=64,LineSize=80,MaxText=6100;
 enum class Status:uint8_t {
   OK,InvalidProgram,InvalidStatement,MissingTarget,StepLimit
 };
 struct Result {
   Status status=Status::OK;
   unsigned steps=0,count=0;
   char output[12][48]={};
 };
 struct Line {uint16_t number=0;char code[LineSize+1]={};};
 void clear(){size_=0;memset(lines_,0,sizeof(lines_));}
 Program(){clear();}
 unsigned count()const{return size_;}
 uint16_t number(unsigned i)const{return i<size_?lines_[i].number:0;}
 const char*statement(unsigned i)const{return i<size_?lines_[i].code:"";}
 bool put(unsigned number,const char*command){
   if(number<1||number>9999||!command||strlen(command)>LineSize)return false;
   for(const char*p=command;*p;++p)
     if(*p<32||*p>126)return false;
   int index=0;while(index<size_&&lines_[index].number<number)++index;
   if(!*command){
     if(index==size_||lines_[index].number!=number)return false;
     for(int j=index;j+1<size_;++j)lines_[j]=lines_[j+1];
     --size_;lines_[size_]=Line{};return true;
   }
   if(index<size_&&lines_[index].number==number){
     strcpy(lines_[index].code,command);return true;
   }
   if(size_>=MaxLines)return false;
   for(int j=size_;j>index;--j)lines_[j]=lines_[j-1];
   lines_[index].number=static_cast<uint16_t>(number);
   strcpy(lines_[index].code,command);++size_;return true;
 }
 bool parse(const char*raw,size_t n){
   if(!raw||n>MaxText||n<6)return false;
   Program next;size_t pos=0;
   while(pos<n){
     const size_t start=pos;
     while(pos<n&&raw[pos]!='\n'){
       const unsigned char c=static_cast<unsigned char>(raw[pos]);
       if(c<32||c>126)return false;
       ++pos;
     }
     if(pos==n||pos-start>LineSize+6)return false;
     char line[LineSize+8]={};
     memcpy(line,raw+start,pos-start);line[pos-start]=0;++pos;
     const char*ptr=line;unsigned number=0;
     if(*ptr<'0'||*ptr>'9')return false;
     while(*ptr>='0'&&*ptr<='9'){
       number=number*10+static_cast<unsigned>(*ptr-'0');++ptr;
       if(number>9999)return false;
     }
     if(*ptr!=' '||!ptr[1]||!next.put(number,ptr+1))return false;
   }
   *this=next;return true;
 }
 size_t encode(char*out,size_t cap)const{
   if(!out||!cap)return 0;
   size_t pos=0;
   for(int i=0;i<size_;++i){
     char digits[8]={};
     const int used=snprintf(digits,sizeof(digits),"%u ",
                             static_cast<unsigned>(lines_[i].number));
     const size_t len=strlen(lines_[i].code);
     if(used<=0||pos+static_cast<size_t>(used)+len+2>cap)return 0;
     memcpy(out+pos,digits,used);pos+=static_cast<size_t>(used);
     memcpy(out+pos,lines_[i].code,len);pos+=len;
     out[pos++]='\n';
   }
   out[pos]=0;return pos;
 }
 Result execute(unsigned maxSteps=1000)const{
   Result result;dev::Basic runtime;runtime.clear();
   if(maxSteps==0||maxSteps>5000||size_==0){
     result.status=Status::InvalidProgram;return result;
   }
   unsigned pc=0;
   while(pc<size_){
     if(result.steps>=maxSteps){
       result.status=Status::StepLimit;return result;
     }
     ++result.steps;
     const char*stmt=lines_[pc].code;
     while(*stmt==' ')++stmt;
     if(matchWord(stmt,"REM")){++pc;continue;}
     if(matchWord(stmt,"END")){result.status=Status::OK;return result;}
     if(matchWord(stmt,"GOTO")){
       if(!jump(stmt+4,pc,result))return result;
       continue;
     }
     if(matchWord(stmt,"IF")){
       const char*then=strstr(stmt," THEN ");
       if(!then){result.status=Status::InvalidStatement;return result;}
       const char*op=nullptr;
       for(const char*p=stmt+3;p<then;++p)
         if(*p=='<'||*p=='>'||*p=='='){op=p;break;}
       if(!op){result.status=Status::InvalidStatement;return result;}
       const size_t left=static_cast<size_t>(op-(stmt+3));
       const size_t right=static_cast<size_t>(then-(op+1));
       if(!left||!right||left>35||right>35){
         result.status=Status::InvalidStatement;return result;
       }
       char lhs[48]="PRINT ",rhs[48]="PRINT ";
       memcpy(lhs+6,stmt+3,left);lhs[6+left]=0;
       memcpy(rhs+6,op+1,right);rhs[6+right]=0;
       const auto a=runtime.run(lhs),b=runtime.run(rhs);
       if(!a.ok||!b.ok){result.status=Status::InvalidStatement;return result;}
       const bool yes=*op=='<'?a.value<b.value:
                      *op=='>'?a.value>b.value:a.value==b.value;
       if(yes){if(!jump(then+6,pc,result))return result;}
       else ++pc;
       continue;
     }
     if(matchWord(stmt,"PRINT")||*stmt=='?'){
       const auto output=runtime.run(stmt);
       if(!output.ok){result.status=Status::InvalidStatement;return result;}
       if(result.count<12){
         snprintf(result.output[result.count++],48,"%.8g",output.value);
       }
       ++pc;continue;
     }
     if(matchWord(stmt,"LET")||matchWord(stmt,"CLEAR")){
       if(!runtime.run(stmt).ok){
         result.status=Status::InvalidStatement;return result;
       }
       ++pc;continue;
     }
     result.status=Status::InvalidStatement;return result;
   }
   return result;
 }
private:
 Line lines_[MaxLines]={};
 int size_=0;
 int find(unsigned number)const{
   for(int i=0;i<size_;++i)if(lines_[i].number==number)return i;
   return -1;
 }
 bool jump(const char*raw,unsigned&pc,Result&result)const{
   if(!raw){result.status=Status::InvalidStatement;return false;}
   while(*raw==' ')++raw;
   if(*raw<'0'||*raw>'9'){
     result.status=Status::InvalidStatement;return false;
   }
   unsigned n=0;
   while(*raw>='0'&&*raw<='9'){
     n=n*10+static_cast<unsigned>(*raw-'0');++raw;
     if(n>9999){result.status=Status::InvalidStatement;return false;}
   }
   while(*raw==' ')++raw;
   if(*raw){result.status=Status::InvalidStatement;return false;}
   const int index=find(n);
   if(index<0){result.status=Status::MissingTarget;return false;}
   pc=static_cast<unsigned>(index);return true;
 }
};
}} // suite::dev14
