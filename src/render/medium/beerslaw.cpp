#include <core/render-core/info.h>

#include "core/render-core/medium.h"

//* The medium only consider absorbtion, which means no scattering or emission
class Beerslaw : public Medium {
 public:
  Beerslaw() = default;
  Beerslaw(const rapidjson::Value &_value) {
    m_absorbtion = getSpectrumRGB("sigma_a", _value);
  }
  virtual ~Beerslaw() = default;

  virtual SpectrumRGB evaluateTr(Point3f start, Point3f end) const override {
    auto r = m_absorbtion.r(), g = m_absorbtion.g(), b = m_absorbtion.b();

    auto dist = (end - start).length();

    return SpectrumRGB{std::exp(-dist * r), std::exp(-dist * g),
                       std::exp(-dist * b)};
  }

  virtual SpectrumRGB Le(const Ray3f &ray) const override {
    return SpectrumRGB{0};
  }

  virtual std::shared_ptr<MediumIntersectionInfo> sampleIntersection(
      Ray3f ray, float tBounds, Point2f sample) const override {
    auto mIts = std::make_shared<MediumIntersectionInfo>();
    mIts->medium = nullptr;  //* Cuz no possibility for inside intersection
    mIts->weight = evaluateTr(ray.ori, ray.at(tBounds));
    //* This is a delta distribution
    mIts->pdf = FINF;
    return mIts;
  }

  virtual std::shared_ptr<MediumIntersectionInfo>
  sampleIntersectionDeterministic(Ray3f ray, float tBounds,
                                  Point2f sample) const override {
    return nullptr;
  }

  virtual SpectrumRGB sigmaS(Point3f) const override { return SpectrumRGB{0}; }

 private:
  SpectrumRGB m_absorbtion;
};

REGISTER_CLASS(Beerslaw, "beerslaw")