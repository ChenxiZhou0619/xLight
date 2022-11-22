// copy from nori 2021

#pragma once
#include "core/geometry/frame.h"
#include "core/geometry/geometry.h"
#include "math.h"

class Warp {
 public:
  /// Dummy warping function: takes uniformly distributed points in a square and
  /// just returns them
  static Point2f squareToUniformSquare(const Point2f &sample) { return sample; }

  /// Probability density of \ref squareToUniformSquare()
  static float squareToUniformSquarePdf(const Point2f &p) {
    return ((p.x <= 1 && p.x >= 0) && (p.y <= 1 && p.y >= 0)) ? 1.f : .0f;
  }

  /// Uniformly sample a vector on a 2D disk with radius 1, centered around the
  /// origin
  static Point2f squareToUniformDisk(const Point2f &sample) {
    float r = std::sqrt(sample[0]), theta = sample[1] * 2 * PI;
    return Point2f{r * std::cos(theta), r * std::sin(theta)};
  }

  /// Probability density of \ref squareToUniformDisk()
  static float squareToUniformDiskPdf(const Point2f &p) {
    if (p.x * p.x + p.y * p.y > 1.f) return .0f;
    return INV_PI;
  }

  /// Uniformly sample a vector on the unit sphere with respect to solid angles
  static Vector3f squareToUniformSphere(const Point2f &sample) {
    float theta = 2.f * M_PI * sample[0], phi = std::acos(2 * sample[1] - 1);
    return Vector3f{std::sin(phi) * std::cos(theta), std::cos(phi),
                    std::sin(phi) * std::sin(theta)};
  }

  /// Probability density of \ref squareToUniformSphere()
  static float squareToUniformSpherePdf(const Vector3f &v) {
    return 0.25f * INV_PI;
  }

  /// Uniformly sample a vector on the unit hemisphere around the pole (0,1,0)
  /// with respect to solid angles
  static Vector3f squareToUniformHemisphere(const Point2f &sample) {
    float theta = 2.f * M_PI * sample[0], phi = std::acos(2 * sample[1] - 1);
    return Vector3f{std::sin(phi) * std::cos(theta), std::abs(std::cos(phi)),
                    std::sin(phi) * std::sin(theta)};
  }

  /// Probability density of \ref squareToUniformHemisphere()
  static float squareToUniformHemispherePdf(const Vector3f &v) {
    return v[1] > .0f ? 0.5f * INV_PI : .0f;
  }

  /// Uniformly sample a vector on the unit hemisphere around the pole (0,1,0)
  /// with respect to projected solid angles
  static Vector3f squareToCosineHemisphere(const Point2f &sample) {
    float phi = 2 * M_PI * sample[0], theta = std::acos(std::sqrt(sample[1]));
    return Vector3f{std::sin(theta) * std::cos(phi), std::cos(theta),
                    std::sin(theta) * std::sin(phi)};
  }

  /// Probability density of \ref squareToCosineHemisphere()
  static float squareToCosineHemispherePdf(const Vector3f &v) {
    return v.y > 0 ? v.y * INV_PI : .0f;
  }

  /// Warp a uniformly distributed square sample to a Beckmann distribution *
  /// cosine for the given 'alpha' parameter
  static Vector3f squareToBeckmann(const Point2f &sample, float alpha) {
    float phi = 2 * M_PI * sample[0];
    float invAlpha2 = 1.f / alpha;
    invAlpha2 *= invAlpha2;
    float theta = acos(sqrt(1 / (1 - alpha * alpha * log(sample[1]))));
    return Vector3f(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
  }

  /// Probability density of \ref squareToBeckmann()
  static float squareToBeckmannPdf(const Vector3f &m, float alpha) {
    float cosTheta = Frame::cosTheta(m);
    float tanTheta = Frame::tanTheta(m);
    if (cosTheta <= 0) return 0.f;

    float azimuthal = 0.5 * INV_PI;
    float longitudinal = 2 * exp((-tanTheta * tanTheta) / (alpha * alpha)) /
                         (alpha * alpha * pow(cosTheta, 3));

    return azimuthal * longitudinal;
  }
};