#pragma once

#include "core/render-core/medium.h"
#include <openvdb/openvdb.h>

class Hetergeneous : public Medium {
public:
    Hetergeneous() = default;

    Hetergeneous(const rapidjson::Value &_value) {
        // donothing
        mPhase = std::make_shared<IsotropicPhase>();
    }

    Hetergeneous(openvdb::FloatGrid::Ptr _density) {
        density = _density;
        mPhase = std::make_shared<IsotropicPhase>();
    }

    virtual bool sampleDistance(MediumSampleRecord *mRec,
                                const Ray3f &ray, float tmax,
                                Sampler *sampler) const override;

    virtual SpectrumRGB getTrans(Point3f start,
                                 Point3f end) const override;

    virtual SpectrumRGB Le(const Ray3f &ray) const override {
        return SpectrumRGB(0.0);
    }

    virtual float pdfFromTo(Point3f from,
                            Point3f end,
                            bool isExceed) const override;

    virtual ~Hetergeneous() = default;

protected:
    openvdb::FloatGrid::Ptr density;

    float scale = 10;

    float step = 0.03;
};