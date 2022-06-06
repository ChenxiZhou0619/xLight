#include "core/render-core/bsdf.h"

class Dielectric : public BSDF {
    float mEta;
public:
    Dielectric() = default;
    Dielectric(const rapidjson::Value &_value) {
        mEta = getFloat("eta", _value);
    }
    ~Dielectric() = default;

    virtual SpectrumRGB evaluate(const BSDFQueryRecord &bRec) const {
        return SpectrumRGB {.0f};
    }

    virtual float pdf(const BSDFQueryRecord &bRec) const {
        
    }

    virtual SpectrumRGB sample(BSDFQueryRecord &bRec, const Point2f &sample) const {

    }

    virtual bool isDiffuse() const {
        return false;
    }
};

REGISTER_CLASS(Dielectric, "dielectric")