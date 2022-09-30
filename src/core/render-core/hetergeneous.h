#pragma once

#include "core/render-core/medium.h"
#include <openvdb/openvdb.h>

class Hetergeneous : public Medium {
public:
    Hetergeneous() = default;

    Hetergeneous(const rapidjson::Value &_value) {
        // donothing
    }

    Hetergeneous(openvdb::FloatGrid::Ptr _density) {
        density = _density;
    }

    virtual bool sampleDistance(MediumSampleRecord *mRec,
                                const Ray3f &ray,
                                Sampler *sampler) const override;

    virtual SpectrumRGB getTrans(Point3f start,
                                 Point3f end) const override;

    virtual SpectrumRGB Le(const Ray3f &ray) const override {
        return SpectrumRGB(0.0);
    }

    virtual ~Hetergeneous() = default;

protected:
    openvdb::FloatGrid::Ptr density;
};