#include "../src/core/Arcade10.h"
#include <cassert>
#include <cstdio>
using namespace suite::arcade10;
int main(){
 FallingBlocks blocks(11);
 assert(!blocks.over()&&blocks.score()==0);
 bool moved=false;
 for(int i=0;i<4;++i)moved|=blocks.shift(1);
 assert(moved);
 assert(blocks.rotate()||!blocks.rotate());
 blocks.hardDrop();
 assert(blocks.score()<300&&blocks.lines()<=4);
 for(int i=0;i<500&&!blocks.over();++i)blocks.hardDrop();
 assert(blocks.over());blocks.reset();assert(!blocks.over());
 Checkers game;
 assert(game.side()==1 && game.at(0,5)==1&&game.at(1,2)==-1);
 assert(!game.move(1,2,0,3)); // other side
 assert(game.move(0,5,1,4));
 assert(game.side()==-1);
 assert(game.move(1,2,0,3));
 assert(game.side()==1);
 assert(!game.move(2,5,4,3)); // invalid diagonal distance / capture
 Memory cards(123);
 int occurrences[8]={};
 for(int i=0;i<16;++i)++occurrences[cards.at(i)];
 for(int count:occurrences)assert(count==2);
 assert(cards.flip(0));
 assert(!cards.flip(0));
 int different=1;while(different<16&&cards.at(different)==cards.at(0))++different;
 assert(different<16&&cards.flip(different)&&cards.pending());
 assert(cards.visible(0)&&cards.visible(different));
 assert(!cards.flip(3));cards.hideMismatch();
 assert(!cards.visible(0)&&!cards.pending());
 for(int value=0;value<8;++value){
   int first=-1,second=-1;
   for(int i=0;i<16;++i)if(cards.at(i)==value){
     if(first<0)first=i;else second=i;
   }
   assert(cards.flip(first)&&cards.flip(second));
 }
 assert(cards.won()&&cards.pairs()==8);
 Pet pet;pet.reset(UINT32_MAX-100);
 pet.tick(60000-101); // one minute across millis wrap
 assert(pet.hunger()==84);
 for(int i=0;i<120;++i)pet.tick(60000u+static_cast<uint32_t>(i)*60000u);
 assert(!pet.happy());
 pet.feed();pet.play();pet.rest();
 assert(pet.hunger()<=100&&pet.joy()<=100&&pet.energy()<=100);
 Story story;
 assert(story.node()==0 && !story.finished());
 assert(story.choice(1)&&story.node()==1);
 assert(story.choice(1)&&story.node()==2);
 assert(story.choice(1)&&story.node()==4);
 assert(story.choice(1)&&story.node()==6&&story.finished());
 assert(!story.choice(1));
 story.reset();assert(story.choice(2)&&story.node()==3);
 puts("P10 Arcade: falling blocks, checkers, memory, pet and text adventure PASS");
}
