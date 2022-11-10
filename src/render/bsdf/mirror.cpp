#include "core/render-core/bsdf.h"
#include <core/render-core/info.h>

class Mirror : public BSDF {
public:
    Mirror() = default;
    Mirror(const rapidjson::Value &_value) {
        // do nothing
        m_type = EBSDFType::EGlossy;
    }
    ~Mirror() = default;

    // delta distribution, always return .0fs
    virtual SpectrumRGB evaluate(const BSDFQueryRecord &bRec) const override{
        return SpectrumRGB {.0f};
    }

    virtual float pdf(const BSDFQueryRecord &bRec) const override{
        return FINF;
    }

    virtual SpectrumRGB sample(BSDFQueryRecord &bRec, const Point2f &sample, 
                               float &pdf, ScatterSampleType *type) const override{
        if (Frame::cosTheta(bRec.wi) <= 0) {
            pdf = .0f;
            return SpectrumRGB{.0f};
        }
        *type = ScatterSampleType::SurfaceReflection;
        bRec.wo = reflect(bRec.wi);
        pdf = FINF;
        bRec.isDelta = true;
        return SpectrumRGB {1.f};
    }

    virtual bool isDiffuse() const override{
        return false;
    }

};

REGISTER_CLASS(Mirror, "mirror")