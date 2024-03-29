#pragma once

#include <core/geometry/geometry.h>
#include <tinyformat/tinyformat.h>

class Spectrum {
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

  explicit SpectrumRGB(float v) : rgb(v) {}

  SpectrumRGB(float r, float g, float b) : rgb(Vector3f{r, g, b}) {}

  explicit SpectrumRGB(const Vector3f &_rgb) : rgb(_rgb), Spectrum(3) {}

  virtual ~SpectrumRGB() = default;

  virtual std::string toString() const {
    return tfm::format("Spectrum = [ %.3f, %.3f, %.3f ]", rgb.x, rgb.y, rgb.z);
  }

  SpectrumRGB operator+(const SpectrumRGB &rhs) const {
    return SpectrumRGB{rgb + rhs.rgb};
  }

  SpectrumRGB &operator+=(const SpectrumRGB &rhs) {
    rgb += rhs.rgb;
    return *this;
  }

  SpectrumRGB operator-(const SpectrumRGB &rhs) const {
    return SpectrumRGB{rgb - rhs.rgb};
  }

  SpectrumRGB &operator-=(const SpectrumRGB &rhs) {
    rgb -= rhs.rgb;
    return *this;
  }

  SpectrumRGB operator*(const SpectrumRGB &rhs) const {
    return SpectrumRGB{
        Vector3f{rgb.x * rhs.rgb.x, rgb.y * rhs.rgb.y, rgb.z * rhs.rgb.z}};
  }

  SpectrumRGB operator*(float f) const {
    return SpectrumRGB{Vector3f{rgb.x * f, rgb.y * f, rgb.z * f}};
  }

  SpectrumRGB &operator*=(const SpectrumRGB &rhs) {
    *this = *this * rhs;
    return *this;
  }

  SpectrumRGB operator/(const SpectrumRGB &rhs) const {
    return SpectrumRGB{
        Vector3f{rgb.x / rhs.rgb.x, rgb.y / rhs.rgb.y, rgb.z / rhs.rgb.z}};
  }

  SpectrumRGB operator/(float v) const {
    return SpectrumRGB{Vector3f{rgb.x / v, rgb.y / v, rgb.z / v}};
  }

  SpectrumRGB &operator/=(const SpectrumRGB &rhs) {
    *this = *this / rhs;
    return *this;
  }

  SpectrumRGB &operator/=(float f) {
    this->rgb /= f;
    return *this;
  }

  float operator[](int i) const { return rgb[i]; }

  bool isZero() const { return rgb.x == 0 && rgb.y == 0 && rgb.z == 0; }

  float max() const { return std::max(rgb[0], std::max(rgb[1], rgb[2])); }

  bool hasNan() const {
    return std::isnan(rgb[0]) || std::isnan(rgb[1]) || std::isnan(rgb[2]);
  }

  float average() const { return (rgb[0] + rgb[1] + rgb[2]) / 3; }

  float r() const { return rgb.x; }
  float g() const { return rgb.y; }
  float b() const { return rgb.z; }

private:
  Vector3f rgb;
};

static inline std::ostream &operator<<(std::ostream &os,
                                       const SpectrumRGB &rgb) {
  os << rgb.toString();
  return os;
}
