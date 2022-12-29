#include <core/geometry/common.h>
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
    Vector3f vx, vy, vz;

    ///* Sample the projection axis
    if (sample1 < 0.5f) {
      vx = info.toWorld(Vector3f(1, 0, 0));
      vy = info.toWorld(Vector3f(0, 1, 0));
      vz = info.toWorld(Vector3f(0, 0, 1));
      sample1 *= 2;
    } else if (sample1 < 0.75f) {
      vx = info.toWorld(Vector3f(0, 0, 1));
      vy = info.toWorld(Vector3f(1, 0, 0));
      vz = info.toWorld(Vector3f(0, 1, 0));
      sample1 = (sample1 - 0.5f) * 4;
    } else {
      vx = info.toWorld(Vector3f(0, 1, 0));
      vy = info.toWorld(Vector3f(0, 0, 1));
      vz = info.toWorld(Vector3f(0, 0, 1));
      sample1 = (sample1 - 0.75f) * 4;
    }

    // TODO Sample the channel

    float r = sample_r(sample2[0]);
    if (r < 0)
      return SpectrumRGB{.0f};
    float phi = 2 * M_PI * sample2[1];

    float r_max = sample_r(0.999f);
    if (r >= r_max)
      return SpectrumRGB{.0f};
    float l = 2 * std::sqrt(r_max * r_max - r * r);

    Point3f start = info.position +
                    r * (vx * std::cos(phi) + vz * std::sin(phi)) -
                    l * vy * 0.5f,
            target = start + l * vy;

    int n_found = 0;
    std::vector<SurfaceIntersectionInfo> chain;
    Ray3f ray{start, target};
    while (true) {
      auto its = scene.intersectWithSurface(ray);
      if (!its->shape) {
        break;
      }
      if (its->shape == info.shape) {
        chain.emplace_back(*its);
        n_found++;
      }
      ray = its->scatterRay(scene, target);
    }

    if (n_found == 0)
      return SpectrumRGB{.0f};
    int selected = std::min((int)(sample1 * n_found), n_found - 1);
    *sampled = chain[selected];
    *pdf = pdf_sp(info, *sampled) / n_found;
    return sp((info.position - sampled->position).length());
  }
  virtual float evaluate_sw(const SurfaceIntersectionInfo &info,
                            Vector3f w) const override {
    auto FresnelMoment1 = [](float eta) -> float {
      float eta2 = eta * eta, eta3 = eta2 * eta, eta4 = eta3 * eta,
            eta5 = eta4 * eta;
      if (eta < 1) {
        return 0.45966f - 1.73965f * eta + 3.37668f * eta2 - 3.904945 * eta3 +
               2.49277f * eta4 - 0.68441f * eta5;
      } else {
        return -4.61686f + 11.1136f * eta - 10.4646f * eta2 + 5.11455f * eta3 -
               1.27198f * eta4 + 0.12746f * eta5;
      }
    };
    float c = 1 - 2 * FresnelMoment1(1 / m_eta);
    float unused;
    return (1 - Fresnel(Frame::cosTheta(info.toLocal(w)), m_eta, &unused)) /
           (c * M_PI);
  }
  virtual float pdf_sw(const SurfaceIntersectionInfo &info,
                       Vector3f w) const override {
    return Warp::squareToCosineHemispherePdf(info.toLocal(w));
  }

  virtual ScatterInfo sample_sw(const SurfaceIntersectionInfo &info,
                                Point2f sample) const override {
    ScatterInfo scatterInfo;
    scatterInfo.wo = info.toWorld(Warp::squareToCosineHemisphere(sample));
    scatterInfo.pdf = pdf_sw(info, scatterInfo.wo);
    scatterInfo.weight =
        SpectrumRGB(evaluate_sw(info, scatterInfo.wo) / scatterInfo.pdf);
    scatterInfo.type = ScatterSampleType::Unknown;
    return scatterInfo;
  }

protected:
  float sample_r(float sample) const {
    if (sample < 0.25f) {
      sample =
          std::min(sample * 4, 1.f - std::numeric_limits<float>::epsilon());
      return d * std::log(1 - (1 - sample));
    } else {
      sample = std::min((sample - 0.25f) / 0.75f,
                        1.f - std::numeric_limits<float>::epsilon());
      return 3 * d * std::log(1 / (1 - sample));
    }
  }

  float pdf_sp(const SurfaceIntersectionInfo &info,
               const SurfaceIntersectionInfo &sampled) const {
    Vector3f d = info.position - sampled.position;
    Vector3f dLocal = info.toLocal(d);
    Vector3f nLocal = info.toLocal(sampled.geometryNormal);

    float rProj[3] = {std::sqrt(dLocal.y * dLocal.y + dLocal.z * dLocal.z),
                      std::sqrt(dLocal.x * dLocal.x + dLocal.z * dLocal.z),
                      std::sqrt(dLocal.x * dLocal.x + dLocal.y * dLocal.y)};

    float pdf = .0f, axisProb[3] = {.25f, .5f, .25f};
    for (int axis = 0; axis < 3; ++axis) {
      pdf += pdf_sr(rProj[axis]) * std::abs(nLocal[axis]) * axisProb[axis];
    }
    return pdf;
  }

  SpectrumRGB sp(float distance) const {
    if (distance < 1e-6f)
      distance = 1e-6f;
    return SpectrumRGB(R) *
           (std::exp(-distance / d) + std::exp(distance / (3 * d))) /
           (8 * M_PI * d * distance);
  }

  float pdf_sr(float distance) const {
    if (distance < 1e-6f)
      distance = 1e-6f;

    return (.25f * std::exp(-distance / d)) / (2 * M_PI * d * distance) +
           (.75f * std::exp(-distance / (3 * d))) / (6 * M_PI * d * distance);
  }

  float m_eta = 1.5;
  float R = 0.3f, d = 1.f;
};

REGISTER_CLASS(SeperateBSSRDF, "seperate")