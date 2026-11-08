#pragma once
// P12 Sudoku and bounded dungeon, no heap/Arduino/PSRAM.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
namespace suite {namespace puzzles12 {
class Sudoku {
public:
  Sudoku(){reset(0);}
  void reset(unsigned variant){
    hints_=0;variant_=variant%9;
    static const char*const puzzle[9]={
      "530070000","600195000","098000060",
      "800060003","400803001","700020006",
      "060000280","000419005","000080079"
    };
    static const char*const answer[9]={
      "534678912","672195348","198342567",
      "859761423","426853791","713924856",
      "961537284","287419635","345286179"
    };
    for(int y=0;y<9;++y)for(int x=0;x<9;++x){
      const int p=puzzle[y][x]-'0',s=answer[y][x]-'0';
      given_[y][x]=p!=0;
      solved_[y][x]=static_cast<uint8_t>((s-1+variant_)%9+1);
      board_[y][x]=p?solved_[y][x]:0;
    }
  }
  unsigned variant()const{return variant_;}
  int at(int x,int y)const{return inside(x,y)?board_[y][x]:0;}
  bool fixed(int x,int y)const{return inside(x,y)&&given_[y][x];}
  bool put(int x,int y,int value){
    if(!inside(x,y)||fixed(x,y)||value<0||value>9)return false;
    board_[y][x]=static_cast<uint8_t>(value);return true;
  }
  bool conflict(int x,int y)const{
    if(!inside(x,y)||!at(x,y))return false;
    const int v=at(x,y);
    for(int i=0;i<9;++i){
      if(i!=x&&at(i,y)==v)return true;
      if(i!=y&&at(x,i)==v)return true;
    }
    const int bx=(x/3)*3,by=(y/3)*3;
    for(int cy=by;cy<by+3;++cy)for(int cx=bx;cx<bx+3;++cx)
      if((cx!=x||cy!=y)&&at(cx,cy)==v)return true;
    return false;
  }
  bool hint(int x,int y){
    if(!inside(x,y)||fixed(x,y)||hints_>=3)return false;
    board_[y][x]=solved_[y][x];given_[y][x]=true;
    ++hints_;return true;
  }
  unsigned hints()const{return hints_;}
  bool won()const{
    for(int y=0;y<9;++y)for(int x=0;x<9;++x)
      if(!board_[y][x]||conflict(x,y))return false;
    return true;
  }
private:
 static bool inside(int x,int y){return x>=0&&x<9&&y>=0&&y<9;}
 uint8_t board_[9][9]={},solved_[9][9]={};
 bool given_[9][9]={};
 unsigned hints_=0,variant_=0;
};

class Dungeon {
public:
 static constexpr int W=19,H=10;
 Dungeon(){reset(17);}
 void reset(uint32_t seed){
   state_=seed?seed:17;floor_=1;health_=12;gold_=0;
   lost_=won_=false;build();
 }
 char at(int x,int y)const {
   if(!inside(x,y))return '#';
   if(x==px_&&y==py_)return '@';
   if(x==mx_&&y==my_&&monsterHP_>0)return 'M';
   return tiles_[y][x];
 }
 int playerX()const{return px_;}int playerY()const{return py_;}
 int floor()const{return floor_;}
 int health()const{return health_;}
 int gold()const{return gold_;}
 bool hasKey()const{return key_;}
 bool lost()const{return lost_;}
 bool won()const{return won_;}
 bool move(int dx,int dy){
   if(lost_||won_||(dx!=0&&dy!=0)||(dx==0&&dy==0)||
      dx<-1||dx>1||dy<-1||dy>1)return false;
   const int nx=px_+dx,ny=py_+dy;
   if(!inside(nx,ny)||tiles_[ny][nx]=='#')return false;
   if(nx==mx_&&ny==my_&&monsterHP_>0){
     --monsterHP_;
     if(monsterHP_>0){--health_;if(health_<=0)lost_=true;}
     else gold_+=3;
     return true;
   }
   if(tiles_[ny][nx]=='K'){key_=true;tiles_[ny][nx]='.';}
   if(tiles_[ny][nx]=='$'){gold_+=5;tiles_[ny][nx]='.';}
   if(tiles_[ny][nx]=='+'){
     health_=health_+3>12?12:health_+3;
     tiles_[ny][nx]='.';
   }
   if(tiles_[ny][nx]=='>'){
     if(!key_)return false;
     if(floor_>=3){won_=true;px_=nx;py_=ny;return true;}
     ++floor_;build();return true;
   }
   px_=nx;py_=ny;return true;
 }
private:
 char tiles_[H][W]={};
 int px_=1,py_=1,mx_=W-2,my_=H/2;
 int floor_=1,health_=12,gold_=0,monsterHP_=2;
 bool key_=false,lost_=false,won_=false;
 uint32_t state_=17;
 uint32_t random(){
   state_^=state_<<13;state_^=state_>>17;state_^=state_<<5;
   return state_;
 }
 static bool inside(int x,int y){return x>=0&&x<W&&y>=0&&y<H;}
 void build(){
   for(int y=0;y<H;++y)for(int x=0;x<W;++x){
     tiles_[y][x]=(x==0||y==0||x==W-1||y==H-1||
         random()%7==0)?'#':'.';
   }
   // Continuous, guaranteed path: start → east corridor → south stairs.
   for(int x=1;x<W-1;++x)tiles_[1][x]='.';
   for(int y=1;y<H-1;++y)tiles_[y][W-2]='.';
   tiles_[1][W/2]='K';tiles_[H-2][W-2]='>';
   tiles_[2][2]='+';
   tiles_[H-3][W-3]='$';
   px_=py_=1;mx_=W-2;my_=H/2;monsterHP_=2+floor_/2;
   key_=false;
 }
};
}} // suite::puzzles12
