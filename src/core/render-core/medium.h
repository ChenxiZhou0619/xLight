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
};

class Medium : public Configurable {
public:
    Medium() = default;
    Medium(const rapidjson::Value &_value) { }
    virtual ~Medium() = default;

    //* Sample the path length before next scatter
    virtual void sampleDistance(MediumSampleRecord *m_rec) const = 0;

    //* Sample the phase function
    virtual Vector3f sampleDirection(Vector3f wi, Vector3f *wo, float *pdf) const = 0;

    //* Evaluate the phase function
    virtual SpectrumRGB evaluatePhase(Vector3f wi, Vector3f wo) const = 0;

    //* Return the pdf
    virtual float pdfPhase(Vector3f wi, Vector3f wo) const = 0;

    //* Given two point, return the transmittance between them
    virtual SpectrumRGB transmittance(Point3f p0, Point3f p1) const = 0;

    virtual void sampleLs (const Scene &scene, SpectrumRGB *Ls) const = 0;


    //* refer to pbrt
    virtual void sample(Sampler *sampler, 
                        MediumSampleRecord *mRec,
                        Point3f ori, 
                        Point3f end) const = 0;
};