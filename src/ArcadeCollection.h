#pragma once
// Seven real playable games in the ARCADE SD firmware collection.
// Stock K132 keyboard and 240x135 display; no PSRAM or add-ons.
#include <Arduino.h>
#include <M5Cardputer.h>
#include "core/ArcadeCore.h"
class ArcadeCollection {
public:
    void begin(){mode_=-1;cursor_=0;dirty_=true;}
    void exit(){}
    void poll(){
        if(mode_==0&&snake_.alive()&&millis()-lastTick_>=170) {
            lastTick_=millis();snake_.tick();dirty_=true;
        } else if((mode_==4||mode_==5)&&millis()-lastTick_>=35) {
            lastTick_=millis();animate();dirty_=true;
        }
    }
    void input(const Keyboard_Class::KeysState& k){
        if(letter(k,'b')) {mode_=-1;dirty_=true;return;}
        if(mode_<0) {
            if(k.up||letter(k,'w'))cursor_=(cursor_+6)%7;
            if(k.down||letter(k,'s'))cursor_=(cursor_+1)%7;
            if(k.enter)open(cursor_);
            dirty_=true;return;
        }
        if(letter(k,'r')){open(mode_);dirty_=true;return;}
        suite::arcade::Dir d=suite::arcade::Dir::Right;
        const bool movement=direction(k,d);
        if(mode_==0) {
            if(movement)snake_.steer(d);
        } else if(mode_==1) {
            if(movement)tiles_.move(d);
        } else if(mode_==2) {
            if(movement){
                if(d==suite::arcade::Dir::Up&&cy_>0)--cy_;
                if(d==suite::arcade::Dir::Down&&cy_<5)++cy_;
                if(d==suite::arcade::Dir::Left&&cx_>0)--cx_;
                if(d==suite::arcade::Dir::Right&&cx_<7)++cx_;
            }
            if(letter(k,'f'))mines_.flag(cx_,cy_);
            if(k.enter||letter(k,'o'))mines_.reveal(cx_,cy_);
        } else if(mode_==3){
            if(movement)sokoban_.move(d);
        } else if(mode_==4||mode_==5) {
            if(k.up||k.left||letter(k,'w')||letter(k,'a'))paddle_-=7;
            if(k.down||k.right||letter(k,'s')||letter(k,'d'))paddle_+=7;
            if(paddle_<0)paddle_=0;
            if(paddle_>76)paddle_=76;
        } else if(mode_==6) {
            for(char c:k.word) {
                if(c>='A'&&c<='Z')c=static_cast<char>(c+32);
                if(c<'a'||c>'z')continue;
                const unsigned bit=1u<<static_cast<unsigned>(c-'a');
                if(guessed_&bit)continue;
                guessed_|=bit;
                bool included=false;
                for(const char* p=word_;*p;++p)if(*p==c)included=true;
                if(!included)++mistakes_;
            }
        }
        dirty_=true;
    }
    bool dirty()const{return dirty_;}
    void render() {
        dirty_=false;
        auto& d=M5Cardputer.Display;
        constexpr uint16_t bg=0x0841,white=0xffff,sel=0x2673,cyan=0x05ff,green=0x5fe9,red=0xf926;
        d.fillScreen(bg);d.fillRect(0,0,240,23,0x10c4);
        d.setTextColor(white,0x10c4);d.setCursor(5,7);
        d.print(mode_<0?"RETRO ARCADE":names_[mode_]);
        d.setTextColor(cyan,0x10c4);d.setCursor(203,7);d.print("SD");
        d.fillRect(0,116,240,19,0x10c4);
        d.setTextColor(white,0x10c4);d.setCursor(4,121);
        d.print(mode_<0?"Enter select | Fn arrows":"B menu  R restart  Fn arrows");
        if(mode_<0){
            for(int i=0;i<7;++i) {
                int y=27+i*12;
                if(cursor_==i)d.fillRect(3,y-1,233,12,sel);
                d.setTextColor(white,cursor_==i?sel:bg);
                d.setCursor(8,y);d.print(String(i+1)+". "+names_[i]);
            }
            return;
        }
        if(mode_==0) {
            d.setTextColor(white,bg);d.setCursor(5,26);
            d.print(String("Score ")+snake_.score()+(snake_.alive()?"":" GAME OVER"));
            for(int i=0;i<snake_.length();++i)
                d.fillRect(8+snake_.x(i)*11,39+snake_.y(i)*7,9,6,green);
            d.fillRect(8+snake_.foodX()*11,39+snake_.foodY()*7,9,6,red);
        } else if(mode_==1) {
            d.setTextColor(white,bg);d.setCursor(5,25);
            d.print(String("2048   Score ")+tiles_.score()+
                    (tiles_.won()?" WIN!":!tiles_.canMove()?" END":""));
            for(int y=0;y<4;++y)for(int x=0;x<4;++x) {
                const int px=6+x*58,py=39+y*19;
                const int v=tiles_.at(x,y);
                d.fillRoundRect(px,py,55,17,3,v?sel:0x18c5);
                d.setTextColor(v?green:white,v?sel:0x18c5);
                d.setCursor(px+6,py+5);
                if(v)d.print(v);
            }
        } else if(mode_==2) {
            d.setTextColor(white,bg);d.setCursor(5,25);
            d.print(mines_.failed()?"BOOM! R restart":mines_.won()?"CLEARED!":"Enter uncover, F flag");
            for(int y=0;y<6;++y)for(int x=0;x<8;++x) {
                const int px=5+x*29,py=37+y*13;
                const bool highlight=(cx_==x&&cy_==y);
                const bool hidden=mines_.hidden(x,y);
                d.fillRect(px,py,27,12,highlight?cyan:hidden?sel:0x18c5);
                char symbol=' ';
                if(mines_.flagged(x,y))symbol='F';
                else if(!hidden||mines_.failed()) {
                    if(mines_.mine(x,y))symbol='*';
                    else if(mines_.neighbors(x,y)>0)
                        symbol=static_cast<char>('0'+mines_.neighbors(x,y));
                }
                d.setTextColor(white,highlight?cyan:hidden?sel:0x18c5);
                d.setCursor(px+10,py+3);d.print(symbol);
            }
        } else if(mode_==3) {
            d.setTextColor(white,bg);d.setCursor(5,25);
            d.print(sokoban_.won()?"PUZZLE SOLVED!":"Push $ box onto . goal");
            for(int y=0;y<7;++y)for(int x=0;x<10;++x) {
                const int px=7+x*22,py=36+y*11;
                const char v=sokoban_.at(x,y);
                if(v=='#')d.fillRect(px,py,20,10,sel);
                else if(v=='$'||v=='*')d.fillRoundRect(px,py,18,9,2,green);
                else if(v=='@')d.fillCircle(px+9,py+5,4,cyan);
                else if(v=='.')d.drawCircle(px+9,py+5,4,red);
            }
        } else if(mode_==4||mode_==5){
            d.setTextColor(white,bg);d.setCursor(5,25);
            d.print(String("Score ")+score_+"  "+(mode_==4?"PONG":"BREAKOUT"));
            if(mode_==4) {
                d.fillRect(9,34+paddle_,3,24,green);
                d.fillRect(226,34+opponent_,3,24,cyan);
            } else {
                d.fillRect(8+paddle_*2,103,40,5,green);
                for(int i=0;i<18;++i)if(bricks_&(1u<<i))
                    d.fillRect(12+(i%9)*24,37+(i/9)*10,21,7,sel);
            }
            d.fillCircle(static_cast<int>(ballX_),static_cast<int>(ballY_),3,red);
        } else if(mode_==6) {
            d.setTextColor(white,bg);d.setCursor(5,28);
            d.print(String("Wrong guesses: ")+mistakes_+"/6");
            bool win=true;
            for(int i=0;word_[i];++i) {
                const unsigned bit=1u<<static_cast<unsigned>(word_[i]-'a');
                if(!(guessed_&bit))win=false;
                d.setTextSize(2);d.setCursor(8+i*20,56);
                d.print((guessed_&bit)?static_cast<char>(word_[i]-32):'_');
                d.setTextSize(1);
            }
            d.setCursor(5,96);
            d.print(win?"YOU WIN!":mistakes_>=6?String("LOSE: ")+word_:"Type letters to guess");
        }
    }
private:
    static constexpr const char* names_[7]={"Snake","2048","Minesweeper","Sokoban","Pong","Breakout","Word Guess"};
    suite::arcade::Snake snake_;
    suite::arcade::Tiles2048 tiles_;
    suite::arcade::Mines mines_;
    suite::arcade::Sokoban sokoban_;
    int mode_=-1,cursor_=0,cx_=0,cy_=0,paddle_=35,opponent_=35,score_=0,mistakes_=0;
    int8_t vx_=2,vy_=1;
    float ballX_=120,ballY_=68;
    uint32_t lastTick_=0;
    uint32_t bricks_=0x3ffff;
    uint32_t guessed_=0;
    const char* word_="computer";
    bool dirty_=true;
    static bool letter(const Keyboard_Class::KeysState& k,char c) {
        for(char v:k.word)if(v==c||v==(c-'a'+'A'))return true;
        return false;
    }
    static bool direction(const Keyboard_Class::KeysState& k,suite::arcade::Dir& d) {
        if(k.up||letter(k,'w')){d=suite::arcade::Dir::Up;return true;}
        if(k.down||letter(k,'s')){d=suite::arcade::Dir::Down;return true;}
        if(k.left||letter(k,'a')){d=suite::arcade::Dir::Left;return true;}
        if(k.right||letter(k,'d')){d=suite::arcade::Dir::Right;return true;}
        return false;
    }
    void open(int index){
        mode_=index;dirty_=true;lastTick_=millis();
        if(index==0)snake_.reset();
        else if(index==1)tiles_.reset();
        else if(index==2){mines_.reset();cx_=cy_=0;}
        else if(index==3)sokoban_.reset();
        else if(index==4||index==5){
            ballX_=120;ballY_=70;vx_=2;vy_=1;paddle_=35;opponent_=35;score_=0;bricks_=0x3ffff;
        } else if(index==6){
            const char* words[]={"computer","software","keyboard","arcade","window","storage","internet"};
            word_=words[(millis()/37)%7];guessed_=0;mistakes_=0;
        }
    }
    void animate(){
        ballX_+=vx_;ballY_+=vy_;
        if(ballY_<38){ballY_=38;vy_=1;}
        if(ballY_>108){ballY_=108;vy_=-1;}
        if(mode_==4){
            const int target=static_cast<int>(ballY_)-45;
            if(target>opponent_)opponent_+=2;
            if(target<opponent_)opponent_-=2;
            if(opponent_<0)opponent_=0;if(opponent_>76)opponent_=76;
            if(ballX_<=14&&ballY_>=34+paddle_&&ballY_<=58+paddle_){
                ballX_=14;vx_=2;++score_;
            }
            if(ballX_>=223&&ballY_>=34+opponent_&&ballY_<=58+opponent_){
                ballX_=223;vx_=-2;
            }
            if(ballX_<3||ballX_>236){ballX_=120;ballY_=70;vx_=ballX_<3?2:-2;score_=0;}
        } else {
            const int paddleX=8+paddle_*2;
            if(ballY_>=101&&ballX_>=paddleX&&ballX_<=paddleX+40){
                ballY_=100;vy_=-1;++score_;
            }
            if(ballY_>113){ballY_=70;ballX_=120;vy_=-1;score_=0;}
            if(ballX_<7){ballX_=7;vx_=2;}
            if(ballX_>235){ballX_=235;vx_=-2;}
            if(ballY_>=37&&ballY_<=57) {
                const int c=static_cast<int>((ballX_-12)/24);
                const int r=static_cast<int>((ballY_-37)/10);
                if(c>=0&&c<9&&r>=0&&r<2&&(bricks_&(1u<<(r*9+c)))){
                    bricks_&=~(1u<<(r*9+c));vy_=-vy_;score_+=10;
                    if(!bricks_)bricks_=0x3ffff;
                }
            }
        }
    }
};
