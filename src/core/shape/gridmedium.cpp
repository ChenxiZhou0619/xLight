#include "gridmedium.h"

#include <openvdb/openvdb.h>

std::unordered_map<std::string, std::shared_ptr<ShapeInterface>>
loadVdbFile(const std::string &filePath) 
{
    std::unordered_map<std::string, 
                       std::shared_ptr<ShapeInterface>> result;

    using namespace openvdb;
    initialize();

    io::File vdbFile (filePath);
    vdbFile.open();

    auto itr = vdbFile.beginName();
    if (itr == vdbFile.endName()) {
        std::cout << "Error!, no grid in file\n";
        std::exit(1);
    }
    auto gridName = itr.gridName();
    std::cout << "GridName = " << gridName << std::endl;
    GridBase::Ptr grid = vdbFile.readGrid(gridName);
    vdbFile.close();

    FloatGrid::Ptr densityGrid = gridPtrCast<FloatGrid>(grid);

    auto worldBound = densityGrid->evalActiveVoxelBoundingBox();
    auto min = densityGrid->indexToWorld(worldBound.min()),
         max = densityGrid->indexToWorld(worldBound.max());


    result[gridName] = std::make_shared<GridMedium>(
        Point3f (min.x(), min.y(), min.z()),
        Point3f (max.x(), max.y(), max.z())
    );

    return result;
}

GridMedium::GridMedium(Point3f pMin, Point3f pMax)
    : mPMin(pMin), mPMax(pMax)
{
    //todo
    this->emitter = nullptr;
}

void rtcGridMediumBoundsFunc(const RTCBoundsFunctionArguments *args)
{
    GridMedium *gridPtr = (GridMedium *)args->geometryUserPtr;
    auto [lx, ly, lz] = gridPtr->mPMin;
    auto [ux, uy, uz] = gridPtr->mPMax;

    RTCBounds *box = args->bounds_o;
    box->lower_x = lx;
    box->lower_y = ly;
    box->lower_z = lz;

    box->upper_x = ux;
    box->upper_y = uy;
    box->upper_z = uz;

}

void rtcGridMediumIntersectFunc(const RTCIntersectFunctionNArguments *args)
{
    int *valid = args->valid;
    if (!valid[0]) return;

    GridMedium *gridMedium = (GridMedium *)args->geometryUserPtr;
    auto min = gridMedium->mPMin,
         max = gridMedium->mPMax;
        
    RTCRayHitN *rayhit = args->rayhit;
    RTCRayN *ray = RTCRayHitN_RayN(rayhit, 1);
    RTCHitN *hit = RTCRayHitN_HitN(rayhit, 1);

    auto ori_x = RTCRayN_org_x(ray, 1, 0),
         ori_y = RTCRayN_org_y(ray, 1, 0),
         ori_z = RTCRayN_org_z(ray, 1, 0),
         dir_x = RTCRayN_dir_x(ray, 1, 0),
         dir_y = RTCRayN_dir_y(ray, 1, 0),
         dir_z = RTCRayN_dir_z(ray, 1, 0);

    auto tnear = RTCRayN_tnear(ray, 1, 0),
         tfar = RTCRayN_tfar(ray, 1, 0);

    Point3f ori {ori_x, ori_y, ori_z};
    Vector3f dir {dir_x, dir_y, dir_z}; 

    float nearT = -std::numeric_limits<float>::infinity();
    float farT  =  std::numeric_limits<float>::infinity();

    for (int axis = 0; axis < 3; ++axis) {
        float origin = ori[axis],
              minVal = min[axis],
              maxVal = max[axis];
        if (dir[axis] == 0) {
            if (origin < minVal || origin > maxVal)
                return;                
        } else {
            float invDir = 1 / dir[axis],
                  t0 = (minVal - origin) * invDir,
                  t1 = (maxVal - origin) * invDir;
            if (t0 > t1) std::swap(t0, t1);
            nearT = std::max(t0, nearT);
            farT  = std::min(t1, farT);
            if (nearT > farT) 
                return;
        }
    }

    if (tnear >= farT || tfar <= nearT)
        return;

    float t = nearT;
    if (t < tnear) t = farT;


    RTCHit result;
    result.geomID = args->geomID;


    Point3f hitpoint = ori + nearT * dir;
    float bias[6];
    for (int axis = 0; axis < 3; ++axis) {
        float a = bias[axis * 2] = std::abs(hitpoint[axis] - min[axis]);
        float b = bias[axis * 2 + 1] = std::abs(hitpoint[axis] - max[axis]);
    }

    int cloestDimension = -1;
    float minBias = std::numeric_limits<float>::max();
    for (int i = 0; i < 6; ++i) {
        if (bias[i] < minBias) {
            minBias = bias[i];
            cloestDimension = i;
        }
    }
    result.Ng_x = result.Ng_y = result.Ng_z = .0;
    if (cloestDimension == 0) {
        result.Ng_x = -1;
    } else if (cloestDimension == 1) {
        result.Ng_x = 1;
    } else if (cloestDimension == 2) {
        result.Ng_y = -1;
    } else if (cloestDimension == 3) {
        result.Ng_y = 1;
    } else if (cloestDimension == 4) {
        result.Ng_z = -1;
    } else if (cloestDimension == 5) {
        result.Ng_z = 1;
    }
    
    
    RTCFilterFunctionNArguments fargs;
    int imask = -1;
    fargs.valid = &imask;
    fargs.geometryUserPtr = args->geometryUserPtr;
    fargs.context = args->context;
    fargs.ray = (RTCRayN *)args->rayhit;
    fargs.hit = (RTCHitN *) &result;
    fargs.N = 1;    

    rtcFilterIntersection(args, &fargs);
    if (*fargs.valid == -1){
        *((RTCHit *)hit) = result;
        RTCRayN_tfar(ray, 1, 0) = t;
    }
    return;
}

// todo
void rtcGridMediumOcculudeFunc(const RTCOccludedFunctionNArguments *args) 
{
    return;
}

void GridMedium::initEmbreeGeometry(RTCDevice device) 
{
    this->embreeGeometry = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_USER);
    rtcSetGeometryUserPrimitiveCount(this->embreeGeometry, 1);
    rtcSetGeometryUserData(this->embreeGeometry, (void *)this);
    rtcSetGeometryBoundsFunction(this->embreeGeometry, rtcGridMediumBoundsFunc, nullptr);
    rtcSetGeometryIntersectFunction(this->embreeGeometry, rtcGridMediumIntersectFunc);
    rtcSetGeometryOccludedFunction(this->embreeGeometry, rtcGridMediumOcculudeFunc);
    rtcCommitGeometry(this->embreeGeometry);
}
 
Point3f GridMedium::getHitPoint(int triIdx, Point2f uv) const
{
    return Point3f();
} 

Normal3f GridMedium::getHitNormal(int triIdx, Point2f uv) const {
    return Normal3f();
}

Normal3f GridMedium::getHitNormal(int triIdx) const {
    return Normal3f();
}

Point2f GridMedium::getHitTextureCoordinate(int triIdx, Point2f uv) const
{
    return Point2f();
} 

void GridMedium::sampleOnSurface(PointQueryRecord *pRec, Sampler *sampler) const 
{
    return;
}

Point3f GridMedium::getVertex(int idx) const 
{
    return Point3f();
}

Point3ui GridMedium::getFace(int idx) const 
{
    return Point3ui();
}

Normal3f GridMedium::getNormal(int idx) const 
{
    return Normal3f();
}

Point2f GridMedium::getUV(int idx) const 
{
    return Point2f();
}