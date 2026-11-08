#pragma once
#include "core/Theme16Runtime.h"
// P10 offline arcade pack: five original K132 games, keyboard + display only.
#include <Arduino.h>
#include <M5Cardputer.h>
#include "core/Arcade10.h"
class Arcade10Collection {
public:
 void begin(){
        SPI.begin(40,39,14,12);SD.begin(12,SPI,10000000);mode_=-1;menu_=0;dirty_=true;last_=millis();pet_.reset(millis());}
 void exit(){}
 void poll(){
   const uint32_t now=millis();
   if(mode_==0&&static_cast<uint32_t>(now-last_)>=480){
     last_=now;if(!blocks_.over())blocks_.tick();dirty_=true;
   }
   if(mode_==2&&cards_.pending()&&static_cast<uint32_t>(now-flippedAt_)>=850){
     cards_.hideMismatch();dirty_=true;
   }
   if(mode_==3){
     const unsigned old=pet_.hunger();
     pet_.tick(now);if(pet_.hunger()!=old)dirty_=true;
   }
 }
 bool dirty()const{return dirty_;}
 void input(const Keyboard_Class::KeysState&k){
   if(mode_<0){
     if(k.up&&menu_>0)--menu_;
     if(k.down&&menu_<4)++menu_;
     if(k.enter)open(menu_);
     dirty_=true;return;
   }
   if(letter(k,'b')){mode_=-1;dirty_=true;return;}
   if(letter(k,'r')){open(mode_);return;}
   if(mode_==0){
     if(k.left||letter(k,'a'))blocks_.shift(-1);
     if(k.right||letter(k,'d'))blocks_.shift(1);
     if(k.up||letter(k,'w'))blocks_.rotate();
     if(k.down||letter(k,'s'))blocks_.tick();
     if(space(k)||k.enter)blocks_.hardDrop();
   }else if(mode_==1){
     steer(k,7,7);
     if(k.enter) {
       if(originX_<0){
         if(checkers_.at(x_,y_)*checkers_.side()>0){
           originX_=x_;originY_=y_;
         }
       }else{
         if(checkers_.move(originX_,originY_,x_,y_)) {
           if(checkers_.mustContinue()){originX_=x_;originY_=y_;}
           else originX_=originY_=-1;
         }else {originX_=originY_=-1;}
       }
     }
   }else if(mode_==2){
     steer(k,3,3);
     if(k.enter){cards_.flip(y_*4+x_);flippedAt_=millis();}
   }else if(mode_==3){
     if(letter(k,'f'))pet_.feed();
     if(letter(k,'p'))pet_.play();
     if(letter(k,'z'))pet_.rest();
   }else if(mode_==4){
     if(letter(k,'1'))story_.choice(1);
     if(letter(k,'2'))story_.choice(2);
   }
   dirty_=true;
 }
 void render(){
   dirty_=false;auto&d=M5Cardputer.Display;
   const auto &p=suite::skin16::colors();
        const uint16_t bg=p.bg,
        head=p.panel,
        white=p.text,
        cyan=p.accent,
        sel=p.selected,
        green=p.good,
        red=p.warning,
        dim=p.muted;
   d.fillScreen(bg);d.fillRect(0,0,240,23,head);
   d.setTextColor(white,head);d.setCursor(5,7);
   d.print(mode_<0?"ARCADE EXPANSION":labels(mode_));
   d.setTextColor(cyan,head);d.setCursor(208,7);d.print("SD");
   d.fillRect(0,116,240,19,head);
   d.setTextColor(dim,head);d.setCursor(5,121);
   d.print(mode_<0?"Fn arrows / Enter":
     mode_==0?"Arrows move/turn Enter drop R reset":
     mode_==1?"Arrows target Enter move R reset":
     mode_==2?"Arrows tile Enter flip R reset":
     mode_==3?"F feed P play Z rest R reset":
     "1/2 choice R restart B menu");
   if(mode_<0){
     for(int i=0;i<5;++i) {
       const int y=29+i*16;
       if(i==menu_)d.fillRect(3,y-1,233,13,sel);
       d.setTextColor(white,i==menu_?sel:bg);
       d.setCursor(8,y+2);d.print(String(i+1)+". "+labels(i));
     }
     return;
   }
   if(mode_==0){
     d.setTextColor(white,bg);d.setCursor(5,30);
     d.print(String("Score ")+blocks_.score());
     d.setCursor(5,46);d.print(String("Lines ")+blocks_.lines());
     d.setCursor(5,64);d.print(blocks_.over()?"GAME OVER":"Falling blocks");
     for(int y=0;y<suite::arcade10::FallingBlocks::H;++y)
       for(int x=0;x<suite::arcade10::FallingBlocks::W;++x){
         const int px=101+x*9,py=28+y*5;
         const uint8_t tile=blocks_.at(x,y);
         if(tile)d.fillRect(px,py,8,4,sel);
       }
     if(!blocks_.over())for(int y=0;y<4;++y)for(int x=0;x<4;++x)
       if(blocks_.cell(x,y)){
         const int px=101+(blocks_.pieceX()+x)*9;
         const int py=28+(blocks_.pieceY()+y)*5;
         if(px>=101&&px<191&&py>=28&&py<108)d.fillRect(px,py,8,4,green);
       }
   }else if(mode_==1){
     d.setTextColor(cyan,bg);d.setCursor(5,26);
     d.print(String("Turn: ")+(checkers_.side()>0?"WHITE":"BLACK")+
             (checkers_.mustContinue()?" MULTI-JUMP":""));
     for(int y=0;y<8;++y)for(int x=0;x<8;++x){
       const int px=26+x*22,py=37+y*9;
       const uint16_t cell=(x+y)%2?sel:0x18c5;
       d.fillRect(px,py,21,8,x==x_&&y==y_?cyan:cell);
       const int piece=checkers_.at(x,y);
       if(piece) {
         d.fillCircle(px+10,py+4,3,piece>0?white:red);
         if(piece==2||piece==-2)d.fillCircle(px+10,py+4,1,green);
       }
       if(x==originX_&&y==originY_)d.drawRect(px,py,21,8,green);
     }
   }else if(mode_==2){
     d.setTextColor(cyan,bg);d.setCursor(5,27);
     d.print(String("Pairs ")+cards_.pairs()+"/8"+(cards_.won()?" WIN!":""));
     for(int y=0;y<4;++y)for(int x=0;x<4;++x){
       const int index=y*4+x;
       const int px=15+x*53,py=42+y*16;
       d.fillRoundRect(px,py,49,14,2,x==x_&&y==y_?cyan:sel);
       if(cards_.visible(index)){
         d.setTextColor(white,x==x_&&y==y_?cyan:sel);
         d.setCursor(px+19,py+4);d.print(cards_.at(index)+1);
       }
     }
   }else if(mode_==3){
     d.setTextColor(cyan,bg);d.setCursor(5,27);
     d.print("Pocket Pet (uptime-based)");
     d.setTextColor(white,bg);d.setCursor(5,46);
     d.print(String("Food   ")+pet_.hunger()+"/100");
     d.setCursor(5,62);d.print(String("Joy    ")+pet_.joy()+"/100");
     d.setCursor(5,78);d.print(String("Energy ")+pet_.energy()+"/100");
     d.setTextColor(green,bg);d.setCursor(5,101);
     d.print(pet_.happy()?"Pet feels great!":"Pet needs care");
   }else{
     d.setTextColor(cyan,bg);d.setCursor(5,28);
     d.print("THE SIGNAL TOWER");
     d.setTextColor(white,bg);d.setCursor(5,48);
     d.print(story_.description());
     d.setTextColor(green,bg);d.setCursor(5,72);
     if(story_.finished())d.print("The End. Press R to restart.");
     else {
       d.print("Press 1 for the first path");
       d.setCursor(5,87);d.print("Press 2 for another path");
     }
   }
 }
private:
 static const char*labels(int i){
   static const char*const names[]={
     "Falling Blocks","Local Checkers","Memory Match",
     "Virtual Pet","Text Adventure"
   };
   return names[i];
 }
 suite::arcade10::FallingBlocks blocks_;
 suite::arcade10::Checkers checkers_;
 suite::arcade10::Memory cards_;
 suite::arcade10::Pet pet_;
 suite::arcade10::Story story_;
 int mode_=-1,menu_=0,x_=0,y_=0,originX_=-1,originY_=-1;
 uint32_t last_=0,flippedAt_=0;
 bool dirty_=true;
 static bool letter(const Keyboard_Class::KeysState&k,char c){
   for(char v:k.word)if(v==c||v==(c-'a'+'A'))return true;
   return false;
 }
 static bool space(const Keyboard_Class::KeysState&k){
   for(char c:k.word)if(c==' ')return true;
   return false;
 }
 void steer(const Keyboard_Class::KeysState&k,int maxX,int maxY){
   if(k.left&&x_>0)--x_;if(k.right&&x_<maxX)++x_;
   if(k.up&&y_>0)--y_;if(k.down&&y_<maxY)++y_;
 }
 void open(int i){
   mode_=i;dirty_=true;x_=y_=0;originX_=originY_=-1;
   last_=millis();
   if(i==0)blocks_.reset();
   if(i==1)checkers_.reset();
   if(i==2)cards_.reset();
   if(i==3)pet_.reset(millis());
   if(i==4)story_.reset();
 }
};
