#pragma once
// Offline fixed-memory games for original no-PSRAM Cardputer K132.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <initializer_list>
namespace suite { namespace arcade10 {
class RNG {
public:
 explicit RNG(uint32_t seed=0x41C6CE57):state_(seed?seed:1){}
 uint32_t next(){uint32_t s=state_;s^=s<<13;s^=s>>17;s^=s<<5;return state_=s;}
private:uint32_t state_;
};
class FallingBlocks {
public:
 static constexpr int W=10,H=16;
 explicit FallingBlocks(uint32_t seed=41):rng_(seed){reset();}
 void reset(){
   memset(board_,0,sizeof(board_));score_=lines_=0;over_=false;spawn();
 }
 uint8_t at(int x,int y)const {return x>=0&&x<W&&y>=0&&y<H?board_[y][x]:0;}
 uint32_t score()const{return score_;}
 uint32_t lines()const{return lines_;}
 bool over()const{return over_;}
 int pieceX()const{return px_;}int pieceY()const{return py_;}
 bool cell(int x,int y)const{
   if(x<0||x>3||y<0||y>3)return false;
   int sx=x,sy=y;
   if(rot_==1){sx=y;sy=3-x;}
   if(rot_==2){sx=3-x;sy=3-y;}
   if(rot_==3){sx=3-y;sy=x;}
   return (shape(type_)&(static_cast<uint16_t>(1u)<<(sy*4+sx)))!=0;
 }
 uint8_t pieceType()const{return type_+1;}
 bool shift(int dx){
   if(over_||collide(px_+dx,py_,rot_))return false;
   px_+=dx;return true;
 }
 bool rotate(){
   if(over_)return false;
   const int next=(rot_+1)%4;
   for(int dx:{0,-1,1,-2,2})
     if(!collide(px_+dx,py_,next)){px_+=dx;rot_=next;return true;}
   return false;
 }
 bool tick(){
   if(over_)return false;
   if(!collide(px_,py_+1,rot_)){++py_;return true;}
   for(int y=0;y<4;++y)for(int x=0;x<4;++x)if(cell(x,y)){
     const int bx=px_+x,by=py_+y;
     if(bx>=0&&bx<W&&by>=0&&by<H)board_[by][bx]=static_cast<uint8_t>(type_+1);
   }
   for(int y=H-1;y>=0;){
     bool filled=true;
     for(int x=0;x<W;++x)if(!board_[y][x])filled=false;
     if(!filled){--y;continue;}
     for(int yy=y;yy>0;--yy)memcpy(board_[yy],board_[yy-1],W);
     memset(board_[0],0,W);++lines_;score_+=100;
   }
   spawn();return !over_;
 }
 void hardDrop(){if(over_)return;while(!collide(px_,py_+1,rot_)){++py_;++score_;}tick();}
private:
 static uint16_t shape(unsigned type){
   static const uint16_t s[7]={0x00F0,0x0066,0x0072,
     0x0036,0x0063,0x0071,0x0074};
   return s[type];
 }
 uint8_t board_[H][W]={};
 uint8_t type_=0;
 int px_=3,py_=0,rot_=0;
 uint32_t score_=0,lines_=0;
 bool over_=false;
 RNG rng_;
 bool collide(int x,int y,int r)const{
   for(int cy=0;cy<4;++cy)for(int cx=0;cx<4;++cx){
     int sx=cx,sy=cy;
     if(r==1){sx=cy;sy=3-cx;}
     if(r==2){sx=3-cx;sy=3-cy;}
     if(r==3){sx=3-cy;sy=cx;}
     if(!(shape(type_)&(static_cast<uint16_t>(1u)<<(sy*4+sx))))continue;
     const int bx=x+cx,by=y+cy;
     if(bx<0||bx>=W||by>=H||by<0)return true;
     if(board_[by][bx])return true;
   }
   return false;
 }
 void spawn(){
   type_=static_cast<uint8_t>(rng_.next()%7);
   rot_=0;px_=3;py_=0;
   if(collide(px_,py_,rot_))over_=true;
 }
};
class Checkers {
public:
 Checkers(){reset();}
 void reset(){
   memset(board_,0,sizeof(board_));
   for(int y=0;y<3;++y)for(int x=0;x<8;++x)
     if((x+y)%2)board_[y][x]=-1;
   for(int y=5;y<8;++y)for(int x=0;x<8;++x)
     if((x+y)%2)board_[y][x]=1;
   side_=1;chainX_=chainY_=-1;
 }
 int at(int x,int y)const{
   return x>=0&&x<8&&y>=0&&y<8?board_[y][x]:0;
 }
 int side()const{return side_;}
 bool move(int x,int y,int tx,int ty){
   if(!inside(x,y)||!inside(tx,ty)||at(x,y)*side_<=0||at(tx,ty)!=0)
     return false;
   if(chainX_>=0&&(chainX_!=x||chainY_!=y))return false;
   const int piece=at(x,y),dx=tx-x,dy=ty-y,ax=dx<0?-dx:dx,ay=dy<0?-dy:dy;
   if(ax!=ay||(ax!=1&&ax!=2))return false;
   if(piece==1&&dy>=0)return false;
   if(piece==-1&&dy<=0)return false;
   const bool capture=ax==2;
   if(!capture&&(chainX_>=0||anyCapture(side_)))return false;
   if(capture) {
     const int jumped=at(x+dx/2,y+dy/2);
     if(jumped*side_>=0)return false;
     board_[y+dy/2][x+dx/2]=0;
   }
   board_[ty][tx]=board_[y][x];board_[y][x]=0;
   bool promoted=false;
   if((board_[ty][tx]==1&&ty==0)||(board_[ty][tx]==-1&&ty==7)) {
     board_[ty][tx]*=2;promoted=true;
   }
   if(capture&&!promoted&&captureAt(tx,ty)){
     chainX_=tx;chainY_=ty;
   }else{
     chainX_=chainY_=-1;side_=-side_;
   }
   return true;
 }
 bool mustContinue()const{return chainX_>=0;}
 bool anyCapture(int side)const{
   for(int y=0;y<8;++y)for(int x=0;x<8;++x)
     if(board_[y][x]*side>0&&captureAt(x,y))return true;
   return false;
 }
private:
 int8_t board_[8][8]={};
 int side_=1,chainX_=-1,chainY_=-1;
 static bool inside(int x,int y){return x>=0&&x<8&&y>=0&&y<8;}
 bool captureAt(int x,int y)const{
   const int piece=at(x,y);
   if(!piece)return false;
   for(int dy=-2;dy<=2;dy+=4){
     if(piece==1&&dy>0)continue;
     if(piece==-1&&dy<0)continue;
     for(int dx=-2;dx<=2;dx+=4) {
       if(inside(x+dx,y+dy)&&at(x+dx,y+dy)==0 &&
          at(x+dx/2,y+dy/2)*piece<0)return true;
     }
   }
   return false;
 }
};
class Memory {
public:
 explicit Memory(uint32_t seed=11):rng_(seed){reset();}
 void reset(){
   for(int i=0;i<16;++i){tiles_[i]=i%8;matched_[i]=false;}
   for(int i=15;i>0;--i){
     int j=static_cast<int>(rng_.next()%static_cast<unsigned>(i+1));
     const uint8_t temp=tiles_[i];tiles_[i]=tiles_[j];tiles_[j]=temp;
   }
   first_=second_=-1;pairs_=0;pending_=false;
 }
 bool flip(int i){
   if(i<0||i>=16||matched_[i])return false;
   if(pending_)return false;
   if(first_<0){first_=i;return true;}
   if(first_==i)return false;
   second_=i;
   if(tiles_[first_]==tiles_[second_]){
     matched_[first_]=matched_[second_]=true;++pairs_;
     first_=second_=-1;
   }else pending_=true;
   return true;
 }
 void hideMismatch(){
   if(pending_){pending_=false;first_=second_=-1;}
 }
 bool visible(int i)const{
   return i>=0&&i<16&&(matched_[i]||i==first_||i==second_);
 }
 uint8_t at(int i)const{return i>=0&&i<16?tiles_[i]:0;}
 int pairs()const{return pairs_;}
 bool pending()const{return pending_;}
 bool won()const{return pairs_==8;}
private:
 uint8_t tiles_[16]={};
 bool matched_[16]={};
 int first_=-1,second_=-1,pairs_=0;
 bool pending_=false;
 RNG rng_;
};
class Pet {
public:
 void reset(uint32_t now){
   hunger_=85;joy_=75;energy_=80;last_=now;
 }
 Pet(){reset(0);}
 void tick(uint32_t now) {
   const uint32_t elapsed=now-last_;
   const unsigned minutes=elapsed/60000;
   if(!minutes)return;
   last_+=minutes*60000u;
   hunger_=hunger_>minutes?hunger_-minutes:0;
   joy_=joy_>minutes?joy_-minutes:0;
   energy_=energy_>minutes?energy_-minutes:0;
 }
 void feed(){hunger_=hunger_+25>100?100:hunger_+25;}
 void play(){joy_=joy_+22>100?100:joy_+22;
             energy_=energy_>12?energy_-12:0;}
 void rest(){energy_=energy_+30>100?100:energy_+30;
             hunger_=hunger_>5?hunger_-5:0;}
 unsigned hunger()const{return hunger_;}
 unsigned joy()const{return joy_;}
 unsigned energy()const{return energy_;}
 bool happy()const{return hunger_>=30&&joy_>=30&&energy_>=20;}
private:
 unsigned hunger_=85,joy_=75,energy_=80;
 uint32_t last_=0;
};
class Story {
public:
 void reset(){node_=0;}
 Story(){reset();}
 int node()const{return node_;}
 const char*description()const{
   static const char* const s[]={
     "You find an old signal tower.",
     "A locked door blocks the stair.",
     "A radio hums inside the room.",
     "A map shows a buried cable.",
     "You repair the rooftop antenna.",
     "You discover a hidden archive.",
     "The tower broadcasts again!",
     "The trail ends for now."
   };
   return s[node_];
 }
 bool choice(unsigned oneOrTwo){
   if(((oneOrTwo!=1)&&(oneOrTwo!=2))||node_>=6)return false;
   static const uint8_t next[6][2]={{1,3},{2,7},{4,5},{5,7},{6,7},{6,7}};
   node_=next[node_][oneOrTwo-1];return true;
 }
 bool finished()const{return node_>=6;}
private:int node_=0;
};
}} // suite::arcade10
