#pragma once
// Host-testable development tools for original K132; no heap/PSRAM dependence.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
namespace suite { namespace dev {
struct Result {bool ok=false;double value=0;};
class Basic {
public:
  void clear(){for(double &v:vars_)v=0;}
  Basic(){clear();}
  // Small bounded BASIC REPL: LET A=expression; PRINT expression; CLEAR.
  // Expressions support numbers, A-Z variables, + - * / ^ and parentheses.
  Result run(const char* command){
    if(!command||strlen(command)>96)return {};
    const char* s=command;spaces(s);
    if(word(s,"CLEAR")){spaces(s);if(*s)return {};clear();return {true,0};}
    bool print=false,let=false;int variable=-1;
    if(word(s,"PRINT"))print=true;
    else if(word(s,"LET")) {
      let=true;spaces(s);
      if(*s>='a'&&*s<='z')variable=*s-'a';
      if(*s>='A'&&*s<='Z')variable=*s-'A';
      if(variable<0)return {};
      ++s;spaces(s);if(*s++!='=')return {};
    } else if(*s=='?'){++s;print=true;}
    else return {};
    steps_=0;
    Result n=expr(s,0);spaces(s);
    if(!n.ok||*s||!isfinite(n.value))return {};
    if(let)vars_[variable]=n.value;
    (void)print;return n;
  }
  double variable(char n)const{
    if(n>='a'&&n<='z')n=static_cast<char>(n-32);
    return n>='A'&&n<='Z'?vars_[n-'A']:0;
  }
private:
  double vars_[26]={};
  int steps_=0;
  static void spaces(const char*&s){while(*s==' '||*s=='\t')++s;}
  static bool word(const char*&s,const char* key){
    const char* p=s;while(*key) {
      char c=*p;if(c>='a'&&c<='z')c=static_cast<char>(c-32);
      if(c!=*key++)return false;++p;
    }
    if((*p>='A'&&*p<='Z')||(*p>='a'&&*p<='z'))return false;
    s=p;return true;
  }
  Result expr(const char*&s,int depth){
    Result lhs=term(s,depth+1);
    if(!lhs.ok)return {};
    for(;;) {
      spaces(s);const char op=*s;
      if(op!='+'&&op!='-')break;++s;
      Result rhs=term(s,depth+1);if(!rhs.ok)return {};
      lhs.value=op=='+'?lhs.value+rhs.value:lhs.value-rhs.value;
      if(!isfinite(lhs.value))return {};
    }
    return lhs;
  }
  Result term(const char*&s,int depth) {
    Result lhs=power(s,depth+1);if(!lhs.ok)return {};
    for(;;){
      spaces(s);const char op=*s;
      if(op!='*'&&op!='/')break;++s;
      Result rhs=power(s,depth+1);if(!rhs.ok)return {};
      if(op=='/'&&rhs.value==0)return {};
      lhs.value=op=='*'?lhs.value*rhs.value:lhs.value/rhs.value;
      if(!isfinite(lhs.value))return {};
    }return lhs;
  }
  Result power(const char*&s,int depth){
    Result lhs=atom(s,depth+1);if(!lhs.ok)return {};
    spaces(s);
    if(*s=='^'){
      ++s;Result rhs=power(s,depth+1);if(!rhs.ok)return {};
      lhs.value=pow(lhs.value,rhs.value);
      if(!isfinite(lhs.value))return {};
    }
    return lhs;
  }
  Result atom(const char*&s,int depth){
    if(depth>32||++steps_>96)return {};
    spaces(s);
    if(*s=='+'||*s=='-'){
      const bool neg=*s++=='-';
      Result r=atom(s,depth+1);
      if(neg)r.value=-r.value;
      return r;
    }
    if(*s=='('){
      ++s;Result r=expr(s,depth+1);
      spaces(s);if(!r.ok||*s!=')')return {};
      ++s;return r;
    }
    char c=*s;if(c>='a'&&c<='z')c=static_cast<char>(c-32);
    if(c>='A'&&c<='Z'){++s;return {true,vars_[c-'A']};}
    if((*s<'0'||*s>'9')&&*s!='.')return {};
    double number=0,scale=0.1;bool fractional=false,digit=false;
    while((*s>='0'&&*s<='9')||*s=='.'){
      if(*s=='.'){if(fractional)return {};fractional=true;}
      else {digit=true;const int d=*s-'0';
        if(fractional){number+=d*scale;scale*=0.1;}
        else number=number*10+d;
      }
      if(!isfinite(number))return {};
      ++s;
    }
    return {digit,number};
  }
};
enum class Science : uint8_t {SquareRoot,Square,Reciprocal,SinDeg,CosDeg,TanDeg,
                              NaturalLog,Log10,Exp,Factorial,Count};
inline const char* scienceName(Science s){
  switch(s){
    case Science::SquareRoot:return "Square root";
    case Science::Square:return "Square";
    case Science::Reciprocal:return "1 / x";
    case Science::SinDeg:return "sin degrees";
    case Science::CosDeg:return "cos degrees";
    case Science::TanDeg:return "tan degrees";
    case Science::NaturalLog:return "ln(x)";
    case Science::Log10:return "log10(x)";
    case Science::Exp:return "exp(x)";
    case Science::Factorial:return "Integer factorial";
    default:return "Unsupported";
  }
}
inline Result science(Science fn,double v){
  if(!isfinite(v))return {};
  double output=0;
  switch(fn){
    case Science::SquareRoot: if(v<0)return {};output=sqrt(v);break;
    case Science::Square: output=v*v;break;
    case Science::Reciprocal: if(v==0)return {};output=1/v;break;
    case Science::SinDeg:output=sin(v*0.017453292519943295);break;
    case Science::CosDeg:output=cos(v*0.017453292519943295);break;
    case Science::TanDeg:
      if(fabs(cos(v*0.017453292519943295))<1e-10)return {};
      output=tan(v*0.017453292519943295);break;
    case Science::NaturalLog:if(v<=0)return {};output=log(v);break;
    case Science::Log10:if(v<=0)return {};output=log10(v);break;
    case Science::Exp:output=exp(v);break;
    case Science::Factorial:
      if(v<0||v>20||floor(v)!=v)return {};
      output=1;for(int i=2;i<=static_cast<int>(v);++i)output*=i;
      break;
    default:return {};
  }
  return {isfinite(output),output};
}
inline bool hexByte(char a,char b,uint8_t& result) {
  auto digit=[](char c)->int {
    if(c>='0'&&c<='9')return c-'0';
    if(c>='a'&&c<='f')return c-'a'+10;
    if(c>='A'&&c<='F')return c-'A'+10;
    return -1;
  };
  const int x=digit(a),y=digit(b);
  if(x<0||y<0)return false;
  result=static_cast<uint8_t>(x*16+y);return true;
}
inline bool hexDecode(const char* text,uint8_t* bytes,size_t cap,size_t& length){
  length=0;if(!text||!bytes)return false;
  const size_t n=strlen(text);
  if(n%2||n/2>cap)return false;
  for(size_t i=0;i<n;i+=2)
    if(!hexByte(text[i],text[i+1],bytes[length++])){length=0;return false;}
  return true;
}
inline bool hexEncode(const uint8_t* bytes,size_t length,char* out,size_t cap){
  if(!out||(!bytes&&length)||cap<=length*2)return false;
  static const char* digits="0123456789ABCDEF";
  for(size_t i=0;i<length;++i) {
    out[i*2]=digits[bytes[i]>>4];out[i*2+1]=digits[bytes[i]&15];
  }
  out[length*2]=0;return true;
}
// Intel 1976 CHIP-8 subset; no undocumented SuperChip opcodes.
class Chip8 {
public:
  static constexpr unsigned Width=64,Height=32,RomMax=3584;
  void reset(){
    memset(mem_,0,sizeof(mem_));memset(v_,0,sizeof(v_));
    memset(screen_,0,sizeof(screen_));memset(keys_,0,sizeof(keys_));
    memset(stack_,0,sizeof(stack_));
    pc_=0x200;i_=0;sp_=0;delay_=sound_=0;halt_=false;
    rng_=0x187B123u;
    static const uint8_t font[80]={
      0xF0,0x90,0x90,0x90,0xF0,0x20,0x60,0x20,0x20,0x70,
      0xF0,0x10,0xF0,0x80,0xF0,0xF0,0x10,0xF0,0x10,0xF0,
      0x90,0x90,0xF0,0x10,0x10,0xF0,0x80,0xF0,0x10,0xF0,
      0xF0,0x80,0xF0,0x90,0xF0,0xF0,0x10,0x20,0x40,0x40,
      0xF0,0x90,0xF0,0x90,0xF0,0xF0,0x90,0xF0,0x10,0xF0,
      0xF0,0x90,0xF0,0x90,0x90,0xE0,0x90,0xE0,0x90,0xE0,
      0xF0,0x80,0x80,0x80,0xF0,0xE0,0x90,0x90,0x90,0xE0,
      0xF0,0x80,0xF0,0x80,0xF0,0xF0,0x80,0xF0,0x80,0x80
    };
    memcpy(mem_,font,80);
  }
  Chip8(){reset();}
  bool load(const uint8_t* data,size_t size){
    if(!data||!size||size>RomMax)return false;
    reset();memcpy(mem_+0x200,data,size);
    return true;
  }
  bool frame(unsigned x,unsigned y)const{return x<64&&y<32&&screen_[y*64+x];}
  void key(uint8_t n,bool pressed){if(n<16)keys_[n]=pressed;}
  uint16_t pc()const{return pc_;}
  uint8_t reg(uint8_t n)const{return n<16?v_[n]:0;}
  uint8_t delayTimer()const{return delay_;}
  uint8_t soundTimer()const{return sound_;}
  bool halted()const{return halt_;}
  void tickTimers(){if(delay_)--delay_;if(sound_)--sound_;}
  bool step(){
    if(halt_)return false;
    if(pc_>4094){halt_=true;return false;}
    const uint16_t op=(static_cast<uint16_t>(mem_[pc_])<<8)|mem_[pc_+1];
    pc_=static_cast<uint16_t>(pc_+2);
    const unsigned x=(op>>8)&15,y=(op>>4)&15,n=op&15;
    const uint8_t kk=static_cast<uint8_t>(op),vY=v_[y];
    const uint16_t nnn=op&0xFFFu;
    auto fail=[this](){halt_=true;return false;};
    switch(op&0xF000u){
      case 0x0000:
        if(op==0x00E0)memset(screen_,0,sizeof(screen_));
        else if(op==0x00EE){if(!sp_)return fail();pc_=stack_[--sp_];}
        else return fail();
        break;
      case 0x1000:pc_=nnn;break;
      case 0x2000:
        if(sp_>=16)return fail();
        stack_[sp_++]=pc_;pc_=nnn;break;
      case 0x3000:if(v_[x]==kk)pc_+=2;break;
      case 0x4000:if(v_[x]!=kk)pc_+=2;break;
      case 0x5000:if(n)return fail();if(v_[x]==vY)pc_+=2;break;
      case 0x6000:v_[x]=kk;break;
      case 0x7000:v_[x]=static_cast<uint8_t>(v_[x]+kk);break;
      case 0x8000:{
        const uint8_t old=v_[x];
        switch(n){
          case 0:v_[x]=vY;break;
          case 1:v_[x]|=vY;break;
          case 2:v_[x]&=vY;break;
          case 3:v_[x]^=vY;break;
          case 4:{const unsigned sum=old+vY;v_[x]=static_cast<uint8_t>(sum);v_[15]=sum>255;break;}
          case 5:v_[x]=static_cast<uint8_t>(old-vY);v_[15]=old>=vY;break;
          case 6:v_[x]=old>>1;v_[15]=old&1;break;
          case 7:v_[x]=static_cast<uint8_t>(vY-old);v_[15]=vY>=old;break;
          case 0xE:v_[x]=static_cast<uint8_t>(old<<1);v_[15]=(old>>7)&1;break;
          default:return fail();
        }break;
      }
      case 0x9000:if(n)return fail();if(v_[x]!=vY)pc_+=2;break;
      case 0xA000:i_=nnn;break;
      case 0xB000:pc_=static_cast<uint16_t>(nnn+v_[0]);break;
      case 0xC000:
        rng_^=rng_<<13;rng_^=rng_>>17;rng_^=rng_<<5;
        v_[x]=static_cast<uint8_t>(rng_)&kk;break;
      case 0xD000:{
        if(i_+n>4096)return fail();
        v_[15]=0;
        for(unsigned row=0;row<n;++row)
          for(unsigned bit=0;bit<8;++bit) {
            if(!(mem_[i_+row]&(0x80u>>bit)))continue;
            const unsigned px=(v_[x]+bit)%64,py=(v_[y]+row)%32;
            uint8_t& pixel=screen_[py*64+px];
            if(pixel)v_[15]=1;pixel^=1;
          }
        break;
      }
      case 0xE000:
        if((op&255)==0x9E){if(keys_[v_[x]&15])pc_+=2;}
        else if((op&255)==0xA1){if(!keys_[v_[x]&15])pc_+=2;}
        else return fail();
        break;
      case 0xF000:
        switch(op&255u) {
          case 0x07:v_[x]=delay_;break;
          case 0x0A:{
            bool found=false;
            for(uint8_t q=0;q<16;++q)
              if(keys_[q]){v_[x]=q;found=true;break;}
            if(!found)pc_=static_cast<uint16_t>(pc_-2);
            break;
          }
          case 0x15:delay_=v_[x];break;
          case 0x18:sound_=v_[x];break;
          case 0x1E:i_=static_cast<uint16_t>(i_+v_[x]);break;
          case 0x29:i_=static_cast<uint16_t>(5*(v_[x]&15));break;
          case 0x33:
            if(i_>4093)return fail();
            mem_[i_]=v_[x]/100;mem_[i_+1]=(v_[x]/10)%10;mem_[i_+2]=v_[x]%10;break;
          case 0x55:
            if(i_+x>=4096)return fail();
            for(unsigned j=0;j<=x;++j)mem_[i_+j]=v_[j];break;
          case 0x65:
            if(i_+x>=4096)return fail();
            for(unsigned j=0;j<=x;++j)v_[j]=mem_[i_+j];break;
          default:return fail();
        }
        break;
      default:return fail();
    }
    if(pc_>4094)return fail();
    return true;
  }
private:
  uint8_t mem_[4096]={},v_[16]={},screen_[2048]={},keys_[16]={};
  uint16_t stack_[16]={},i_=0,pc_=0x200;
  uint8_t sp_=0,delay_=0,sound_=0;
  uint32_t rng_=1;
  bool halt_=false;
};
}}
