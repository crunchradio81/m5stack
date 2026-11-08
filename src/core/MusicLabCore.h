#pragma once
// Morse training and tap-tempo logic without Arduino dependencies.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
namespace suite { namespace musiclab {
inline const char* morse(char c) {
    static const char* const letters[26]={
      ".-","-...","-.-.","-..",".","..-.","--.","....","..",
      ".---","-.-",".-..","--","-.","---",".--.","--.-",".-.","...",
      "-","..-","...-",".--","-..-","-.--","--.."
    };
    static const char* const digits[10]={
      "-----",".----","..---","...--","....-",".....","-....",
      "--...","---..","----."
    };
    if(c>='a'&&c<='z')c=static_cast<char>(c-('a'-'A'));
    if(c>='A'&&c<='Z')return letters[c-'A'];
    if(c>='0'&&c<='9')return digits[c-'0'];
    return nullptr;
}
// Return code stream of dots, dashes, spaces and / between words.
inline bool encodeMorse(const char* source,char* out,size_t cap) {
    if(!source||!out||cap<2||!*source)return false;
    size_t len=0;bool lastWord=false;
    for(const char* p=source;*p;++p) {
        if(*p==' '){
            if(!len)continue;
            if(out[len-1]==' ')--len;
            if(len+2>=cap)return false;
            out[len++]='/';out[len++]=' ';lastWord=true;
            continue;
        }
        const char* code=morse(*p);
        if(!code)return false;
        const size_t n=strlen(code);
        if(len+n+2>=cap)return false;
        memcpy(out+len,code,n);len+=n;
        out[len++]=' ';lastWord=false;
    }
    if(!len)return false;
    if(out[len-1]==' ')--len;
    if(len && out[len-1]=='/')--len;
    if(!len)return false;
    out[len]=0;(void)lastWord;return true;
}
class TapTempo {
public:
    void clear(){
        count_=0;index_=0;last_=0;active_=false;
        for(auto &v:intervals_)v=0;
    }
    TapTempo(){clear();}
    void tap(uint32_t now) {
        if(active_) {
            const uint32_t delta=now-last_;
            if(delta<120||delta>3000) {
                count_=index_=0;
            } else {
                intervals_[index_]=delta;
                index_=(index_+1)%8;
                if(count_<8)++count_;
            }
        }
        active_=true;last_=now;
    }
    unsigned bpm()const {
        if(!count_)return 0;
        uint64_t sum=0;
        for(unsigned i=0;i<count_;++i)sum+=intervals_[i];
        const uint64_t rounded=(60000ull*count_+sum/2)/sum;
        return static_cast<unsigned>(rounded);
    }
    unsigned samples()const{return count_;}
private:
    uint32_t intervals_[8]={};
    uint32_t last_=0;
    unsigned count_=0,index_=0;
    bool active_=false;
};
}}
