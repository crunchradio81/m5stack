#pragma once
// P15 HTTPS text tools: strict URL parts, opt-in sites and bounded parsing.
// No Arduino or heap. CA validation remains mandatory in firmware.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
namespace suite {namespace web15 {
struct Config {
 char webHost[64]={},webPath[128]="/";
 char rssHost[64]={},rssPath[128]="/";
 char apiHost[64]={},apiPath[128]="/";
 char wikiTitle[49]="Cardputer";
 double latitude=0,longitude=0;
 bool location=false;
};
inline bool validHost(const char*s){
 if(!s)return false;
 const size_t n=strlen(s);
 if(!n||n>63||s[0]=='.'||s[n-1]=='.'||strstr(s,".."))return false;
 for(size_t i=0;i<n;++i){
   const char c=s[i];
   if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
        (c>='0'&&c<='9')||c=='.'||c=='-'))return false;
 }
 return true;
}
inline bool validPath(const char*s){
 if(!s||s[0]!='/'||strlen(s)>127)return false;
 for(const char*p=s;*p;++p){
   const unsigned char c=static_cast<unsigned char>(*p);
   if(c<33||c>126||c=='#'||c=='\\'||c=='"'||c=='<')return false;
 }
 return true;
}
inline bool wikiTitle(const char*s){
 if(!s)return false;
 const size_t n=strlen(s);
 if(!n||n>48)return false;
 for(const char*p=s;*p;++p){
   const char c=*p;
   if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
        (c>='0'&&c<='9')||c==' '||c=='-'||c=='_'||c=='('||c==')'))return false;
 }
 return true;
}
inline bool copy(char*dest,size_t cap,const char*s){
 if(!s||strlen(s)>=cap)return false;
 strcpy(dest,s);return true;
}
inline bool decimal(const char*value,double min,double max,double&out){
 if(!value||!*value||strlen(value)>16)return false;
 const char*p=value;
 if(*p=='+'||*p=='-')++p;
 unsigned digits=0;bool dot=false;
 for(;*p;++p){
   if(*p=='.'&&!dot){dot=true;continue;}
   if(*p<'0'||*p>'9')return false;
   if(++digits>11)return false;
 }
 if(!digits)return false;
 char*end=nullptr;const double d=strtod(value,&end);
 if(!end||*end||!isfinite(d)||d<min||d>max)return false;
 out=d;return true;
}
inline bool parse(const char*data,size_t n,Config&out){
 if(!data||n<9||n>1024)return false;
 Config next;unsigned seen=0;size_t pos=0;bool first=true;
 while(pos<n){
   const size_t start=pos;
   while(pos<n&&data[pos]!='\n'){
     const unsigned char c=static_cast<unsigned char>(data[pos]);
     if(c<32||c>126)return false;
     ++pos;
   }
   const size_t len=pos-start;
   if(pos==n||len>200)return false;
   ++pos;
   char line[201]={};memcpy(line,data+start,len);
   if(first){if(strcmp(line,"K132WEB1"))return false;first=false;continue;}
   if(!len)continue;
   char*sep=strchr(line,'=');
   if(!sep)return false;*sep++=0;
   bool ok=false;unsigned flag=0;
   if(!strcmp(line,"WEB_HOST")){flag=1;ok=copy(next.webHost,sizeof(next.webHost),sep)&&validHost(next.webHost);}
   else if(!strcmp(line,"WEB_PATH")){flag=2;ok=copy(next.webPath,sizeof(next.webPath),sep)&&validPath(next.webPath);}
   else if(!strcmp(line,"RSS_HOST")){flag=4;ok=copy(next.rssHost,sizeof(next.rssHost),sep)&&validHost(next.rssHost);}
   else if(!strcmp(line,"RSS_PATH")){flag=8;ok=copy(next.rssPath,sizeof(next.rssPath),sep)&&validPath(next.rssPath);}
   else if(!strcmp(line,"API_HOST")){flag=16;ok=copy(next.apiHost,sizeof(next.apiHost),sep)&&validHost(next.apiHost);}
   else if(!strcmp(line,"API_PATH")){flag=32;ok=copy(next.apiPath,sizeof(next.apiPath),sep)&&validPath(next.apiPath);}
   else if(!strcmp(line,"WIKI_TITLE")){flag=64;ok=copy(next.wikiTitle,sizeof(next.wikiTitle),sep)&&wikiTitle(next.wikiTitle);}
   else if(!strcmp(line,"LATITUDE")){flag=128;ok=decimal(sep,-90,90,next.latitude);}
   else if(!strcmp(line,"LONGITUDE")){flag=256;ok=decimal(sep,-180,180,next.longitude);}
   else return false;
   if(!ok||seen&flag)return false;
   seen|=flag;
 }
 if(((seen&128)!=0)!=((seen&256)!=0))return false;
 next.location=(seen&128)!=0;
 out=next;return true;
}
inline bool wikiPath(const char*title,char*out,size_t cap){
 if(!wikiTitle(title)||!out)return false;
 static const char base[]="/w/api.php?action=query&prop=extracts&explaintext=1&exintro=1&format=json&titles=";
 const size_t n=strlen(base),len=strlen(title);
 if(n+len*3+1>cap)return false;
 memcpy(out,base,n);size_t p=n;
 for(size_t i=0;i<len;++i){
   const char c=title[i];
   if(c==' '){out[p++]='%';out[p++]='2';out[p++]='0';}
   else out[p++]=c;
 }
 out[p]=0;return true;
}
inline size_t asciiClean(const char*in,size_t n,char*out,size_t cap){
 if(!in||!out||!cap)return 0;
 size_t p=0;
 for(size_t i=0;i<n&&p<cap-1;++i){
   const unsigned char c=static_cast<unsigned char>(in[i]);
   if(c=='\r')continue;
   out[p++]=(c>=32&&c<=126)||c=='\n'?'?':'?';
   if((c>=32&&c<=126)||c=='\n')out[p-1]=static_cast<char>(c);
 }
 out[p]=0;return p;
}
inline size_t decodeEntities(const char*src,size_t n,char*out,size_t cap){
 if(!src||!out||!cap)return 0;
 size_t p=0;
 for(size_t i=0;i<n&&p<cap-1;++i){
   if(src[i]=='&'){
     struct Entity {const char*e;char c;};
     static const Entity items[]={
       {"amp;",'&'},{"lt;",'<'},{"gt;",'>'},{"quot;",'"'},
       {"apos;",'\''},{"#39;",'\''}
     };
     bool decoded=false;
     for(const auto&e:items){
       const size_t length=strlen(e.e);
       if(i+1+length<=n&&!memcmp(src+i+1,e.e,length)){
         out[p++]=e.c;i+=length;decoded=true;break;
       }
     }
     if(decoded)continue;
   }
   const unsigned char c=static_cast<unsigned char>(src[i]);
   if((c>=32&&c<=126)||c=='\n')out[p++]=static_cast<char>(c);
   else out[p++]='?';
 }
 out[p]=0;return p;
}
inline bool firstRssTitle(const char*xml,char*out,size_t cap){
 if(!xml||!out||!cap)return false;
 const char*item=strstr(xml,"<item");
 if(!item)item=strstr(xml,"<entry");
 if(!item)return false;
 const char*start=strstr(item,"<title");
 if(!start||!(start=strchr(start,'>')))return false;
 ++start;const char*end=strstr(start,"</title>");
 if(!end||end-start>400)return false;
 return decodeEntities(start,static_cast<size_t>(end-start),out,cap)>0;
}
inline bool htmlText(const char*html,char*out,size_t cap){
 if(!html||!out||!cap)return false;
 bool inTag=false;size_t p=0;
 for(const char*s=html;*s&&p+1<cap;++s){
   if(*s=='<'){inTag=true;continue;}
   if(*s=='>'){inTag=false;out[p++]=' ';continue;}
   if(inTag)continue;
   const unsigned char c=static_cast<unsigned char>(*s);
   if((c>=32&&c<=126)||c=='\n')out[p++]=static_cast<char>(c);
 }
 out[p]=0;
 // Decode only built-in safe entities, no script execution or markup.
 char temp[1025]={};
 const size_t n=p<1024?p:1024;
 memcpy(temp,out,n);temp[n]=0;
 decodeEntities(temp,n,out,cap);
 return out[0]!=0;
}
inline bool jsonString(const char*json,const char*field,
                       char*out,size_t cap){
 if(!json||!field||!out||cap<2||strlen(field)>50)return false;
 char token[56]={};
 strcpy(token,"\"");strcat(token,field);strcat(token,"\"");
 const char*p=strstr(json,token);
 if(!p)return false;
 p+=strlen(token);
 while(*p==' ')++p;
 if(*p++!=':')return false;
 while(*p==' ')++p;
 if(*p++!='"')return false;
 size_t n=0;
 for(;*p&&n+1<cap;++p){
   if(*p=='"'){out[n]=0;return true;}
   if(*p=='\\'){
     ++p;if(!*p)return false;
     if(*p=='n')out[n++]=' ';
     else if(*p=='"'||*p=='\\'||*p=='/')out[n++]=*p;
     else return false; // no JSON unicode decoding claim
   }else{
     const unsigned char c=static_cast<unsigned char>(*p);
     out[n++]=(c>=32&&c<=126)?static_cast<char>(c):'?';
   }
 }
 return false;
}
inline bool jsonNumber(const char*json,const char*field,double&out){
 if(!json||!field||strlen(field)>50)return false;
 char token[56]={};strcpy(token,"\"");strcat(token,field);strcat(token,"\"");
 const char*p=strstr(json,token);if(!p)return false;
 p+=strlen(token);while(*p==' ')++p;
 if(*p++!=':')return false;
 while(*p==' ')++p;
 char digits[24]={};size_t n=0;
 if(*p=='-')digits[n++]=*p++;
 bool dot=false;
 while(*p&&n<sizeof(digits)-1){
   if(*p=='.'&&!dot){digits[n++]=*p++;dot=true;continue;}
   if(*p<'0'||*p>'9')break;
   digits[n++]=*p++;
 }
 if(!n||n==1&&digits[0]=='-')return false;
 digits[n]=0;char*end=nullptr;
 const double v=strtod(digits,&end);
 if(!end||*end||!isfinite(v))return false;
 out=v;return true;
}
}} // suite::web15
