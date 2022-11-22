#include <core/render-core/integrator.h>

class VolPathTracer : public PixelIntegrator {
 public:
  VolPathTracer() : mMaxDepth(5), mRRThreshold(3) {}

  VolPathTracer(const rapidjson::Value &_value) {
    mMaxDepth = getInt("maxDepth", _value);
    mRRThreshold = getInt("rrThreshold", _value);
  }

  // *Single scattering
  virtual SpectrumRGB getLi(const Scene &scene, Ray3f ray,
                            Sampler *sampler) const override {
    SpectrumRGB Li{.0f}, beta{1.f};
    int bounces = 0;
    PathInfo pathInfo = samplePath(scene, ray, sampler);
    const auto &itsInfo = pathInfo.itsInfo;
    while (bounces <= mMaxDepth) {
      beta *= pathInfo.weight;
      if (beta.isZero()) break;
      //* Evaluate the Le
      {
        SpectrumRGB Le = itsInfo->evaluateLe();
        float pdf = itsInfo->pdfLe();
        float misw = powerHeuristic(pathInfo.pdfDirection, pdf);
        Li += beta * Le * misw;
      }
      if (itsInfo->terminate()) break;
      //* Sample direct
      {
        LightSourceInfo lightSourceInfo =
            scene.sampleLightSource(*itsInfo, sampler);
        Ray3f shadowRay = itsInfo->scatterRay(scene, lightSourceInfo.position);
        SpectrumRGB Tr = tr(scene, shadowRay);
        auto light = lightSourceInfo.light;
        auto [LeWeight, pdf] =
            light->evaluate(lightSourceInfo, itsInfo->position);
        SpectrumRGB f = itsInfo->evaluateScatter(shadowRay.dir);
        float misw = powerHeuristic(pdf, itsInfo->pdfScatter(shadowRay.dir));
        Li += beta * f * LeWeight * Tr * misw;
      }
      //* Sample the scatter
      ScatterInfo scatterInfo = itsInfo->sampleScatter(sampler->next2D());
      ray = itsInfo->scatterRay(scene, scatterInfo.wo);
      pathInfo = samplePath(scene, ray, sampler, &scatterInfo);
      if (bounces++ > mRRThreshold) {
        if (sampler->next1D() > 0.95f) break;
        beta /= 0.95;
      }
    }
    return Li;
  }

 protected:
  int mMaxDepth;
  int mRRThreshold;

  //* This method samples a intersection (surface or medium) along the ray in
  // scene,
  //* The empty surface will be skipped
  //*     1. If the scatter is medium, next scatter event will be surface
  // intersection
  //*     2. If the scatter is surface, next scatter will be surface / medium
  // intersection
  PathInfo samplePath(const Scene &scene, Ray3f ray, Sampler *sampler,
                      const ScatterInfo *info = nullptr) const {
    //* If no scatter info is provided, pdfDirection if FINF
    PathInfo pathInfo;
    pathInfo.pdfDirection = info ? info->pdf : FINF;
    pathInfo.length = 0;
    pathInfo.weight = info ? info->weight : SpectrumRGB{1};
    //* Single scattering, so no medium intersection in this situation
    if (info && info->scatterType == ScatterInfo::ScatterType::Medium) {
      std::shared_ptr<SurfaceIntersectionInfo> sIts;
      do {
        sIts = scene.intersectWithSurface(ray);
        if (auto medium = ray.medium; medium) {
          pathInfo.weight *= medium->evaluateTr(ray.ori, sIts->position);
        }
        pathInfo.length += sIts->distance;
        if (sIts->terminate()) break;
        ray = sIts->scatterRay(scene, ray.dir);
      } while (sIts->shape &&
               sIts->shape->getBSDF()->m_type == BSDF::EBSDFType::EEmpty);
      pathInfo.itsInfo = sIts;
      pathInfo.vertex = sIts->position;
    }
    //* Sample a medium intersection or surface intersection along the ray
    else {
      std::shared_ptr<SurfaceIntersectionInfo> sIts;
      do {
        sIts = scene.intersectWithSurface(ray);
        if (auto medium = ray.medium; medium) {
          auto mIts = medium->sampleIntersection(ray, sIts->distance,
                                                 sampler->next2D());
          pathInfo.weight *= mIts->weight;
          if (mIts->medium) {
            //* Successfully sample a medium intersection
            pathInfo.itsInfo = mIts;
            pathInfo.vertex = mIts->position;
            pathInfo.length += mIts->distance;
            break;
          }
        }
        //* No, hit the surface
        pathInfo.itsInfo = sIts;
        pathInfo.vertex = sIts->position;
        pathInfo.length += sIts->distance;
        if (sIts->terminate()) break;
        ray = sIts->scatterRay(scene, ray.dir);
      } while (sIts->shape &&
               sIts->shape->getBSDF()->m_type == BSDF::EBSDFType::EEmpty);
    }
    return pathInfo;
  }

