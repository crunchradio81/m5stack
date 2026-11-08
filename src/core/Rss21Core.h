#pragma once
// P21: bounded, read-only RSS2/Atom title extraction for trusted TLS input.
// No heap, no HTML execution, no network connection, no arbitrary data copies.
#include <stddef.h>
#include <stdint.h>
#include <string.h>
namespace suite {namespace rss21 {
constexpr size_t MaxResponse=3072, MaxTitle=80;
constexpr unsigned MaxItems=8;
struct Feed {
 char titles[MaxItems][MaxTitle]={};
 unsigned count=0;
 bool capped=false;
};
inline bool asciiLetter(char c){
 return (c>='a'&&c<='z')||(c>='A'&&c<='Z');
}
inline bool tagBoundary(char c){return c=='>'||c==' '||c=='\t'||c=='\r'||c=='\n';}
inline const char*locate(const char*p,const char*end,const char*tag){
 const size_t n=strlen(tag);
 while(p && p+n<=end){
  const char*where=static_cast<const char*>(memchr(p,'<',static_cast<size_t>(end-p)));
  if(!where)return nullptr;
  if(where+n<=end&&memcmp(where,tag,n)==0&&
     (where+n==end||tagBoundary(where[n])))return where;
  p=where+1;
 }
 return nullptr;
}
inline const char*literal(const char*begin,const char*end,const char*needle){
 const size_t n=strlen(needle);
 for(const char*p=begin;p+n<=end;++p)if(!memcmp(p,needle,n))return p;
 return nullptr;
}
inline bool append(char*out,size_t cap,size_t&used,char c){
 if(used+1>=cap)return false;
 out[used++]=c;out[used]=0;return true;
}
inline bool decodeTitle(const char*begin,const char*end,char*out,size_t cap){
 if(!out||cap<2||begin>=end)return false;
 out[0]=0;
 // Remove optional CDATA wrapper before interpreting entities.
 const char*cd=literal(begin,end,"<![CDATA[");
 if(cd==begin){
  const char*close=literal(begin+9,end,"]]>");
  if(close){begin+=9;end=close;}
 }
 size_t used=0;bool inTag=false;
 for(const char*p=begin;p<end;++p){
  unsigned char c=static_cast<unsigned char>(*p);
  if(c=='<'){inTag=true;continue;}
  if(c=='>'&&inTag){inTag=false;continue;}
  if(inTag)continue; // never display any embedded markup
  if(c=='&'){
   const char*term=static_cast<const char*>(memchr(p,';',static_cast<size_t>(end-p)));
   if(term&&term-p<=10){
    const size_t len=static_cast<size_t>(term-p);
    char substitute=0;
    if(len==4&&!memcmp(p,"&amp",4))substitute='&';
    if(len==3&&!memcmp(p,"&lt",3))substitute='<';
    if(len==3&&!memcmp(p,"&gt",3))substitute='>';
    if(len==5&&!memcmp(p,"&quot",5))substitute='"';
    if(len==5&&!memcmp(p,"&apos",5))substitute='\'';
    if(substitute){c=static_cast<unsigned char>(substitute);p=term;}
   }
  }
  if(c=='\r'||c=='\n'||c=='\t')c=' ';
  if(c<32||c>126)c='?';
  if(!append(out,cap,used,static_cast<char>(c)))break;
 }
 while(used&&out[used-1]==' ')out[--used]=0;
 return used>0;
}
inline bool parse(const char*data,size_t len,Feed&destination){
 if(!data||!len||len>MaxResponse||memchr(data,0,len)!=nullptr)return false;
 const char*end=data+len;
 Feed result;
 const char*position=data;
 // Restrict processing to real <item> / <entry> envelopes, not channel title.
 while(position<end){
  const char*item=locate(position,end,"<item");
  const char*entry=locate(position,end,"<entry");
  const bool atom=(!item&&entry)||(entry&&item&&entry<item);
  const char*start=atom?entry:item;
  if(!start)break;
  const char*open=static_cast<const char*>(memchr(start,'>',static_cast<size_t>(end-start)));
  if(!open)break;
  ++open;
  const char*closing=literal(open,end,atom?"</entry>":"</item>");
  if(!closing){result.capped=true;break;}
  const char*title=locate(open,closing,"<title");
  if(title){
   const char*openTitle=static_cast<const char*>(memchr(title,'>',static_cast<size_t>(closing-title)));
   if(openTitle){
    ++openTitle;
    const char*endTitle=literal(openTitle,closing,"</title>");
    if(endTitle){
     char decoded[MaxTitle]={};
     if(decodeTitle(openTitle,endTitle,decoded,sizeof(decoded))){
      if(result.count==MaxItems){result.capped=true;break;}
      strcpy(result.titles[result.count++],decoded);
     }
    }
   }
  }
  position=closing+(atom?8:7);
 }
 if(!result.count)return false;
 destination=result;return true;
}
}} // namespace suite::rss21
