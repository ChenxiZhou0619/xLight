#include <core/math/common.h>
#include <core/math/math.h>
#include <core/math/warp.h>
#include <core/render-core/bssrdf.h>
#include <core/scene/scene.h>

class SeperateBSSRDF : public BSSRDF {
public:
  SeperateBSSRDF() = default;
  SeperateBSSRDF(const rapidjson::Value &_value) : BSSRDF(_value) {}
  virtual ~SeperateBSSRDF() = default;

  virtual SpectrumRGB sample_sp(const Scene &scene,
                                const SurfaceIntersectionInfo &info,
                                float sample1, Point2f sample2,
                                SurfaceIntersectionInfo *sampled,
                                float *pdf) const override {
    //
    *sampled = info;
    *pdf = 1;
    return SpectrumRGB(1);
  }
  virtual float evaluate_sw(const SurfaceIntersectionInfo &info,
                            Vector3f w) const override {
    w = info.toLocal(w);
    // TODO Possible problems
    float unused;
    float res = 1 - Fresnel(Frame::cosTheta(w), eta, &unused);
    res *= INV_PI;
    res /= (1 - 2 * FresnelMoment1(1 / eta));
    return res;
  }
  virtual float pdf_sw(const SurfaceIntersectionInfo &info,
                       Vector3f w) const override {
    w = info.toLocal(w);
    return INV_PI * std::abs(Frame::cosTheta(w));
  }

  virtual ScatterInfo sample_sw(const SurfaceIntersectionInfo &info,
                                Point2f sample) const override {
    ScatterInfo res;
    Vector3f wo_local = Warp::squareToCosineHemisphere(sample);
    res.wo = info.toWorld(wo_local);
    res.pdf = pdf_sw(info, res.wo);
    res.weight = SpectrumRGB(evaluate_sw(info, res.wo) / res.pdf);
    res.type = ScatterSampleType::SurfaceTransmission;
    return res;
  }

protected:
  //  virtual float sample_r(float u1) const = 0;
};

REGISTER_CLASS(SeperateBSSRDF, "seperate")