#include <core/math/common.h>
#include <core/render-core/integrator.h>
#include <spdlog/spdlog.h>

class PathTracer : public PixelIntegrator {
public:
  PathTracer() : mMaxDepth(5), mRRThreshold(3) {}

  PathTracer(const rapidjson::Value &_value) {
    mMaxDepth = getInt("maxDepth", _value);
    mRRThreshold = getInt("rrThreshold", _value);
  }

  virtual SpectrumRGB getLi(const Scene &scene, Ray3f ray,
                            Sampler *sampler) const override {
    SpectrumRGB Li{.0f}, beta{1.f};
    int bounces = 0;
    PathInfo pathInfo = samplePath(scene, ray, FINF, SpectrumRGB{1});
    const auto &itsInfo = pathInfo.itsInfo;
    while (true) {
      beta *= pathInfo.weight;
      if (beta.isZero())
        break;
      //* Evaluate the Le using mis
      //* <L> = beta * Le * misw(pdfPath, pdfLe)
      {
        // give the emission of the hitpoint and the pdf of sampling this point
        // when sample direct
        SpectrumRGB Le = itsInfo->evaluateLe();
        float pdf = itsInfo->pdfLe();
        float misw = powerHeuristic(pathInfo.pdfDirection, pdf);
        Li += beta * Le * misw;
      }
      //* Not hit the scene, terminate
      if (itsInfo->terminate())
        break;
      if (bounces >= mMaxDepth)
        break;
      //* Sample the direct
      {
        LightSourceInfo lightSourceInfo =
            scene.sampleLightSource(*itsInfo, sampler);
        Ray3f shadowRay = itsInfo->scatterRay(scene, lightSourceInfo.position);
        if (!scene.occlude(shadowRay)) {
          auto light = lightSourceInfo.light;
          auto [LeWeight, pdf] =
              light->evaluate(lightSourceInfo, itsInfo->position);
          SpectrumRGB f = itsInfo->evaluateScatter(shadowRay.dir);
          float misw = powerHeuristic(pdf, itsInfo->pdfScatter(shadowRay.dir));
          if (!f.isZero()) {
            Li += beta * f * LeWeight * misw;
          }
        }
      }
      //* Sample the bsdf
      ScatterInfo scatterInfo = itsInfo->sampleScatter(sampler->next2D());
      ray = itsInfo->scatterRay(scene, scatterInfo.wo);
      pathInfo = samplePath(scene, ray, scatterInfo.pdf, scatterInfo.weight);
      if (bounces++ > mRRThreshold) {
        if (sampler->next1D() > 0.95f)
          break;
        beta /= 0.95;
      }
    }
    return Li;
  }

protected:
  PathInfo samplePath(const Scene &scene, Ray3f ray, float pdfDirection,
                      SpectrumRGB weight) const {
    PathInfo pathInfo;
    //* In path tracer(with out volumes), length sampling is a dirac delta
    // distribution(given a direction)
    auto itsInfo = scene.intersectWithSurface(ray);
    pathInfo.itsInfo = itsInfo;
    pathInfo.length = itsInfo->distance;
    pathInfo.pdfLength = FINF;
    pathInfo.pdfDirection = pdfDirection;
    pathInfo.vertex = itsInfo->position;
    pathInfo.weight = weight;
    return pathInfo;
  }

protected:
  int mMaxDepth;
  int mRRThreshold;
};

REGISTER_CLASS(PathTracer, "path-tracer")