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
    bool entering = cosThetaI > .0f;
    if (!entering) {
        eta = 1 / eta;
        cosThetaI = std::abs(cosThetaI);
    }
    float sinThetaI = std::sqrt(std::max(.0f, 1 - cosThetaI * cosThetaI));
    float sinThetaT = eta * eta * sinThetaI;
    // total internal reflection
    if (sinThetaT >= 1){ 
        cosThetaT = 0;
        return 1;
    }
    cosThetaT = std::sqrt(std::max(.0f, 1 - sinThetaT * sinThetaT));
    float Rparl = ((eta * cosThetaI) - (1 * cosThetaT)) / 
                  ((eta * cosThetaI) + (1 * cosThetaT)),
          Rperp = ((1 * cosThetaI) - (eta * cosThetaT)) / 
                  ((1 * cosThetaI) + (eta * cosThetaT));
    cosThetaT = cosThetaT * (entering ? -1 : 1);
    //std::cout << "cosThetaI = " << cosThetaI << std::endl;
    //std::cout << "fresnel = " << (Rparl * Rparl + Rperp * Rperp) * .5f << std::endl;
    //return (Rparl * Rparl + Rperp * Rperp) * .5f;
    return 0.f;
}