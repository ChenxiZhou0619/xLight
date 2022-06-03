#include "core/render-core/bsdf.h"

class Mirror : public BSDF {
public:
    Mirror() = default;
    Mirror(const rapidjson::Value &_value) {
        // do nothing
    }
    ~Mirror() = default;

    // delta distribution, always return .0fs
    virtual SpectrumRGB evaluate(const BSDFQueryRecord &bRec) const {
        return SpectrumRGB {.0f};
    }

    virtual float pdf(const BSDFQueryRecord &bRec) const {
        return 1.f;
    }

    virtual SpectrumRGB sample(BSDFQueryRecord &bRec, const Point2f &sample) const {
        bRec.wo = reflect(bRec.wi);
        return SpectrumRGB {1.f};
    }

    virtual bool isDiffuse() const {
        return false;
    }

};

REGISTER_CLASS(Mirror, "mirror")