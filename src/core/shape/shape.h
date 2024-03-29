#pragma once
#include <core/geometry/geometry.h>
#include <core/render-core/emitter.h>
#include <embree3/rtcore.h>
class Emitter;
class BSDF;
class BSSRDF;

class ShapeInterface {
public:
  bool two_side = false;
  friend class Scene;

  ShapeInterface() = default;

  virtual ~ShapeInterface() = default;

  virtual void initEmbreeGeometry(RTCDevice device) = 0;

  //* Only for mesh like shape
  virtual Point3f getHitPoint(int triIdx, Point2f uv) const = 0;

  //* Only for mesh like shape
  virtual Normal3f getHitNormal(int triIdx, Point2f uv) const = 0;

  virtual Normal3f getHitNormal(int triIdx) const = 0;

  virtual Vector3f getHitTangent(int triIdx, Point2f uv) const = 0;

  virtual Vector3f dpdu(int triIdx) const = 0;

  virtual Vector3f dpdv(int triIdx) const = 0;

  //* Only for mesh like shape
  virtual Point2f getHitTextureCoordinate(int triIdx, Point2f) const = 0;

  bool HasUV() const { return hasUV; }

  bool HasTangent() const { return hasTangent; }

  bool isEmitter() const { return emitter != nullptr; }

  std::shared_ptr<Emitter> getEmitter() const { return emitter; }

  std::shared_ptr<BSDF> getBSDF() const { return bsdf; }

  std::shared_ptr<BSSRDF> getBSSRDF() const { return bssrdf; }

  virtual void sampleOnSurface(PointQueryRecord *pRec,
                               Point3f sample) const = 0;

  float getSurfaceArea() const { return m_surface_area; }

  void setBSDF(std::shared_ptr<BSDF> bsdf) { this->bsdf = bsdf; }

  void setBSSRDF(std::shared_ptr<BSSRDF> bssrdf) { this->bssrdf = bssrdf; }

  void setEmitter(std::shared_ptr<Emitter> emitter) { this->emitter = emitter; }

  std::shared_ptr<Medium> getInsideMedium() const { return medium; }

  std::shared_ptr<Medium> getOutsideMedium() const { return nullptr; }

  bool hasMedium() const { return medium != nullptr; }

  void setMedium(std::shared_ptr<Medium> medium) { this->medium = medium; }

  //* Return dpdu and dpdv
  virtual std::pair<Vector3f, Vector3f>
  positionDifferential(int triIdx) const = 0;

protected:
  virtual Point3f getVertex(int idx) const = 0;

  virtual Point3ui getFace(int idx) const = 0;

  virtual Normal3f getNormal(int idx) const = 0;

  virtual Point2f getUV(int idx) const = 0;

  RTCGeometry embreeGeometry;

  bool hasUV = false;

  bool hasTangent = false;

  float m_surface_area;

  std::shared_ptr<Emitter> emitter;

  std::shared_ptr<BSDF> bsdf;

  std::shared_ptr<BSSRDF> bssrdf;

  std::shared_ptr<Medium> medium;
};

struct ShapeIntersection {
  float distance;

  Point3f hitPoint;

  Normal3f geometryN, shadingN;

  Frame geometryF, shadingF;

  Point2f uv;

  Vector3f dpdu, dpdv;

  std::shared_ptr<ShapeInterface> shape;

  int primID = -1;

  Vector3f toWorld(const Vector3f &local) const {
    return shadingF.toWorld(local);
  }

  Vector3f toLocal(const Vector3f &world) const {
    return shadingF.toLocal(world);
  }

  void clear() { *this = ShapeIntersection(); }
};