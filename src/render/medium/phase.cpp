#include "core/render-core/phase.h"

class IsotropicPhase : public PhaseFunction {
public:
    IsotropicPhase() = default;

    IsotropicPhase(const rapidjson::Value &_value) { }

    virtual ~IsotropicPhase() = default;

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
REGISTER_CLASS(IsotropicPhase, "isotropic")

class HenyeyGreensteinPhase : public PhaseFunction {
public:
    HenyeyGreensteinPhase() = default;

    HenyeyGreensteinPhase(const rapidjson::Value &_value) {
        mG = getFloat("g", _value);
    }

    virtual ~HenyeyGreensteinPhase() = default;

    virtual SpectrumRGB evaluate(const PhaseQueryRecord &pRec) const override
    {
        return SpectrumRGB{HGPhase(dot(pRec.wi, pRec.wo))};    
    }

    virtual float pdf(const PhaseQueryRecord &pRec) const override
    {
        return HGPhase(dot(pRec.wi, pRec.wo));
    }

    virtual SpectrumRGB sample(PhaseQueryRecord *pRec, Point2f sample) const override
    {
        float s = 2 * sample[0] - 1;
        float cosTheta;
        if (std::abs(mG) < 1e-3) {
            cosTheta = s + 1.5 * mG * (1 - s * s);
        } else {
            float sqrtTerm = (1 - mG * mG) / (1 + mG * s);
            cosTheta = (1 + mG * mG - sqrtTerm * sqrtTerm) / (2 * mG);
        }

        float phi = 2 * M_PI * sample[1],
              sinTheta = std::sqrt(std::max(0.f, 1 - cosTheta * cosTheta));

        pRec->wo = Vector3f{
            sinTheta * std::cos(phi),
            cosTheta,
            sinTheta * std::sin(phi)
        };

        pRec->pdf = HGPhase(cosTheta);

        return SpectrumRGB{1};
    }
protected:

    float HGPhase(float cosTheta) const {
        float denom = 1 + mG * mG + 2 * mG * cosTheta;
        return 0.25f * INV_PI * (1 - mG * mG) / 
            (denom * std::sqrt(denom));
    }

    float mG;

};

REGISTER_CLASS(HenyeyGreensteinPhase, "hg")