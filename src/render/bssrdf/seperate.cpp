#include <core/render-core/bssrdf.h>

class SeperateBSSRDF : public BSSRDF {
public:
  SeperateBSSRDF() = default;
  SeperateBSSRDF(const rapidjson::Value &_value) {}
  virtual ~SeperateBSSRDF() = default;

  virtual SpectrumRGB sample_sp(const Scene &scene, float sample1,
                                Point2f sample2, SurfaceIntersectionInfo *info,
                                float *pdf) const override {}
  virtual SpectrumRGB evaluate_sw(const SurfaceIntersectionInfo &info,
                                  Vector3f w) const override;
  virtual float pdf_sw(const SurfaceIntersectionInfo &info,
                       Vector3f w) const override;

  virtual ScatterInfo sample_sw(const SurfaceIntersectionInfo &info,
                                Point2f sample) const override;
};

REGISTER_CLASS(SeperateBSSRDF, "seperate")