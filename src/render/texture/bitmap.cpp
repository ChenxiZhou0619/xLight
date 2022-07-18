#include "core/render-core/texture.h"
#include "core/math/common.h"

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

    virtual SpectrumRGB evaluate(const Point2f &uv, float du, float dv) const override{
        int x = std::min(static_cast<int>(std::abs(uv.x) * mWidth), mWidth - 1);
        int y = std::min(static_cast<int>(std::abs(uv.y) * mHeight), mHeight - 1);

        int x_ = std::min(static_cast<int>(std::abs(uv.x + du) * mWidth), mWidth - 1);
        int y_ = std::min(static_cast<int>(std::abs(uv.y + dv) * mHeight), mHeight - 1);

        return (mData[x][y] + mData[x][y_] + mData[x_][y]) / 3;
    }

    virtual SpectrumRGB average() const override {
        // TODO no implement
        return SpectrumRGB{.5f};
    }

    virtual Vector2i getResolution() const override {
        return Vector2i {
            mWidth, mHeight
        };
    }

    virtual SpectrumRGB dfdu(Point2f uv) const override {
        int x = std::min(static_cast<int>(std::abs(uv.x) * mWidth), mWidth - 1);
        int y = std::min(static_cast<int>(std::abs(uv.y) * mHeight), mHeight - 1);

        int x_ = std::min(static_cast<int>(std::abs(uv.x + 1) * mWidth), mWidth - 1);
        
        return mData[x_][y] - mData[x][y];
    }

    virtual SpectrumRGB dfdv(Point2f uv) const override {
        int x = std::min(static_cast<int>(std::abs(uv.x) * mWidth), mWidth - 1);
        int y = std::min(static_cast<int>(std::abs(uv.y) * mHeight), mHeight - 1);

        int y_ = std::min(static_cast<int>(std::abs(uv.y + 1) * mHeight), mHeight - 1);
        
        return mData[x][y_] - mData[x][y];
    }
};

REGISTER_CLASS(Bitmap, "bitmap")