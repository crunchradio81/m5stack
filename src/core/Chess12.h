#pragma once
// P12 offline chess: local 2-player, standard movement, check, castling,
// en passant, automatic queen promotion and checkmate/stalemate detection.
// Fixed RAM; no chess engine AI, opening book, 50-move or repetition rule.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
namespace suite { namespace chess12 {
class Game {
public:
  // Pawn=1 knight=2 bishop=3 rook=4 queen=5 king=6. White > 0.
  Game(){reset();}
  void reset(){
    memset(board_,0,sizeof(board_));
    static const int8_t back[8]={4,2,3,5,6,3,2,4};
    for(int x=0;x<8;++x) {
      board_[0][x]=-back[x];board_[1][x]=-1;
      board_[6][x]=1;board_[7][x]=back[x];
    }
    side_=1;castle_=15;epx_=epy_=-1;
    winner_=0;ended_=false;halfmove_=0;
  }
  int at(int x,int y)const{return inside(x,y)?board_[y][x]:0;}
  int side()const{return side_;}
  bool ended()const{return ended_;}
  int winner()const{return winner_;}
  bool inCheck(int side)const{
    int kx=-1,ky=-1;
    for(int y=0;y<8;++y)for(int x=0;x<8;++x)
      if(board_[y][x]==6*side){kx=x;ky=y;}
    return kx<0||attacked(kx,ky,-side);
  }
  bool legal(int x,int y,int tx,int ty)const{
    if(ended_||!inside(x,y)||!inside(tx,ty)||board_[y][x]*side_<=0||
       board_[ty][tx]*side_>0||x==tx&&y==ty)return false;
    Game draft=*this;
    if(!draft.apply(x,y,tx,ty,false))return false;
    return !draft.inCheck(side_);
  }
  bool move(int x,int y,int tx,int ty) {
    if(!legal(x,y,tx,ty))return false;
    apply(x,y,tx,ty,true);
    const int opponent=side_;
    if(!anyLegalMove()){
      ended_=true;winner_=inCheck(opponent)?-opponent:0;
    }
    return true;
  }
  bool anyLegalMove()const{
    for(int y=0;y<8;++y)for(int x=0;x<8;++x)
      if(board_[y][x]*side_>0)
        for(int ty=0;ty<8;++ty)for(int tx=0;tx<8;++tx)
          if(legal(x,y,tx,ty))return true;
    return false;
  }
private:
  int8_t board_[8][8]={};
  int side_=1,castle_=15,epx_=-1,epy_=-1,halfmove_=0;
  bool ended_=false;
  int winner_=0;
  static bool inside(int x,int y){return x>=0&&x<8&&y>=0&&y<8;}
  static int ab(int v){return v<0?-v:v;}
  bool clear(int x,int y,int tx,int ty)const{
    const int dx=tx>x?1:tx<x?-1:0;
    const int dy=ty>y?1:ty<y?-1:0;
    for(x+=dx,y+=dy;x!=tx||y!=ty;x+=dx,y+=dy)
      if(board_[y][x])return false;
    return true;
  }
  bool attacked(int x,int y,int enemy)const{
    for(int sy=0;sy<8;++sy)for(int sx=0;sx<8;++sx){
      const int p=board_[sy][sx];
      if(p*enemy<=0)continue;
      const int type=ab(p),dx=x-sx,dy=y-sy,ax=ab(dx),ay=ab(dy);
      if(type==1&&ax==1&&dy==-enemy)return true;
      if(type==2&&((ax==1&&ay==2)||(ax==2&&ay==1)))return true;
      if(type==6&&ax<=1&&ay<=1)return true;
      if(type==3&&ax==ay&&ax>0&&clear(sx,sy,x,y))return true;
      if(type==4&&(dx==0||dy==0)&&clear(sx,sy,x,y))return true;
      if(type==5&&((ax==ay&&ax>0)||dx==0||dy==0)&&
         clear(sx,sy,x,y))return true;
    }
    return false;
  }
  bool apply(int x,int y,int tx,int ty,bool changeSide){
    const int p=board_[y][x],type=ab(p),side=p>0?1:-1;
    const int dx=tx-x,dy=ty-y,ax=ab(dx),ay=ab(dy);
    const int target=board_[ty][tx];
    bool enPassant=false,castleMove=false;
    if(type==1) {
      if(dx==0&&dy==-side&&target==0) {}
      else if(dx==0&&dy==-2*side&&
              y==(side==1?6:1)&&target==0&&board_[y-side][x]==0) {}
      else if(ax==1&&dy==-side&&target*side<0) {}
      else if(ax==1&&dy==-side&&target==0&&
              tx==epx_&&ty==epy_&&board_[y][tx]==-side)enPassant=true;
      else return false;
    } else if(type==2) {
      if(!((ax==1&&ay==2)||(ax==2&&ay==1)))return false;
    } else if(type==3) {
      if(ax==0||ax!=ay||!clear(x,y,tx,ty))return false;
    } else if(type==4) {
      if(!((dx==0||dy==0)&&clear(x,y,tx,ty)))return false;
    } else if(type==5) {
      if(!(((ax==ay&&ax>0)||(dx==0||dy==0))&&clear(x,y,tx,ty)))return false;
    } else if(type==6) {
      if(ax<=1&&ay<=1) {}
      else if(dy==0&&ax==2&&x==4&&
              y==(side==1?7:0)&&!inCheck(side)){
        const bool right=dx>0;
        const int mask=side==1?(right?1:2):(right?4:8);
        const int rookX=right?7:0;
        const int rookTarget=right?5:3;
        if(!(castle_&mask)||board_[y][rookX]!=4*side||
           !clear(x,y,rookX,y)||
           attacked(x+(right?1:-1),y,-side)||
           attacked(tx,ty,-side))return false;
        board_[y][rookTarget]=board_[y][rookX];
        board_[y][rookX]=0;castleMove=true;
      }else return false;
    }else return false;
    if(target==6||target==-6)return false; // never capture king
    if(p==4) {
      if(x==0&&y==7)castle_&=~2;
      if(x==7&&y==7)castle_&=~1;
    }else if(p==-4){
      if(x==0&&y==0)castle_&=~8;
      if(x==7&&y==0)castle_&=~4;
    }
    if(type==6)castle_&=~(side==1?3:12);
    if(target==4){
      if(tx==0&&ty==7)castle_&=~2;
      if(tx==7&&ty==7)castle_&=~1;
    }else if(target==-4){
      if(tx==0&&ty==0)castle_&=~8;
      if(tx==7&&ty==0)castle_&=~4;
    }
    board_[ty][tx]=board_[y][x];board_[y][x]=0;
    if(enPassant)board_[y][tx]=0;
    if(type==1&&ty==(side==1?0:7))board_[ty][tx]=static_cast<int8_t>(5*side);
    epx_=epy_=-1;
    if(type==1&&ay==2){epx_=x;epy_=y-side;}
    halfmove_=type==1||target?0:halfmove_+1;
    (void)castleMove;
    if(changeSide)side_=-side;
    return true;
  }
};
}} // suite::chess12
