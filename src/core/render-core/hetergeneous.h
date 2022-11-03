#pragma once

#include "core/render-core/medium.h"
#include <openvdb/openvdb.h>

class Hetergeneous : public Medium {
public:
    Hetergeneous() = default;

    Hetergeneous(const rapidjson::Value &_value) {
        // donothing
        //mPhase = std::make_shared<IsotropicPhase>();
    }

    Hetergeneous(openvdb::FloatGrid::Ptr _density);

    virtual bool sampleDistance(MediumSampleRecord *mRec,
                                const Ray3f &ray, float tmax,
                                Sampler *sampler) const override;

    virtual bool samplePath(MediumSampleRecord *mRec,
                            const Ray3f &ray, float tmax,
                            const LightSourceInfo *info,
                            Sampler *sampler) const override
    {
        //todo
    }

    virtual SpectrumRGB getTrans(Point3f start,
                                 Point3f end) const override;

    virtual SpectrumRGB Le(const Ray3f &ray) const override {
        return SpectrumRGB(0.0);
    }

    virtual float pdfFromTo(Point3f from,
                            Point3f end,
                            bool isExceed) const override;

    virtual ~Hetergeneous() = default;

    virtual std::shared_ptr<MediumIntersectionInfo>
    sampleIntersection(Ray3f ray, float tBounds, Point2f sample) const override;

    virtual std::shared_ptr<MediumIntersectionInfo>
    sampleIntersectionDeterministic(Ray3f ray, float tBounds, Point2f sample) const override;


protected:
    openvdb::FloatGrid::Ptr density;

    SpectrumRGB sigmaTMax{.0f};

    static constexpr float scale = 1.5;

    float step = 0.1;//delete
};