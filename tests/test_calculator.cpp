#include "../src/core/Calculator.h"
#include <cassert>
#include <cmath>
#include <cstdio>
static void expect(const char *s,double v) {double r=0;assert(calc::evaluate(s,r));assert(std::fabs(r-v)<0.000001);}
static void fail(const char *s) {double r=0;assert(!calc::evaluate(s,r));}
int main() {
    expect("1+2*3",7); expect("(1+2)*3",9); expect("-2+3*(4+1)",13);
    expect(".5+1.25",1.75); expect("8/2/2",2); expect("--5",5);
    fail(""); fail("3/"); fail("1/0"); fail("2+("); fail("hello"); fail("2+2oops");
    puts("Calculator: 12 PASS");
}
