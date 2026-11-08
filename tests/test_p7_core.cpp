#include "../src/core/P7Core.h"
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace suite::field;
int main(){
 uint8_t header[44]={};WavInfo parsed;
 assert(wavHeader(header,16000,32000));
 assert(r32(header+24)==16000 && r32(header+40)==32000);
 assert(wavParse(header,44,32044,parsed)&&parsed.rate==16000&&parsed.bytes==32000);
 assert(!wavParse(header,44,32042,parsed));
 assert(!wavHeader(header,12345,200));
 assert(!wavHeader(header,8000,3));
 assert(!wavHeader(header,8000,4000002));
 assert(wavHeader(header,8000,0)&&wavParse(header,44,44,parsed));
 header[20]=3;assert(!wavParse(header,44,44,parsed));
 assert(safeFile("LOG_001.CSV")&&!safeFile("..")&&!safeFile("../X"));
 assert(!safeFile(".hidden")&&!safeFile("foo/bar"));
 assert(safeLeaf("A.WAV",".WAV")&&!safeLeaf("A.MP3",".WAV"));
 uint32_t v=0;
 assert(validHex("ABCD",4,v)&&v==0xabcd);
 assert(!validHex("QBCD",4,v)&&!validHex("ABC",4,v));
 NecProfile p;
 const char* profile="IRP1,Living Room,0000,45,46,47,48,49\n";
 assert(parseIrProfile(profile,strlen(profile),p));
 assert(strcmp(p.name,"Living Room")==0&&p.power==0x45&&p.play==0x49);
 assert(!parseIrProfile("IRP1,Living Room,0000,XX,46,47,48,49\n",strlen(profile),p));
 SignalTrend trend;assert(trend.count()==0&&trend.latest()==-127);
 for(int i=0;i<20;++i)assert(trend.sample(-80+i));
 assert(trend.count()==16&&trend.at(0)==-76&&trend.latest()==-61);
 assert(trend.average()==-68);
 assert(!trend.sample(-128)&&!trend.sample(1));
 char out[60]={};
 assert(csvEscape("Hi,\"cardputer\"",out,sizeof(out)));
 assert(strcmp(out,"\"Hi,\"\"cardputer\"\"\"")==0);
 assert(!csvEscape("bad\nline",out,sizeof(out)));
 assert(!csvEscape("Very long name",out,4));
 puts("P7 Core: WAV safety, IR profile, RSSI history, CSV and safe files PASS");
}
