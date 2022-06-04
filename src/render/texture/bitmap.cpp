#include "core/render-core/texture.h"

class Bitmap : public Texture {
    SpectrumRGB **mData;
    int mWidth, mHeight, mChannels;
public:
    Bitmap() = default;
    Bitmap(const rapidjson::Value &_value) {
        // load the texture
        std::string filepath = _value["filepath"].GetString();
        mData = TextureLoader::toRGB(
            filepath,
            mWidth,
            mHeight,
            mChannels
        );
    }
    Bitmap(const Bitmap &rhs) = delete;
    Bitmap& operator()(const Bitmap &rhs) = delete;
    virtual ~Bitmap() {
        for (int i = 0; i < mWidth; ++i)
            delete [] mData[i];
        delete [] mData;
    }

    virtual SpectrumRGB evaluate(const Point2f &uv) const {
        return mData[int(uv.x * mWidth)][int(uv.y * mHeight)];
    }
};

REGISTER_CLASS(Bitmap, "bitmap")