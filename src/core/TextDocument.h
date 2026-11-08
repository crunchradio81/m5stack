#pragma once
// Fixed-capacity ASCII editor logic. Host-testable; no String/Arduino dependency.
#include <stddef.h>
#include <string.h>
namespace suite {
class TextDocument {
public:
    static constexpr size_t Capacity=4096;
    TextDocument() { clear(); }
    void clear() { length_=cursor_=column_=0; text_[0]=0; changed_=false; }
    size_t length() const { return length_; }
    size_t cursor() const { return cursor_; }
    const char *data() const { return text_; }
    bool changed() const { return changed_; }
    void markSaved() { changed_=false; }
    // Fail without replacing current document; normalize CRLF/CR and tab.
    bool load(const char *source,size_t length) {
        if (!source || length>Capacity) return false;
        for(size_t i=0;i<length;++i) {
            const unsigned char c=static_cast<unsigned char>(source[i]);
            if(c!='\r' && c!='\n' && c!='\t' && (c<32 || c>126)) return false;
        }
        size_t out=0;
        for(size_t i=0;i<length;++i) {
            char c=source[i];
            if(c=='\r') { c='\n'; if(i+1<length && source[i+1]=='\n') ++i; }
            if(c=='\t') c=' ';
            text_[out++]=c;
        }
        length_=out; cursor_=column_=0; text_[out]=0; changed_=false;
        return true;
    }
    bool insert(char c) {
        if(length_>=Capacity || !(c=='\n' || (c>=32 && c<=126))) return false;
        memmove(text_+cursor_+1,text_+cursor_,length_-cursor_+1);
        text_[cursor_++]=c; ++length_; changed_=true; sync(); return true;
    }
    bool backspace() {
        if(!cursor_) return false;
        memmove(text_+cursor_-1,text_+cursor_,length_-cursor_+1);
        --cursor_;--length_;changed_=true;sync();return true;
    }
    bool eraseForward() {
        if(cursor_==length_) return false;
        memmove(text_+cursor_,text_+cursor_+1,length_-cursor_);
        --length_;changed_=true;sync();return true;
    }
    bool left() { if(!cursor_) return false; --cursor_;sync();return true; }
    bool right() { if(cursor_==length_) return false; ++cursor_;sync();return true; }
    void home() { cursor_=lineStart(cursor_);sync(); }
    void end() { cursor_=lineEnd(cursor_);sync(); }
    void toStart() { cursor_=0;sync(); }
    void toEnd() { cursor_=length_;sync(); }
    bool up() {
        const size_t start=lineStart(cursor_);
        if(!start) return false;
        const size_t previous=lineStart(start-1);
        const size_t end=lineEnd(previous);
        cursor_=previous+min(column_,end-previous);return true;
    }
    bool down() {
        const size_t next=lineEnd(cursor_);
        if(next==length_) return false;
        const size_t start=next+1;
        cursor_=start+min(column_,lineEnd(start)-start);return true;
    }
    void visualPosition(size_t position,size_t cols,size_t &row,size_t &column) const {
        row=column=0; if(!cols) return;
        if(position>length_) position=length_;
        for(size_t i=0;i<position;++i) {
            if(text_[i]=='\n') { ++row; column=0; }
            else if(++column==cols) { ++row;column=0; }
        }
    }
private:
    char text_[Capacity+1];
    size_t length_=0,cursor_=0,column_=0;
    bool changed_=false;
    static size_t min(size_t a,size_t b) { return a<b?a:b; }
    size_t lineStart(size_t p) const {
        while(p && text_[p-1]!='\n') --p;
        return p;
    }
    size_t lineEnd(size_t p) const {
        while(p<length_ && text_[p]!='\n') ++p;
        return p;
    }
    void sync() { column_=cursor_-lineStart(cursor_); }
};
}
