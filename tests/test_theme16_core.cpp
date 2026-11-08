#include "../src/core/Theme16Core.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace suite::skin16;
int main(){
 Settings original;
 assert(original.activeIndex()==0);
 assert(strcmp(original.active().name,"DEFAULT")==0);
 assert(original.at(1).theme==1 && original.at(2).theme==5);
 assert(!original.choose(3));
 assert(original.choose(1)&&original.active().brightness==1);
 assert(original.theme(3)&&original.brightness(3)&&original.volume(100));
 assert(!original.theme(6)&&!original.brightness(4)&&!original.volume(101));
 original.autosave(false);
 assert(original.rename(1,"MUSIC_LAB")&&!original.rename(2,"MUSIC_LAB"));
 assert(!original.rename(1,"../BAD")&&!original.rename(1,"")&&
        !original.rename(0,"XXXXXXXXXXXXXXXXX"));
 char bytes[Settings::MaxBytes+1]={};
 const size_t n=original.encode(bytes,sizeof(bytes));
 assert(n>80&&n<Settings::MaxBytes&&strstr(bytes,"HASH="));
 Settings copied;
 assert(copied.decode(bytes,n));
 assert(copied.activeIndex()==1);
 assert(strcmp(copied.active().name,"MUSIC_LAB")==0);
 assert(copied.active().theme==3);
 assert(copied.active().brightness==3);
 assert(copied.active().volume==100&&!copied.active().autosave);
 char damaged[Settings::MaxBytes+1]={};
 memcpy(damaged,bytes,n);
 char*needle=strstr(damaged,"MUSIC_LAB");
 assert(needle);needle[0]='X';
 const unsigned before=copied.activeIndex();
 assert(!copied.decode(damaged,n)&&copied.activeIndex()==before);
 assert(!copied.decode(bytes,n-1));
 assert(!copied.decode(bytes,Settings::MaxBytes+1));
 assert(!original.encode(bytes,50));
 assert(original.choose(0)&&original.rename(0,"Desk"));
 assert(original.rename(1,"Night")&&original.rename(2,"Contrast"));
 assert(original.theme(5));
 assert(original.encode(bytes,sizeof(bytes)));
 assert(copied.decode(bytes,strlen(bytes)));
 for(unsigned i=0;i<6;++i){
   const Palette p=palette(i);
   assert(p.bg!=p.text);
   assert(p.selected!=p.bg);
   assert(strlen(themeName(i))>=4);
 }
 // Strict parser must reject intact-checksum duplicate names and invalid
 // numeric choices, not just an altered digest.
 char forged[Settings::MaxBytes+1]={};
 const char*prefix=
  "K132PREF2\nACTIVE=1\n"
  "P0=Same|0|2|60|1\n"
  "P1=Same|1|1|40|1\n"
  "P2=Third|5|3|75|0\n";
 const size_t pLen=strlen(prefix);
 const uint32_t hash=Settings::checksum(prefix,pLen);
 const int suffix=snprintf(forged+pLen,sizeof(forged)-pLen,"HASH=%08lX\n",
                             static_cast<unsigned long>(hash));
 assert(suffix==14);
 assert(!copied.decode(forged,pLen+static_cast<size_t>(suffix)));
 const char*badValue=
  "K132PREF2\nACTIVE=0\n"
  "P0=Good|6|2|60|1\n"
  "P1=Second|1|1|40|1\n"
  "P2=Third|5|3|75|0\n";
 const size_t badN=strlen(badValue);
 snprintf(forged,sizeof(forged),"%sHASH=%08lX\n",badValue,
     static_cast<unsigned long>(Settings::checksum(badValue,badN)));
 assert(!copied.decode(forged,strlen(forged)));
 assert(!Settings::validName("A|B"));
 assert(Settings::validName("Night Mode"));
 puts("P16 Theme/Profiles: presets, 3 editable profiles, checksum, atomic decode and corruption rejection PASS");
}
