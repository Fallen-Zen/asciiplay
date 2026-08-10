//
//  asciiart.cpp
//  asciiplay
//
//  Created by Piotr Panasewicz on 10/08/2026.
//  Copyright © 2026 Codice. All rights reserved.
//
//  Licensed under the MIT licence. See LICENSE in the project root
//  for the full text.
//
#include "asciiart.h"
#include "platform.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

void die(const std::string& msg) {
    plat::terminalLeave();
    std::fprintf(stderr, "asciiplay: %s\n", msg.c_str());
    std::exit(1);
}

std::vector<std::string> splitAny(const std::string& s, const char* seps) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (std::strchr(seps, c) && c != '\0') { out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    out.push_back(cur);
    return out;
}

// ------------------------------------------------------------ media probe --

MediaInfo probeMedia(const std::string& path) {
    // key=value form: stills report nb_frames=N/A and no usable duration, so
    // positional parsing would misread them.  Match on the key instead.
    plat::Proc p = plat::Proc::spawn({"ffprobe", "-v", "error",
        "-select_streams", "v:0",
        "-show_entries", "stream=width,height,r_frame_rate,nb_frames",
        "-show_entries", "format=duration",
        "-of", "default=nw=1", path}, true);
    if (!p.valid())
        die("could not run ffprobe -- is ffmpeg installed and on PATH?");

    std::string s = p.readAll();
    if (s.empty())
        die("ffprobe found no video stream in '" + path + "'");

    MediaInfo m;
    long nbFrames = -1;
    bool haveDuration = false;

    for (const auto& rawLine : splitAny(s, "\n")) {
        std::string line = rawLine;
        if (!line.empty() && line.back() == '\r') line.pop_back();   // CRLF
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq), v = line.substr(eq + 1);
        if (v.empty() || v == "N/A") continue;

        if      (k == "width")     m.w = std::atoi(v.c_str());
        else if (k == "height")    m.h = std::atoi(v.c_str());
        else if (k == "nb_frames") nbFrames = std::atol(v.c_str());
        else if (k == "duration")  { m.duration = std::atof(v.c_str()); haveDuration = true; }
        else if (k == "r_frame_rate") {
            auto fr = splitAny(v, "/");                    // e.g. "30/1"
            double num = std::atof(fr[0].c_str());
            double den = fr.size() > 1 ? std::atof(fr[1].c_str()) : 1.0;
            m.fps = (den > 0) ? num / den : 0;
        }
    }
    if (m.w <= 0 || m.h <= 0)
        die("could not read dimensions from '" + path + "'");

    // Video if it has more than one frame, or a duration long enough to play.
    // A JPEG reports duration=0.04 (one frame at the default 25fps); a PNG
    // reports nothing at all.
    if (nbFrames > 1)                          m.isVideo = true;
    else if (nbFrames == 1)                    m.isVideo = false;
    else if (haveDuration && m.duration > 0.5) m.isVideo = true;
    else                                       m.isVideo = false;

    if (m.fps <= 0 || m.fps > 1000) m.fps = 25.0;
    return m;
}

static std::string num(double v) {
    char t[64];
    std::snprintf(t, sizeof t, "%.6f", v);
    return t;
}

std::vector<std::string> decoderArgs(const std::string& path, int pw, int ph,
                                     bool isVideo, double fps, double seek) {
    // -nostdin matters: without it ffmpeg polls the terminal for interactive
    // keys, and in its own process group that raises SIGTTIN and stops it dead.
    std::vector<std::string> a = {"ffmpeg", "-nostdin", "-v", "error"};
    if (seek > 0.01) { a.push_back("-ss"); a.push_back(num(seek)); }
    a.push_back("-i"); a.push_back(path);

    std::string vf = "scale=" + std::to_string(pw) + ":" + std::to_string(ph)
                   + ":flags=bilinear";
    if (isVideo && fps > 0) vf += ",fps=" + num(fps);
    a.push_back("-vf"); a.push_back(vf);

    if (!isVideo) { a.push_back("-frames:v"); a.push_back("1"); }
    a.push_back("-f");       a.push_back("rawvideo");
    a.push_back("-pix_fmt"); a.push_back("rgb24");
    a.push_back("-");
    return a;
}

std::vector<std::string> audioArgs(const std::string& path, double seek) {
    std::vector<std::string> a = {"ffplay", "-nodisp", "-vn", "-autoexit",
                                  "-loglevel", "quiet"};
    if (seek > 0.01) { a.push_back("-ss"); a.push_back(num(seek)); }
    a.push_back(path);
    return a;
}

