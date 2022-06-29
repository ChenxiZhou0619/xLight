#pragma once
#include <algorithm>
#include <cmath>
#include <iostream>

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