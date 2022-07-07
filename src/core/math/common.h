#pragma once
#include <algorithm>
#include <cmath>
#include <iostream>

inline int sign(float t) {
    return t > 0 ? 1 : -1;
}


inline float powerHeuristic(float fpdf, float gpdf) {
    fpdf *= fpdf;
    gpdf *= gpdf;
    return fpdf / (fpdf + gpdf);
}

inline float fresnelDielectric(float cosThetaI, float eta, float &cosThetaT) {
    // schlick approximation
    // TODO replace the schlick approximation
    float R0 = ((1 - eta) * (1 - eta)) / ((1 + eta) * (1 + eta));
    float F = R0 + (1 - R0) * std::pow((1 - std::abs(cosThetaI)), 5);

    // caculate the cosThetaT
    float sinThetaI = std::sqrt(
        std::max(
            0.f,
            1 - cosThetaI * cosThetaI
        )
    );
    float sinThetaT = sinThetaI *
        ((cosThetaI < 0) ? eta : (1 / eta));

    if (sinThetaT >= 1) {
        // total internal reflectance
        return 1.f;
    } else {
        cosThetaT = std::sqrt(
            std::max(
                .0f,
                1 - sinThetaT * sinThetaT
            )
        );
        // consider the sign of cosThetaT
        cosThetaT = (cosThetaI > 0) ? -cosThetaT : cosThetaT;
        return F;
    }

}

inline float FresnelDielectric(float cosThetaI, float eta) {
    // Schlick approximation
    float R0 = (1 - eta) / (1 + eta);
    R0 *= R0;

    float F = R0 + (1 - R0) * std::pow((1 - std::abs(cosThetaI)), 5);

    // caculate the cosThetaT
    float sinThetaI = std::sqrt(
        std::max(
            0.f,
            1 - cosThetaI * cosThetaI
        )
    );
    float sinThetaT = sinThetaI *
        ((cosThetaI < 0) ? eta : (1 / eta));

    if (sinThetaT >= 1)
        // total internal reflectance
        return 1.f;    

    return F;
}

// eta = eta_t / eta_i
inline float FresnelDielectricAccurate(float cos_theta_i, float eta) {
    // compute the cos_theta_t first
    float sin_theta_i = std::sqrt(std::max(0.f, 1 - cos_theta_i * cos_theta_i));
    float sin_theta_t = sin_theta_i / eta;
    // totally internal reflec
    if (sin_theta_t >= 1) {
        return 1;
    }
    // compute the fresnel term
    cos_theta_i = std::abs(cos_theta_i);
    float cos_theta_t = std::sqrt(std::max(.0f, 1 - sin_theta_t * sin_theta_t));
    float r_parl = (eta * cos_theta_i - cos_theta_t) 
                   / (eta * cos_theta_i + cos_theta_t);
    float r_perp = (cos_theta_i - eta * cos_theta_t)
                   / (cos_theta_i + eta * cos_theta_t);
    return .5f * (r_perp * r_perp + r_parl * r_parl);
}