// ---------------------------------------------------------------- glyphset --

void GlyphSet::add(const std::string& utf8, const std::vector<float>& mask) {
    chars.push_back(utf8);
    cov.insert(cov.end(), mask.begin(), mask.end());
}

std::vector<float> GlyphSet::resample(const uint8_t* src, int sw, int sh,
                                      int dw, int dh) {
    std::vector<float> out((std::size_t)dw * dh, 0.f);
    for (int y = 0; y < dh; ++y) {
        int y0 = y * sh / dh, y1 = std::max(y0 + 1, (y + 1) * sh / dh);
        for (int x = 0; x < dw; ++x) {
            int x0 = x * sw / dw, x1 = std::max(x0 + 1, (x + 1) * sw / dw);
            float acc = 0; int cnt = 0;
            for (int yy = y0; yy < y1; ++yy)
                for (int xx = x0; xx < x1; ++xx) { acc += src[yy * sw + xx]; ++cnt; }
            out[(std::size_t)y * dw + x] = acc / (255.f * (float)cnt);
        }
    }
    return out;
}

void GlyphSet::finalise() {
    dim = cw * ch;
    const int N = n();
    corr.assign((std::size_t)N * dim, 0.f);
    gn2.assign(N, 0.f);
    gsum.assign(N, 0.f);

    float leastInk = 1e9f, mostInk = -1e9f;
    for (int i = 0; i < N; ++i) {
        const float* g = &cov[(std::size_t)i * dim];
        float sum = 0, n2 = 0;
        for (int d = 0; d < dim; ++d) { sum += g[d]; n2 += g[d] * g[d]; }
        gsum[i] = sum;
        gn2[i]  = n2;

        float mean = sum / (float)dim, ss = 0;
        for (int d = 0; d < dim; ++d) { float t = g[d] - mean; ss += t * t; }
        float inv = ss > 1e-9f ? 1.0f / std::sqrt(ss) : 0.f;
        float* c = &corr[(std::size_t)i * dim];
        for (int d = 0; d < dim; ++d) c[d] = (g[d] - mean) * inv;

        if (sum < leastInk) { leastInk = sum; blankIdx = i; }
        if (sum > mostInk)  { mostInk  = sum; fullIdx  = i; }
    }
}

void GlyphSet::addBlocks() {
    auto box = [&](const char* s, float x0, float y0, float x1, float y1) {
        std::vector<float> m((std::size_t)cw * ch, 0.f);
        for (int y = 0; y < ch; ++y)
            for (int x = 0; x < cw; ++x) {
                // fraction of this subpixel covered by the rectangle
                float px0 = (float)x / cw, px1 = (float)(x + 1) / cw;
                float py0 = (float)y / ch, py1 = (float)(y + 1) / ch;
                float ox = std::max(0.f, std::min(px1, x1) - std::max(px0, x0));
                float oy = std::max(0.f, std::min(py1, y1) - std::max(py0, y0));
                m[(std::size_t)y * cw + x] = ox * oy / ((px1 - px0) * (py1 - py0));
            }
        add(s, m);
    };
    auto flat = [&](const char* s, float v) {
        add(s, std::vector<float>((std::size_t)cw * ch, v));
    };

    flat(" ", 0.f);
    flat("█", 1.f);                                        // full block
    flat("░", .25f); flat("▒", .5f); flat("▓", .75f);  // shades
    box("▀", 0, 0, 1, .5f);   box("▄", 0, .5f, 1, 1);       // halves
    box("▌", 0, 0, .5f, 1);   box("▐", .5f, 0, 1, 1);

    struct Q { const char* s; int m; };
    static const Q qs[] = {
        {"▘", 0b0001}, {"▝", 0b0010}, {"▖", 0b0100},
        {"▗", 0b1000}, {"▚", 0b1001}, {"▞", 0b0110},
        {"▛", 0b0111}, {"▜", 0b1011}, {"▙", 0b1101},
        {"▟", 0b1110},
    };
    for (const auto& q : qs) {
        std::vector<float> m((std::size_t)cw * ch, 0.f);
        for (int y = 0; y < ch; ++y)
            for (int x = 0; x < cw; ++x) {
                int bit = (y < ch / 2 ? 0 : 2) + (x < cw / 2 ? 0 : 1);
                m[(std::size_t)y * cw + x] = (float)((q.m >> bit) & 1);
            }
        add(q.s, m);
    }

    // eighth-blocks: bars growing from the bottom, and from the left
    static const char* lower[] = {"▁","▂","▃","▅","▆","▇"};
    static const char* left[]  = {"▏","▎","▍","▋","▊","▉"};
    static const int   eighth[] = {1, 2, 3, 5, 6, 7};
    for (int k = 0; k < 6; ++k) {
        box(lower[k], 0, 1.f - eighth[k] / 8.f, 1, 1);
        box(left[k],  0, 0, eighth[k] / 8.f, 1);
    }
}

