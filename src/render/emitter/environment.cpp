#include <core/render-core/info.h>
#include <spdlog/spdlog.h>

#include <type_traits>

#include "core/math/math.h"
#include "core/render-core/emitter.h"
#include "core/render-core/sampler.h"
#include "core/render-core/texture.h"
#include "stb/stb_image_write.h"

class EnvironmentEmitter : public Emitter {
 public:
  EnvironmentEmitter() = default;

  EnvironmentEmitter(const rapidjson::Value &_value) {
    // do nothing currently
    m_energy_scale = getFloat("energyScale", _value);
  }

  virtual ~EnvironmentEmitter() = default;

  virtual void initialize() override {
    std::cout << "Start to initialize EnvironmentEmitter\n";

    // Construct a pixel-wise importance sampling distribution
    Vector2i resolution = m_envmap->getResolution();
    int width = resolution.x, height = resolution.y;
    m_env_distribution = std::make_unique<Distribution2D>(height, width);
    for (int v = 0; v < height; ++v) {
      float vp = (float)v / height;
      float sin_theta = std::sin(M_PI * float(v + .5f) / float(height));
      for (int u = 0; u < width; ++u) {
        float up = (float)u / width;
        SpectrumRGB energy = m_envmap->evaluate(Point2f(up, vp));
        float value = (energy[0] * 0.212671f + energy[1] * 0.715160f +
                       energy[2] * 0.072169f) *
                      sin_theta;
        m_env_distribution->appendAtX(v, value);
      }
    }
    m_env_distribution->normalize();

    std::cout << "Finish envmap distribution construction\n";
  }

  virtual void setTexture(Texture *texture) override { m_envmap = texture; }

  virtual float pdf(const EmitterHitInfo &info) const override {
    /*
            assert(m_envmap != nullptr);
            double cosTheta = info.dir.y,
                   tanPhi = info.dir.z / info.dir.x;
            double theta = std::acos(cosTheta),
                   phi = std::atan(tanPhi);
            if (phi < 0)
                phi += info.dir.x > 0 ? 2 * M_PI : M_PI;
            else {
                phi += info.dir.x > 0 ? .0f : M_PI;
            }
            double u = phi / (2 * M_PI),
                   v = theta / M_PI;

            int x = u * m_envmap->getResolution().x,
                y = v * m_envmap->getResolution().y;
            return m_env_distribution->pdf(Vector2i{y, x}) /
                    (2 * M_PI * M_PI * std::sin(theta)) *
                    m_envmap->getResolution().x *
                    m_envmap->getResolution().y;
    */
    assert(m_envmap != nullptr);
    double cosTheta = info.dir.z, tanPhi = info.dir.y / info.dir.x;
    double theta = std::acos(cosTheta), phi = std::atan(tanPhi);
    if (phi < 0)
      phi += info.dir.x > 0 ? 2 * M_PI : M_PI;
    else {
      phi += info.dir.x > 0 ? .0f : M_PI;
    }
    double u = phi / (2 * M_PI), v = theta / M_PI;

    int x = u * m_envmap->getResolution().x,
        y = v * m_envmap->getResolution().y;
    return m_env_distribution->pdf(Vector2i{y, x}) /
           (2 * M_PI * M_PI * std::sin(theta)) * m_envmap->getResolution().x *
           m_envmap->getResolution().y;
  }

  virtual std::pair<SpectrumRGB, float> evaluate(
      const LightSourceInfo &info, Point3f destination) const override {
    /*
            double cosTheta = info.direction.y,
                   tanPhi = info.direction.z / (info.direction.x + EPSILON);
            double theta = std::acos(cosTheta),
                   phi = std::atan(tanPhi);
            if (phi < 0)
                phi += info.direction.x > 0 ? 2 * M_PI : M_PI;
            else {
                phi += info.direction.x > 0 ? .0f : M_PI;
            }
            return {m_envmap->evaluate(Point2f(
                phi / (2 * M_PI),
                theta / M_PI
            )) * m_energy_scale / info.pdf, info.pdf};
    */
    double cosTheta = info.direction.z,
           tanPhi = info.direction.y / (info.direction.x + EPSILON);
    double theta = std::acos(cosTheta), phi = std::atan(tanPhi);
    if (phi < 0)
      phi += info.direction.x > 0 ? 2 * M_PI : M_PI;
    else {
      phi += info.direction.x > 0 ? .0f : M_PI;
    }
    return {m_envmap->evaluate(Point2f(phi / (2 * M_PI), theta / M_PI)) *
                m_energy_scale / info.pdf,
            info.pdf};
  }

