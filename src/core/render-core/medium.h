#pragma once
#include "core/utils/configurable.h"
#include "core/math/math.h"
#include "sampler.h"
#include "core/math/warp.h"
#include "phase.h"
class Scene;

struct MediumSampleRecord {
    float pathLength;

    float pdf {1.f};

    bool isValid {false};

    SpectrumRGB transmittance {1.f};

    SpectrumRGB sigmaS;

    SpectrumRGB sigmaA;

    SpectrumRGB albedo;

    const Medium *medium = nullptr;
};

class Medium : public Configurable {
public:
    Medium() = default;
    Medium(const rapidjson::Value &_value) { }
    virtual ~Medium() = default;

    //* Sample the path length before next scatter
    virtual bool sampleDistance(MediumSampleRecord *m_rec,
                                const Ray3f &ray, float tmax,
                                Sampler *sampler) const = 0;

    virtual SpectrumRGB getTrans(Point3f start,
                                 Point3f end) const = 0;

    virtual float pdfFromTo(Point3f from,
                            Point3f end,
                            bool isExceed) const = 0;

    virtual SpectrumRGB Le(const Ray3f &ray) const = 0;

    SpectrumRGB evaluatePhase(const PhaseQueryRecord &pRec) const {
        return mPhase->evaluate(pRec);
    }

    float pdfPhase(const PhaseQueryRecord &pRec) const {
        return mPhase->pdf(pRec);
    }

    SpectrumRGB samplePhase(PhaseQueryRecord *pRec, Point2f sample) const {
        return mPhase->sample(pRec, sample);
    }

    void setPhase(std::shared_ptr<PhaseFunction> phase) {
        this->mPhase = phase;
    }

protected:
    std::shared_ptr<PhaseFunction> mPhase;
};


struct MediumIntersection {
    float distance;

    Point3f scatterPoint;

    Vector3f forwardDirection;

    Frame forwardFrame;

    std::shared_ptr<Medium> medium;

    SpectrumRGB sigmaS, sigmaA;

    SpectrumRGB albedo;
};