// Braille gives the densest subpixel grid available in a character cell: 2
// across by 4 down, all 256 combinations, still two colours per cell.  The
// masks are idealised (a dot is a filled quarter-by-eighth of the cell) rather
// than the font's actual round dots, because what we want is a 2x4 pixel grid.
void GlyphSet::addBraille() {
    // Unicode orders the dots 1,2,3,7 down the left column and 4,5,6,8 down
    // the right, which is not row-major -- hence the explicit table.
    static const int bitOf[4][2] = {   // [y][x] -> bit index
        {0, 3},
        {1, 4},
        {2, 5},
        {6, 7},
    };
    for (int m = 0; m < 256; ++m) {
        std::vector<float> mask((std::size_t)cw * ch, 0.f);
        for (int y = 0; y < ch; ++y)
            for (int x = 0; x < cw; ++x) {
                int sy = y * 4 / ch, sx = x * 2 / cw;      // which of the 2x4
                mask[(std::size_t)y * cw + x] =
                    (float)((m >> bitOf[sy][sx]) & 1);
            }
        unsigned cp = 0x2800u + (unsigned)m;               // always 3-byte UTF-8
        char u[4] = {
            (char)(0xE0 | (cp >> 12)),
            (char)(0x80 | ((cp >> 6) & 0x3F)),
            (char)(0x80 | (cp & 0x3F)),
            '\0'
        };
        add(u, mask);
    }
}

GlyphSet GlyphSet::build(const std::string& kind, int cw, int ch) {
    GlyphSet gs;
    gs.cw = cw; gs.ch = ch;

    bool wantAscii   = kind.find("ascii")   != std::string::npos;
    bool wantBlocks  = kind.find("blocks")  != std::string::npos;
    bool wantBraille = kind.find("braille") != std::string::npos;
    if (!wantAscii && !wantBlocks && !wantBraille)
        die("--glyphs must name at least one of: ascii, blocks, braille");

    if (wantAscii)
        for (int i = 0; i < GLYPH_N; ++i)
            gs.add(std::string(1, (char)(GLYPH_FIRST + i)),
                   resample(GLYPH_COVERAGE[i], GLYPH_W, GLYPH_H, cw, ch));
    if (wantBlocks)  gs.addBlocks();
    if (wantBraille) gs.addBraille();

    gs.finalise();
    return gs;
}

// ----------------------------------------------------------------- matcher --

// Both criteria reduce to a dot product against the glyph matrix:
//
//   mono  : minimise ||b - g||^2  ==  minimise  ||g||^2 - 2 b.g
//   colour: maximise the correlation between b and g.  Because the glyph rows
//           are mean-centred, sum(corr_n) == 0, so the block's own mean drops
//           out of the correlation and we can maximise b.corr_n directly --
//           no per-cell preprocessing at all.
static void matchRange(const GlyphSet& gs, const float* B, int32_t* out,
                       int first, int last, bool mono) {
    const int dim = gs.dim, N = gs.n();
    const float* G = mono ? gs.cov.data() : gs.corr.data();

    for (int i = first; i < last; ++i) {
        const float* b = B + (std::size_t)i * dim;

        if (!mono) {   // a flat block has no correlation signal: use a solid
            float mn = b[0], mx = b[0];
            for (int d = 1; d < dim; ++d) { mn = std::min(mn, b[d]); mx = std::max(mx, b[d]); }
            if (mx - mn < 1e-4f) { out[i] = gs.blankIdx; continue; }
        }

        int   best = 0;
        float bestScore = mono ? 1e30f : -1e30f;
        for (int k = 0; k < N; ++k) {
            const float* g = G + (std::size_t)k * dim;
            float dot = 0;
            for (int d = 0; d < dim; ++d) dot += b[d] * g[d];
            float sc = mono ? (gs.gn2[k] - 2.f * dot) : dot;
            if (mono ? (sc < bestScore) : (sc > bestScore)) { bestScore = sc; best = k; }
        }
        out[i] = best;
    }
}

