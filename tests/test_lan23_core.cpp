#include "../src/core/Lan23Core.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
using namespace suite::lan23;
int main(){
 const std::string cfg="K132LAN1\nKEY="+std::string(64,'a')+"\n";
 Config key;assert(parseConfig(cfg.c_str(),cfg.size(),key));
 assert(key.key[0]==0xaa&&key.key[31]==0xaa);
 Config copy=key;
 for(const char*s:{"K132LAN1\nKEY=bad\n",
     "K132LAN1\nKEY=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaz\n",
     "K132LAN1\nKEY=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
     "K132LAN1\nHOST=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n"})
  assert(!parseConfig(s,strlen(s),copy)&&copy.key[0]==0xaa);
 const std::string challengeText=std::string(ChallengeMagic)+std::string(32,'b');
 assert(challenge(challengeText.c_str()));
 assert(!challenge("K132CHAL1 deadbeef")&&!challenge("K132CHAL1 zzbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"));
 const std::string digest(64,'c'),sig(64,'d');
 const std::string header="K132FILE1|PHOTO.TXT|1024|"+digest+"|"+sig+"\n";
 Header h;assert(parseHeader(header.c_str(),header.size(),h));
 assert(!strcmp(h.filename,"PHOTO.TXT")&&h.bytes==1024);
 char sign[220]={};assert(signatureBody(h,challengeText.c_str(),sign,sizeof(sign)));
 assert(std::string(sign).find("|PHOTO.TXT|1024|")==9);
 assert(!signatureBody(h,"bad",sign,sizeof(sign)));
 assert(!parseHeader(("K132FILE1|../A.TXT|1024|"+digest+"|"+sig+"\n").c_str(),
    header.size()+3,h));
 for(const std::string& value:{"0","0001","262145","4294967296","-1"}){
  const std::string bad="K132FILE1|VALID.BIN|"+value+"|"+digest+"|"+sig+"\n";
  assert(!parseHeader(bad.c_str(),bad.size(),h));
 }
 const std::string oversized="K132FILE1|NOTFOUND.BIN|1|"+digest+"|"+sig+"\n";
 assert(parseHeader(oversized.c_str(),oversized.size(),h));
 uint8_t sample[32]={};sample[1]=255;
 char expected[65];toHex(sample,32,expected);
 assert(constantTimeHex(expected,sample));
 assert(!constantTimeHex(sig.c_str(),sample));
 assert(!constantTimeHex("bad",sample));
 assert(number("1",h.bytes)&&!number("0",h.bytes));
 puts("P23 LAN file framing: fixed 256KiB cap, safe names, challenge, key, HMAC syntax PASS");
}
