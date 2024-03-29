#pragma once
#include "core/shape/shape.h"
#include "core/utils/configurable.h"
#include "spectrum.h"
#include "texture.h"

struct SurfaceIntersectionInfo;
struct ScatterInfo;
enum class ScatterSampleType;

struct BSDFQueryRecord {
  // ! both point from the origin in local
  Vector3f wi, wo;
  Point2f uv;
  float du, dv;
  bool isDelta = false;

  BSDFQueryRecord() = default;
  BSDFQueryRecord(const Vector3f &_wi) : wi(_wi) {}
  BSDFQueryRecord(const Vector3f &_wi, Point2f _uv) : wi(_wi), uv(_uv) {}
  BSDFQueryRecord(const Vector3f &_wi, const Vector3f &_wo)
      : wi(_wi), wo(_wo) {}
  BSDFQueryRecord(const Vector3f &_wi, const Vector3f &_wo, Point2f _uv)
      : wi(_wi), wo(_wo), uv(_uv) {}

  BSDFQueryRecord(const RayIntersectionRec &i_rec, const Ray3f &ri) {
    wi = i_rec.toLocal(-ri.dir);
    uv = i_rec.UV;
    if (ri.is_ray_differential && i_rec.can_diff) {
      du = i_rec.dudx + i_rec.dudy;
      dv = i_rec.dvdx + i_rec.dvdy;
    } else {
      du = dv = .0f;
    }
  }

  BSDFQueryRecord(const RayIntersectionRec &i_rec, const Ray3f &ri,
                  const Ray3f &ro) {
    wi = i_rec.toLocal(-ri.dir);
    wo = i_rec.toLocal(ro.dir);
    uv = i_rec.UV;
    if (ri.is_ray_differential && i_rec.can_diff) {
      du = i_rec.dudx + i_rec.dudy;
      dv = i_rec.dvdx + i_rec.dvdy;
    } else {
      du = dv = .0f;
    }
  }

  BSDFQueryRecord(const ShapeIntersection &its, Ray3f ri, Ray3f ro) {
    wi = its.toLocal(-ri.dir);
    wo = its.toLocal(ro.dir);
    uv = its.uv;
    du = dv = 0;
  }

  BSDFQueryRecord(const ShapeIntersection &its, Vector3f _wi, Vector3f _wo) {
    wi = its.toLocal(_wi);
    wo = its.toLocal(_wo);
    uv = its.uv;
    du = dv = 0;
  }

  BSDFQueryRecord(const ShapeIntersection &its, Vector3f _wi) {
    wi = its.toLocal(_wi);
    uv = its.uv;
    du = dv = 0;
  }

  BSDFQueryRecord(const ShapeIntersection &its, Ray3f ri) {
    wi = its.toLocal(-ri.dir);
    uv = its.uv;
    du = dv = 0;
  }
};

class BSDF : public Configurable {
 protected:
  std::shared_ptr<Texture> m_texture;
  Texture *m_bumpmap = nullptr;
  std::shared_ptr<Texture> m_normalmap = nullptr;

 public:
  BSDF() = default;
  BSDF(const rapidjson::Value &_value);
  virtual ~BSDF() = default;

  void setTexture(std::shared_ptr<Texture> texture) { m_texture = texture; }

  void setBumpmap(Texture *texture) { m_bumpmap = texture; }

  void setNormalmap(std::shared_ptr<Texture> texture) { m_normalmap = texture; }

  virtual void initialize() {
    // do nothing
  }

  /**
   * @brief return the bsdf value * cosTheta
   *
   * @param iRec
   * @return SpectrumRGB
   */
  virtual SpectrumRGB evaluate(const BSDFQueryRecord &bRec) const = 0;

  SpectrumRGB evaluate(const SurfaceIntersectionInfo &info, Vector3f wo) const;

  ScatterInfo sample(const SurfaceIntersectionInfo &info, Point2f sample) const;

  float pdf(const SurfaceIntersectionInfo &info, Vector3f wo) const;

  float pdf(const SurfaceIntersectionInfo &info, Vector3f wi,
            Vector3f wo) const;

  /**
   * @brief return the pdf of given wo
   *
   * @param iRec
   * @return float
   */
  virtual float pdf(const BSDFQueryRecord &bRec) const = 0;

  /**
   * @brief sample a wo, return the bsdf value * cosTheta / pdf
   *
   * @param iRec
   * @return SpectrumRGB
   */
  virtual SpectrumRGB sample(BSDFQueryRecord &bRec, const Point2f &sample,
                             float &pdf, ScatterSampleType *type) const = 0;

  virtual void bumpComputeShadingNormal(RayIntersectionRec *i_rec) const;

  void computeShadingNormal(RayIntersectionRec *i_rec) const;

  void computeShadingFrame(SurfaceIntersectionInfo *its) const;

  virtual bool isDiffuse() const = 0;

  enum class EBSDFType {
    EUnknown = 0,
    EEmpty,
    EGlossy,
    EDiffuse,
    ETrans
  } m_type;
};

class BlackHole : public BSDF {
 public:
  BlackHole() { m_type = EBSDFType::EDiffuse; }

  BlackHole(const rapidjson::Value &_value) {
    // do nothing
    m_type = EBSDFType::EDiffuse;
  }

  virtual bool isDiffuse() const override { return true; }

  virtual SpectrumRGB evaluate(const BSDFQueryRecord &bRec) const override {
    return SpectrumRGB{.0f};
  }

  virtual float pdf(const BSDFQueryRecord &bRec) const override { return .0f; }

  virtual SpectrumRGB sample(BSDFQueryRecord &bRec, const Point2f &sample,
                             float &pdf,
                             ScatterSampleType *type) const override;
};