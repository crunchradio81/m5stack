#pragma once
// P12 independent SD Arcade 3: chess, Klondike, Sudoku and Dungeon.
// Only stock original K132 keyboard/display, no audio/Grove/PSRAM.
#include <Arduino.h>
#include <M5Cardputer.h>
#include <esp_system.h>
#include "core/Chess12.h"
#include "core/Solitaire12.h"
#include "core/P12Puzzles.h"
class Arcade12Collection {
public:
 void begin(){mode_=-1;menu_=0;dirty_=true;selX_=selY_=0;fromX_=-1;}
 void exit(){}
 void poll(){}
 bool dirty()const{return dirty_;}
 void input(const Keyboard_Class::KeysState& k){
   if(mode_<0){
     if(k.up&&menu_>0)--menu_;
     if(k.down&&menu_<3)++menu_;
     if(k.enter)open(menu_);
     dirty_=true;return;
   }
   if(letter(k,'b')||k.esc){mode_=-1;dirty_=true;return;}
   if(letter(k,'r')){open(mode_);return;}
   if(mode_==0)chessInput(k);
   else if(mode_==1)solitaireInput(k);
   else if(mode_==2)sudokuInput(k);
   else if(mode_==3)dungeonInput(k);
   dirty_=true;
 }
 void render(){
   dirty_=false;auto&d=M5Cardputer.Display;
   constexpr uint16_t bg=0x0841,head=0x10c4,white=0xffff,
     cyan=0x05ff,green=0x5fe9,sel=0x2673,dim=0xad55,red=0xf926;
   d.fillScreen(bg);d.fillRect(0,0,240,23,head);
   d.setTextColor(white,head);d.setCursor(5,7);
   d.print(mode_<0?"ARCADE 3":names(mode_));
   d.setTextColor(cyan,head);d.setCursor(207,7);d.print("SD");
   d.fillRect(0,116,240,19,head);
   d.setTextColor(dim,head);d.setCursor(5,121);
   d.print(mode_<0?"Fn arrows / Enter":
     mode_==0?"Arrows cell Enter move R reset B":
     mode_==1?"D draw F found G waste Enter move":
     mode_==2?"Arrows 1-9 0 clear H hint R":
     "Arrows move/attack R reset B");
   if(mode_<0){
     for(int i=0;i<4;++i){
       const int y=33+i*20;
       if(i==menu_)d.fillRect(3,y-1,233,16,sel);
       d.setTextColor(white,i==menu_?sel:bg);
       d.setCursor(8,y+3);d.print(String(i+1)+". "+names(i));
     }
     return;
   }
   if(mode_==0){
     for(int y=0;y<8;++y)for(int x=0;x<8;++x){
       const int px=55+21*x,py=26+11*y;
       uint16_t back=(x+y)%2?sel:0x2124;
       if(x==selX_&&y==selY_)back=cyan;
       if(x==fromX_&&y==fromY_)back=green;
       d.fillRect(px,py,20,10,back);
       const int p=chess_.at(x,y);
       if(p){
         d.setTextColor(p>0?white:red,back);d.setCursor(px+6,py+1);
         static const char* pieces=".PNBRQK";
         d.print(pieces[p>0?p:-p]);
       }
     }
     d.setTextColor(white,bg);d.setCursor(3,27);
     d.print(chess_.side()>0?"White":"Black");
     d.setCursor(3,42);d.print(chess_.inCheck(chess_.side())?"CHECK":"");
     d.setCursor(3,58);d.print(chess_.ended()?
       (chess_.winner()==0?"DRAW":"MATE"):"2P");
   }else if(mode_==1){
     d.setTextColor(cyan,bg);d.setCursor(5,27);
     d.print(String("Stock ")+cards_.stockCount()+" Waste "+
       cardName(cards_.wasteTop()));
     d.setTextColor(green,bg);d.setCursor(5,39);
     for(int s=0;s<4;++s)d.print(String(" ")+
       cardName(s*13+cards_.foundationCount(s)-1).substring(0,2));
     for(int c=0;c<7;++c){
       const int px=5+c*33;
       d.setTextColor(c==selX_?cyan:white,bg);d.setCursor(px,51);
       d.print(String(c+1)+(fromX_==c?"*":":"));
       const int n=cards_.tableauCount(c);
       const int first=n>6?n-6:0;
       for(int i=first;i<n;++i){
         const int py=63+(i-first)*8;
         d.setTextColor(c==selX_&&i==selY_?green:white,bg);
         d.setCursor(px,py);
         d.print(cards_.isFaceUp(c,i)?
           cardName(cards_.tableauAt(c,i)):"##");
       }
     }
     if(cards_.won()){
       d.setTextColor(green,bg);d.setCursor(150,40);d.print("YOU WIN!");
     }
   }else if(mode_==2){
     for(int y=0;y<9;++y)for(int x=0;x<9;++x){
       const int px=26+x*21,py=27+y*9;
       uint16_t back=(x/3+y/3)%2?0x10c4:0x2104;
       if(x==selX_&&y==selY_)back=sel;
       d.fillRect(px,py,20,8,back);
       const int v=sudoku_.at(x,y);
       if(v){
         const bool conflict=sudoku_.conflict(x,y);
         d.setTextColor(conflict?red:sudoku_.fixed(x,y)?cyan:white,back);
         d.setCursor(px+7,py);
         d.print(v);
       }
     }
     d.setTextColor(green,bg);d.setCursor(5,110);
     d.print(sudoku_.won()?"Solved!":"H: hint (max 3)");
   }else if(mode_==3){
     for(int y=0;y<suite::puzzles12::Dungeon::H;++y){
       for(int x=0;x<suite::puzzles12::Dungeon::W;++x){
         const char cell=dungeon_.at(x,y);
         d.setTextColor(cell=='@'?green:cell=='M'?red:
           cell=='K'||cell=='$'?cyan:white,bg);
         d.setCursor(8+x*11,26+y*9);d.print(cell);
       }
     }
     d.setTextColor(green,bg);d.setCursor(5,109);
     d.print(String("F")+dungeon_.floor()+" HP"+dungeon_.health()+
             " Gold"+dungeon_.gold()+(dungeon_.hasKey()?" Key":""));
     if(dungeon_.won()||dungeon_.lost()){
       d.setTextColor(dungeon_.won()?green:red,bg);
       d.setCursor(185,110);d.print(dungeon_.won()?"WIN":"LOST");
     }
   }
 }
private:
 static const char*names(int i){
   static const char*const menu[]={
     "Local Chess","Klondike Solitaire","Sudoku","Dungeon Crawler"};
   return menu[i];
 }
 static bool letter(const Keyboard_Class::KeysState&k,char c){
   for(char v:k.word)if(v==c||v==(c-'a'+'A'))return true;
   return false;
 }
 static String cardName(int card){
   if(card<0)return "--";
   static const char rank[]=" A23456789TJQK";
   static const char suit[]="HDSC";
   const int r=suite::solitaire12::Klondike::rank(card);
   const int s=suite::solitaire12::Klondike::suit(card);
   return String(rank[r])+suit[s];
 }
 int mode_=-1,menu_=0,selX_=0,selY_=0,fromX_=-1,fromY_=-1;
 bool dirty_=true;
 suite::chess12::Game chess_;
 suite::solitaire12::Klondike cards_;
 suite::puzzles12::Sudoku sudoku_;
 suite::puzzles12::Dungeon dungeon_;
 void open(int mode){
   mode_=mode;dirty_=true;
   selX_=selY_=0;fromX_=fromY_=-1;
   if(mode==0)chess_.reset();
   else if(mode==1){cards_.reset(esp_random());}
   else if(mode==2)sudoku_.reset(esp_random()%9);
   else if(mode==3)dungeon_.reset(esp_random());
 }
 void chessInput(const Keyboard_Class::KeysState&k){
   if(k.left&&selX_>0)--selX_;
   if(k.right&&selX_<7)++selX_;
   if(k.up&&selY_>0)--selY_;
   if(k.down&&selY_<7)++selY_;
   if(k.enter){
     if(fromX_<0){
       if(chess_.at(selX_,selY_)*chess_.side()>0) {
         fromX_=selX_;fromY_=selY_;
       }
     }else{
       chess_.move(fromX_,fromY_,selX_,selY_);
       fromX_=fromY_=-1;
     }
   }
 }
 void solitaireInput(const Keyboard_Class::KeysState&k){
   if(k.left&&selX_>0)--selX_;
   if(k.right&&selX_<6)++selX_;
   if(fromX_<0) {
     const int n=cards_.tableauCount(selX_);
     if(k.up&&selY_>0)--selY_;
     if(k.down&&selY_+1<n)++selY_;
     if(selY_>=n)selY_=n?n-1:0;
   }
   if(letter(k,'d'))cards_.draw();
   if(letter(k,'g'))cards_.wasteToTableau(selX_);
   if(letter(k,'f')){
     if(!cards_.wasteToFoundation())cards_.tableauToFoundation(selX_);
   }
   if(letter(k,'z'))fromX_=fromY_=-1;
   if(k.enter){
     if(fromX_<0){
       if(cards_.tableauCount(selX_)>0){
         fromX_=selX_;
         fromY_=selY_;
       }
     }else{
       cards_.tableauToTableau(fromX_,fromY_,selX_);
       fromX_=fromY_=-1;selY_=0;
     }
   }
 }
 void sudokuInput(const Keyboard_Class::KeysState&k){
   if(k.left&&selX_>0)--selX_;
   if(k.right&&selX_<8)++selX_;
   if(k.up&&selY_>0)--selY_;
   if(k.down&&selY_<8)++selY_;
   for(char c:k.word){
     if(c>='0'&&c<='9')sudoku_.put(selX_,selY_,c-'0');
   }
   if(letter(k,'h'))sudoku_.hint(selX_,selY_);
 }
 void dungeonInput(const Keyboard_Class::KeysState&k){
   if(k.left||letter(k,'a'))dungeon_.move(-1,0);
   if(k.right||letter(k,'d'))dungeon_.move(1,0);
   if(k.up||letter(k,'w'))dungeon_.move(0,-1);
   if(k.down||letter(k,'s'))dungeon_.move(0,1);
 }
};
