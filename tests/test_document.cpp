#include "../src/core/TextDocument.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
int main() {
    suite::TextDocument d;
    assert(d.insert('a') && d.insert('b') && d.insert('c'));
    assert(d.left() && d.left() && d.insert('X'));
    assert(strcmp(d.data(),"aXbc")==0);
    assert(d.backspace() && strcmp(d.data(),"abc")==0);
    assert(d.eraseForward() && strcmp(d.data(),"ac")==0);
    d.toEnd(); assert(!d.eraseForward());
    d.toStart(); assert(!d.backspace());
    const char *sample="one\ntwelve\nxyz";
    assert(d.load(sample,strlen(sample)) && !d.changed());
    d.toStart();d.right();d.right();
    assert(d.down() && d.cursor()==6);
    assert(d.down() && d.cursor()==13);
    assert(d.up() && d.cursor()==6);
    assert(d.up() && d.cursor()==2);
    d.end();assert(d.cursor()==3);d.home();assert(d.cursor()==0);
    d.toEnd();size_t row=0,col=0;d.visualPosition(d.cursor(),5,row,col);
    assert(row==3 && col==3);
    const char *crlf="a\r\nb\rc\td";
    assert(d.load(crlf,strlen(crlf)) && strcmp(d.data(),"a\nb\nc d")==0);
    std::string old=d.data();
    assert(!d.load("a\0b",3) && strcmp(d.data(),old.c_str())==0);
    assert(!d.load("\xC3\xA9",2) && strcmp(d.data(),old.c_str())==0);
    std::string huge(suite::TextDocument::Capacity+1,'x');
    assert(!d.load(huge.data(),huge.size()));
    d.clear();for(size_t i=0;i<suite::TextDocument::Capacity;++i) assert(d.insert('z'));
    assert(!d.insert('z'));d.markSaved();assert(!d.changed());assert(d.backspace() && d.changed());
    puts("Document: PASS");
}