  SpectrumRGB tr(const Scene &scene, Ray3f ray) const {
    SpectrumRGB Tr{1.f};
    Point3f destination = ray.at(ray.tmax - 0.001);
    auto info = scene.intersectWithSurface(ray);
    while (true) {
      if (info->shape &&
          info->shape->getBSDF()->m_type != BSDF::EBSDFType::EEmpty) {
        Tr = SpectrumRGB{.0f};
        break;
      } else if (!info->shape) {
        if (auto medium = ray.medium; medium) {
          Tr *= medium->evaluateTr(ray.ori, destination);
        }
        break;
      } else if (info->shape->getBSDF()->m_type == BSDF::EBSDFType::EEmpty) {
        if (auto medium = ray.medium; medium) {
          Tr *= medium->evaluateTr(ray.ori, info->position);
        }
        ray = info->scatterRay(scene, ray.dir);
        info = scene.intersectWithSurface(ray);
      }
    }
    return Tr;
  }
};
REGISTER_CLASS(VolPathTracer, "volpath-tracer")

class VolPathTracerPlus : public PixelIntegrator {
 public:
  VolPathTracerPlus() : mMaxDepth(5), mRRThreshold(3) {}

  VolPathTracerPlus(const rapidjson::Value &_value) {
    mMaxDepth = getInt("maxDepth", _value);
    mRRThreshold = getInt("rrThreshold", _value);
  }

  virtual SpectrumRGB getLi(const Scene &scene, Ray3f ray,
                            Sampler *sampler) const override {
    return equi_angular(scene, ray, sampler);
    SpectrumRGB Li{.0f}, beta{1.f};
    int bounces = 0;
    auto paths = samplePath(scene, ray, sampler);
    const auto &surfacePath = paths.first, &mediumPath = paths.second;
    while (bounces <= mMaxDepth) {
      //* Evaluate Le
      // medium no emission now
      // ...
      //* Surface emission
      {
        SpectrumRGB Le = surfacePath.itsInfo->evaluateLe();
        float pdf = surfacePath.itsInfo->pdfLe();
        float misw = powerHeuristic(surfacePath.pdfDirection, pdf);
        if (!surfacePath.weight.isZero())
          Li += beta * surfacePath.weight * Le * misw;
      }

      //* Evaluate direct
      {
        //* Evaluate medium direct
        if (mediumPath.itsInfo) {
          const auto &itsInfo = mediumPath.itsInfo;
          LightSourceInfo lightSourceInfo =
              scene.sampleLightSource(*itsInfo, sampler);
          Ray3f shadowRay =
              itsInfo->scatterRay(scene, lightSourceInfo.position);
          SpectrumRGB Tr = tr(scene, shadowRay);
          auto light = lightSourceInfo.light;
          auto [LeWeight, pdf] =
              light->evaluate(lightSourceInfo, itsInfo->position);
          SpectrumRGB f = itsInfo->evaluateScatter(shadowRay.dir);
          float misw = powerHeuristic(pdf, itsInfo->pdfScatter(shadowRay.dir));
          if (!mediumPath.weight.isZero())
            Li += beta * mediumPath.weight * f * LeWeight * Tr * misw;
        }
        //* Evaluate surface direct
        if (!surfacePath.itsInfo->terminate()) {
          const auto &itsInfo = surfacePath.itsInfo;
          LightSourceInfo lightSourceInfo =
              scene.sampleLightSource(*itsInfo, sampler);
          Ray3f shadowRay =
              itsInfo->scatterRay(scene, lightSourceInfo.position);
          SpectrumRGB Tr = tr(scene, shadowRay);
          auto light = lightSourceInfo.light;
          auto [LeWeight, pdf] =
              light->evaluate(lightSourceInfo, itsInfo->position);
          SpectrumRGB f = itsInfo->evaluateScatter(shadowRay.dir);
          float misw = powerHeuristic(pdf, itsInfo->pdfScatter(shadowRay.dir));
          if (!surfacePath.weight.isZero())
            Li += beta * surfacePath.weight * f * LeWeight * Tr * misw;
        }
      }
      //* If no mediumIntersection and surfaceIntersection terminate
      if (!mediumPath.itsInfo && surfacePath.itsInfo->terminate()) break;

      //* Choose one path to continue
      PathInfo pathInfo;
      if (surfacePath.itsInfo->terminate()) {
        pathInfo = mediumPath;
      } else if (!mediumPath.itsInfo) {
        pathInfo = surfacePath;
      } else {
        float xi = sampler->next1D(),
              ratio =
                  mediumPath.weight.average() + surfacePath.weight.average(),
              sample = xi * ratio;
        //                if (ratio == 0) break;
        if (sample < mediumPath.weight.average()) {
          //* Choose medium scatter
          pathInfo = mediumPath;
          pathInfo.weight /= (mediumPath.weight.average() / ratio);
        } else {
          //* Choose surface scatter
          pathInfo = surfacePath;
          pathInfo.weight /= (surfacePath.weight.average() / ratio);
        }
      }
      beta *= pathInfo.weight;
      //* Sample scatter
      ScatterInfo scatterInfo =
          pathInfo.itsInfo->sampleScatter(sampler->next2D());
      ray = pathInfo.itsInfo->scatterRay(scene, scatterInfo.wo);
      paths = samplePath(scene, ray, sampler, &scatterInfo);
      //* rr
      if (bounces++ > mRRThreshold) {
        if (sampler->next1D() > 0.95f) break;
        beta /= 0.95;
      }
    }
    return Li;
  }

