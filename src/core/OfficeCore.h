#pragma once
// Fixed-size plain-text records for the original Cardputer's SD office apps.
// Strict, atomic in-memory parsing; caller owns backup/rename file procedure.
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
namespace suite { namespace office {
class Records {
public:
    static constexpr int Max=40,Width=52;
    void clear(){count_=0;memset(text_,0,sizeof(text_));memset(done_,0,sizeof(done_));}
    Records(){clear();}
    int count()const{return count_;}
    const char* at(int i)const{return i>=0&&i<count_?text_[i]:"";}
    bool done(int i)const{return i>=0&&i<count_?done_[i]:false;}
    bool add(const char* t) {
        if(!validText(t)||count_>=Max)return false;
        strcpy(text_[count_],t);done_[count_]=false;++count_;return true;
    }
    bool toggle(int i) {if(i<0||i>=count_)return false;done_[i]=!done_[i];return true;}
    bool erase(int i){
        if(i<0||i>=count_)return false;
        for(int p=i;p+1<count_;++p) {
            strcpy(text_[p],text_[p+1]);done_[p]=done_[p+1];
        }
        --count_;text_[count_][0]=0;done_[count_]=false;return true;
    }
    static bool validText(const char* t) {
        if(!t)return false;
        const size_t n=strlen(t);
        if(!n||n>=Width)return false;
        for(size_t i=0;i<n;++i) {
            const uint8_t c=static_cast<uint8_t>(t[i]);
            if(c<32||c>126)return false;
        }
        return true;
    }
    // On parse failure, do not overwrite previous valid records.
    bool parse(const char* bytes,size_t len) {
        if(!bytes||len>4096)return false;
        Records next;
        size_t offset=0;
        while(offset<len) {
            if(offset+2>len)return false;
            const char mark=bytes[offset++];
            if((mark!='0'&&mark!='1')||bytes[offset++]!='|')return false;
            char value[Width]={};size_t n=0;
            while(offset<len&&bytes[offset]!='\n') {
                if(n+1>=Width)return false;
                value[n++]=bytes[offset++];
            }
            if(offset==len||!n)return false; // require final newline
            ++offset;
            if(!next.add(value))return false;
            next.done_[next.count_-1]=(mark=='1');
        }
        *this=next;return true;
    }
    size_t serialize(char* out,size_t cap) const {
        if(!out||!cap)return 0;
        size_t pos=0;
        for(int i=0;i<count_;++i) {
            const size_t n=strlen(text_[i]);
            if(pos+2+n+1>=cap)return 0;
            out[pos++]=done_[i]?'1':'0';out[pos++]='|';
            memcpy(out+pos,text_[i],n);pos+=n;
            out[pos++]='\n';
        }
        out[pos]=0;return pos;
    }
private:
    char text_[Max][Width];
    bool done_[Max];
    int count_=0;
};
inline bool leap(int year){
    return year%4==0&&(year%100!=0||year%400==0);
}
inline int daysInMonth(int year,int month){
    if(year<1600||year>9999||month<1||month>12)return 0;
    constexpr int days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    return days[month-1]+(month==2&&leap(year)?1:0);
}
// Gregorian day-of-week, 0=Sunday. Time-zone and RTC independent.
inline int weekday(int y,int m,int d) {
    if(!daysInMonth(y,m)||d<1||d>daysInMonth(y,m))return -1;
    constexpr int t[12]={0,3,2,5,0,3,5,1,4,6,2,4};
    if(m<3)--y;
    return (y+y/4-y/100+y/400+t[m-1]+d)%7;
}
inline bool parseCents(const char *text,int32_t& out){
    if(!text||!*text)return false;
    const char* p=strrchr(text,' ');
    if(!p||!p[1])return false;
    ++p;uint32_t dollars=0,cents=0;
    int decimal=0,places=0;
    for(;*p;++p) {
        const char c=*p;
        if(c=='.') {
            if(decimal)return false;
            decimal=1;continue;
        }
        if(c<'0'||c>'9')return false;
        if(!decimal) {
            if(dollars>1000000u)return false;
            dollars=dollars*10u+(c-'0');
        } else {
            if(++places>2)return false;
            cents=cents*10u+(c-'0');
        }
    }
    if(!decimal||places!=2||dollars>1000000u)return false;
    const uint64_t n=static_cast<uint64_t>(dollars)*100u+cents;
    if(n>INT32_MAX)return false;
    out=static_cast<int32_t>(n);return true;
}
}} // namespace suite::office
