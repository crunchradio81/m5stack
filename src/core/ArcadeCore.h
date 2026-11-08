#pragma once
// Deterministic bounded game mechanics for no-PSRAM original Cardputer K132.
// Host-testable C++17. Rendering and physical keyboard input live separately.
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace suite { namespace arcade {
enum class Dir:uint8_t{Up,Down,Left,Right};
class Random {
public:
    explicit Random(uint32_t seed=0xC0FFEEu):state_(seed?seed:1){}
    uint32_t next() {
        uint32_t x=state_;
        x^=x<<13;x^=x>>17;x^=x<<5;return state_=x;
    }
private: uint32_t state_;
};
class Tiles2048 {
public:
    explicit Tiles2048(uint32_t seed=17):rng_(seed) {reset();}
    void reset() {
        memset(cells_,0,sizeof(cells_));score_=0;
        spawn();spawn();
    }
    uint16_t at(int x,int y)const {
        return x>=0&&x<4&&y>=0&&y<4?cells_[y*4+x]:0;
    }
    uint32_t score()const{return score_;}
    void setForTest(int x,int y,uint16_t v) {if(x>=0&&x<4&&y>=0&&y<4)cells_[y*4+x]=v;}
    bool move(Dir d) {
        uint16_t next[16]={};
        for(int axis=0;axis<4;++axis) {
            uint16_t input[4]={};
            int count=0;
            for(int k=0;k<4;++k){
                const int x=(d==Dir::Left)?k:(d==Dir::Right)?3-k:axis;
                const int y=(d==Dir::Up)?k:(d==Dir::Down)?3-k:axis;
                uint16_t v=at(x,y);
                if(v) input[count++]=v;
            }
            int outIndex=0;
            for(int k=0;k<count;++k) {
                uint16_t v=input[k];
                if(k+1<count && input[k+1]==v && v<=16384) {
                    v=static_cast<uint16_t>(v*2u);score_+=v;++k;
                }
                const int x=(d==Dir::Left)?outIndex:(d==Dir::Right)?3-outIndex:axis;
                const int y=(d==Dir::Up)?outIndex:(d==Dir::Down)?3-outIndex:axis;
                next[y*4+x]=v;
                ++outIndex;
            }
        }
        if(memcmp(next,cells_,sizeof(cells_))==0)return false;
        memcpy(cells_,next,sizeof(cells_));
        spawn();return true;
    }
    bool canMove() const {
        for(int y=0;y<4;++y)for(int x=0;x<4;++x) {
            const int p=y*4+x;
            if(cells_[p]==0)return true;
            if(x<3&&cells_[p]==cells_[p+1])return true;
            if(y<3&&cells_[p]==cells_[p+4])return true;
        }
        return false;
    }
    bool won()const {for(auto v:cells_)if(v>=2048)return true;return false;}
private:
    uint16_t cells_[16]={};
    uint32_t score_=0;
    Random rng_;
    void spawn() {
        int empty[16],n=0;
        for(int i=0;i<16;++i)if(!cells_[i])empty[n++]=i;
        if(!n)return;
        const uint32_t value=rng_.next();
        cells_[empty[value%static_cast<unsigned>(n)]]=
            (rng_.next()%10==0)?4:2;
    }
};
class Snake {
public:
    static constexpr int W=20,H=10,Max=100;
    explicit Snake(uint32_t seed=31):rng_(seed){reset();}
    void reset() {
        length_=3;bodyX_[0]=5;bodyY_[0]=5;
        bodyX_[1]=4;bodyY_[1]=5;
        bodyX_[2]=3;bodyY_[2]=5;
        direction_=Dir::Right;alive_=true;score_=0;
        placeFood();
    }
    void steer(Dir d) {
        if((direction_==Dir::Up&&d==Dir::Down)||
           (direction_==Dir::Down&&d==Dir::Up)||
           (direction_==Dir::Left&&d==Dir::Right)||
           (direction_==Dir::Right&&d==Dir::Left))return;
        direction_=d;
    }
    bool tick(){
        if(!alive_)return false;
        int x=bodyX_[0],y=bodyY_[0];
        switch(direction_){
            case Dir::Up:--y;break;case Dir::Down:++y;break;
            case Dir::Left:--x;break;case Dir::Right:++x;break;
        }
        if(x<0||y<0||x>=W||y>=H) {alive_=false;return false;}
        const bool grow=x==foodX_&&y==foodY_;
        const int collisionCount=length_-(grow?0:1);
        for(int i=0;i<collisionCount;++i)
            if(bodyX_[i]==x&&bodyY_[i]==y){alive_=false;return false;}
        if(grow&&length_<Max){++length_;++score_;}
        for(int i=length_-1;i>0;--i) {
            bodyX_[i]=bodyX_[i-1];bodyY_[i]=bodyY_[i-1];
        }
        bodyX_[0]=static_cast<uint8_t>(x);
        bodyY_[0]=static_cast<uint8_t>(y);
        if(grow)placeFood();
        return true;
    }
    int length()const{return length_;}
    int x(int i)const{return i>=0&&i<length_?bodyX_[i]:-1;}
    int y(int i)const{return i>=0&&i<length_?bodyY_[i]:-1;}
    int foodX()const{return foodX_;}
    int foodY()const{return foodY_;}
    bool alive()const{return alive_;}
    int score()const{return score_;}
private:
    uint8_t bodyX_[Max]={},bodyY_[Max]={};
    int length_=0,foodX_=0,foodY_=0,score_=0;
    Dir direction_=Dir::Right;
    bool alive_=true;
    Random rng_;
    void placeFood() {
        if(length_>=Max){alive_=false;return;}
        for(int attempt=0;attempt<W*H;++attempt) {
            const uint32_t index=rng_.next()%(W*H);
            const int x=static_cast<int>(index%W);
            const int y=static_cast<int>(index/W);
            bool occupied=false;
            for(int i=0;i<length_;++i)
                if(bodyX_[i]==x&&bodyY_[i]==y){occupied=true;break;}
            if(!occupied){foodX_=x;foodY_=y;return;}
        }
        // Guaranteed fallback even if unlucky random positions repeated.
        for(int y=0;y<H;++y)for(int x=0;x<W;++x) {
            bool occupied=false;
            for(int i=0;i<length_;++i)
                if(bodyX_[i]==x&&bodyY_[i]==y){occupied=true;break;}
            if(!occupied){foodX_=x;foodY_=y;return;}
        }
    }
};
class Mines {
public:
    static constexpr int W=8,H=6,Count=8;
    explicit Mines(uint32_t seed=11):rng_(seed){reset();}
    void reset() {
        memset(cell_,0,sizeof(cell_));
        started_=false;failed_=false;revealed_=0;
    }
    bool reveal(int x,int y) {
        if(x<0||y<0||x>=W||y>=H||failed_)return false;
        const int index=y*W+x;
        if(cell_[index]&2)return false; // flagged
        if(!started_){place(index);started_=true;}
        if(cell_[index]&1){failed_=true;cell_[index]|=4;return false;}
        if(cell_[index]&4)return true;
        int stack[W*H],size=0;
        stack[size++]=index;
        while(size){
            const int i=stack[--size];
            if(cell_[i]&(1|2|4))continue;
            cell_[i]|=4;++revealed_;
            if(neighbors(i%W,i/W)!=0)continue;
            for(int yy=i/W-1;yy<=i/W+1;++yy)
                for(int xx=i%W-1;xx<=i%W+1;++xx) {
                    if(xx<0||yy<0||xx>=W||yy>=H)continue;
                    const int j=yy*W+xx;
                    if(!(cell_[j]&(1|2|4))) {
                        bool present=false;
                        for(int t=0;t<size;++t)if(stack[t]==j)present=true;
                        if(!present&&size<W*H)stack[size++]=j;
                    }
                }
        }
        return true;
    }
    void flag(int x,int y){
        if(x<0||y<0||x>=W||y>=H||failed_)return;
        const int i=y*W+x;if(!(cell_[i]&4))cell_[i]^=2;
    }
    bool hidden(int x,int y)const{return !(cell_[y*W+x]&4);}
    bool flagged(int x,int y)const{return (cell_[y*W+x]&2)!=0;}
    bool mine(int x,int y)const{return (cell_[y*W+x]&1)!=0;}
    int neighbors(int x,int y)const{
        int n=0;
        for(int yy=y-1;yy<=y+1;++yy)for(int xx=x-1;xx<=x+1;++xx)
            if(xx>=0&&yy>=0&&xx<W&&yy<H&&!(xx==x&&yy==y)&&
               (cell_[yy*W+xx]&1))++n;
        return n;
    }
    bool failed()const{return failed_;}
    bool won()const{return started_&&!failed_&&revealed_==W*H-Count;}
private:
    uint8_t cell_[W*H]={};
    int revealed_=0;
    bool started_=false,failed_=false;
    Random rng_;
    void place(int safe) {
        int n=0;
        while(n<Count) {
            const int p=static_cast<int>(rng_.next()%(W*H));
            if(p==safe||(cell_[p]&1))continue;
            cell_[p]|=1;++n;
        }
    }
};
class Sokoban {
public:
    static constexpr int W=10,H=7;
    Sokoban(){reset();}
    void reset(){x_=4;y_=5;boxX_=4;boxY_=3;goalX_=4;goalY_=2;}
    bool wall(int x,int y)const {
        if(x<=0||x>=W-1||y<=0||y>=H-1)return true;
        return (x==2&&y==2) || (x==7&&y==3);
    }
    bool move(Dir d) {
        int dx=0,dy=0;
        if(d==Dir::Left)dx=-1;
        if(d==Dir::Right)dx=1;
        if(d==Dir::Up)dy=-1;
        if(d==Dir::Down)dy=1;
        const int nx=x_+dx,ny=y_+dy;
        if(wall(nx,ny))return false;
        if(nx==boxX_&&ny==boxY_) {
            if(wall(boxX_+dx,boxY_+dy))return false;
            boxX_+=dx;boxY_+=dy;
        }
        x_=nx;y_=ny;return true;
    }
    char at(int x,int y)const {
        if(wall(x,y))return '#';
        if(x_==x&&y_==y)return '@';
        if(boxX_==x&&boxY_==y)return won()?'*':'$';
        if(goalX_==x&&goalY_==y)return '.';
        return ' ';
    }
    bool won()const{return boxX_==goalX_&&boxY_==goalY_;}
private:
    int x_=4,y_=5,boxX_=4,boxY_=3,goalX_=4,goalY_=2;
};
}}
