#include "core/render-core/medium.h"

class Homogeneous : public Medium {
public:
    Homogeneous() = default;
    Homogeneous(const rapidjson::Value &_value) {

    }
    virtual ~Homogeneous() = default;

    // TODO
    virtual float propagate() const override {

    }

    // TODO
    virtual SpectrumRGB transmittance(const Scene &scene, Point3f p0, Point3f p1) const override {

    }



};

REGISTER_CLASS(Homogeneous, "homegeneous")
