#include <core/render-core/info.h>

#include "core/math/math.h"
#include "core/math/warp.h"
#include "core/render-core/bsdf.h"

class Diffuse : public BSDF {
 public:
  Diffuse() = default;

  Diffuse(const rapidjson::Value &_value) {
    // do nothing
    m_type = EBSDFType::EDiffuse;
  }

  virtual ~Diffuse() = default;

  virtual bool isDiffuse() const override { return true; }

  virtual SpectrumRGB evaluate(const BSDFQueryRecord &bRec) const override {
    if (Frame::cosTheta(bRec.wi) <= 0 || Frame::cosTheta(bRec.wo) <= 0)
      return SpectrumRGB{.0f};
    return m_texture->evaluate(bRec.uv, bRec.du, bRec.dv) * INV_PI *
           std::abs(Frame::cosTheta(bRec.wo));
  }

  virtual float pdf(const BSDFQueryRecord &bRec) const override {
    if (Frame::cosTheta(bRec.wi) <= 0 || Frame::cosTheta(bRec.wo) <= 0)
      return .0f;
    return INV_PI * std::abs(Frame::cosTheta(bRec.wo));
  }

  virtual SpectrumRGB sample(BSDFQueryRecord &bRec, const Point2f &sample,
                             float &pdf,
                             ScatterSampleType *type) const override {
    if (Frame::cosTheta(bRec.wi) <= 0) {
      pdf = .0f;
      return SpectrumRGB{.0f};
    }
    bRec.wo = Warp::squareToCosineHemisphere(sample);
    pdf = INV_PI * std::abs(Frame::cosTheta(bRec.wo));
    *type = ScatterSampleType::SurfaceReflection;
    return m_texture->evaluate(bRec.uv, bRec.du, bRec.dv);
  }
};

REGISTER_CLASS(Diffuse, "diffuse")