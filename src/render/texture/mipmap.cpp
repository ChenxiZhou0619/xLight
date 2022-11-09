#include "mipmap.h"

SpectrumRGB MipMap::evaluate(const Point2f &uv, float du, float dv) const 
{
    float width = std::max(du, dv);
    float level = maps.size() - 1 + std::log2(std::max(width, 1e-8f));

    if (level < 0) return maps[0]->evaluate(uv);
    if (level >= maps.size() - 1) return maps.back()->evaluate(uv);

    //* do interpolation
    int lowLevel = level, highLevel = lowLevel + 1;
    SpectrumRGB lerpl = maps[lowLevel]->evaluate(uv, true),
                lerph = maps[highLevel]->evaluate(uv, true);
    return lerpl * (level - lowLevel) + lerph * (highLevel - level); 
}

REGISTER_CLASS(MipMap, "mipmap")