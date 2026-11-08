#include "../src/core/Ssdp21Core.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
using namespace suite::ssdp21;
int main(){
 Device d;
 const char*valid="HTTP/1.1 200 OK\r\nCACHE-CONTROL: max-age=100\r\n"
 "ST: urn:schemas-upnp-org:device:MediaServer:1\r\n"
 "SERVER: C64-LAN/1.0\r\n\r\n";
 assert(parse(valid,strlen(valid),d));
 assert(!strcmp(d.type,"urn:schemas-upnp-org:device:MediaServer:1"));
 assert(!strcmp(d.server,"C64-LAN/1.0"));
 const char*bad[]={"HTTP/1.1 404 FAIL\r\nST: ssdp:all\r\n\r\n",
                   "HTTP/1.1 200 OK\r\nLOCATION: http://127.0.0.1\r\n\r\n",
                   "HTTP/1.1 200 OK\r\nST: s\r\nSERVER: bad\x01name\r\n\r\n",
                   "HTTP/1.1 200 OK\r\nST: s\r\n"};
 for(const char*text:bad)assert(!parse(text,strlen(text),d));
 std::string tooLong="HTTP/1.1 200 OK\r\nST: ";
 tooLong.append(300,'A');tooLong+="\r\n\r\n";
 assert(!parse(tooLong.c_str(),tooLong.size(),d));
 assert(!parse(valid,1025,d));
 puts("P21 SSDP: strict status/headers, bounds and malformed packet rejection PASS");
}
