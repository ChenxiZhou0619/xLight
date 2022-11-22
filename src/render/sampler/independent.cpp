#include "core/render-core/sampler.h"

class Independent : public Sampler {
 public:
  Independent() : Sampler() {}

  Independent(const rapidjson::Value &_value) {
    // do nothing
  }

  ~Independent() {}

  virtual void startPixel(const Point2i &p) override {
    // do nothing
  }

  virtual float next1D() override { return dist(rng); }

  virtual Point2f next2D() override { return Point2f(next1D(), next1D()); }

  virtual Point3f next3D() override {
    return Point3f{next1D(), next1D(), next1D()};
  }

  virtual std::shared_ptr<Sampler> clone() const override {
    return std::make_shared<Independent>();
  }

  virtual void nextSample() override {
    // do nothing
  }
};

REGISTER_CLASS(Independent, "independent")