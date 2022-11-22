#pragma once

#include "shape.h"

std::unordered_map<std::string, std::shared_ptr<ShapeInterface>> loadVdbFile(
    const std::string &filePath, float scale);

class GridMedium : public ShapeInterface {
 public:
  GridMedium() = default;

  GridMedium(Point3f pMin, Point3f pMax, std::shared_ptr<Medium> gridMedium);

  virtual ~GridMedium() = default;

  virtual void initEmbreeGeometry(RTCDevice device) override;

  virtual Point3f getHitPoint(int triIdx, Point2f uv) const override;

  virtual Normal3f getHitNormal(int triIdx, Point2f uv) const override;

  virtual Normal3f getHitNormal(int triIdx) const override;

  virtual Point2f getHitTextureCoordinate(int triIdx, Point2f) const override;

  virtual Vector3f getHitTangent(int tirIdx, Point2f uv) const override {
    return Vector3f{};
  }

  virtual Vector3f dpdu(int triIdx) const override { return Vector3f{}; }

  virtual Vector3f dpdv(int triIdx) const override { return Vector3f{}; }

  virtual void sampleOnSurface(PointQueryRecord *pRec,
                               Point3f sample) const override;

  //* Return dpdu and dpdv
  virtual std::pair<Vector3f, Vector3f> positionDifferential(
      int triIdx) const override {
    //* do nothing
    return {Vector3f{}, Vector3f{}};
  }

  friend void rtcGridMediumBoundsFunc(const RTCBoundsFunctionArguments *args);

  friend void rtcGridMediumIntersectFunc(
      const RTCIntersectFunctionNArguments *args);

 protected:
  virtual Point3f getVertex(int idx) const override;

  virtual Point3ui getFace(int idx) const override;

  virtual Normal3f getNormal(int idx) const override;

  virtual Point2f getUV(int idx) const override;

  Point3f mPMin, mPMax;
};