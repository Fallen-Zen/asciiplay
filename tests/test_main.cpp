//
//  test_main.cpp
//  asciiplay
//
//  Created by Piotr Panasewicz on 05/10/2026.
//  Copyright © 2026 Codice. All rights reserved.
//
//  Licensed under the MIT licence. See LICENSE in the project root
//  for the full text.
//
// Runs every registered test; exits non-zero if any check failed.  An
// argument runs only the tests whose name contains it.
#include "check.h"

#include <cstdio>
#include <cstring>

namespace check {

namespace {
int g_failures = 0;
const char* g_current = "";
} // namespace

std::vector<Case>& registry() {
    static std::vector<Case> cases;
    return cases;
}

void fail(const char* file, int line, const std::string& what) {
    ++g_failures;
    std::printf("FAIL %s  (%s:%d)\n      %s\n", g_current, file, line, what.c_str());
}

} // namespace check

int main(int argc, char** argv) {
    const char* filter = argc > 1 ? argv[1] : "";
    int ran = 0;
    for (const auto& c : check::registry()) {
        if (!std::strstr(c.name, filter)) continue;
        check::g_current = c.name;
        c.fn();
        ++ran;
    }
    std::printf("%d tests, %d failed checks\n", ran, check::g_failures);
    return check::g_failures == 0 && ran > 0 ? 0 : 1;
}
