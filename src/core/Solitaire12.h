#pragma once
// P12 bounded Klondike draw-one solitaire. Fixed cards; stock/waste,
// 7 tableau columns, 4 ascending suit foundations, one stock recycle.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
namespace suite {namespace solitaire12 {
class RNG {
public:
 explicit RNG(uint32_t seed):s_(seed?seed:1){}
 uint32_t next(){s_^=s_<<13;s_^=s_>>17;s_^=s_<<5;return s_;}
private:uint32_t s_;
};
class Klondike {
public:
 explicit Klondike(uint32_t seed=17){reset(seed);}
 void reset(uint32_t seed){
   RNG rng(seed);
   uint8_t deck[52]={};
   for(int i=0;i<52;++i)deck[i]=static_cast<uint8_t>(i);
   for(int i=51;i>0;--i){
     const int j=static_cast<int>(rng.next()%static_cast<unsigned>(i+1));
     const uint8_t c=deck[i];deck[i]=deck[j];deck[j]=c;
   }
   memset(count_,0,sizeof(count_));
   memset(face_,0,sizeof(face_));
   memset(foundation_,0,sizeof(foundation_));
   memset(tableau_,0,sizeof(tableau_));
   stockCount_=wasteCount_=0;recycles_=0;
   int pos=0;
   for(int col=0;col<7;++col){
     for(int j=0;j<=col;++j){
       tableau_[col][j]=deck[pos++];
       face_[col][j]=(j==col);
     }
     count_[col]=col+1;
   }
   while(pos<52)stock_[stockCount_++]=deck[pos++];
 }
 static int rank(int card){return card>=0&&card<52?card%13+1:0;}
 static int suit(int card){return card>=0&&card<52?card/13:-1;}
 static bool red(int card){const int s=suit(card);return s==1||s==2;}
 int stockCount()const{return stockCount_;}
 int wasteCount()const{return wasteCount_;}
 int foundationCount(int s)const{return s>=0&&s<4?foundation_[s]:0;}
 int tableauCount(int c)const{return c>=0&&c<7?count_[c]:0;}
 int tableauAt(int c,int i)const{
   return c>=0&&c<7&&i>=0&&i<count_[c]?tableau_[c][i]:-1;
 }
 bool isFaceUp(int c,int i)const{
   return c>=0&&c<7&&i>=0&&i<count_[c]&&face_[c][i];
 }
 int wasteTop()const{return wasteCount_?waste_[wasteCount_-1]:-1;}
 bool draw(){
   if(stockCount_>0){
     if(wasteCount_>=52)return false;
     waste_[wasteCount_++]=stock_[--stockCount_];
     return true;
   }
   if(!wasteCount_||recycles_>=1)return false;
   // Reverse waste onto new stock so top-to-bottom turn order stays stable.
   while(wasteCount_)stock_[stockCount_++]=waste_[--wasteCount_];
   ++recycles_;
   return true;
 }
 bool wasteToFoundation(){
   const int card=wasteTop();
   if(card<0||!acceptFoundation(card))return false;
   foundation_[suit(card)]++;
   --wasteCount_;return true;
 }
 bool tableauToFoundation(int column){
   if(column<0||column>6||!count_[column])return false;
   const int idx=count_[column]-1,card=tableau_[column][idx];
   if(!face_[column][idx]||!acceptFoundation(card))return false;
   ++foundation_[suit(card)];--count_[column];flipTop(column);return true;
 }
 bool wasteToTableau(int target){
   if(target<0||target>=7||wasteCount_==0)return false;
   const int card=wasteTop();
   if(!acceptColumn(card,target))return false;
   if(count_[target]>=20)return false;
   tableau_[target][count_[target]]=static_cast<uint8_t>(card);
   face_[target][count_[target]++]=true;
   --wasteCount_;return true;
 }
 bool tableauToTableau(int from,int start,int to){
   if(from<0||from>=7||to<0||to>=7||from==to||
      start<0||start>=count_[from]||!face_[from][start])return false;
   for(int i=start;i+1<count_[from];++i)
     if(!face_[from][i+1]||red(tableau_[from][i])==red(tableau_[from][i+1])||
        rank(tableau_[from][i])!=rank(tableau_[from][i+1])+1)return false;
   const int moved=count_[from]-start;
   if(count_[to]+moved>20||!acceptColumn(tableau_[from][start],to))return false;
   for(int i=start;i<count_[from];++i){
     tableau_[to][count_[to]]=tableau_[from][i];
     face_[to][count_[to]++]=true;
   }
   count_[from]=start;flipTop(from);return true;
 }
 bool won()const{
   for(int s=0;s<4;++s)if(foundation_[s]!=13)return false;
   return true;
 }
 bool invariant()const{
   bool seen[52]={};int total=0;
   for(int i=0;i<stockCount_;++i){
     if(stock_[i]>=52||seen[stock_[i]])return false;
     seen[stock_[i]]=true;++total;
   }
   for(int i=0;i<wasteCount_;++i){
     if(waste_[i]>=52||seen[waste_[i]])return false;
     seen[waste_[i]]=true;++total;
   }
   for(int c=0;c<7;++c)for(int i=0;i<count_[c];++i){
     const int card=tableau_[c][i];
     if(card>=52||seen[card])return false;
     seen[card]=true;++total;
   }
   for(int suit=0;suit<4;++suit)for(int r=0;r<foundation_[suit];++r){
     const int card=suit*13+r;
     if(seen[card])return false;
     seen[card]=true;++total;
   }
   return total==52;
 }
private:
 uint8_t tableau_[7][20]={},stock_[52]={},waste_[52]={};
 bool face_[7][20]={};
 int count_[7]={},stockCount_=0,wasteCount_=0,recycles_=0;
 int foundation_[4]={};
 void flipTop(int c){if(count_[c])face_[c][count_[c]-1]=true;}
 bool acceptFoundation(int card)const{
   return foundation_[suit(card)]+1==rank(card);
 }
 bool acceptColumn(int card,int col)const{
   const int n=count_[col];
   if(!n)return rank(card)==13;
   const int top=tableau_[col][n-1];
   return face_[col][n-1]&&red(top)!=red(card)&&
     rank(top)==rank(card)+1;
 }
};
}} // suite::solitaire12
