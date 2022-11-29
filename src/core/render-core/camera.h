#pragma once
#include <core/render-core/info.h>

#include "core/geometry/geometry.h"
#include "core/math/math.h"
#include "core/utils/configurable.h"
#include "sampler.h"

class Camera : public Configurable {
public:
  /**
   * @brief
   * Generate ray in local coordinate (lefthand)
   * pos = (0, 0, 0), lookAt = (0, 0, 1), up = (0, 1, 0)
   *
   * @param offset
   * @return Ray3f
   */
  Camera() = delete;

  Camera(const rapidjson::Value &_value)
      : Camera(getPoint3f("position", _value), getPoint3f("lookAt", _value),
               getVector3f("up", _value), getFloat("aspectRatio", _value),
               getFloat("vertFov", _value)) {}

  ~Camera() = default;

  /**
   * @brief Construct a new Camera object
   * Refer the pbrt v3
   * @param _pos
   * @param lookAt
   * @param up
   */

  Camera(const Point3f &_pos, const Point3f lookAt, const Vector3f &up,
         float aspect_ratio, float vert_fov)
      : pos(_pos), aspectRatio(aspect_ratio), vertFov(vert_fov),
        distToFilm(1.f) {
    // initialize the translation part
    cameraToWorld(0, 3) = _pos.x;
    cameraToWorld(1, 3) = _pos.y;
    cameraToWorld(2, 3) = _pos.z;
    cameraToWorld(3, 3) = 1;

    // initial the rotation part
    Vector3f dir = normalize(lookAt - _pos);
    Vector3f right = normalize(cross(normalize(up), dir));
    Vector3f newUp = cross(dir, right);

    cameraToWorld(0, 0) = right.x;
    cameraToWorld(1, 0) = right.y;
    cameraToWorld(2, 0) = right.z;
    cameraToWorld(3, 0) = .0f;
    cameraToWorld(0, 1) = newUp.x;
    cameraToWorld(1, 1) = newUp.y;
    cameraToWorld(2, 1) = newUp.z;
    cameraToWorld(3, 1) = .0f;
    cameraToWorld(0, 2) = dir.x;
    cameraToWorld(1, 2) = dir.y;
    cameraToWorld(2, 2) = dir.z;
    cameraToWorld(3, 2) = .0f;

    //        cameraToWorld = cameraToWorld.inverse();
  }

  Camera(const Mat4f &_cameraToWorld)
      : cameraToWorld(_cameraToWorld), aspectRatio(1.7778f), vertFov(30.f),
        distToFilm(1.f) {
    pos = Point3f{_cameraToWorld(0, 3), _cameraToWorld(1, 3),
                  _cameraToWorld(2, 3)};
  }

  virtual Ray3f sampleRay(const Vector2i &offset, const Vector2i &resolution,
                          const CameraSample &sample) const = 0;

  virtual Ray3f sampleRayDifferential(const Point2i &offset,
                                      const Point2i &resolution,
                                      const CameraSample &sample) const = 0;

  virtual SpectrumRGB sampleWi(Point3f ref_p, Point2f u, Vector3f *wi,
                               float *pdf, Point2f *pRaster) const = 0;

  virtual void pdfWe(const Ray3f &ray, float *pdf_pos,
                     float *pdf_dir) const = 0;

  Point3f get_position() const { return pos; }

  Vector3f vec2local(Vector3f v) const {
    return cameraToWorld.inverse().rotate(v);
  }

  Point3f local2film(Vector3f v) const {
    if (v.z < 0)
      return Point3f(-1, -1, 0);
    Point3f point_on_film = Point3f(0) + v / v.z;
    Point3f sample = filmToSample * point_on_film;
    return sample;
  }

protected:
  Point3f pos;

  float aspectRatio, vertFov, distToFilm;

  Mat4f cameraToWorld;

  Mat4f sampleToFilm, filmToSample;

  float A;
};