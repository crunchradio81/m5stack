#pragma once
// P11 pocket editing primitives; ASCII-only, bounded for 320 KiB SRAM K132.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limits.h>
namespace suite {namespace pocket11 {
class Editor {
public:
 static constexpr size_t Max=1024;
 Editor(){clear();}
 void clear(){text_[0]=0;length_=position_=0;}
 const char*text()const{return text_;}
 size_t size()const{return length_;}
 size_t cursor()const{return position_;}
 bool set(const char*data,size_t len) {
   if(!data||len>=Max)return false;
   for(size_t i=0;i<len;++i){
     const unsigned char c=static_cast<unsigned char>(data[i]);
     if(!((c>=32&&c<=126)||c=='\n'))return false;
   }
   memcpy(text_,data,len);text_[len]=0;length_=position_=len;return true;
 }
 bool insert(char c){
   if(length_+1>=Max||!((c>=32&&c<=126)||c=='\n'))return false;
   memmove(text_+position_+1,text_+position_,length_-position_+1);
   text_[position_++]=c;++length_;return true;
 }
 bool backspace() {
   if(position_==0)return false;
   memmove(text_+position_-1,text_+position_,length_-position_+1);
   --position_;--length_;return true;
 }
 void left(){if(position_)--position_;}
 void right(){if(position_<length_)++position_;}
private:
 char text_[Max]={};
 size_t length_=0,position_=0;
};
class Sheet {
public:
 static constexpr int W=6,H=6,Limit=99999;
 void clear(){memset(cells_,0,sizeof(cells_));}
 Sheet(){clear();}
 bool set(int x,int y,int32_t value){
   if(x<0||x>=W||y<0||y>=H||value<-Limit||value>Limit)return false;
   cells_[y][x]=value;return true;
 }
 int32_t at(int x,int y)const{
   return x>=0&&x<W&&y>=0&&y<H?cells_[y][x]:0;
 }
 int64_t sumRow(int row)const{
   if(row<0||row>=H)return 0;
   int64_t sum=0;
   for(int x=0;x<W;++x)sum+=cells_[row][x];
   return sum;
 }
 int64_t sumCol(int col)const{
   if(col<0||col>=W)return 0;
   int64_t sum=0;for(int y=0;y<H;++y)sum+=cells_[y][col];
   return sum;
 }
 static bool number(const char*s,size_t n,int32_t &result){
   if(!s||!n||n>7)return false;
   size_t p=0;bool negative=false;
   if(s[p]=='-'){negative=true;++p;}
   if(p==n)return false;
   int32_t value=0;
   for(;p<n;++p){
     if(s[p]<'0'||s[p]>'9')return false;
     value=value*10+(s[p]-'0');
     if(value>Limit)return false;
   }
   result=negative?-value:value;return true;
 }
 // Six CSV data rows, six signed integer cells, LF; no formulas/macros.
 bool parse(const char*data,size_t n){
   if(!data||n<8||n>512)return false;
   static const char hdr[]="SHEET1\n";
   if(n<sizeof(hdr)-1||memcmp(data,hdr,sizeof(hdr)-1))return false;
   Sheet next;size_t p=sizeof(hdr)-1;
   for(int row=0;row<H;++row)for(int col=0;col<W;++col){
     const size_t start=p;
     while(p<n&&data[p]!=','&&data[p]!='\n')++p;
     int32_t value=0;
     if(!number(data+start,p-start,value))return false;
     if(p>=n||data[p]!=(col==W-1?'\n':','))return false;
     ++p;next.cells_[row][col]=value;
   }
   if(p!=n)return false;
   *this=next;return true;
 }
 size_t encode(char*out,size_t cap)const{
   if(!out||cap<8)return 0;
   size_t p=0;memcpy(out,"SHEET1\n",7);p=7;
   for(int y=0;y<H;++y)for(int x=0;x<W;++x){
     int32_t value=cells_[y][x];
     char digits[12]={};size_t n=0;
     const bool neg=value<0;
     unsigned v=static_cast<unsigned>(neg?-value:value);
     do{digits[n++]=static_cast<char>('0'+v%10);v/=10;}while(v);
     if(neg)digits[n++]='-';
     if(p+n+2>=cap)return 0;
     while(n)out[p++]=digits[--n];
     out[p++]=(x==W-1)?'\n':',';
   }
   out[p]=0;return p;
 }
private:
 int32_t cells_[H][W]={};
};
class Flashcards {
public:
 static constexpr int Max=20,Width=42;
 Flashcards(){clear();}
 void clear(){count_=0;memset(q_,0,sizeof(q_));memset(a_,0,sizeof(a_));}
 int count()const{return count_;}
 const char*question(int i)const{return i>=0&&i<count_?q_[i]:"";}
 const char*answer(int i)const{return i>=0&&i<count_?a_[i]:"";}
 bool parse(const char*data,size_t n){
   if(!data||n>1800)return false;
   Flashcards next;
   size_t p=0;
   while(p<n){
     if(next.count_>=Max)return false;
     const size_t start=p;
     while(p<n&&data[p]!='|'&&data[p]!='\n')++p;
     const size_t qlen=p-start;
     if(p>=n||data[p]!='|'||qlen==0||qlen>=Width)return false;
     char question[Width]={};memcpy(question,data+start,qlen);++p;
     const size_t ansStart=p;
     while(p<n&&data[p]!='\n')++p;
     const size_t alen=p-ansStart;
     if(p>=n||alen==0||alen>=Width)return false;
     char answer[Width]={};memcpy(answer,data+ansStart,alen);++p;
     for(size_t i=0;i<qlen;++i)if(question[i]<32||question[i]>126)return false;
     for(size_t i=0;i<alen;++i)if(answer[i]<32||answer[i]>126||
                                           answer[i]=='|')return false;
     strcpy(next.q_[next.count_],question);strcpy(next.a_[next.count_],answer);
     ++next.count_;
   }
   *this=next;return true;
 }
 bool search(const char*term,int&index)const{
   if(!term||!*term)return false;
   for(int i=0;i<count_;++i)
     for(const char*p=q_[i];*p;++p) {
       const char*a=p,*b=term;
       while(*a&&*b) {
         char x=*a,y=*b;
         if(x>='A'&&x<='Z')x=static_cast<char>(x+32);
         if(y>='A'&&y<='Z')y=static_cast<char>(y+32);
         if(x!=y)break;++a;++b;
       }
       if(!*b){index=i;return true;}
     }
   return false;
 }
private:
 char q_[Max][Width]={},a_[Max][Width]={};
 int count_=0;
};
template<class RNG>bool randomString(char*out,size_t cap,unsigned length,RNG rng){
 static const char alphabet[]=
   "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789";
 const uint32_t n=static_cast<uint32_t>(sizeof(alphabet)-1);
 if(!out||length<8||length>32||cap<=length)return false;
 const uint32_t cutoff=UINT32_MAX-UINT32_MAX%n;
 for(unsigned i=0;i<length;++i){
   uint32_t v=0;
   unsigned tries=0;
   do{
     v=rng();
     if(++tries>200)return false;
   }while(v>=cutoff);
   out[i]=alphabet[v%n];
 }
 out[length]=0;return true;
}
}} // suite::pocket11
