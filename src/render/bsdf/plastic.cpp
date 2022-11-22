#include <core/render-core/info.h>

#include "core/math/common.h"
#include "core/math/math.h"
#include "core/math/ndf.h"
#include "core/math/warp.h"
#include "core/render-core/bsdf.h"

class Plastic : public BSDF {
 public:
  Plastic() = default;

  Plastic(const rapidjson::Value &_value) {
    m_type = EBSDFType::EGlossy;

    m_alpha = getFloat("alpha", _value);

    m_int_eta = getFloat("intIOR", _value);

    m_ext_eta = getFloat("extIOR", _value);
  }

  virtual ~Plastic() = default;

  virtual void initialize() override {
    assert(m_texture != nullptr);
    m_diffuse_weight = m_texture->average().max();
    m_specular_weight = 1 - m_diffuse_weight;
  }

  virtual bool isDiffuse() const override { return false; }

  virtual SpectrumRGB evaluate(const BSDFQueryRecord &bRec) const override {
    if (Frame::cosTheta(bRec.wi) <= 0 || Frame::cosTheta(bRec.wo) <= 0)
      return SpectrumRGB{0.f};
    SpectrumRGB diffuse_term = m_texture->evaluate(bRec.uv) * m_diffuse_weight *
                               INV_PI * Frame::cosTheta(bRec.wo);

    Vector3f m = normalize(bRec.wi + bRec.wo);
    float eta = m_int_eta / m_ext_eta;

    float F = FresnelDielectricAccurate(Frame::cosTheta(bRec.wi), eta);
    float G = BeckmannDistribution::G(bRec.wi, bRec.wo, m_alpha);
    float D = BeckmannDistribution::D(m_alpha, m);
    // TODO, hack, multiple it by 2
    SpectrumRGB specular_term = SpectrumRGB{m_specular_weight * D * F * G /
                                            (4 * Frame::cosTheta(bRec.wi)) * 2};
    return diffuse_term + specular_term;
  }

  virtual float pdf(const BSDFQueryRecord &bRec) const override {
    if (Frame::cosTheta(bRec.wi) <= 0 || Frame::cosTheta(bRec.wo) <= 0)
      return .0f;
    Vector3f m = normalize(bRec.wi + bRec.wo);
    float dwh_dho = 0.25f / dot(m, bRec.wo);
    float specular_pdf =
        m_specular_weight * Warp::squareToBeckmannPdf(m, m_alpha) * dwh_dho;
    float diffuse_pdf = m_diffuse_weight * Frame::cosTheta(bRec.wo) * INV_PI;
    return specular_pdf + diffuse_pdf;
  }

  virtual SpectrumRGB sample(BSDFQueryRecord &bRec, const Point2f &sample,
                             float &pdf,
                             ScatterSampleType *type) const override {
    float prob = .5f * (sample[0] + sample[1]);
    float microfacet_pdf;
    if (prob < m_specular_weight) {
      // sample according to specular
      Vector3f m =
          BeckmannDistribution::sampleWh(m_alpha, sample, microfacet_pdf);
      bRec.wo = 2.0f * dot(m, bRec.wi) * m - bRec.wi;
    } else {
      // sample according to lambertian
      bRec.wo = Warp::squareToCosineHemisphere(sample);
    }
    SpectrumRGB bsdf_value = evaluate(bRec);
    *type = ScatterSampleType::SurfaceReflection;
    if (bsdf_value.isZero()) return SpectrumRGB{.0f};
    pdf = this->pdf(bRec);
    if (pdf == 0) return SpectrumRGB{.0f};
    return bsdf_value / pdf;
  }

 private:
  float m_alpha;

  float m_int_eta;

  float m_ext_eta;

  float m_specular_weight;

  float m_diffuse_weight;
};

REGISTER_CLASS(Plastic, "plastic")