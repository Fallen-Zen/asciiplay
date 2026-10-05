//
//  test_decoder.cpp
//  asciiplay
//
//  Created by Piotr Panasewicz on 05/10/2026.
//  Copyright © 2026 Codice. All rights reserved.
//
//  Licensed under the MIT licence. See LICENSE in the project root
//  for the full text.
//
// The ffmpeg and ffplay command lines, and where a seek goes in them.
#include "check.h"
#include "asciiart.h"

#include <algorithm>

namespace {

using Args = std::vector<std::string>;

int at(const Args& a, const std::string& s) {
    auto it = std::find(a.begin(), a.end(), s);
    return it == a.end() ? -1 : (int)(it - a.begin());
}

} // namespace

TEST(decoder_no_seek_from_the_start) {
    Args a = decoderArgs("in.mp4", 160, 90, true, 24, 0);
    CHECK_EQ(at(a, "-ss"), -1);
    CHECK_EQ(a.front(), "ffmpeg");
    CHECK(at(a, "-nostdin") > 0);                 // never reads the terminal
    CHECK_EQ(a[at(a, "-i") + 1], "in.mp4");
}

TEST(decoder_ignores_a_negligible_seek) {
    CHECK_EQ(at(decoderArgs("in.mp4", 16, 9, true, 24, 0.005), "-ss"), -1);
}

TEST(decoder_seeks_on_input_by_default) {
    Args a = decoderArgs("in.mp4", 160, 90, true, 24, 12.5);
    CHECK(at(a, "-ss") >= 0);
    CHECK(at(a, "-ss") < at(a, "-i"));            // jumps straight there
    CHECK_EQ(a[at(a, "-ss") + 1], "12.500000");
}

TEST(decoder_seeks_by_decoding_when_asked) {
    Args a = decoderArgs("in.h264", 160, 90, true, 24, 12.5, true);
    CHECK(at(a, "-ss") > at(a, "-i"));            // decodes up to it
    CHECK_EQ(a[at(a, "-ss") + 1], "12.500000");
}

TEST(decoder_scales_and_paces_video) {
    Args a = decoderArgs("in.mp4", 160, 90, true, 25, 0);
    CHECK_EQ(a[at(a, "-vf") + 1], "scale=160:90:flags=bilinear,fps=25.000000");
    CHECK_EQ(a.back(), "-");
}

TEST(decoder_still_reads_one_frame) {
    Args a = decoderArgs("in.png", 160, 90, false, 0, 0);
    CHECK_EQ(a[at(a, "-frames:v") + 1], "1");
}

TEST(audio_follows_the_seek) {
    Args a = audioArgs("in.mp4", 0);
    CHECK_EQ(at(a, "-ss"), -1);
    a = audioArgs("in.mp4", 30);
    CHECK_EQ(a[at(a, "-ss") + 1], "30.000000");
    CHECK_EQ(a.back(), "in.mp4");
}
