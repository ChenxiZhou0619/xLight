#include "core/geometry/geometry.h"
#include "core/render-core/spectrum.h"
#include "core/render-core/texture.h"
#include "core/utils/configurable.h"

class Constant : public Texture {
    SpectrumRGB m_albedo;

public:
    Constant() = default;
    Constant(const rapidjson::Value &_value) {
        m_albedo = getSpectrumRGB("albedo", _value);
    }

    virtual SpectrumRGB evaluate(const Point2f &uv, float du, float dv) const override {
        return m_albedo;
    }

    virtual SpectrumRGB average() const override {
        return m_albedo;
    }

    virtual Vector2i getResolution() const override {
        std::cout << "Error, Constant Texture shouldn't have resolution!\n";
        std::exit(1);
    }

};

REGISTER_CLASS(Constant, "constant")