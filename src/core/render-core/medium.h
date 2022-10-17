#pragma once
#include "core/utils/configurable.h"
#include "core/math/math.h"
#include "sampler.h"
#include "core/math/warp.h"
class Scene;

struct MediumSampleRecord {
    float pathLength;

    float pdf {1.f};

    bool isValid {false};

    SpectrumRGB transmittance {1.f};

    SpectrumRGB sigmaS;

    SpectrumRGB sigmaA;

    float albedo;

    const Medium *medium = nullptr;
};

struct PhaseQueryRecord {
    Point3f scatterPoint;
    
    Vector3f wi, wo;

    Frame localFrame;

    float pdf;

    PhaseQueryRecord() = default;

    PhaseQueryRecord(Point3f from, Vector3f _wi)
        : scatterPoint(from), wi(_wi), localFrame(_wi) { }

    PhaseQueryRecord(Point3f from, Vector3f _wi, Vector3f _wo) 
        : scatterPoint(from), wi(_wi), localFrame(_wi), wo(_wo) { }
    
};

class PhaseFunction {
public:
    virtual SpectrumRGB evaluate(const PhaseQueryRecord &pRec) const = 0;

    virtual float pdf(const PhaseQueryRecord &pRec) const = 0;

    virtual SpectrumRGB sample(PhaseQueryRecord *pRec, Point2f sample) const = 0;
};

class IsotropicPhase : public PhaseFunction {
public:
    virtual SpectrumRGB evaluate(const PhaseQueryRecord &pRec) const override
    {
        return SpectrumRGB{0.25 * INV_PI};
    }

    virtual float pdf(const PhaseQueryRecord &pRec) const override 
    {
        return 0.25 * INV_PI;
    }

    virtual SpectrumRGB sample(PhaseQueryRecord *pRec, Point2f sample) const override
    {
        Vector3f localWo = Warp::squareToUniformSphere(sample);
        pRec->wo = pRec->localFrame.toWorld(localWo);
        pRec->pdf = Warp::squareToUniformSpherePdf(localWo);
        return evaluate(*pRec) / pRec->pdf;
    }
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

protected:
    std::shared_ptr<PhaseFunction> mPhase;
};


struct MediumIntersection {
    float distance;

    Point3f scatterPoint;

    Vector3f forwardDirection;

    Frame forwardFrame;

    std::shared_ptr<Medium> medium;
};