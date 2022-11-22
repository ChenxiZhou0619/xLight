#include <core/render-core/info.h>

#include "core/render-core/emitter.h"

class SpotEmitter : public Emitter {
 public:
  SpotEmitter() = default;

  SpotEmitter(const rapidjson::Value &_value) {
    lightEnergy = getSpectrumRGB("lightEnergy", _value);
    position = getPoint3f("position", _value);
  }

  virtual ~SpotEmitter() = default;

  // todo delete this
  virtual void setTexture(Texture *envmap) override {}

  virtual float pdf(const EmitterHitInfo &info) const override { return 1; }

  virtual std::pair<SpectrumRGB, float> evaluate(
      const LightSourceInfo &info, Point3f destination) const override {
    return {lightEnergy / (destination - info.position).length2(), FINF};
  }

  virtual SpectrumRGB evaluate(
      const SurfaceIntersectionInfo &itsInfo) const override {
    //* No implement
    std::cout << "No implement!\n";
    std::exit(1);
  }

  virtual float pdf(const SurfaceIntersectionInfo &info) const override {
    return FINF;
  }

  virtual LightSourceInfo sampleLightSource(const IntersectionInfo &info,
                                            Point3f sample) const override {
    LightSourceInfo lightInfo;
    lightInfo.lightType = LightSourceInfo::LightType::Spot;
    lightInfo.light = this;
    lightInfo.direction = normalize(position - info.position);
    lightInfo.position = position;
    lightInfo.pdf = FINF;
    lightInfo.Le = lightEnergy;
    return lightInfo;
  }

  virtual LightSourceInfo sampleLightSource(Point3f sample) const override {
    LightSourceInfo lightInfo;
    lightInfo.lightType = LightSourceInfo::LightType::Spot;
    lightInfo.light = this;
    lightInfo.position = position;
    lightInfo.pdf = FINF;
    lightInfo.Le = lightEnergy;
    return lightInfo;
  }

 protected:
  Point3f position;
  SpectrumRGB lightEnergy;
};
REGISTER_CLASS(SpotEmitter, "spot")