  SpectrumRGB equi_angular(const Scene &scene, Ray3f ray,
                           Sampler *sampler) const {
    SpectrumRGB Li{.0f}, beta{1.f};
    int bounces = 0;
    LightSourceInfo lightInfo = scene.sampleLightSource(sampler);
    auto paths = samplePath(scene, ray, sampler, nullptr, &lightInfo);
    const auto &surfacePath = paths.first, &mediumPath = paths.second;

    while (bounces <= mMaxDepth) {
      //* Evaluate Le
      // medium no emission now
      // ...
      //* Surface emission
      {
        SpectrumRGB Le = surfacePath.itsInfo->evaluateLe();
        float pdf = surfacePath.itsInfo->pdfLe();
        float misw = powerHeuristic(surfacePath.pdfDirection, pdf);
        if (!surfacePath.weight.isZero())
          Li += beta * surfacePath.weight * Le * misw;
      }

      //* Evaluate direct
      {
        //* Evaluate medium direct
        if (mediumPath.itsInfo) {
          const auto &itsInfo = mediumPath.itsInfo;
          Ray3f shadowRay = itsInfo->scatterRay(scene, lightInfo.position);
          SpectrumRGB Tr = tr(scene, shadowRay);
          auto light = lightInfo.light;
          auto [LeWeight, pdf] = light->evaluate(lightInfo, itsInfo->position);
          SpectrumRGB f = itsInfo->evaluateScatter(shadowRay.dir);
          float misw = powerHeuristic(pdf, itsInfo->pdfScatter(shadowRay.dir));
          if (!mediumPath.weight.isZero())
            Li += beta * mediumPath.weight * f * LeWeight * Tr * misw;
        }
        //* Evaluate surface direct
        if (!surfacePath.itsInfo->terminate()) {
          const auto &itsInfo = surfacePath.itsInfo;
          Ray3f shadowRay = itsInfo->scatterRay(scene, lightInfo.position);
          SpectrumRGB Tr = tr(scene, shadowRay);
          auto light = lightInfo.light;
          auto [LeWeight, pdf] = light->evaluate(lightInfo, itsInfo->position);
          SpectrumRGB f = itsInfo->evaluateScatter(shadowRay.dir);
          float misw = powerHeuristic(pdf, itsInfo->pdfScatter(shadowRay.dir));
          if (!surfacePath.weight.isZero())
            Li += beta * surfacePath.weight * f * LeWeight * Tr * misw;
        }
      }
      if (!mediumPath.itsInfo && surfacePath.itsInfo->terminate()) break;

      //* Choose one path to continue
      PathInfo pathInfo;
      if (surfacePath.itsInfo->terminate()) {
        pathInfo = mediumPath;
      } else if (!mediumPath.itsInfo) {
        pathInfo = surfacePath;
      } else {
        float xi = Sampler::sample1D(),
              ratio =
                  mediumPath.weight.average() + surfacePath.weight.average(),
              sample = xi * ratio;
        if (ratio == 0) break;
        if (sample < mediumPath.weight.average()) {
          //* Choose medium scatter
          pathInfo = mediumPath;
          pathInfo.weight /= (mediumPath.weight.average() / ratio);
        } else {
          //* Choose surface scatter
          pathInfo = surfacePath;
          pathInfo.weight /= (surfacePath.weight.average()) / ratio;
        }
      }

      beta *= pathInfo.weight;
      ScatterInfo scatterInfo =
          pathInfo.itsInfo->sampleScatter(sampler->next2D());
      ray = pathInfo.itsInfo->scatterRay(scene, scatterInfo.wo);
      lightInfo = scene.sampleLightSource(sampler);
      paths = samplePath(scene, ray, sampler, &scatterInfo, &lightInfo);
      //* rr
      if (bounces++ > mRRThreshold) {
        if (sampler->next1D() > 0.95f) break;
        beta /= 0.95;
      }
    }
    return Li;
  }

