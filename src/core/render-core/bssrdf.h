#pragma once

#include <core/render-core/info.h>
#include <core/utils/configurable.h>

class Scene;
struct SurfaceIntersectionInfo;

class BSSRDF : public Configurable {
public:
  BSSRDF() = default;
  BSSRDF(const rapidjson::Value &_value){};
  virtual ~BSSRDF() = default;

  virtual SpectrumRGB sample_sp(const Scene &scene, float sample1,
                                Point2f sample2, SurfaceIntersectionInfo *info,
                                float *pdf) const = 0;

  virtual SpectrumRGB evaluate_sw(const SurfaceIntersectionInfo &info,
                                  Vector3f w) const = 0;
  virtual float pdf_sw(const SurfaceIntersectionInfo &info,
                       Vector3f w) const = 0;
  virtual ScatterInfo sample_sw(const SurfaceIntersectionInfo &info,
                                Point2f sample) const = 0;
};