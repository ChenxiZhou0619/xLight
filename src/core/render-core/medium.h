#pragma once
#include "core/utils/configurable.h"
#include "sampler.h"
class Scene;

struct MediumSampleRecord {
    float pathLength;

    float pdf;

    bool isValid;

    SpectrumRGB transmittance;

    SpectrumRGB sigmaS;

    SpectrumRGB sigmaA;

    const Medium *medium = nullptr;
};

class Medium : public Configurable {
public:
    Medium() = default;
    Medium(const rapidjson::Value &_value) { }
    virtual ~Medium() = default;

    //* Sample the path length before next scatter
    virtual bool sampleDistance(MediumSampleRecord *m_rec,
                                const Ray3f &ray,
                                Sampler *sampler) const = 0;

    virtual SpectrumRGB getTrans(Point3f start,
                                 Point3f end) const = 0;
};