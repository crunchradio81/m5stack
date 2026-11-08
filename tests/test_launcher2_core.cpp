#include "../src/core/Launcher2Core.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace suite::launch2;
int main(){
  using suite::appmanifest::App;
  assert(category("METRONOME")==Category::Music);
  assert(category("NETTOOLS")==Category::Wireless);
  assert(category("ARCADE2")==Category::Games);
  assert(category("POCKET11")==Category::Office);
  assert(category("IRTOOLS")==Category::Infrared);
  assert(includes("MIDI Deep Tools","deep"));
  assert(includes("WIFI Survey","wifi"));
  assert(!includes("ARCADE","midi"));
  Preferences p;
  assert(!p.favorite("ARCADE2"));
  assert(p.toggle("ARCADE2")&&p.favorite("ARCADE2"));
  assert(p.toggle("NETTOOLS")&&p.favorite("NETTOOLS"));
  assert(!p.toggle("../INJECT"));
  assert(p.remember("ARCADE2")&&p.remember("NETTOOLS"));
  assert(p.remember("ARCADE2")&&p.recentCount()==2);
  assert(strcmp(p.recentAt(0),"ARCADE2")==0);
  char text[512]={};
  const size_t n=p.encode(text,sizeof(text));
  assert(n>10&&strstr(text,"F=ARCADE2\n")&&strstr(text,"R=NETTOOLS\n"));
  Preferences recovered;
  assert(recovered.parse(text,n)&&recovered.favorite("NETTOOLS")&&
         recovered.recent("ARCADE2"));
  assert(!recovered.parse("K132MENU1\nF=../BAD\n",20));
  assert(!recovered.parse("K132MENU1\nF=ARCADE2\nF=ARCADE2\n",30));
  assert(recovered.favoriteCount()==2);
  App apps[5]={};
  strcpy(apps[0].id,"ARCADE2");strcpy(apps[0].name,"Expanded Arcade");
  strcpy(apps[1].id,"MIDITOOLS");strcpy(apps[1].name,"MIDI Deep Tools");
  strcpy(apps[2].id,"NETTOOLS");strcpy(apps[2].name,"Network and Internet");
  strcpy(apps[3].id,"POCKET11");strcpy(apps[3].name,"Pocket Productivity");
  strcpy(apps[4].id,"IRTOOLS");strcpy(apps[4].name,"Infrared Remote Tools");
  uint8_t order[8]={};
  assert(filter(apps,5,Category::All,p,"",order,8)==5);
  assert(filter(apps,5,Category::Music,p,"",order,8)==1&&order[0]==1);
  assert(filter(apps,5,Category::Games,p,"",order,8)==1&&order[0]==0);
  assert(filter(apps,5,Category::Favorites,p,"",order,8)==2);
  assert(filter(apps,5,Category::Recent,p,"",order,8)==2);
  assert(order[0]==0&&order[1]==2);
  assert(filter(apps,5,Category::All,p,"pocket",order,8)==1&&order[0]==3);
  assert(filter(apps,5,Category::All,p,"NOTHING",order,8)==0);
  puts("Launcher2: categories, favorites, recency, search and malformed config PASS");
}
