/*
* Copyright 2016 Google Inc.
*
* Use of this source code is governed by a BSD-style license that can be
* found in the LICENSE file.
*/

#include "Benchmark.h"
#include "SkCanvas.h"
#include "SkRect.h"
#include "SkString.h"

class DrawLatticeBench : public Benchmark {
public:
    DrawLatticeBench(int* xDivs, int xCount, int* yDivs, int yCount, const SkISize& srcSize,
                     const SkRect& dst, const char* desc)
        : fSrcSize(srcSize)
        , fDst(dst)
    {
        fLattice.fXDivs = xDivs;
        fLattice.fXCount = xCount;
        fLattice.fYDivs = yDivs;
        fLattice.fYCount = yCount;

        fName = SkStringPrintf("DrawLattice_%s", desc);
    }

    const char* onGetName() override {
        return fName.c_str();
    }

    bool isSuitableFor(Backend backend) override {
        return kRaster_Backend == backend || kGPU_Backend == backend;
    }

    void onDelayedSetup() override {
        fBitmap.allocN32Pixels(fSrcSize.width(), fSrcSize.height());
        fBitmap.eraseColor(0x880000FF);
    }

    void onDraw(int loops, SkCanvas* canvas) override {
        for (int i = 0; i < loops; i++) {
            canvas->drawBitmapLattice(fBitmap, fLattice, fDst);
        }
    }

private:
    SkISize           fSrcSize;
    SkCanvas::Lattice fLattice;
    SkRect            fDst;
    SkString          fName;
    SkBitmap          fBitmap;

    typedef Benchmark INHERITED;
};

static int gDivs[2] = { 250, 750, };
DEF_BENCH(return new DrawLatticeBench(gDivs, 2, gDivs, 2, SkISize::Make(1000, 1000),
                                      SkRect::MakeWH(4000.0f, 4000.0f), "StandardNine");)