 protected:
  int mMaxDepth;
  int mRRThreshold;

  //* This method samples a pair intersection (surface and medium)
  //* The empty will be skipped
  //*     1. If the scatter is medium, only surfaceIts is valid
  //*     2. If the scatter is surface, return two
  std::pair<PathInfo, PathInfo> samplePath(
      const Scene &scene, Ray3f ray, Sampler *sampler,
      const ScatterInfo *info = nullptr,
      const LightSourceInfo *lightInfo = nullptr) const {
    PathInfo surfacePath, mediumPath;
    surfacePath.pdfDirection = mediumPath.pdfDirection =
        info ? info->pdf : FINF;
    surfacePath.length = mediumPath.length = 0;
    surfacePath.weight = mediumPath.weight =
        info ? info->weight : SpectrumRGB{1};
    std::shared_ptr<SurfaceIntersectionInfo> sIts;
    do {
      sIts = scene.intersectWithSurface(ray);
      if (auto medium = ray.medium; medium && !mediumPath.itsInfo) {
        surfacePath.weight *= medium->evaluateTr(ray.ori, sIts->position);
        if (!mediumPath.itsInfo &&
            (!info ||
             info && info->scatterType == ScatterInfo::ScatterType::Surface)) {
          auto mIts = lightInfo ? medium->sampleIntersectionEquiAngular(
                                      ray, sIts->distance, sampler->next2D(),
                                      *lightInfo)
                                : medium->sampleIntersectionDeterministic(
                                      ray, sIts->distance, sampler->next2D());
          if (mIts) {
            mediumPath.weight *= mIts->weight;
            mediumPath.vertex = mIts->position;
            mediumPath.length += mIts->distance;
            mediumPath.itsInfo = mIts;
          }
        }
      }
      surfacePath.itsInfo = sIts;
      surfacePath.vertex = sIts->position;
      surfacePath.length += sIts->distance;
      if (sIts->terminate()) break;
      ray = sIts->scatterRay(scene, ray.dir);
    } while (sIts->shape &&
             sIts->shape->getBSDF()->m_type == BSDF::EBSDFType::EEmpty);
    return {surfacePath, mediumPath};
  }

  SpectrumRGB tr(const Scene &scene, Ray3f ray) const {
    SpectrumRGB Tr{1.f};
    Point3f destination = ray.at(ray.tmax);
    auto info = scene.intersectWithSurface(ray);
    while (true) {
      if (info->shape &&
          info->shape->getBSDF()->m_type != BSDF::EBSDFType::EEmpty) {
        Tr = SpectrumRGB{.0f};
        break;
      } else if (!info->shape) {
        if (auto medium = ray.medium; medium) {
          Tr *= medium->evaluateTr(ray.ori, destination);
        }
        break;
      } else if (info->shape->getBSDF()->m_type == BSDF::EBSDFType::EEmpty) {
        if (auto medium = ray.medium; medium) {
          Tr *= medium->evaluateTr(ray.ori, info->position);
        }
        ray = info->scatterRay(scene, ray.dir);
        info = scene.intersectWithSurface(ray);
      }
    }
    return Tr;
  }
};
REGISTER_CLASS(VolPathTracerPlus, "volpath-tracer-plus")