#include "../src/core/Chess12.h"
#include "../src/core/Solitaire12.h"
#include "../src/core/P12Puzzles.h"
#include <cassert>
#include <cstdio>
using suite::chess12::Game;
using suite::solitaire12::Klondike;
using suite::puzzles12::Sudoku;
using suite::puzzles12::Dungeon;
static void chess(){
 Game g;
 assert(g.at(4,7)==6&&g.at(4,0)==-6);
 assert(g.side()==1&&!g.ended()&&!g.inCheck(1));
 assert(!g.legal(4,7,4,5)); // own pawn
 assert(!g.move(0,6,0,3)); // pawn cannot leap three squares
 assert(g.move(5,6,5,5)); // f2 f3
 assert(g.move(4,1,4,3)); // e7 e5
 assert(g.move(6,6,6,4)); // g2 g4
 assert(g.move(3,0,7,4)); // black Qd8-h4 checkmate
 assert(g.ended()&&g.winner()==-1);
 g.reset();
 assert(g.move(4,6,4,4)); // e2e4
 assert(g.move(4,1,4,3)); // e7e5
 assert(g.move(6,7,5,5)); // g1f3
 assert(g.move(1,0,2,2)); // b8c6
 assert(g.move(5,7,4,6)); // f1e2
 assert(g.move(6,0,5,2)); // g8f6
 assert(g.legal(4,7,6,7)); // castle king side (f1,g1 clear, no check)
 assert(g.move(4,7,6,7));
 assert(g.at(6,7)==6 && g.at(5,7)==4);
 g.reset();
 // En passant e2e4 a7a6 e4e5 d7d5 e5d6
 assert(g.move(4,6,4,4));
 assert(g.move(0,1,0,2));
 assert(g.move(4,4,4,3));
 assert(g.move(3,1,3,3));
 assert(g.move(4,3,3,2));
 assert(g.at(3,2)==1&&g.at(3,3)==0);
}
static void solitaire(){
 Klondike k(7);
 assert(k.invariant()&&k.stockCount()==24);
 int total=0;
 for(int c=0;c<7;++c){
   assert(k.tableauCount(c)==c+1);
   assert(k.isFaceUp(c,c));
   total+=k.tableauCount(c);
 }
 assert(total==28&&k.wasteCount()==0);
 for(int i=0;i<24;++i)assert(k.draw());
 assert(k.stockCount()==0&&k.wasteCount()==24&&k.invariant());
 assert(k.draw()); // one recycle
 assert(k.wasteCount()==0&&k.stockCount()==24&&k.invariant());
 for(int i=0;i<24;++i)assert(k.draw());
 assert(!k.draw()); // capped recycle
 assert(k.invariant());
 assert(!k.tableauToFoundation(-1));
 assert(!k.wasteToTableau(8));
 assert(!k.tableauToTableau(0,0,0));
}
static void sudoku(){
 Sudoku s;
 assert(s.fixed(0,0)&&s.at(0,0)==5);
 assert(!s.put(0,0,1)&&!s.put(-1,0,5));
 assert(s.at(2,0)==0&&s.put(2,0,5)&&s.conflict(2,0));
 assert(s.put(2,0,4)&&!s.conflict(2,0));
 assert(!s.won());
 assert(s.hint(3,0)&&s.hints()==1&&!s.put(3,0,9));
 s.reset(5);
 assert(s.at(0,0)==((5-1+5)%9+1));
 assert(!s.conflict(0,0));
 for(int i=0;i<5;++i)for(int j=0;j<9;++j)
   assert(!s.conflict(j,i));
}
static void dungeon(){
 Dungeon d;
 assert(d.floor()==1&&d.playerX()==1&&d.playerY()==1);
 assert(!d.move(1,1)&&!d.move(-1,0)&&!d.move(0,-1));
 for(int x=1;x<Dungeon::W/2;++x)assert(d.move(1,0));
 assert(d.hasKey());
 for(int x=Dungeon::W/2;x<Dungeon::W-2;++x)assert(d.move(1,0));
 // Monster blocks corridor; walking into it consumes attacks.
 for(int y=1;y<Dungeon::H-2;++y) {
   if(d.at(Dungeon::W-2,y+1)=='M') {
     for(int i=0;i<5;++i)assert(d.move(0,1));
   }else assert(d.move(0,1));
 }
 assert(d.move(0,1)); // stair descend
 assert(d.floor()==2);
 assert(d.health()>0);
 d.reset(22);assert(d.floor()==1&&!d.won()&&!d.lost());
}
int main(){
 chess();solitaire();sudoku();dungeon();
 puts("P12 Arcade: legal chess/mate/castling/en-passant, Klondike, Sudoku, dungeon PASS");
}
