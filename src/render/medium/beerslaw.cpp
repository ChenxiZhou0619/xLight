#include "core/render-core/medium.h"

//* The medium only consider absorbtion, which means no scattering or emission
class Beerslaw : public Medium {
public:
    Beerslaw() = default;
    Beerslaw(const rapidjson::Value &_value) {
        m_absorbtion = getSpectrumRGB("sigma_a", _value);
    }
    virtual ~Beerslaw() = default;

    //* Because no scattering, just span the ray through the mesh
    virtual float propagate() const override {
        
    }

    virtual SpectrumRGB transmittance(const Scene &scene, Point3f p0, Point3f p1) const override {
        
    }

private:
    SpectrumRGB m_absorbtion;
};

REGISTER_CLASS(Beerslaw, "beerslaw")