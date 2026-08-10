//
//  asciiart.h
//  asciiplay
//
//  Created by Piotr Panasewicz on 10/08/2026.
//  Copyright © 2026 Codice. All rights reserved.
//
//  Licensed under the MIT licence. See LICENSE in the project root
//  for the full text.
//
// Portable core: glyph shapes, shape matching, colour extraction, ANSI output.
// No OS-specific code lives here -- see platform.h for that.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "glyph_table.h"

enum class ColorMode { None, Ansi256, True };

struct Options {
    std::string path;
    int         cols = 0;        // 0 => fit terminal
    int         rows = 0;
    int         cellW = GLYPH_W; // match resolution; smaller is faster
    int         cellH = GLYPH_H;
    ColorMode   color = ColorMode::True;
    std::string glyphs = "ascii+blocks";
    double      fps = 0;         // 0 => source rate
    double      gamma = 1.0;
    int         equalise = -1;   // -1 => auto (on for mono, off for colour)
    bool        invert = false;  // for light-background terminals
    bool        dither = false;  // Floyd-Steinberg; pairs with --cell 2x4
    bool        cellBg = false;  // also paint the cell background (--bg)
    int         colorTol = 4;    // per-channel slack when merging colour runs
    bool        audio = true;
    bool        loop = false;
    bool        forceImage = false, forceVideo = false;
    int         threads = 0;     // 0 => hardware concurrency
    std::string out;             // dump to a file instead of playing
};

[[noreturn]] void die(const std::string& msg);

std::vector<std::string> splitAny(const std::string& s, const char* seps);

// ------------------------------------------------------------ media probe --

struct MediaInfo {
    int    w = 0, h = 0;
    double fps = 0, duration = 0;
    bool   isVideo = false;
};

MediaInfo probeMedia(const std::string& path);

// argv for the ffmpeg/ffplay children.
std::vector<std::string> decoderArgs(const std::string& path, int pw, int ph,
                                     bool isVideo, double fps, double seek);
std::vector<std::string> audioArgs(const std::string& path, double seek);

// ---------------------------------------------------------------- glyphset --

struct GlyphSet {
    int cw = 0, ch = 0, dim = 0;
    std::vector<std::string> chars;   // UTF-8, one entry per candidate
    std::vector<float> cov;           // n*dim ink coverage, 0..1
    std::vector<float> corr;          // n*dim mean-centred + L2-normalised
    std::vector<float> gn2;           // ||cov_n||^2
    std::vector<float> gsum;          // sum(cov_n)
    int blankIdx = 0, fullIdx = 0;

    int n() const { return (int)chars.size(); }

    static GlyphSet build(const std::string& kind, int cw, int ch);

private:
    void add(const std::string& utf8, const std::vector<float>& mask);
    void addBlocks();
    void addBraille();
    void finalise();
    static std::vector<float> resample(const uint8_t* src, int sw, int sh,
                                       int dw, int dh);
};

// ------------------------------------------------------------------ output --

struct Cell {
    int16_t g = -1;
    uint8_t f[3] = {0, 0, 0}, b[3] = {0, 0, 0};   // truecolour
    uint8_t fi = 0, bi = 0;                       // --color 256 palette slots

    // Comparisons run on what will actually be emitted: under --color 256 two
    // cells that quantise to the same swatch are one run, not two escapes.
    // tol is the per-channel slack allowed when merging neighbours into a run
    // (truecolour only -- 256 merges by palette slot); 0 compares exactly.
    static bool sameColors(const Cell& a, const Cell& b, ColorMode m,
                           int tol, bool cellBg);
    bool sameStyle(const Cell& o, ColorMode m) const;   // exact: frame diffing
};

int rgbTo256(int r, int g, int b);

// Repaints only the cells that changed since the previous frame.
struct Renderer {
    ColorMode mode = ColorMode::True;
    bool      cellBg = false;
    int       tol = 0;
    int cols = 0, rows = 0;
    std::vector<Cell> prev;
    std::string buf;

    void reset(int cols, int rows);
    void draw(const std::vector<Cell>& cur, const GlyphSet& gs);
};

// ------------------------------------------------------------------ engine --

// Turns one decoded RGB frame into a grid of styled cells.
struct Engine {
    const Options&  opt;
    const GlyphSet& gs;
    int cols = 0, rows = 0, pw = 0, ph = 0;
    std::vector<uint8_t> rgb;      // pw*ph*3, filled by the caller
    std::vector<float>   luma;
    std::vector<float>   blocks;
    std::vector<int32_t> idx;
    std::vector<Cell>    cells;
    int threads = 1;

    Engine(const Options& o, const GlyphSet& g);

    void        resize(int cols, int rows);
    std::size_t frameBytes() const { return (std::size_t)pw * ph * 3; }
    void        process();                       // rgb -> cells
    std::string toText(bool withColor) const;    // whole frame, for -o / stdout
};
