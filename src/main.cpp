//
//  main.cpp
//  asciiplay
//
//  Created by Piotr Panasewicz on 10/08/2026.
//  Copyright © 2026 Codice. All rights reserved.
//
//  Licensed under the MIT licence. See LICENSE in the project root
//  for the full text.
//
// asciiplay -- render images and play video as ASCII art in the terminal.
//
// Characters are chosen by shape matching: every candidate glyph is rasterised
// from a real monospace font into an 8x16 coverage map (baked into
// glyph_table.h by gen_glyphs.py), and for each block of the source image we
// pick the glyph whose ink pattern best fits.  That reproduces edges and
// structure, not just average brightness, which is what a luminance ramp
// throws away.
//
// Decoding is delegated to ffmpeg over a pipe, so anything ffmpeg reads works
// and there is nothing to link against.

#include "asciiart.h"
#include "platform.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

void usage() {
    std::puts(
"asciiplay -- shape-matched ASCII art from images and video\n"
"\n"
"  asciiplay FILE [options]\n"
"\n"
"Geometry\n"
"  -c, --cols N       output width in characters (default: fit terminal)\n"
"  -r, --rows N       output height in characters\n"
"      --cell WxH     match resolution per cell (default 8x16; 4x8 is ~4x faster)\n"
"\n"
"Look\n"
"      --ascii        classic ASCII art: letters only, no colour, gamma 1.4\n"
"      --glyphs SET   any mix of ascii, blocks, braille, joined by +\n"
"                     (default ascii+blocks; braille = densest, 2x4 per cell)\n"
"      --color MODE   true | 256 | none              (default true)\n"
"      --color-tol N  merge neighbouring colours within N per channel into one\n"
"                     escape; the lever for terminal-bound playback (default 4)\n"
"      --gamma G      tone curve, >1 darkens midtones (default 1.0)\n"
"      --eq / --no-eq histogram equalisation (default: on unless --bg)\n"
"      --invert       for light-background terminals\n"
"      --dither       Floyd-Steinberg; use with --glyphs braille --cell 2x4\n"
"      --bg / --no-bg fill the cell background with a second colour\n"
"                     (default off: only the glyph ink is coloured)\n"
"\n"
"Playback\n"
"      --fps N        override frame rate\n"
"      --no-audio     do not spawn ffplay\n"
"      --verbose      show ffmpeg/ffplay errors (use with --no-audio or a still)\n"
"      --loop         repeat until quit\n"
"  -j, --threads N    matcher threads (default: all cores)\n"
"\n"
"Output\n"
"  -o, --out FILE     write to a file instead of playing (video: first frame)\n"
"      --image        treat the input as a still\n"
"      --video        treat the input as video\n"
"\n"
"Keys during playback:  q / Esc quit    space pause\n"
"\n"
"Requires ffmpeg, ffprobe and (for sound) ffplay on PATH.\n");
}

// A terminal cell is about twice as tall as it is wide, so the character
// aspect (cellW:cellH) is what keeps the picture from being stretched.
void pickGrid(const Options& opt, const MediaInfo& mi, const GlyphSet& gs,
              bool isVideo, int& cols, int& rows) {
    int tc = 80, tr = 24;
    plat::terminalSize(tc, tr);

    const double srcAR  = (double)mi.h / (double)mi.w;    // height / width
    const double cellAR = (double)gs.ch / (double)gs.cw;

    if (opt.cols > 0 && opt.rows > 0) { cols = opt.cols; rows = opt.rows; return; }
    if (opt.cols > 0) {
        cols = opt.cols;
        rows = std::max(1, (int)std::lround(cols * srcAR / cellAR));
        return;
    }
    if (opt.rows > 0) {
        rows = opt.rows;
        cols = std::max(1, (int)std::lround(rows * cellAR / srcAR));
        return;
    }
    cols = tc;
    rows = std::max(1, (int)std::lround(cols * srcAR / cellAR));
    // Video has to fit on screen; a still printed to stdout may scroll.
    if (isVideo && rows > tr) {
        rows = tr;
        cols = std::max(1, (int)std::lround(rows * cellAR / srcAR));
    }
}

