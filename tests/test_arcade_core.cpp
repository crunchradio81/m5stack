#include "../src/core/ArcadeCore.h"
#include <cassert>
#include <cstdio>
using namespace suite::arcade;
int main(){
    Tiles2048 g(19);
    int count=0;
    for(int y=0;y<4;++y)for(int x=0;x<4;++x)if(g.at(x,y))++count;
    assert(count==2);
    g.reset();
    for(int y=0;y<4;++y)for(int x=0;x<4;++x)g.setForTest(x,y,0);
    g.setForTest(0,0,2);g.setForTest(1,0,2);
    g.setForTest(2,0,2);g.setForTest(3,0,2);
    assert(g.move(Dir::Left));
    assert(g.at(0,0)==4 && g.at(1,0)==4 && g.score()==8);
    assert(g.canMove());
    for(int y=0;y<4;++y)for(int x=0;x<4;++x)
        g.setForTest(x,y,static_cast<uint16_t>((x+y)%2?4:2));
    assert(!g.canMove());assert(!g.move(Dir::Left));
    g.setForTest(0,0,2048);assert(g.won());

    Snake snake(2);
    assert(snake.alive() && snake.length()==3);
    assert(snake.x(0)==5&&snake.y(0)==5);
    snake.steer(Dir::Left); // reverse not allowed
    assert(snake.tick() && snake.x(0)==6);
    snake.steer(Dir::Up);assert(snake.tick() && snake.y(0)==4);
    snake.steer(Dir::Down);assert(snake.tick() && snake.y(0)==3);
    for(int i=0;i<10 && snake.alive();++i)snake.tick();
    assert(!snake.alive());
    snake.reset();assert(snake.alive() && snake.score()==0);

    Mines mine(44);
    assert(mine.reveal(0,0) && !mine.mine(0,0));
    int mines=0;
    for(int y=0;y<Mines::H;++y)for(int x=0;x<Mines::W;++x)
        if(mine.mine(x,y)) ++mines;
    assert(mines==Mines::Count);
    mine.flag(1,1);assert(mine.flagged(1,1));
    mine.flag(1,1);assert(!mine.flagged(1,1));
    int mx=-1,my=-1;
    for(int y=0;y<Mines::H;++y)for(int x=0;x<Mines::W;++x)
        if(mine.mine(x,y)){mx=x;my=y;}
    assert(mx>=0);assert(!mine.reveal(mx,my) && mine.failed());
    mine.reset();assert(!mine.failed());

    Sokoban puzzle;
    assert(!puzzle.won() && puzzle.at(4,5)=='@' && puzzle.at(4,3)=='$');
    assert(puzzle.move(Dir::Up));
    assert(puzzle.move(Dir::Up));
    assert(puzzle.won() && puzzle.at(4,2)=='*');
    assert(puzzle.move(Dir::Up)); // box may be moved off its goal
    assert(!puzzle.won());
    puzzle.reset();assert(!puzzle.won());
    puts("Arcade: 2048, Snake, Mines, Sokoban PASS");
}
