#include "../src/core/WifiCsv.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
using namespace suite::wifi_csv;
int main() {
    Record a;
    strcpy(a.session,"A12B34C5");
    a.uptimeMs=155000;
    a.scan=22;
    strcpy(a.bssid,"02:11:22:33:44:55");
    strcpy(a.ssid,"Cafe, \"Free\" WiFi");
    a.rssi=-63;
    a.channel=11;
    strcpy(a.security,"WPA2");
    char line[MaxLine]={};
    const size_t n=encode(a,line,sizeof(line));
    assert(n>0 && n==strlen(line) && line[n-1]=='\n');
    assert(strstr(line,"\"Cafe, \"\"Free\"\" WiFi\"")!=nullptr);
    line[n-1]=0;
    Record b;
    assert(decode(line,b));
    assert(strcmp(b.session,a.session)==0);
    assert(strcmp(b.bssid,a.bssid)==0);
    assert(strcmp(b.ssid,a.ssid)==0);
    assert(b.rssi==-63 && b.scan==22 && b.channel==11);
    assert(b.uptimeMs==155000 && strcmp(b.security,"WPA2")==0);
    // A public access point can advertise arbitrary SSID data.
    strcpy(a.ssid,"=1+1");
    assert(encode(a,line,sizeof(line))>0);
    assert(strstr(line,"\"'=1+1\"")!=nullptr); // no spreadsheet formula execution
    strcpy(a.ssid,"A\r\nB\tC");
    assert(encode(a,line,sizeof(line))>0);
    assert(strstr(line,"\"A__B_C\"")!=nullptr);
    strcpy(a.ssid,"");
    a.hidden=true;
    assert(encode(a,line,sizeof(line))>0);
    line[strlen(line)-1]=0;
    assert(decode(line,b) && b.hidden && b.ssid[0]==0);
    char small[10];
    assert(!encode(a,small,sizeof(small)));
    std::string tooLong(33,'Z');
    strcpy(a.ssid,tooLong.c_str());
    assert(!encode(a,line,sizeof(line)));
    assert(!decode("broken,record",b));
    assert(!decode("A12B34C5,100,1,02:11:22:33:44:55,\"oops,-50,1,WPA2,0",b));
    assert(!decode("A12B34C5,100,1,02:11:22:33:44:55,\"hey\",-90,999,WPA2,0",b));
    assert(!decode("A12B34C5,100,1,02:11:22:33:44:55,\"hey\",-90,1,WPA2,0,extra",b));
    puts("Wi-Fi CSV: PASS");
}
