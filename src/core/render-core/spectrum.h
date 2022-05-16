#pragma once

#include <core/geometry/geometry.h>
#include <tinyformat/tinyformat.h>

class Spectrum{
public:
    Spectrum() = default;
    Spectrum(unsigned _nSpectrumSamples) : nSpectrumSamples(_nSpectrumSamples) {}
    virtual ~Spectrum() = default;
    virtual std::string toString() const = 0;
protected:
    unsigned nSpectrumSamples;
};

class SpectrumRGB : public Spectrum {
public:
    SpectrumRGB() = default;

    SpectrumRGB(float v) : rgb(v) { }

    SpectrumRGB(float r, float g, float b) : rgb(Vector3f{r, g, b}) { }

    explicit SpectrumRGB(const Vector3f &_rgb) : rgb(_rgb), Spectrum(3) { } 

    virtual ~SpectrumRGB() = default;

    virtual std::string toString() const {
        return tfm::format("Spectrum = [ %.3f, %.3f, %.3f ]", rgb.x, rgb.y, rgb.z);
    }

    SpectrumRGB operator+(const SpectrumRGB &rhs) const {
        return SpectrumRGB {rgb + rhs.rgb};
    }

    SpectrumRGB& operator+=(const SpectrumRGB &rhs) {
        rgb += rhs.rgb;
        return *this;
    }

    SpectrumRGB operator-(const SpectrumRGB &rhs) const {
        return SpectrumRGB {rgb - rhs.rgb};
    }

    SpectrumRGB& operator-=(const SpectrumRGB &rhs) {
        rgb -= rhs.rgb;
        return *this;
    }

    SpectrumRGB operator*(const SpectrumRGB &rhs) const {
        return SpectrumRGB {
            Vector3f {
                rgb.x * rhs.rgb.x, 
                rgb.y * rhs.rgb.y,
                rgb.z * rhs.rgb.z
            }
        };
    }

    SpectrumRGB& operator*=(const SpectrumRGB &rhs) {
        *this = *this * rhs;
        return *this;
    }

    SpectrumRGB operator/(const SpectrumRGB &rhs) const {
        return SpectrumRGB {
            Vector3f {
                rgb.x / rhs.rgb.x, 
                rgb.y / rhs.rgb.y,
                rgb.z / rhs.rgb.z
            }
        };
    }

    SpectrumRGB& operator/=(const SpectrumRGB &rhs) {
        *this = *this / rhs;
        return *this;
    }

    float operator[](int i) const {
        return rgb[i];
    }

private:
    Vector3f rgb;
};

static inline std::ostream& operator<<(std::ostream &os, const SpectrumRGB &rgb) {
    os << rgb.toString();
    return os;
}