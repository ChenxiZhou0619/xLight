#pragma once
#include "core/geometry/geometry.h"

class BeckmannDistribution {
 public:
  // Given the half vector and vector, return the D(wh)
  //             exp(-tan^2(theta_h))(cos^2(phi_h)/ax^2 + sin^2(phi_h)/ay^2)
  // D(wh) =   ---------------------------------------------------------------
  //                  pi * ax * ay * cos^4 (theta_h)
  static float D(float alpha, const Vector3f &wh) {
    float tan2Theta = Frame::tanTheta(wh) * Frame::tanTheta(wh);
    if (std::isinf(tan2Theta)) return .0f;  // almost vertical to the normal
    float cos4Theta = std::pow(Frame::cosTheta(wh), 4);
    return std::exp(-tan2Theta / (alpha * alpha)) /
           (M_PI * alpha * alpha * cos4Theta);
  }

  // .5f * (erf(a) - 1 + exp(-a^2) / (a * sqrt(pi)) )
  static float Lambda(float alpha, const Vector3f &w) {
    float absTanTheta = std::abs(Frame::tanTheta(w));
    if (std::isinf(absTanTheta)) return .0f;
    float a = 1 / (alpha * absTanTheta);
    if (a > 1.6f) return .0f;
    return (1 - 1.259f * a + 0.396f * a * a) / (3.535f * a + 2.181f * a * a);
  }

  static float G(const Vector3f &wi, const Vector3f &wo, float alpha) {
    return 1 / (1 + Lambda(alpha, wi) + Lambda(alpha, wo));
  }

  static Vector3f sampleWh(float alpha, const Point2f &sample) {
    float tan2Theta, phi;
    float logSample = std::log(sample[0]);
    if (std::isinf(logSample)) logSample = 0;
    tan2Theta = -alpha * alpha * logSample;
    phi = sample[1] * 2 * M_PI;
    float cosTheta = 1 / std::sqrt(1 + tan2Theta),
          sinTheta = std::sqrt(std::max(.0f, 1 - cosTheta * cosTheta));
    Vector3f wh{sinTheta * std::cos(phi), cosTheta, sinTheta * std::sin(phi)};
    return wh;
  }

  static Vector3f sampleWh(float alpha, const Point2f &sample, float &pdf) {
    float tan2Theta, phi;
    float logSample = std::log(sample[0]);
    if (std::isinf(logSample)) logSample = 0;
    tan2Theta = -alpha * alpha * logSample;
    phi = sample[1] * 2 * M_PI;
    float cosTheta = 1 / std::sqrt(1 + tan2Theta),
          sinTheta = std::sqrt(std::max(.0f, 1 - cosTheta * cosTheta));
    Vector3f wh{sinTheta * std::cos(phi), cosTheta, sinTheta * std::sin(phi)};
    pdf = sample[0] / (M_PI * alpha * alpha * cosTheta * cosTheta * cosTheta);
    return wh;
  }
};