static void matchAll(const GlyphSet& gs, const float* B, int32_t* out,
                     int cells, bool mono, int threads) {
    if (threads <= 1 || cells < 512) {
        matchRange(gs, B, out, 0, cells, mono);
        return;
    }
    std::vector<std::thread> pool;
    int chunk = (cells + threads - 1) / threads;
    for (int t = 0; t < threads; ++t) {
        int a = t * chunk, b = std::min(cells, a + chunk);
        if (a >= b) break;
        pool.emplace_back(matchRange, std::cref(gs), B, out, a, b, mono);
    }
    for (auto& th : pool) th.join();
}

// ------------------------------------------------------------------ output --

static bool nearRgb(const uint8_t* x, const uint8_t* y, int tol) {
    return std::abs(x[0] - y[0]) <= tol
        && std::abs(x[1] - y[1]) <= tol
        && std::abs(x[2] - y[2]) <= tol;
}

bool Cell::sameColors(const Cell& a, const Cell& b, ColorMode m,
                      int tol, bool cellBg) {
    if (m == ColorMode::None)    return true;
    if (m == ColorMode::Ansi256) return a.fi == b.fi && (!cellBg || a.bi == b.bi);
    return nearRgb(a.f, b.f, tol) && (!cellBg || nearRgb(a.b, b.b, tol));
}

// Frame diffing has to be exact.  The run tolerance is measured against the
// colour the terminal is actually holding, which is re-anchored every frame, so
// the on-screen error stays inside tol instead of drifting frame over frame.
bool Cell::sameStyle(const Cell& o, ColorMode m) const {
    return g == o.g && sameColors(*this, o, m, 0, true);
}

int rgbTo256(int r, int g, int b) {
    // the grey ramp beats the colour cube for near-neutral pixels
    if (std::abs(r - g) < 8 && std::abs(g - b) < 8 && std::abs(r - b) < 8) {
        int lvl = (r * 299 + g * 587 + b * 114) / 1000;
        if (lvl < 8)   return 16;
        if (lvl > 247) return 231;
        return 232 + (lvl - 8) * 24 / 240;
    }
    auto q = [](int v) { return (v * 5 + 127) / 255; };
    return 16 + 36 * q(r) + 6 * q(g) + q(b);
}

// Only the ink is coloured by default; --bg adds the second colour that fills
// the cell around it.  Nothing here emits SGR 49 to clear the background --
// every write already ends in SGR 0, so one reset per frame does that job (see
// draw and toText).  At ~10k cells a frame those five bytes were a quarter of
// the output.
static void appendStyle(std::string& buf, ColorMode mode, const Cell& c,
                        bool cellBg) {
    char t[64];
    if (mode == ColorMode::True) {
        if (cellBg) std::snprintf(t, sizeof t, "\x1b[38;2;%d;%d;%dm\x1b[48;2;%d;%d;%dm",
                                  c.f[0], c.f[1], c.f[2], c.b[0], c.b[1], c.b[2]);
        else        std::snprintf(t, sizeof t, "\x1b[38;2;%d;%d;%dm",
                                  c.f[0], c.f[1], c.f[2]);
    } else if (mode == ColorMode::Ansi256) {
        if (cellBg) std::snprintf(t, sizeof t, "\x1b[38;5;%dm\x1b[48;5;%dm", c.fi, c.bi);
        else        std::snprintf(t, sizeof t, "\x1b[38;5;%dm", c.fi);
    } else return;
    buf += t;
}

void Renderer::reset(int c, int r) {
    cols = c; rows = r;
    prev.assign((std::size_t)c * r, Cell{});
    for (auto& p : prev) p.g = -2;          // force a full first paint
}

void Renderer::draw(const std::vector<Cell>& cur, const GlyphSet& gs) {
    buf.clear();
    int curX = -99, curY = -99;
    bool styled = false;
    Cell style{};

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            std::size_t i = (std::size_t)y * cols + x;
            if (cur[i].sameStyle(prev[i], mode)) continue;

            if (curY != y || curX != x) {
                char t[32];
                std::snprintf(t, sizeof t, "\x1b[%d;%dH", y + 1, x + 1);
                buf += t;
                curX = x; curY = y;
            }
            if (mode != ColorMode::None &&
                (!styled || !Cell::sameColors(style, cur[i], mode, tol, cellBg))) {
                appendStyle(buf, mode, cur[i], cellBg);
                style = cur[i];
                styled = true;
            }
            buf += gs.chars[cur[i].g];
            ++curX;
        }
    }
    if (!buf.empty()) {
        if (mode != ColorMode::None) {
            // One reset in front covers whatever the terminal was left in, and
            // the trailing one puts it back -- so the ink-only path never has
            // to restate the background per cell.
            if (!cellBg) buf.insert(0, "\x1b[49m");
            buf += "\x1b[0m";
        }
        plat::writeOut(buf);
    }
    prev = cur;
}

