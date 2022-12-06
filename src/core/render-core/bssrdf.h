#pragma once

#include <core/render-core/info.h>
#include <core/utils/configurable.h>

class Scene;
struct SurfaceIntersectionInfo;

class BSSRDF : public Configurable {
public:
  BSSRDF() = default;
  BSSRDF(const rapidjson::Value &_value) { eta = getFloat("eta", _value); };
  virtual ~BSSRDF() = default;

  virtual SpectrumRGB sample_sp(const Scene &scene,
                                const SurfaceIntersectionInfo &info,
                                float sample1, Point2f sample2,
                                SurfaceIntersectionInfo *sampled,
                                float *pdf) const = 0;

  virtual float evaluate_sw(const SurfaceIntersectionInfo &info,
                            Vector3f w) const = 0;

  virtual ScatterInfo sample_sw(const SurfaceIntersectionInfo &info,
                                Point2f sample) const = 0;

  virtual float pdf_sw(const SurfaceIntersectionInfo &info,
                       Vector3f w) const = 0;

protected:
  float eta;
};

inline float FresnelMoment1(float eta) {
  float eta2 = eta * eta, eta3 = eta2 * eta, eta4 = eta3 * eta,
        eta5 = eta4 * eta;
  if (eta < 1)
    return 0.45966f - 1.73965f * eta + 3.37668f * eta2 - 3.904945 * eta3 +
           2.49277f * eta4 - 0.68441f * eta5;
  else
    return -4.61686f + 11.1136f * eta - 10.4646f * eta2 + 5.11455f * eta3 -
           1.27198f * eta4 + 0.12746f * eta5;
}

inline float FresnelMoment2(float eta) {
  float eta2 = eta * eta, eta3 = eta2 * eta, eta4 = eta3 * eta,
        eta5 = eta4 * eta;
  if (eta < 1) {
    return 0.27614f - 0.87350f * eta + 1.12077f * eta2 - 0.65095f * eta3 +
           0.07883f * eta4 + 0.04860f * eta5;
  } else {
    float r_eta = 1 / eta, r_eta2 = r_eta * r_eta, r_eta3 = r_eta2 * r_eta;
    return -547.033f + 45.3087f * r_eta3 - 218.725f * r_eta2 +
           458.843f * r_eta + 404.557f * eta - 189.519f * eta2 +
           54.9327f * eta3 - 9.00603f * eta4 + 0.63942f * eta5;
  }
}