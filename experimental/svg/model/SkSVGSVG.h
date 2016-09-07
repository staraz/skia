/*
 * Copyright 2016 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef SkSVGSVG_DEFINED
#define SkSVGSVG_DEFINED

#include "SkSVGContainer.h"
#include "SkSVGTypes.h"

class SkSVGSVG : public SkSVGContainer {
public:
    virtual ~SkSVGSVG() = default;

    static sk_sp<SkSVGSVG> Make() { return sk_sp<SkSVGSVG>(new SkSVGSVG()); }

    void setX(const SkSVGLength&);
    void setY(const SkSVGLength&);
    void setWidth(const SkSVGLength&);
    void setHeight(const SkSVGLength&);

protected:
    void onSetAttribute(SkSVGAttribute, const SkSVGValue&) override;

private:
    SkSVGSVG();

    SkSVGLength fX      = SkSVGLength(0);
    SkSVGLength fY      = SkSVGLength(0);
    SkSVGLength fWidth  = SkSVGLength(100, SkSVGLength::Unit::kPercentage);
    SkSVGLength fHeight = SkSVGLength(100, SkSVGLength::Unit::kPercentage);

    typedef SkSVGContainer INHERITED;
};

#endif // SkSVGSVG_DEFINED
