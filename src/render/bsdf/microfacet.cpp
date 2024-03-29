/**
 * @file microfacet.cpp
 * @author Chenxi Zhou
 * @brief BeckmannDistribution
 * @version 0.1
 * @date 2022-06-29
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <core/render-core/info.h>

#include "core/math/common.h"
#include "core/math/ndf.h"
#include "core/render-core/bsdf.h"

class Microfacet : public BSDF {
protected:
  float alpha;

public:
  Microfacet() = default;

  Microfacet(const rapidjson::Value &_value) {
    m_type = EBSDFType::EDiffuse;

    alpha = getFloat("alpha", _value);
  }

  virtual ~Microfacet() = default;

  virtual SpectrumRGB evaluate(const BSDFQueryRecord &bRec) const {
    Vector3f wh = normalize(bRec.wi + bRec.wo);
    float D = BeckmannDistribution::D(alpha, wh),
          G = BeckmannDistribution::G(bRec.wi, bRec.wo, alpha);
    float cosThetaT;
    //    float F =
    // fresnelDielectric(Frame::cosTheta(bRec.wi), (1.f / 1000), cosThetaT);
    // TODO fresnel term
    return m_texture->evaluate(bRec.uv) * D * G /
           (4.f * Frame::cosTheta(bRec.wo));
  }

  virtual float pdf(const BSDFQueryRecord &bRec) const {
    Vector3f wh = normalize(bRec.wi + bRec.wo);
    return BeckmannDistribution::D(alpha, wh) * std::abs(Frame::cosTheta(wh)) /
           (4 * dot(bRec.wo, wh));
  }

  virtual SpectrumRGB sample(BSDFQueryRecord &bRec, const Point2f &sample,
                             float &pdf, ScatterSampleType *type) const {
    *type = ScatterSampleType::SurfaceReflection;
    Vector3f wh = BeckmannDistribution::sampleWh(alpha, sample);
    bRec.wo = normalize(2 * dot(bRec.wi, wh) * wh - bRec.wi);
    pdf = this->pdf(bRec);
    return evaluate(bRec) / pdf;
  }

  virtual bool isDiffuse() const { return true; }
};

REGISTER_CLASS(Microfacet, "microfacet")
