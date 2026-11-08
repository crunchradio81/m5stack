#pragma once
// Launcher 2.0: bounded, host-testable SD categories/search/favorites/recent.
// Physical original K132 240x135 acceptance still required.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "SDAppManifest.h"
namespace suite {namespace launch2 {
enum class Category:uint8_t {
  All,Music,Wireless,Games,Office,Tools,Infrared,Favorites,Recent,Count
};
inline const char*categoryName(Category c){
  switch(c){
    case Category::All:return "ALL";
    case Category::Music:return "MUSIC";
    case Category::Wireless:return "WIRELESS";
    case Category::Games:return "GAMES";
    case Category::Office:return "OFFICE";
    case Category::Tools:return "TOOLS";
    case Category::Infrared:return "INFRARED";
    case Category::Favorites:return "FAVORITES";
    case Category::Recent:return "RECENT";
    default:return "UNKNOWN";
  }
}
inline Category category(const char*id){
  if(!strcmp(id,"METRONOME")||!strcmp(id,"PIANO")||
     !strcmp(id,"MUSICLAB")||!strcmp(id,"MIDITOOLS")||
     !strcmp(id,"AUDIOLAB"))return Category::Music;
  if(!strcmp(id,"BLE")||!strcmp(id,"CHANNELS")||
     !strcmp(id,"WIRELESS")||!strcmp(id,"FIELD2")||!strcmp(id,"NETTOOLS"))
    return Category::Wireless;
  if(!strcmp(id,"ARCADE")||!strcmp(id,"ARCADE2")||!strcmp(id,"ARCADE3")||!strcmp(id,"GAMEBOY")||!strcmp(id,"C64"))return Category::Games;
  if(!strcmp(id,"OFFICE")||!strcmp(id,"POCKET11")||
     !strcmp(id,"NOTEBOOK"))return Category::Office;
  if(!strcmp(id,"IRTOOLS"))return Category::Infrared;
  return Category::Tools;
}
inline bool includes(const char*haystack,const char*needle){
  if(!haystack||!needle)return false;
  if(!*needle)return true;
  for(const char*p=haystack;*p;++p){
    const char*a=p,*b=needle;
    while(*a&&*b){
      char x=*a,y=*b;
      if(x>='A'&&x<='Z')x=static_cast<char>(x+32);
      if(y>='A'&&y<='Z')y=static_cast<char>(y+32);
      if(x!=y)break;
      ++a;++b;
    }
    if(!*b)return true;
  }
  return false;
}
class Preferences {
public:
  static constexpr int MaxFav=16,MaxRecent=4,MaxBytes=512;
  Preferences(){clear();}
  void clear(){
    nf_=nr_=0;
    memset(fav_,0,sizeof(fav_));
    memset(recent_,0,sizeof(recent_));
  }
  bool favorite(const char*id)const{
    for(int i=0;i<nf_;++i)if(!strcmp(fav_[i],id))return true;
    return false;
  }
  bool recent(const char*id)const{
    for(int i=0;i<nr_;++i)if(!strcmp(recent_[i],id))return true;
    return false;
  }
  bool toggle(const char*id){
    if(!appmanifest::authorizedId(id))return false;
    for(int i=0;i<nf_;++i)if(!strcmp(fav_[i],id)){
      for(int j=i;j+1<nf_;++j)strcpy(fav_[j],fav_[j+1]);
      --nf_;fav_[nf_][0]=0;return true;
    }
    if(nf_>=MaxFav)return false;
    strcpy(fav_[nf_++],id);return true;
  }
  bool remember(const char*id){
    if(!appmanifest::authorizedId(id))return false;
    char prev[MaxRecent][12]={};int count=0;
    strcpy(prev[count++],id);
    for(int i=0;i<nr_&&count<MaxRecent;++i){
      if(strcmp(recent_[i],id))strcpy(prev[count++],recent_[i]);
    }
    memset(recent_,0,sizeof(recent_));
    for(int i=0;i<count;++i)strcpy(recent_[i],prev[i]);
    nr_=count;return true;
  }
  unsigned favoriteCount()const{return nf_;}
  unsigned recentCount()const{return nr_;}
  const char*recentAt(unsigned i)const{return i<static_cast<unsigned>(nr_)?recent_[i]:"";}
  bool parse(const char*data,size_t len){
    static const char head[]="K132MENU1\n";
    if(!data||len<sizeof(head)-1||len>MaxBytes||
       memcmp(data,head,sizeof(head)-1))return false;
    Preferences next;size_t p=sizeof(head)-1;
    while(p<len){
      const size_t start=p;
      while(p<len&&data[p]!='\n')++p;
      if(p==len||p-start<3||p-start>14)return false;
      const size_t n=p-start;++p;
      if(data[start+1]!='=')return false;
      char id[12]={};
      if(n-2>=sizeof(id))return false;
      memcpy(id,data+start+2,n-2);
      if(!appmanifest::authorizedId(id))return false;
      if(data[start]=='F'){
        if(next.favorite(id)||!next.toggle(id))return false;
      }else if(data[start]=='R'){
        if(next.recent(id)||next.nr_>=MaxRecent)return false;
        strcpy(next.recent_[next.nr_++],id);
      }else return false;
    }
    *this=next;return true;
  }
  size_t encode(char*out,size_t cap)const{
    static const char hdr[]="K132MENU1\n";
    if(!out||cap<sizeof(hdr))return 0;
    size_t p=sizeof(hdr)-1;memcpy(out,hdr,p);
    for(int kind=0;kind<2;++kind){
      const int count=kind==0?nf_:nr_;
      for(int i=0;i<count;++i){
        const char*id=kind==0?fav_[i]:recent_[i];
        const size_t n=strlen(id);
        if(p+n+3>=cap)return 0;
        out[p++]=kind==0?'F':'R';out[p++]='=';
        memcpy(out+p,id,n);p+=n;out[p++]='\n';
      }
    }
    out[p]=0;return p;
  }
private:
  char fav_[MaxFav][12]={},recent_[MaxRecent][12]={};
  int nf_=0,nr_=0;
};
inline int filter(const appmanifest::App*apps,int count,Category group,
                  const Preferences&prefs,const char*query,uint8_t*output,int cap) {
  if(!apps||!output||!query||count<0||count>appmanifest::MaxApps||
     cap<0||cap>appmanifest::MaxApps)return 0;
  int found=0;
  for(int i=0;i<count&&found<cap;++i){
    const auto&a=apps[i];
    if(!appmanifest::authorizedId(a.id))continue;
    if(group==Category::Favorites&&!prefs.favorite(a.id))continue;
    if(group==Category::Recent&&!prefs.recent(a.id))continue;
    if(group!=Category::All&&group!=Category::Favorites&&
       group!=Category::Recent&&group!=category(a.id))continue;
    if(!includes(a.name,query)&&!includes(a.id,query))continue;
    output[found++]=static_cast<uint8_t>(i);
  }
  // Recent category sorts by launch order; others preserve alphabetical list.
  if(group==Category::Recent)for(int i=0;i<found;++i)
    for(int j=i+1;j<found;++j){
      auto rank=[&prefs,&apps,&output](int k)->int{
        for(int r=0;r<static_cast<int>(prefs.recentCount());++r)
          if(!strcmp(apps[output[k]].id,prefs.recentAt(r)))return r;
        return 100;
      };
      if(rank(j)<rank(i)){
        const uint8_t t=output[i];output[i]=output[j];output[j]=t;
      }
    }
  return found;
}
}} // namespace suite::launch2
