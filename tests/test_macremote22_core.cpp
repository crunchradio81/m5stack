#include "../src/core/MacRemote22Core.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
using namespace suite::mac22;
int main(){
 const char*cfg="K132MAC1\nHOST=192.168.1.22\n"
 "TOKEN=00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff\n";
 Config result;
 assert(parse(cfg,strlen(cfg),result));
 assert(strcmp(result.address,"192.168.1.22")==0);
 assert(result.secret[0]==0x00&&result.secret[31]==0xff);
 assert(address("10.1.2.3")&&address("172.31.1.33"));
 assert(!address("172.32.0.2")&&!address("8.8.8.8"));
 assert(!address("192.168.1.0")&&!address("192.168.1.255"));
 assert(!address("192.168.01.23")&&!address("192.168.1.999"));
 const char*bad[]={
  "K132MAC1\nHOST=8.8.8.8\nTOKEN=00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff\n",
  "K132MAC1\nHOST=192.168.1.22\nTOKEN=not-a-secret\n",
  "K132MAC1\nHOST=192.168.1.22\nHOST=192.168.1.23\nTOKEN=00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff\n",
  "K132MAC1\nHOST=192.168.1.22\nTOKEN=00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff",
  "K132MAC1\nHOST=192.168.1.22\nNOPE=x\nTOKEN=00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff\n"
 };
 for(const char*s:bad){
  Config prior=result;
  assert(!parse(s,strlen(s),result));
  assert(strcmp(prior.address,result.address)==0);
 }
 assert(macroIndex('1')==1&&macroIndex('0')==10&&macroIndex('-')==11);
 assert(macroIndex('=')==12&&macroIndex('x')==0);
 assert(move(-40,40)&&!move(41,1));
 puts("P22 Mac remote: private IPv4, bounded hex secret, fail-closed config & keymap PASS");
}
