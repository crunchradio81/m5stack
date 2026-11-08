#pragma once
#include <stddef.h>
#include <string.h>
namespace suite { namespace paths {
constexpr size_t MaxPath = 96;
inline bool validPart(const char *p) {
    if (!p || !*p || strcmp(p, ".") == 0 || strcmp(p, "..") == 0) return false;
    const size_t n = strlen(p);
    if (n > 60) return false;
    for (size_t i=0;i<n;++i) {
        const unsigned char c = static_cast<unsigned char>(p[i]);
        if (c=='/' || c=='\\' || c<32 || c>126) return false;
    }
    return true;
}
inline bool join(const char *dir, const char *name, char *out, size_t cap) {
    if (!dir || dir[0]!='/' || !validPart(name) || !out) return false;
    const size_t a=strlen(dir), b=strlen(name);
    if (!a || a>=MaxPath || (a>1 && dir[a-1]=='/')) return false;
    const size_t n=a+(a==1?0:1)+b;
    if (n>=cap || n>=MaxPath) return false;
    memcpy(out,dir,a); size_t p=a;
    if (a>1) out[p++]='/';
    memcpy(out+p,name,b+1);
    return true;
}
inline bool parent(const char *path, char *out, size_t cap) {
    if (!path || !out || path[0]!='/' || cap<2) return false;
    size_t n=strlen(path);
    if (!n || n>=MaxPath || n>=cap) return false;
    if (n==1) { out[0]='/'; out[1]=0; return true; }
    while (n>1 && path[n-1]!='/') --n;
    if (n>1) --n;
    memcpy(out,path,n); out[n]=0; return true;
}
inline const char* basename(const char *p) {
    if (!p) return "";
    const char *s=strrchr(p,'/');
    return s?s+1:p;
}
inline bool textFile(const char *path) {
    const char *s=strrchr(basename(path), '.');
    if (!s) return false;
    const char *exts[]={".txt",".md",".log",".ini",".csv",".bas"};
    for (const char *ext:exts) {
        if (strlen(s)!=strlen(ext)) continue;
        bool eq=true;
        for (size_t i=0;s[i];++i) {
            char c=s[i];
            if (c>='A' && c<='Z') c=static_cast<char>(c-'A'+'a');
            if (c!=ext[i]) { eq=false;break; }
        }
        if (eq) return true;
    }
    return false;
}
}}
