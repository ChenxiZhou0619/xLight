#include "core/render-core/bsdf.h"

class Diffuse : public BSDF {
public:
    Diffuse() = default;
    
    Diffuse(const rapidjson::Value &_value) {
        // do nothing
    }

    ~Diffuse() = default;

    virtual bool isDiffuse() const {
        return true;
    }
    
    virtual SpectrumRGB evaluate (const BSDFQueryRecord &bRec) const {
        
    }

    virtual float pdf (const BSDFQueryRecord &bRec) const {

    }

    virtual SpectrumRGB sample(BSDFQueryRecord &bRec, const Point2f &sample) const {

    }
};

REGISTER_CLASS(Diffuse, "diffuse")