Options parseArgs(int argc, char** argv) {
    Options opt;
    std::vector<std::string> args(argv + 1, argv + argc);

    auto need = [&](std::size_t& i, const char* what) -> std::string {
        if (i + 1 >= args.size()) die(std::string("missing value for ") + what);
        return args[++i];
    };

    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& a = args[i];
        if      (a == "-h" || a == "--help")    { usage(); std::exit(0); }
        else if (a == "-c" || a == "--cols")    opt.cols = std::atoi(need(i, "--cols").c_str());
        else if (a == "-r" || a == "--rows")    opt.rows = std::atoi(need(i, "--rows").c_str());
        else if (a == "--cell") {
            auto p = splitAny(need(i, "--cell"), "xX");
            if (p.size() != 2) die("--cell wants WxH, e.g. 4x8");
            opt.cellW = std::atoi(p[0].c_str());
            opt.cellH = std::atoi(p[1].c_str());
            if (opt.cellW < 1 || opt.cellH < 1 ||
                opt.cellW > GLYPH_W || opt.cellH > GLYPH_H)
                die("--cell must be between 1x1 and 8x16");
        }
        else if (a == "--glyphs") opt.glyphs = need(i, "--glyphs");
        else if (a == "--color" || a == "--colour") {
            std::string v = need(i, "--color");
            if      (v == "true" || v == "24")   opt.color = ColorMode::True;
            else if (v == "256"  || v == "8")    opt.color = ColorMode::Ansi256;
            else if (v == "none" || v == "mono") opt.color = ColorMode::None;
            else die("--color wants true, 256 or none");
        }
        else if (a == "--gamma")     opt.gamma = std::atof(need(i, "--gamma").c_str());
        else if (a == "--eq")        opt.equalise = 1;
        else if (a == "--no-eq")     opt.equalise = 0;
        else if (a == "--invert")    opt.invert = true;
        else if (a == "--dither")    opt.dither = true;
        else if (a == "--color-tol") {
            opt.colorTol = std::atoi(need(i, "--color-tol").c_str());
            if (opt.colorTol < 0 || opt.colorTol > 255)
                die("--color-tol wants 0..255");
        }
        else if (a == "--bg")        opt.cellBg = true;
        else if (a == "--no-bg")     opt.cellBg = false;
        else if (a == "--ascii") {          // shortcut: classic ASCII art
            opt.glyphs = "ascii";
            opt.color  = ColorMode::None;
            if (opt.gamma == 1.0) opt.gamma = 1.4;
        }
        else if (a == "--fps")       opt.fps = std::atof(need(i, "--fps").c_str());
        else if (a == "--no-audio")  opt.audio = false;
        else if (a == "--verbose")   plat::setChildStderrVisible(true);
        else if (a == "--loop")      opt.loop = true;
        else if (a == "-j" || a == "--threads") opt.threads = std::atoi(need(i, "--threads").c_str());
        else if (a == "-o" || a == "--out")     opt.out = need(i, "--out");
        else if (a == "--image")     opt.forceImage = true;
        else if (a == "--video")     opt.forceVideo = true;
        else if (!a.empty() && a[0] == '-')     die("unknown option: " + a);
        else if (opt.path.empty())   opt.path = a;
        else die("more than one input file given");
    }
    if (opt.path.empty()) { usage(); std::exit(1); }
    return opt;
}

int renderStill(const Options& opt, const MediaInfo& mi, const GlyphSet& gs,
                bool isVideo) {
    Engine eng(opt, gs);
    int c, r;
    pickGrid(opt, mi, gs, isVideo && opt.out.empty(), c, r);
    eng.resize(c, r);

    plat::Proc dec = plat::Proc::spawn(
        decoderArgs(opt.path, eng.pw, eng.ph, false, 0, 0), true);
    if (!dec.valid() || !dec.readExact(eng.rgb.data(), eng.frameBytes()))
        die("ffmpeg produced no frame for '" + opt.path + "'");
    dec.stop();

    eng.process();
    std::string text = eng.toText(opt.color != ColorMode::None);

    if (!opt.out.empty()) {
        FILE* f = std::fopen(opt.out.c_str(), "wb");
        if (!f) die("cannot write '" + opt.out + "'");
        std::fwrite(text.data(), 1, text.size(), f);
        std::fclose(f);
        std::fprintf(stderr, "wrote %s (%dx%d cells)\n", opt.out.c_str(), c, r);
    } else {
        std::fwrite(text.data(), 1, text.size(), stdout);
        std::fflush(stdout);
    }
    return 0;
}

