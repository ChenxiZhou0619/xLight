#include "core/render-core/texture.h"

class CheckBoard : public Texture {
public:
    CheckBoard() : m_light(SpectrumRGB(0.9f)), m_dark(SpectrumRGB(0.4f)) { }

    CheckBoard(const rapidjson::Value &_value) {
        m_light = SpectrumRGB(0.9f);

        m_dark  = SpectrumRGB(0.4f);
    }

    virtual SpectrumRGB evaluate(const Point2f &uv) const override {
        int x = static_cast<int>(uv.x / m_grid) % 2,
            y = static_cast<int>(uv.y / m_grid) % 2;
        if ((x + y) % 2 == 1)
            return m_light;
        else
            return m_dark;
    }

    virtual SpectrumRGB average() const override {
        // TODO, no implement
        return SpectrumRGB(.65f);
    }


private:
    SpectrumRGB m_light;

    SpectrumRGB m_dark;

    float m_grid = 0.1; // default

};

REGISTER_CLASS(CheckBoard, "checkboard")