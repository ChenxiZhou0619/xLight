#pragma once

inline float powerHeuristic(float fpdf, float gpdf) {
    fpdf *= fpdf;
    gpdf *= gpdf;
    return fpdf / (fpdf + gpdf);
}