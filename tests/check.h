//
//  check.h
//  asciiplay
//
//  Created by Piotr Panasewicz on 05/10/2026.
//  Copyright © 2026 Codice. All rights reserved.
//
//  Licensed under the MIT licence. See LICENSE in the project root
//  for the full text.
//
// A minimal test runner, so the tests link nothing the player does not.
//
//   TEST(name) { CHECK(cond); CHECK_EQ(actual, expected); }
//
// A failed check records the file, line and both values and carries on, so
// one run reports every failure.
#pragma once

#include <cmath>
#include <sstream>
#include <string>
#include <vector>

namespace check {

struct Case {
    const char* name;
    void (*fn)();
};

std::vector<Case>& registry();
void fail(const char* file, int line, const std::string& what);

struct Register {
    Register(const char* name, void (*fn)()) { registry().push_back({name, fn}); }
};

template <class T>
std::string show(const T& v) {
    std::ostringstream o;
    o << v;
    return o.str();
}

// Escape sequences are most of what these tests compare; print them readably.
inline std::string show(const std::string& v) {
    std::string out = "\"";
    for (unsigned char c : v) {
        if (c == 0x1b)              out += "\\e";
        else if (c < 0x20)          out += "\\x" + show((int)c);
        else                        out += (char)c;
    }
    return out + "\"";
}
inline std::string show(const char* v) { return show(std::string(v)); }

} // namespace check

#define TEST(name)                                                     \
    static void name();                                                \
    static const check::Register name##_registered(#name, name);       \
    static void name()

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) check::fail(__FILE__, __LINE__, #cond);           \
    } while (0)

#define CHECK_EQ(actual, expected)                                     \
    do {                                                               \
        const auto a_ = (actual);      /* copies: either may be a */  \
        const auto e_ = (expected);    /* temporary's member      */  \
        if (!(a_ == e_))                                               \
            check::fail(__FILE__, __LINE__,                            \
                        #actual " == " #expected "\n      got      "  \
                        + check::show(a_) + "\n      expected "       \
                        + check::show(e_));                            \
    } while (0)

#define CHECK_NEAR(actual, expected, eps)                              \
    do {                                                               \
        const double a_ = (actual), e_ = (expected);                   \
        if (!(std::fabs(a_ - e_) <= (eps)))                            \
            check::fail(__FILE__, __LINE__,                            \
                        #actual " ~= " #expected "\n      got      "  \
                        + check::show(a_) + "\n      expected "       \
                        + check::show(e_));                            \
    } while (0)
