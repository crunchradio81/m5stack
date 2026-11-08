#include "../src/core/Web15Core.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace suite::web15;
static void reject(const char*text){
 Config out;assert(!parse(text,strlen(text),out));
}
int main(){
 const char*config=
  "K132WEB1\nWEB_HOST=example.org\nWEB_PATH=/index.html\n"
  "RSS_HOST=feeds.example.org\nRSS_PATH=/rss.xml\n"
  "API_HOST=api.example.org\nAPI_PATH=/v1/status\n"
  "LATITUDE=33.4484\nLONGITUDE=-112.0740\nWIKI_TITLE=Computer history\n";
 Config c;assert(parse(config,strlen(config),c));
 assert(strcmp(c.webHost,"example.org")==0&&c.location);
 assert(c.latitude>33&&c.longitude< -112);
 reject("K132WEB1\nWEB_HOST=abc/def\n");
 reject("K132WEB1\nWEB_PATH=/index\nWEB_PATH=/next\n");
 reject("K132WEB1\nWEB_HOST=abc..def\n");
 reject("K132WEB1\nWEB_HOST=example.com\nLATITUDE=32.1\n");
 reject("K132WEB1\nLONGITUDE=-180.1\nLATITUDE=33\n");
 reject("K132WEB1\nLATITUDE=91\nLONGITUDE=100\n");
 reject("K132WEB1\nWEB_PATH=http://injection\n");
 reject("K132WEB1\nWEB_PATH=/a#fragment\n");
 reject("K132WEB1\nWIKI_TITLE=hi&evil=1\n");
 reject("K132WEB1\nUNKNOWN=1\n");
 assert(validHost("en.wikipedia.org")&&!validHost("https://example.org"));
 assert(validPath("/api?q=hi")&&!validPath("/bad\r\nHeader:evil"));
 char titlePath[220]={};
 assert(wikiPath("Computer history",titlePath,sizeof(titlePath)));
 assert(strstr(titlePath,"Computer%20history"));
 assert(!wikiPath("Hi & inject",titlePath,sizeof(titlePath)));
 const char*feed="<rss><channel><title>Channel</title><item><title>A &amp; B</title></item></channel></rss>";
 char text[256]={};
 assert(firstRssTitle(feed,text,sizeof(text)));
 assert(strcmp(text,"A & B")==0);
 assert(!firstRssTitle("<rss><channel><title>None</title></channel></rss>",text,sizeof(text)));
 assert(htmlText("<html><body>Hello <b>World</b> &lt;safe&gt;</body></html>",
                 text,sizeof(text)));
 assert(strstr(text,"Hello")&&strstr(text,"<safe>"));
 const char*js="{\"pages\":{\"1\":{\"extract\":\"An early computer.\",\"id\":12}},\"temp\":23.75}";
 assert(jsonString(js,"extract",text,sizeof(text)));
 assert(strcmp(text,"An early computer.")==0);
 double n=0;
 assert(jsonNumber(js,"temp",n)&&n>23&&n<24);
 assert(!jsonNumber(js,"missing",n));
 assert(!jsonString(js,"bad",text,sizeof(text)));
 puts("P15 Secure Web Core: URL config, RSS, HTML, JSON text & injection guards PASS");
}
