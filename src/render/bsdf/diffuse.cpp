#include "core/render-core/bsdf.h"

class Diffuse : public BSDF {
public:
    Diffuse() = default;
    
    Diffuse(const rapidjson::Value &_value) {
        // do nothing
    }

    ~Diffuse() = default;

    virtual SpectrumRGB evaluate (const RayIntersectionRec &iRec) const {
        return m_texture->evaluate(iRec.UV);
    }

    virtual float pdf (const RayIntersectionRec &iRec) const {

    }

    virtual SpectrumRGB sample(RayIntersectionRec &iRec, const Point2f &sample) const {

    }
};

REGISTER_CLASS(Diffuse, "diffuse")