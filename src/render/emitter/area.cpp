#include <core/render-core/info.h>

#include "core/render-core/emitter.h"
#include "core/shape/shape.h"
class AreaEmitter : public Emitter {
  SpectrumRGB m_lightEnergy;

 public:
  AreaEmitter() = default;
  AreaEmitter(const rapidjson::Value &_value) {
    m_lightEnergy = getSpectrumRGB("lightEnergy", _value);
  }
  ~AreaEmitter() = default;

  virtual void setTexture(Texture *texture) override {
    //! no implement
    std::cout << "AreaEmitter::setTexture no implement!\n";
    std::exit(1);
  }
  // todo depend the sampling strategy
  virtual float pdf(const EmitterHitInfo &info) const override {
    if (auto shape_ptr = shape.lock(); shape_ptr) {
      float pdf = 1.f / shape_ptr->getSurfaceArea(),
            jacob =
                info.dist * info.dist / std::abs(dot(info.normal, info.dir));
      return pdf * jacob;
    }
    return .0f;
  }

  virtual std::pair<SpectrumRGB, float> evaluate(
      const LightSourceInfo &info, Point3f destination) const override {
    Vector3f light2point = destination - info.position;
    if (dot(light2point, info.normal) <= 0) return {SpectrumRGB{0}, .0f};
    float jacob = light2point.length2() /
                  std::abs(dot(info.normal, normalize(light2point))),
          pdf = info.pdf * jacob;
    return {m_lightEnergy / pdf, pdf};
  }

  virtual SpectrumRGB evaluate(
      const SurfaceIntersectionInfo &info) const override {
    if (dot(info.wi, info.geometryNormal) <= 0) return SpectrumRGB{0};
    return m_lightEnergy;
  }

  virtual float pdf(const SurfaceIntersectionInfo &info) const override {
    auto shape_ptr = shape.lock();
    assert(shape_ptr != nullptr);
    float pdf = 1.f / shape_ptr->getSurfaceArea(),
          jacob = info.distance * info.distance /
                  std::abs(dot(info.geometryNormal, info.wi));
    return pdf * jacob;
  }

  virtual LightSourceInfo sampleLightSource(const IntersectionInfo &info,
                                            Point3f sample) const override {
    auto shape_ptr = shape.lock();
    assert(shape_ptr != nullptr);

    PointQueryRecord pRec;
    shape_ptr->sampleOnSurface(&pRec, sample);

    LightSourceInfo lightInfo;
    lightInfo.lightType = LightSourceInfo::LightType::Area;
    lightInfo.position = pRec.p;
    lightInfo.normal = pRec.normal;
    lightInfo.direction = normalize(pRec.p - info.position);
    lightInfo.light = this;
    lightInfo.Le = m_lightEnergy;
    //        lightInfo.pdf = pRec.pdf * (lightInfo.position -
    //        info.position).length2() /
    //                        std::abs(dot(pRec.normal, normalize(pRec.p -
    //                        info.position)));
    lightInfo.pdf = pRec.pdf;
    return lightInfo;
  }

  virtual LightSourceInfo sampleLightSource(Point3f sample) const override {
    auto shape_ptr = shape.lock();
    assert(shape_ptr != nullptr);

    PointQueryRecord pRec;
    shape_ptr->sampleOnSurface(&pRec, sample);

    LightSourceInfo lightInfo;
    lightInfo.lightType = LightSourceInfo::LightType::Area;
    lightInfo.position = pRec.p;
    lightInfo.normal = pRec.normal;
    lightInfo.light = this;
    lightInfo.pdf = pRec.pdf;
    lightInfo.Le = m_lightEnergy;
    return lightInfo;
  }
};

REGISTER_CLASS(AreaEmitter, "area")