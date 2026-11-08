#include "../src/core/InspectorCore.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace suite::inspect;
int main(){
    for(const char* s:{"{}","[]","true","false","null","42","-12.75e+2",
        "{\"a\":[1,2,3],\"b\":true}","{\"escaped\":\"a\\n\\u0041\"}"})
        assert(Json::valid(s,strlen(s)));
    for(const char* s:{"","{","[1,]","{\"a\":}","{\"a\":1,}","01",
         "1e","1.","\"unterminated","\"bad\\q\"","NaN","[true false]",
         "{\"a\":2} extraneous"})
        assert(!Json::valid(s,strlen(s)));
    assert(asciiSearch("Hello World","WORLD"));
    assert(!asciiSearch("Hello World","Moon"));
    assert(!asciiSearch("",""));
    char cols[8][40]={{0}};unsigned n=0;
    const char* csv="\"Cafe, Central\",\"a\"\"b\",-60";
    assert(csvColumns(csv,strlen(csv),cols,n)&&n==3);
    assert(strcmp(cols[0],"Cafe, Central")==0);
    assert(strcmp(cols[1],"a\"b")==0);
    assert(strcmp(cols[2],"-60")==0);
    assert(!csvColumns("\"unterminated",13,cols,n));
    assert(!csvColumns("a,b,c,d,e,f,g,h,i",17,cols,n));
    const char* bad="a,\tbyte";
    assert(!csvColumns(bad,strlen(bad),cols,n));
    puts("Inspector: JSON validation, CSV quoting, bounded search PASS");
}
