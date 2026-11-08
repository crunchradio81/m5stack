#include "../src/core/Net8Core.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace suite::net8;
static void reject(const char*data){Config c;assert(!parse(data,strlen(data),c));}
int main(){
  const char* raw=
    "K132NET1\nSSID=Home Lab\nPASSWORD=my private password\n"
    "DNS_HOST=example.org\nTCP_HOST=192.168.1.1\nTCP_PORT=8080\n"
    "HTTP_HOST=192.168.1.10\nHTTP_PORT=8000\nHTTP_PATH=/status\n"
    "MQTT_HOST=192.168.1.20\nMQTT_TOPIC=cardputer/demo\nCHAT_NAME=Field User\n";
  Config config;
  assert(parse(raw,strlen(raw),config));
  assert(strcmp(config.ssid,"Home Lab")==0);
  assert(strcmp(config.httpPath,"/status")==0);
  assert(config.tcpPort==8080&&config.httpPort==8000);
  assert(strcmp(config.chatName,"Field User")==0);
  reject("K132NET1\nPASSWORD=a\n");
  reject("K132NET1\nSSID=A\nSSID=B\n");
  reject("K132NET1\nSSID=abc\nTCP_PORT=0\n");
  reject("K132NET1\nSSID=abc\nTCP_PORT=65536\n");
  reject("K132NET1\nSSID=abc\nTCP_PORT=1x\n");
  reject("K132NET1\nSSID=abc\nHTTP_HOST=abc/def\n");
  reject("K132NET1\nSSID=abc\nHTTP_PATH=/../admin\n");
  reject("K132NET1\nSSID=abc\nMQTT_TOPIC=a/+\n");
  reject("K132NET1\nSSID=abc\nUNKNOWN=yes\n");
  reject("K132NET1\nSSID=abc");
  reject("BADCFG\nSSID=abc\n");
  assert(host("192.168.0.1")&&host("example.org"));
  assert(!host("a..b")&&!host("a@b")&&!host(""));
  uint16_t number=0;
  assert(port("443",number)&&number==443);
  assert(!port("00000",number)&&!port("65536",number));
  char output[120]={},sender[22]={},message[73]={};
  assert(makeChat("K132","Hello LAN",output,sizeof(output)));
  assert(parseChat(reinterpret_cast<const uint8_t*>(output),strlen(output),sender,message));
  assert(strcmp(sender,"K132")==0&&strcmp(message,"Hello LAN")==0);
  assert(!parseChat(reinterpret_cast<const uint8_t*>("bad"),3,sender,message));
  assert(!makeChat("A","Hello|inject",output,sizeof(output)));
  assert(!makeChat("A","\n",output,sizeof(output)));
  assert(!makeChat("A","Hello",output,4));
  char clean[12]={};
  const char in[]={'H','i','\x01','X','\r'};
  safePrintLine(in,sizeof(in),clean,sizeof(clean));
  assert(strcmp(clean,"Hi?X\r")==0);
  puts("P8 Network Core: config guards, ports, chat framing, sanitization PASS");
}
