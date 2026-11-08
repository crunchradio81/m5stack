#include "../src/core/Rss21Core.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <string>
using namespace suite::rss21;
int main(){
 Feed f;
 const char* rss="<rss><channel><title>Channel title</title>"
 "<item><title>First &amp; Second</title><description>skip</description></item>"
 "<item><title><![CDATA[Third <b>Signal</b>]]></title></item>"
 "</channel></rss>";
 assert(parse(rss,strlen(rss),f));
 assert(f.count==2&&!strcmp(f.titles[0],"First & Second"));
 assert(!strcmp(f.titles[1],"Third Signal"));
 const char*atom="<feed><entry><title type=\"html\">Hello &lt;World&gt;</title></entry>"
 "<entry><title>Night &quot;Radio&quot;</title></entry></feed>";
 assert(parse(atom,strlen(atom),f) && f.count==2);
 assert(!strcmp(f.titles[0],"Hello <World>"));
 assert(!strcmp(f.titles[1],"Night \"Radio\""));
 const char*bad[]={"","<rss><channel><title>not an item</title></channel></rss>",
                   "<rss><item><title>truncated"};
 for(const char*p:bad){
  Feed prior=f;
  assert(!parse(p,strlen(p),f));
  assert(f.count==prior.count);
 }
 assert(!parse(atom,MaxResponse+1,f));
 std::string many="<rss>";
 for(int i=0;i<12;++i)many+="<item><title>Item "+std::to_string(i)+"</title></item>";
 many+="</rss>";
 assert(parse(many.c_str(),many.size(),f) && f.count==MaxItems && f.capped);
 assert(decodeTitle("  A&B  ","  A&B  "+7,f.titles[0],MaxTitle));
 puts("P21 RSS: bounded RSS2/Atom titles, entities, CDATA, malformed and overflow PASS");
}
