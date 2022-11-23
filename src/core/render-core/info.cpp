#include <core/math/common.h>
#include <core/render-core/info.h>
#include <core/scene/scene.h>

//* IntersectionInfo
Vector3f IntersectionInfo::toLocal(Vector3f v) const {
  return shadingFrame.toLocal(v);
}
Vector3f IntersectionInfo::toWorld(Vector3f v) const {
  return shadingFrame.toWorld(v);
}

//* SurfaceIntersectionInfo
Ray3f SurfaceIntersectionInfo::scatterRay(const Scene &scene,
                                          Point3f destination) const {
  Ray3f ray{position, destination};
  ray.ori += ray.dir * 0.0001;
  bool outwards = dot(ray.dir, geometryNormal) > 0;
  auto medium = outwards ? scene.getEnvMedium() : shape->getInsideMedium();
  ray.medium = medium ? medium.get() : nullptr;
  return ray;
}

Ray3f SurfaceIntersectionInfo::scatterRay(const Scene &scene,
                                          Vector3f direction) const {
  Ray3f ray{position, direction};
  ray.ori += ray.dir * 0.0001;
  bool outwards = dot(ray.dir, geometryNormal) > 0;
  auto medium = outwards ? scene.getEnvMedium() : shape->getInsideMedium();
  ray.medium = medium ? medium.get() : nullptr;
  return ray;
}

SpectrumRGB SurfaceIntersectionInfo::evaluateScatter(Vector3f wo) const {
  assert(shape);
  return shape->getBSDF()->evaluate(*this, wo);
}

float SurfaceIntersectionInfo::pdfScatter(Vector3f wo) const {
  assert(shape);
  return shape->getBSDF()->pdf(*this, wo);
}

float SurfaceIntersectionInfo::pdfScatter(Vector3f wi, Vector3f wo) const {
  assert(shape);
  return shape->getBSDF()->pdf(*this, wi, wo);
}

ScatterInfo SurfaceIntersectionInfo::sampleScatter(Point2f sample) const {
  assert(shape);
  auto sInfo = shape->getBSDF()->sample(*this, sample);
  sInfo.scatterType = ScatterInfo::ScatterType::Surface;
  return sInfo;
}

SpectrumRGB SurfaceIntersectionInfo::evaluateLe() const {
  return light ? light->evaluate(*this) : SpectrumRGB{.0f};
}

float SurfaceIntersectionInfo::pdfLe() const {
  return light ? light->pdf(*this) : .0f;
}

bool SurfaceIntersectionInfo::terminate() const { return (!shape); }

void SurfaceIntersectionInfo::computeShadingFrame() {
  if (!terminate()) this->shape->getBSDF()->computeShadingFrame(this);
}

void SurfaceIntersectionInfo::computeDifferential(const Ray3f &ray) {
  if (!ray.is_ray_differential) return;
  //* Compute dudx, dudy, dvdx and dvdy
  float d = dot(geometryNormal, Vector3f{position.x, position.y, position.z});
  float tx = -(dot(geometryNormal, Vector3f{ray.ori.x, ray.ori.y, ray.ori.z}) -
               d) /
             dot(geometryNormal, ray.direction_dx),
        ty = -(dot(geometryNormal, Vector3f{ray.ori.x, ray.ori.y, ray.ori.z}) -
               d) /
             dot(geometryNormal, ray.direction_dy);

  if (std::isinf(tx) || std::isnan(tx) || std::isinf(ty) || std::isnan(ty)) {
    std::cout << "NaN or inf in computeDifferential\n";
    std::exit(1);
  }

  Point3f px = ray.at(tx), py = ray.at(ty);

  Vector3f dpdx = px - position, dpdy = py - position;

  int dim[2];
  if (std::abs(geometryNormal.x) > std::abs(geometryNormal.y) &&
      std::abs(geometryNormal.x) > std::abs(geometryNormal.z)) {
    dim[0] = 1;
    dim[1] = 2;
  } else if (std::abs(geometryNormal.y) > std::abs(geometryNormal.z)) {
    dim[0] = 0;
    dim[1] = 2;
  } else {
    dim[0] = 0;
    dim[1] = 1;
  }
  // Initialize _A_, _Bx_, and _By_ matrices for offset computation
  float A[2][2] = {{dpdu[dim[0]], dpdv[dim[0]]}, {dpdu[dim[1]], dpdv[dim[1]]}};
  float Bx[2] = {px[dim[0]] - position[dim[0]], px[dim[1]] - position[dim[1]]};
  float By[2] = {py[dim[0]] - position[dim[0]], py[dim[1]] - position[dim[1]]};
  if (!solveLinearSys2X2(A, Bx, &dudx, &dvdx)) {
    dudx = dvdx = 0;
  }
  if (!solveLinearSys2X2(A, By, &dudy, &dvdy)) {
    dudy = dvdy = 0;
  }
}

//* MediumIntersectionInfo
Ray3f MediumIntersectionInfo::scatterRay(const Scene &scene,
                                         Point3f destination) const {
  Ray3f ray{position, destination};
  ray.medium = medium;
  return ray;
}

Ray3f MediumIntersectionInfo::scatterRay(const Scene &scene,
                                         Vector3f direction) const {
  Ray3f ray{position, direction};
  ray.medium = medium;
  return ray;
}

SpectrumRGB MediumIntersectionInfo::evaluateScatter(Vector3f wo) const {
  assert(medium);
  PhaseQueryRecord pRec{position, wi, wo};
  return medium->evaluatePhase(pRec);
}

float MediumIntersectionInfo::pdfScatter(Vector3f wo) const {
  assert(medium);
  PhaseQueryRecord pRec{position, wi, wo};
  return medium->pdfPhase(pRec);
}

float MediumIntersectionInfo::pdfScatter(Vector3f _wi, Vector3f _wo) const {
  assert(medium);
  PhaseQueryRecord pRec{position, _wi, _wo};
  return medium->pdfPhase(pRec);
}

ScatterInfo MediumIntersectionInfo::sampleScatter(Point2f sample) const {
  assert(medium);
  ScatterInfo info;
  PhaseQueryRecord pRec{position, wi};
  info.weight = medium->samplePhase(&pRec, sample);
  info.wo = toWorld(pRec.wo);
  info.pdf = medium->pdfPhase(pRec);
  info.scatterType = ScatterInfo::ScatterType::Medium;
  return info;
}

SpectrumRGB MediumIntersectionInfo::evaluateLe() const {
  // todo no emission medium now
  return SpectrumRGB{0};
}

float MediumIntersectionInfo::pdfLe() const {
  // todo no emission medium now
  return .0f;
}

bool MediumIntersectionInfo::terminate() const {
  // todo no emission, so always false
  return false;
}

void MediumIntersectionInfo::computeShadingFrame() {
  //* do nothing
}

void MediumIntersectionInfo::computeDifferential(const Ray3f &ray) {
  //* do nothing
}