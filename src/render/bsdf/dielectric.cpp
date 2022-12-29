#include <core/render-core/info.h>

#include "core/math/common.h"
#include "core/render-core/bsdf.h"
class Dielectric : public BSDF {
  float extIOR, intIOR;

  Vector3f refract(const Vector3f &wi, float eta, float cosThetaT) const {
    float scale = -((cosThetaT < 0) ? (1 / eta) : eta);
    return Vector3f{wi.x * scale, cosThetaT, wi.z * scale};
  }

public:
  Dielectric() = default;

  Dielectric(const rapidjson::Value &_value) {
    m_type = EBSDFType::ETrans;

    extIOR = getFloat("extIOR", _value);
    intIOR = getFloat("intIOR", _value);
    // TODO transmittance/reflectance
  }

  ~Dielectric() = default;

  // almost delta distribution, always return 0
  virtual SpectrumRGB evaluate(const BSDFQueryRecord &bRec) const override {
    return SpectrumRGB{.0f};
  }

  virtual float pdf(const BSDFQueryRecord &bRec) const override { return 0; }

  virtual SpectrumRGB sample(BSDFQueryRecord &bRec, const Point2f &sample,
                             float &pdf,
                             ScatterSampleType *type) const override {
    float cosThetaT;

    float eta = (intIOR / extIOR);
    float F = Fresnel(Frame::cosTheta(bRec.wi), eta, &cosThetaT);

    if (sample.x <= F) {
      // reflect
      bRec.wo = reflect(bRec.wi);
      // pdf = F;
      pdf = FINF;
      *type = ScatterSampleType::SurfaceReflection;
      // TODO replace with specular reflectance
      return SpectrumRGB{1.f};
    } else {
      // refract
      bRec.wo = refract(bRec.wi, (intIOR / extIOR), cosThetaT);
      // pdf = 1 - F;
      pdf = FINF;
      *type = ScatterSampleType::SurfaceTransmission;
      // TODO consider the transmittance and factor
      return SpectrumRGB{1};
    }
  }

  virtual bool isDiffuse() const override { return false; }
};

REGISTER_CLASS(Dielectric, "dielectric")