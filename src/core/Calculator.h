#pragma once
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
namespace calc {
class Parser {
public:
    explicit Parser(const char *s) : p_(s ? s : ""), ok_(true) {}
    bool evaluate(double &out) {
        out = expression(); skip();
        return ok_ && *p_ == '\0' && std::isfinite(out);
    }
private:
    const char *p_;
    bool ok_;
    void skip() { while (isspace(static_cast<unsigned char>(*p_))) ++p_; }
    bool take(char c) { skip(); if (*p_ == c) { ++p_; return true; } return false; }
    double expression() {
        double v = term();
        while (ok_) {
            if (take('+')) v += term();
            else if (take('-')) v -= term();
            else break;
        }
        return v;
    }
    double term() {
        double v = factor();
        while (ok_) {
            if (take('*')) v *= factor();
            else if (take('/')) {
                double rhs = factor();
                if (rhs == 0.0) { ok_ = false; return 0; }
                v /= rhs;
            } else break;
        }
        return v;
    }
    double factor() {
        if (take('+')) return factor();
        if (take('-')) return -factor();
        if (take('(')) {
            const double v = expression();
            if (!take(')')) ok_ = false;
            return v;
        }
        skip();
        char *end = nullptr;
        const double v = strtod(p_, &end);
        if (end == p_) { ok_ = false; return 0; }
        p_ = end; return v;
    }
};
inline bool evaluate(const char *s, double &out) { Parser parser(s); return parser.evaluate(out); }
}
