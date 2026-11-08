#include "../src/core/SafeFiles17Core.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace suite::safefiles17;
int main(){
 assert(safeName("photo.jpg")&&safeName("DATA_2026.CSV"));
 assert(safeName("README")&&safeName("Alice-Sketch_2.JSON"));
 assert(!safeName("../boot")&&!safeName("x/../test.TXT"));
 assert(!safeName(".hidden")&&!safeName("a..txt"));
 assert(!safeName("A\\B")&&!safeName("X:T")&&!safeName("X\nY"));
 assert(!safeName("CON.txt")&&!safeName("lpt1")&&!safeName("PRN.DAT"));
 assert(!safeName("1234567890123456789012345.TXT"));
 assert(!safeName("x.123456789"));
 char newName[MaxName]={};
 assert(restored("MY-OWNER-DATA_01.txt",1,newName,sizeof(newName)));
 assert(strcmp(newName,"R1-MY-OWNER-DATA_01.txt")==0);
 assert(safeName(newName));
 assert(restored("ABCDEFGHIJKLMNOPQRSTUVWX.ABCDEFGH",9,newName,sizeof(newName)));
 assert(safeName(newName)&&strlen(newName)<MaxName);
 assert(!restored("../boot",1,newName,sizeof(newName)));
 assert(!restored("ABC.TXT",0,newName,sizeof(newName)));
 assert(!restored("ABC.TXT",10,newName,sizeof(newName)));
 assert(!restored("ABC.TXT",1,newName,5));
 Index index;
 assert(index.add("Z-last.bin",100));
 assert(index.add("A-FIRST.txt",0));
 assert(index.add("Mid-02.csv",MaxFileSize));
 assert(index.count()==3);
 assert(strcmp(index.at(0).name,"A-FIRST.txt")==0);
 assert(strcmp(index.at(2).name,"Z-last.bin")==0);
 assert(index.find("a-first.TXT")==0);
 assert(!index.add("a-first.txt",1));
 assert(!index.add("tooLarge.dat",MaxFileSize+1));
 uint8_t src[]={0,1,2,3,0xff},dst[]={0,1,2,3,0xff};
 assert(equalBlock(src,dst,sizeof(src)));
 dst[4]=1;
 assert(!equalBlock(src,dst,sizeof(src)));
 assert(digest(src,sizeof(src))!=digest(dst,sizeof(dst)));
 assert(!equalBlock(nullptr,dst,sizeof(dst)));
 puts("P17 SafeFiles: path rejection, restore naming, case-insensitive indexing and byte comparison PASS");
}
