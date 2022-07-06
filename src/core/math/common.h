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

// cos_theta_i = cos (bRec.wi, m)
inline float FresnelDielectricAccurate(float cos_theta_i, float eta_i, float eta_t) {
    if (cos_theta_i < 0)
        std::swap(eta_i, eta_t);

    // first, compute cos_theta_t
    float cos_theta_t_2 = 
        1 - (1 - cos_theta_i * cos_theta_i) * (eta_i/eta_t) * (eta_i/eta_t);
    
    if (cos_theta_t_2 <= 0) {
        // totally internal reflect
        return 1;
    }
    float cos_theta_t = std::sqrt(cos_theta_t_2);

    float rPral = (eta_t * cos_theta_i - eta_i * cos_theta_t)
                / (eta_t * cos_theta_i + eta_i * cos_theta_t);
    float rPerp = (eta_i * cos_theta_i - eta_t * cos_theta_t)
                / (eta_i * cos_theta_i + eta_t * cos_theta_t);
    return .5f * (rPral * rPral + rPerp * rPerp); 
}