int playVideo(const Options& opt, const MediaInfo& mi, const GlyphSet& gs) {
    const double fps = opt.fps > 0 ? opt.fps : mi.fps;

    plat::installQuitHandler();
    plat::terminalEnter();

    Engine   eng(opt, gs);
    Renderer ren;
    ren.mode = opt.color;
    ren.cellBg = opt.cellBg;
    ren.tol    = opt.colorTol;

    double seek = 0.0;                 // where in the file this pass started
    long   shown = 0, dropped = 0;
    bool   paused = false;

    for (;;) {                         // one pass; restarts on resize or --loop
        int c, r;
        pickGrid(opt, mi, gs, true, c, r);
        eng.resize(c, r);
        ren.reset(c, r);
        plat::writeOut("\x1b[2J", 4);

        int lastTc = 0, lastTr = 0;
        plat::terminalSize(lastTc, lastTr);

        plat::Proc dec = plat::Proc::spawn(
            decoderArgs(opt.path, eng.pw, eng.ph, true, fps, seek), true);
        if (!dec.valid()) { plat::terminalLeave(); die("could not start ffmpeg"); }

        plat::Proc aud;
        if (opt.audio) aud = plat::Proc::spawn(audioArgs(opt.path, seek), false);

        const double t0 = plat::nowSeconds();
        double pauseAccum = 0, pauseStart = 0, target = 0;
        long   frame = 0;
        bool   restart = false;

        while (!plat::quitRequested() && !restart) {
            int tc = 0, tr = 0;
            plat::terminalSize(tc, tr);
            if (opt.cols <= 0 && opt.rows <= 0 && (tc != lastTc || tr != lastTr)) {
                restart = true;
                break;
            }

            if (!dec.readExact(eng.rgb.data(), eng.frameBytes())) break;   // EOF

            target = (double)frame / fps;
            ++frame;

            for (;;) {                                   // pacing and input
                int k = plat::pollKey();
                if (k == 'q' || k == 'Q' || k == 27) { plat::requestQuit(); break; }
                if (k == ' ') {
                    paused = !paused;
                    if (paused) {
                        pauseStart = plat::nowSeconds();
                        aud.stop();
                    } else {
                        pauseAccum += plat::nowSeconds() - pauseStart;
                        if (opt.audio)
                            aud = plat::Proc::spawn(audioArgs(opt.path, seek + target), false);
                    }
                }
                if (plat::quitRequested()) break;
                if (paused) { plat::sleepMs(20); continue; }

                double elapsed = plat::nowSeconds() - t0 - pauseAccum;
                if (elapsed >= target) break;
                plat::sleepMs(std::min(5, (int)((target - elapsed) * 1000.0)));
            }
            if (plat::quitRequested()) break;

            double elapsed = plat::nowSeconds() - t0 - pauseAccum;
            if (elapsed > target + 2.0 / fps) { ++dropped; continue; }  // behind

            eng.process();
            ren.draw(eng.cells, gs);
            ++shown;
        }

        const double played = plat::nowSeconds() - t0 - pauseAccum;
        dec.stop();
        aud.stop();

        if (plat::quitRequested()) break;
        if (restart) {
            seek += played;
            if (mi.duration > 0 && seek >= mi.duration) break;
            continue;
        }
        if (opt.loop) { seek = 0; continue; }
        break;
    }

    plat::terminalLeave();
    if (dropped)
        std::fprintf(stderr,
            "asciiplay: %ld frames shown, %ld dropped to keep sync"
            " (try --cell 4x8 or fewer --cols)\n", shown, dropped);
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    Options opt = parseArgs(argc, argv);

    if (FILE* f = std::fopen(opt.path.c_str(), "rb")) std::fclose(f);
    else die("cannot read '" + opt.path + "'");

    MediaInfo mi = probeMedia(opt.path);
    const bool isVideo = opt.forceVideo ? true
                       : opt.forceImage ? false
                       : mi.isVideo;

    GlyphSet gs = GlyphSet::build(opt.glyphs, opt.cellW, opt.cellH);

    if (!isVideo || !opt.out.empty())
        return renderStill(opt, mi, gs, isVideo);
    return playVideo(opt, mi, gs);
}
