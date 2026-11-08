#pragma once
// Low-memory bounded JSON and CSV inspection, host-testable.
#include <stddef.h>
#include <stdint.h>
#include <string.h>
namespace suite { namespace inspect {
class Json {
public:
    static bool valid(const char* text,size_t length) {
        if(!text || length>4096)return false;
        Json p(text,length);p.space();
        const bool ok=p.value(0);
        p.space();
        return ok&&p.pos_==length;
    }
private:
    const char* text_;
    size_t size_,pos_=0;
    Json(const char* p,size_t n):text_(p),size_(n){}
    char peek()const {return pos_<size_?text_[pos_]:'\0';}
    bool take(char c){if(peek()==c){++pos_;return true;}return false;}
    void space(){
        while(peek()==' '||peek()=='\r'||peek()=='\n'||peek()=='\t')++pos_;
    }
    bool literal(const char* s){
        const size_t n=strlen(s);
        if(pos_+n>size_||memcmp(text_+pos_,s,n)!=0)return false;
        pos_+=n;return true;
    }
    bool string() {
        if(!take('"'))return false;
        while(pos_<size_) {
            const uint8_t c=static_cast<uint8_t>(text_[pos_++]);
            if(c=='"')return true;
            if(c<32 || c>126)return false; // ASCII-only document profile
            if(c=='\\') {
                if(pos_>=size_)return false;
                const char esc=text_[pos_++];
                if(esc=='u') {
                    for(int i=0;i<4;++i) {
                        if(pos_>=size_)return false;
                        const char h=text_[pos_++];
                        if(!((h>='0'&&h<='9')||(h>='a'&&h<='f')||
                             (h>='A'&&h<='F')))return false;
                    }
                } else if(!(esc=='"'||esc=='\\'||esc=='/'||esc=='b'||
                             esc=='f'||esc=='n'||esc=='r'||esc=='t'))return false;
            }
        }
        return false;
    }
    bool number() {
        take('-');
        if(take('0')) {
            if(peek()>='0'&&peek()<='9')return false;
        } else {
            if(peek()<'1'||peek()>'9')return false;
            while(peek()>='0'&&peek()<='9')++pos_;
        }
        if(take('.')) {
            if(peek()<'0'||peek()>'9')return false;
            while(peek()>='0'&&peek()<='9')++pos_;
        }
        if(take('e')||take('E')) {
            if(peek()=='+'||peek()=='-')++pos_;
            if(peek()<'0'||peek()>'9')return false;
            while(peek()>='0'&&peek()<='9')++pos_;
        }
        return true;
    }
    bool value(unsigned depth) {
        if(depth>12)return false;
        space();
        if(take('{')) {
            space();if(take('}'))return true;
            do {
                space();if(!string())return false;
                space();if(!take(':'))return false;
                if(!value(depth+1))return false;
                space();if(take('}'))return true;
            }while(take(','));
            return false;
        }
        if(take('[')) {
            space();if(take(']'))return true;
            do {
                if(!value(depth+1))return false;
                space();if(take(']'))return true;
            }while(take(','));
            return false;
        }
        if(peek()=='"')return string();
        if(peek()=='t')return literal("true");
        if(peek()=='f')return literal("false");
        if(peek()=='n')return literal("null");
        if(peek()=='-'||(peek()>='0'&&peek()<='9'))return number();
        return false;
    }
};
inline bool asciiSearch(const char* haystack,const char* needle) {
    if(!haystack||!needle||!*needle)return false;
    const size_t n=strlen(needle);
    for(const char* p=haystack;*p;++p){
        size_t i=0;
        for(;i<n&&p[i];++i) {
            char a=p[i],b=needle[i];
            if(a>='A'&&a<='Z')a=static_cast<char>(a+32);
            if(b>='A'&&b<='Z')b=static_cast<char>(b+32);
            if(a!=b)break;
        }
        if(i==n)return true;
    }
    return false;
}
inline bool csvColumns(const char* line,size_t length,char out[8][40],unsigned& cols) {
    if(!line||length>480)return false;
    cols=0;size_t pos=0;
    while(pos<=length) {
        if(cols>=8)return false;
        size_t n=0;bool quoted=false;
        if(pos<length&&line[pos]=='"'){quoted=true;++pos;}
        bool endedQuote=false;
        while(pos<length) {
            const char c=line[pos];
            if(quoted) {
                if(c=='"') {
                    if(pos+1<length&&line[pos+1]=='"'){
                        if(n+1>=40)return false;
                        out[cols][n++]='"';pos+=2;continue;
                    }
                    ++pos;endedQuote=true;break;
                }
            }else if(c==',')break;
            if(static_cast<unsigned char>(c)<32||n+1>=40)return false;
            out[cols][n++]=c;++pos;
        }
        if(quoted&&!endedQuote)return false;
        out[cols][n]=0;++cols;
        if(pos==length)break;
        if(line[pos]!=',')return false;
        ++pos;
    }
    return true;
}
}}