  virtual SpectrumRGB evaluate(
      const SurfaceIntersectionInfo &info) const override {
    /*
            double cosTheta = info.wi.y,
                   tanPhi = info.wi.z / info.wi.x;
            double theta = std::acos(cosTheta),
                   phi = std::atan(tanPhi);
            if (phi < 0)
                phi += info.wi.x > 0 ? 2 * M_PI : M_PI;
            else {
                phi += info.wi.x > 0 ? .0f : M_PI;
            }
    //        spdlog::info("Theta {}, Phi {}", theta, phi);
            return m_envmap->evaluate(Point2f(
                phi / (2 * M_PI),
                theta / M_PI
            )) * m_energy_scale;
    */
    double cosTheta = info.wi.z, tanPhi = info.wi.y / info.wi.x;
    double theta = std::acos(cosTheta), phi = std::atan(tanPhi);
    if (phi < 0)
      phi += info.wi.x > 0 ? 2 * M_PI : M_PI;
    else {
      phi += info.wi.x > 0 ? .0f : M_PI;
    }
    //        spdlog::info("Theta {}, Phi {}", theta, phi);
    return m_envmap->evaluate(Point2f(phi / (2 * M_PI), theta / M_PI)) *
           m_energy_scale;
  }

  virtual float pdf(const SurfaceIntersectionInfo &info) const override {
    /*
    assert(m_envmap != nullptr);
    double cosTheta = info.wi.y,
           tanPhi = info.wi.z / info.wi.x;
    double theta = std::acos(cosTheta),
           phi = std::atan(tanPhi);
    if (phi < 0)
        phi += info.wi.x > 0 ? 2 * M_PI : M_PI;
    else {
        phi += info.wi.x > 0 ? .0f : M_PI;
    }
    double u = phi / (2 * M_PI),
           v = theta / M_PI;

    int x = u * m_envmap->getResolution().x,
        y = v * m_envmap->getResolution().y;
    return m_env_distribution->pdf(Vector2i{y, x}) /
            (2 * M_PI * M_PI * std::sin(theta)) *
            m_envmap->getResolution().x *
            m_envmap->getResolution().y;
    */
    assert(m_envmap != nullptr);
    double cosTheta = info.wi.z, tanPhi = info.wi.y / info.wi.x;
    double theta = std::acos(cosTheta), phi = std::atan(tanPhi);
    if (phi < 0)
      phi += info.wi.x > 0 ? 2 * M_PI : M_PI;
    else {
      phi += info.wi.x > 0 ? .0f : M_PI;
    }
    double u = phi / (2 * M_PI), v = theta / M_PI;

    int x = u * m_envmap->getResolution().x,
        y = v * m_envmap->getResolution().y;
    return m_env_distribution->pdf(Vector2i{y, x}) /
           (2 * M_PI * M_PI * std::sin(theta)) * m_envmap->getResolution().x *
           m_envmap->getResolution().y;
  }

  virtual LightSourceInfo sampleLightSource(const IntersectionInfo &info,
                                            Point3f sample) const override {
    auto [width, height] = m_envmap->getResolution();
    float pdf = .0;
    Vector2i vu = m_env_distribution->sample({sample[1], sample[2]}, &pdf);
    double u = (double)vu[1] / width, v = (double)vu[0] / height;
    double theta = v * M_PI, phi = u * 2 * M_PI;
    /*
            Vector3f dir (
                std::sin(theta) * std::cos(phi),
                std::cos(theta),
                std::sin(theta) * std::sin(phi)
            );
    */
    Vector3f dir(std::sin(theta) * std::cos(phi),
                 std::sin(theta) * std::sin(phi), std::cos(theta));
    LightSourceInfo lightInfo;
    lightInfo.position = info.position + m_envshpere_radius * dir;
    lightInfo.lightType = LightSourceInfo::LightType::Environment;
    lightInfo.light = this;
    lightInfo.direction = dir;
    lightInfo.pdf = width * height * pdf / (2 * M_PI * M_PI * std::sin(theta));
    return lightInfo;
  }

  virtual LightSourceInfo sampleLightSource(Point3f sample) const override {
    auto [width, height] = m_envmap->getResolution();
    float pdf = .0;
    Vector2i vu = m_env_distribution->sample({sample[1], sample[2]}, &pdf);
    double u = (double)vu[1] / width, v = (double)vu[0] / height;
    double theta = v * M_PI, phi = u * 2 * M_PI;
    /*
            Vector3f dir (
                std::sin(theta) * std::cos(phi),
                std::cos(theta),
                std::sin(theta) * std::sin(phi)
            );
    */
    Vector3f dir(std::sin(theta) * std::cos(phi),
                 std::sin(theta) * std::sin(phi), std::cos(theta));
    LightSourceInfo lightInfo;
    lightInfo.position = Point3f(0) + m_envshpere_radius * dir;
    lightInfo.lightType = LightSourceInfo::LightType::Environment;
    lightInfo.light = this;
    lightInfo.direction = dir;
    lightInfo.pdf = width * height * pdf / (2 * M_PI * M_PI * std::sin(theta));
    return lightInfo;
  }

 private:
  Texture *m_envmap = nullptr;

  std::unique_ptr<Distribution2D> m_env_distribution;

  float m_envshpere_radius = 100000.f;

  float m_energy_scale = 1.f;
};

REGISTER_CLASS(EnvironmentEmitter, "envemitter")