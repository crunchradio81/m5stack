#include "../src/core/SDAppManifest.h"
#include <cassert>
#include <cstdio>
#include <string>
#include <cstring>
using namespace suite::appmanifest;
int main() {
    const std::string valid=
      "K132APP1\nNAME=Pocket Metronome\nID=METRONOME\nIMAGE=METRONOME.BIN\n"
      "SIZE=123456\nSHA256=0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\n";
    App app;assert(parse(valid.c_str(),valid.size(),app));
    assert(strcmp(app.id,"METRONOME")==0&&app.size==123456);
    assert(strcmp(app.image,"METRONOME.BIN")==0);
    const auto replace=[](std::string s,const char* a,const char* b) {
        const auto p=s.find(a); assert(p!=std::string::npos);
        s.replace(p,strlen(a),b);return s;
    };
    auto bad=replace(valid,"K132APP1","WRONGVER");
    assert(!parse(bad.c_str(),bad.size(),app));
    bad=replace(valid,"ID=METRONOME","ID=NOPE");
    assert(!parse(bad.c_str(),bad.size(),app));
    bad=replace(valid,"METRONOME.BIN","../SECRET.BIN");
    assert(!parse(bad.c_str(),bad.size(),app));
    bad=replace(valid,"METRONOME.BIN","PIANO.BIN");
    assert(!parse(bad.c_str(),bad.size(),app));
    bad=replace(valid,"123456","4294967296");
    assert(!parse(bad.c_str(),bad.size(),app));
    bad=replace(valid,"123456","1");
    assert(!parse(bad.c_str(),bad.size(),app));
    bad=replace(valid,"SHA256=012345","SHA256=Z12345");
    assert(!parse(bad.c_str(),bad.size(),app));
    bad=valid+std::string("SIZE=123456\n");
    assert(!parse(bad.c_str(),bad.size(),app));
    bad=valid+std::string("EXTRA=1\n");
    assert(!parse(bad.c_str(),bad.size(),app));
    bad=valid.substr(0,valid.size()-2);
    assert(!parse(bad.c_str(),bad.size(),app));
    bad=valid+std::string(500,'X');
    assert(!parse(bad.c_str(),bad.size(),app));
    for(const char* id:{"METRONOME","PIANO","CHANNELS","BLE",
                        "ARCADE","OFFICE","INSPECT","MUSICLAB",
                        "DEVTOOLS","SYSTEM"})
        assert(authorizedId(id));
    assert(!authorizedId("CAMERA"));
    assert(safeFilename("BLE.BIN") && !safeFilename("BLE.bin"));
    puts("SD App Manifests: PASS (bounded fields, file names, exact app IDs)");
}