// ------------------------------------------------------------------ engine --

Engine::Engine(const Options& o, const GlyphSet& g) : opt(o), gs(g) {
    threads = opt.threads > 0
            ? opt.threads
            : (int)std::max(1u, std::thread::hardware_concurrency());
}

void Engine::resize(int c, int r) {
    cols = c; rows = r;
    pw = cols * gs.cw; ph = rows * gs.ch;
    rgb.assign((std::size_t)pw * ph * 3, 0);
    luma.assign((std::size_t)pw * ph, 0.f);
    blocks.assign((std::size_t)cols * rows * gs.dim, 0.f);
    idx.assign((std::size_t)cols * rows, 0);
    cells.assign((std::size_t)cols * rows, Cell{});
}

void Engine::process() {
    const std::size_t n = (std::size_t)pw * ph;

    for (std::size_t i = 0; i < n; ++i)
        luma[i] = 0.2126f * rgb[i * 3] + 0.7152f * rgb[i * 3 + 1]
                + 0.0722f * rgb[i * 3 + 2];

    // Tone mapping only steers glyph choice -- colours are read from the raw
    // RGB below, so equalising here never shifts the palette.
    // Without a cell background the ink carries the tone, exactly as in mono,
    // so equalisation earns its keep there too.
    const bool inkOnly = opt.color == ColorMode::None || !opt.cellBg;
    bool eq = opt.equalise >= 0 ? (opt.equalise != 0) : inkOnly;
    if (eq) {
        uint32_t hist[256] = {0};
        for (std::size_t i = 0; i < n; ++i)
            hist[(int)std::clamp(luma[i], 0.f, 255.f)]++;
        float cdf[256]; uint32_t run = 0;
        for (int i = 0; i < 256; ++i) { run += hist[i]; cdf[i] = (float)run / (float)n; }
        for (std::size_t i = 0; i < n; ++i)
            luma[i] = cdf[(int)std::clamp(luma[i], 0.f, 255.f)];
    } else {
        float mn = 1e9f, mx = -1e9f;
        for (std::size_t i = 0; i < n; ++i) { mn = std::min(mn, luma[i]); mx = std::max(mx, luma[i]); }
        float sc = (mx - mn) > 1e-6f ? 1.0f / (mx - mn) : 0.f;
        for (std::size_t i = 0; i < n; ++i) luma[i] = (luma[i] - mn) * sc;
    }
    if (opt.gamma != 1.0)
        for (std::size_t i = 0; i < n; ++i) luma[i] = std::pow(luma[i], (float)opt.gamma);
    if (opt.invert)
        for (std::size_t i = 0; i < n; ++i) luma[i] = 1.f - luma[i];

    // Error diffusion.  Glyph sets whose subpixels are strictly on/off (braille
    // above all) cannot render a mid grey inside one cell, so without this a
    // flat area snaps to all-dots or no-dots and the picture goes blotchy.
    // Diffusing the rounding error into the neighbours trades spatial noise for
    // apparent tone.  Most useful at --cell 2x4, where one luma sample is
    // exactly one braille dot.
    if (opt.dither) {
        for (int y = 0; y < ph; ++y)
            for (int x = 0; x < pw; ++x) {
                std::size_t i = (std::size_t)y * pw + x;
                float old = luma[i];
                float neu = old > 0.5f ? 1.f : 0.f;
                luma[i] = neu;
                float e = old - neu;
                if (x + 1 < pw) luma[i + 1] += e * 7.f / 16.f;
                if (y + 1 < ph) {
                    if (x > 0)      luma[i + pw - 1] += e * 3.f / 16.f;
                    luma[i + pw] += e * 5.f / 16.f;
                    if (x + 1 < pw) luma[i + pw + 1] += e * 1.f / 16.f;
                }
            }
    }

    // image -> one contiguous vector per cell
    const int cw = gs.cw, ch = gs.ch, dim = gs.dim;
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c) {
            float* dst = &blocks[((std::size_t)r * cols + c) * dim];
            for (int y = 0; y < ch; ++y)
                std::memcpy(dst + (std::size_t)y * cw,
                            &luma[(std::size_t)(r * ch + y) * pw + c * cw],
                            sizeof(float) * cw);
        }

    // The correlation criterion only ranks shapes, and answers "flat block" with
    // a blank glyph because the background was going to carry the colour.  With
    // no background that would erase every flat area, so match on absolute ink
    // instead: bright blocks fill, dark blocks empty.
    matchAll(gs, blocks.data(), idx.data(), cols * rows, inkOnly, threads);

    // foreground = mean colour under the ink, background = mean colour behind it
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c) {
            std::size_t ci = (std::size_t)r * cols + c;
            int gi = idx[ci];
            Cell& out = cells[ci];
            out.g = (int16_t)gi;
            if (opt.color == ColorMode::None) continue;

            const float* m = &gs.cov[(std::size_t)gi * dim];

            if (!opt.cellBg) {
                // Ink colour only.  Inkless cells keep a black foreground so
                // whole empty regions share one style and cost one escape.
                float w = 0, a[3] = {0, 0, 0};
                for (int y = 0; y < ch; ++y) {
                    const uint8_t* px = &rgb[((std::size_t)(r * ch + y) * pw + c * cw) * 3];
                    for (int x = 0; x < cw; ++x) {
                        float cvg = m[(std::size_t)y * cw + x];
                        w += cvg;
                        for (int k = 0; k < 3; ++k) a[k] += cvg * px[x * 3 + k];
                    }
                }
                for (int k = 0; k < 3; ++k) {
                    out.f[k] = w > 1e-4f
                             ? (uint8_t)std::clamp((int)std::lround(a[k] / w), 0, 255)
                             : 0;
                    out.b[k] = 0;
                }
                if (opt.color == ColorMode::Ansi256)
                    out.fi = (uint8_t)rgbTo256(out.f[0], out.f[1], out.f[2]);
                continue;
            }

            float wf = 0, wb = 0, af[3] = {0, 0, 0}, ab[3] = {0, 0, 0};
            for (int y = 0; y < ch; ++y) {
                const uint8_t* px = &rgb[((std::size_t)(r * ch + y) * pw + c * cw) * 3];
                for (int x = 0; x < cw; ++x) {
                    float a = m[(std::size_t)y * cw + x], ia = 1.f - a;
                    wf += a; wb += ia;
                    for (int k = 0; k < 3; ++k) {
                        af[k] += a  * px[x * 3 + k];
                        ab[k] += ia * px[x * 3 + k];
                    }
                }
            }
            // A glyph with no ink (or all ink) leaves one colour undefined;
            // fall back to the block mean so the cell still reads correctly.
            float mean[3] = {0, 0, 0};
            if (wf < 1e-4f || wb < 1e-4f) {
                for (int y = 0; y < ch; ++y) {
                    const uint8_t* px = &rgb[((std::size_t)(r * ch + y) * pw + c * cw) * 3];
                    for (int x = 0; x < cw; ++x)
                        for (int k = 0; k < 3; ++k) mean[k] += px[x * 3 + k];
                }
                for (int k = 0; k < 3; ++k) mean[k] /= (float)(cw * ch);
            }
            for (int k = 0; k < 3; ++k) {
                float f = wf > 1e-4f ? af[k] / wf : mean[k];
                float b = wb > 1e-4f ? ab[k] / wb : mean[k];
                out.f[k] = (uint8_t)std::clamp((int)std::lround(f), 0, 255);
                out.b[k] = (uint8_t)std::clamp((int)std::lround(b), 0, 255);
            }
            if (opt.color == ColorMode::Ansi256) {
                out.fi = (uint8_t)rgbTo256(out.f[0], out.f[1], out.f[2]);
                out.bi = (uint8_t)rgbTo256(out.b[0], out.b[1], out.b[2]);
            }
        }
}

std::string Engine::toText(bool withColor) const {
    std::string s;
    Cell style{};
    bool styled = false;
    // Every line ends in SGR 0, so one leading reset is all the ink-only path
    // needs to be sure of the background.
    if (withColor && !opt.cellBg) s += "\x1b[49m";
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            const Cell& cell = cells[(std::size_t)r * cols + c];
            if (withColor && (!styled ||
                !Cell::sameColors(style, cell, opt.color, opt.colorTol, opt.cellBg))) {
                appendStyle(s, opt.color, cell, opt.cellBg);
                style = cell;
                styled = true;
            }
            s += gs.chars[cell.g];
        }
        if (withColor) { s += "\x1b[0m"; styled = false; }
        s += '\n';
    }
    return s;
}
