#include "../src/core/Paths.h"
#include <cassert>
#include <cstring>
#include <cstdio>
int main() {
    char out[suite::paths::MaxPath];
    assert(suite::paths::join("/","TEST.TXT",out,sizeof(out)) && strcmp(out,"/TEST.TXT")==0);
    assert(suite::paths::join("/SUITE","A.txt",out,sizeof(out)) && strcmp(out,"/SUITE/A.txt")==0);
    assert(!suite::paths::join("/SUITE","../SECRET",out,sizeof(out)));
    assert(!suite::paths::join("/SUITE","..",out,sizeof(out)));
    assert(!suite::paths::join("/SUITE","DIR/NESTED",out,sizeof(out)));
    assert(!suite::paths::join("SUITE","A.TXT",out,sizeof(out)));
    char small[4];assert(!suite::paths::join("/SUITE","A.TXT",small,sizeof(small)));
    assert(suite::paths::parent("/SUITE/A.TXT",out,sizeof(out)) && strcmp(out,"/SUITE")==0);
    assert(suite::paths::parent("/SUITE",out,sizeof(out)) && strcmp(out,"/")==0);
    assert(suite::paths::parent("/",out,sizeof(out)) && strcmp(out,"/")==0);
    assert(strcmp(suite::paths::basename("/SUITE/file.txt"),"file.txt")==0);
    assert(suite::paths::textFile("/A/B/C.TXT"));
    assert(suite::paths::textFile("/a.md"));
    assert(suite::paths::textFile("/a.BAS"));
    assert(!suite::paths::textFile("/a.bin"));
    puts("Paths: PASS");
}
