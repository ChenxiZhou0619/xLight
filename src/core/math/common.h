#pragma once
#include <algorithm>
#include <cmath>

inline float powerHeuristic(float fpdf, float gpdf) {
    fpdf *= fpdf;
    gpdf *= gpdf;
    return fpdf / (fpdf + gpdf);
}

inline float fresnelDielectric(float cosThetaI, float etai, float etat, float &cosThetaT) {
    bool entering = cosThetaI > .0f;
    if (!entering) {
        std::swap(etai, etat);
        cosThetaI = std::abs(cosThetaI);
    }
    float sinThetaI = std::sqrt(std::max(.0f, 1 - cosThetaI * cosThetaI));
    float sinThetaT = etai / etat * sinThetaI;
    // total internal reflection
    if (sinThetaT >= 1) 
        return 1;
    cosThetaT = std::sqrt(std::max(.0f, 1 - sinThetaT * sinThetaT));
    float Rparl = ((etat * cosThetaI) - (etai * cosThetaT)) / 
                  ((etat * cosThetaI) + (etai * cosThetaT)),
          Rperp = ((etai * cosThetaI) - (etat * cosThetaT)) / 
                  ((etai * cosThetaI) + (etat * cosThetaT));
    return (Rparl * Rparl + Rperp * Rperp) * .5f;
}