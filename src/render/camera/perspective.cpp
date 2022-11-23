#include "core/render-core/camera.h"

class PerspectiveCamera : public Camera {
 public:
  PerspectiveCamera() = delete;

  PerspectiveCamera(const rapidjson::Value &_value) : Camera(_value) {
    distToFilm = getFloat("distToFilm", _value);
    // configure the filmToSample
    sampleToFilm = Mat4f::Perspective(vertFov, aspectRatio, distToFilm,
                                      std::numeric_limits<float>::max());
    sampleToFilm = Mat4f::Scale(Vector3f{.5f, -.5f, 1.f}) *
                   Mat4f::Translate(Vector3f{1.f, -1.f, .0f}) * sampleToFilm;
    filmToSample = sampleToFilm;
    sampleToFilm = sampleToFilm.inverse();
    Point3f pmin = sampleToFilm * Point3f(0, 0, 0),
            pmax = sampleToFilm * Point3f(1, 1, 0);
    A = std::abs((pmin.x - pmax.x) * (pmin.y - pmax.y));
  }

  PerspectiveCamera(const Point3f &pos, const Point3f lookAt,
                    const Vector3f &up)
      : Camera(pos, lookAt, up) {}

  PerspectiveCamera(const Mat4f &_cameraToWorld) : Camera(_cameraToWorld) {}

  friend std::ostream &operator<<(std::ostream &os,
                                  const PerspectiveCamera &camera);

  /**
   * @brief e.g. f = (.5f, .5f) the center
   *
   * @param offset
   * @return Ray3f
   */
  virtual Ray3f sampleRay(const Vector2i &offset, const Vector2i &resolution,
                          const CameraSample &sample) const override {
    Point3f pointOnFilm =
        sampleToFilm *
        Point3f{((float)offset.x + sample.sampleXY.x) / (float)resolution.x,
                ((float)offset.y + sample.sampleXY.y) / (float)resolution.y,
                .0f};

    Vector3f rayDirLocal = normalize(pointOnFilm - Point3f(0, 0, 0));

    // turn to the world coordinate
    Vector3f rayDirWorld = cameraToWorld.rotate(rayDirLocal);

    return Ray3f{pos, rayDirWorld};
  }

  virtual Ray3f sampleRayDifferential(
      const Point2i &offset, const Point2i &resolution,
      const CameraSample &sample) const override {
    Point3f pointOnFilm =
        sampleToFilm *
        Point3f{((float)offset.x + sample.sampleXY.x) / (float)resolution.x,
                ((float)offset.y + sample.sampleXY.y) / (float)resolution.y,
                .0f};

    Vector3f rayDirLocal = normalize(pointOnFilm - Point3f(0, 0, 0));

    // turn to the world coordinate
    Vector3f rayDirWorld = cameraToWorld.rotate(rayDirLocal);

    Ray3f ray{pos, rayDirWorld};

    //* set the ray differential part
    Point3f point_on_film_dx =
        sampleToFilm *
        Point3f{((float)offset.x + sample.sampleXY.x + 1) / (float)resolution.x,
                ((float)offset.y + sample.sampleXY.y) / (float)resolution.y,
                .0f};
    ray.direction_dx =
        cameraToWorld.rotate(normalize(point_on_film_dx - Point3f(0, 0, 0)));

    Point3f point_on_film_dy =
        sampleToFilm *
        Point3f{((float)offset.x + sample.sampleXY.x) / (float)resolution.x,
                ((float)offset.y + sample.sampleXY.y + 1) / (float)resolution.y,
                .0f};
    ray.direction_dy =
        cameraToWorld.rotate(normalize(point_on_film_dy - Point3f(0, 0, 0)));

    ray.is_ray_differential = true;
    return ray;
  }

  virtual SpectrumRGB sampleWi(Point3f ref_p, Point2f u, Vector3f *wi,
                               float *pdf, Point2f *pRaster) const override {
    Vector3f pinhole_normal = cameraToWorld.rotate(Vector3f(0, 0, 1));
    *wi = pos - ref_p;
    float dist = wi->length();
    *wi /= dist;

    Point3f pfilm = local2film(vec2local(-*wi));
    *pRaster = {pfilm.x, pfilm.y};
    float lens_area = 1;
    *pdf = (dist * dist) / (std::abs(dot(*wi, pinhole_normal)) * lens_area);

    return SpectrumRGB{1 / (A * std::pow(dot(*wi, pinhole_normal), 4.f))};
  }
};

inline std::ostream &operator<<(std::ostream &os,
                                const PerspectiveCamera &camera) {
  os << "Perspective Camera:\n"
     << "cameraToWorld matrix:\n"
     << camera.cameraToWorld << std::endl
     << "position: " << camera.pos << std::endl;
  return os;
}

REGISTER_CLASS(PerspectiveCamera, "perspective")