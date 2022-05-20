#include "octree.h"

std::vector<AABB3f> OcNode::getSubBounds() {
    //std::cout << nodeBox.toString() << std::endl;

    Point3f center {bounds.getCentroid()};
    Vector3f halfEdge {center - bounds.min};

    std::vector<AABB3f> subBoxes;

    subBoxes.emplace_back(
        AABB3f {
            bounds.min,
            center
        }
    );
    subBoxes.emplace_back(
        AABB3f {
            bounds.min + Vector3f{halfEdge[0], .0f, .0f},
            center + Vector3f{halfEdge[0], .0f, .0f}
        }
    );
    subBoxes.emplace_back(
        AABB3f {
            bounds.min + Vector3f{halfEdge[0], halfEdge[1], .0f},
            center + Vector3f{halfEdge[0], halfEdge[1], .0f}
        }
    );
    subBoxes.emplace_back(
       AABB3f {
            bounds.min + Vector3f{.0f, halfEdge[1], .0f},
            center + Vector3f{.0f, halfEdge[1], .0f}
        }
    );
    subBoxes.emplace_back(
        AABB3f {
            bounds.min + Vector3f{.0f, .0f, halfEdge[2]},
            center + Vector3f{.0f, .0f, halfEdge[2]}
        }
    );
    subBoxes.emplace_back(
        AABB3f {
            bounds.min + Vector3f{halfEdge[0], .0f, halfEdge[2]},
            center + Vector3f{halfEdge[0], .0f, halfEdge[2]}
        }
    );
    subBoxes.emplace_back(
        AABB3f {
            bounds.min + Vector3f{halfEdge[0], halfEdge[1], halfEdge[2]},
            center + Vector3f{halfEdge[0], halfEdge[1], halfEdge[2]}
        }
    );
    subBoxes.emplace_back(
        AABB3f {
            bounds.min + Vector3f{.0f, halfEdge[1], halfEdge[2]},
            center + Vector3f{.0f, halfEdge[1], halfEdge[2]}
        }
    );

    //for (const auto &box : subBoxes) {
    //    std::cout << box.toString () << std::endl;
    //}

    return subBoxes;
}

OcNode* Accel::buildOcTree(const AABB3f &_bounds, const std::vector<uint32_t> &faceBuf, int _depth) {
    if (faceBuf.size() == 0) return nullptr;

    if (faceBuf.size() < ocLeafMaxSize) {
        OcNode *node = new OcNode {_bounds, faceBuf, _depth};
        return node;
    }

    if (_depth > maxDepth) {
        OcNode *node = new OcNode {_bounds, faceBuf, _depth};
        return node;
    }

    std::vector<uint32_t> faceBufs[nSubs];
    OcNode *node = new OcNode {_bounds, _depth};
    const auto& subBounds = node->getSubBounds();

    for (const auto &faceIdx : faceBuf) {
        const auto &triBounds = meshSetPtr
    }
}



bool OcNode::rayIntersect(const Ray3f &ray, RayIntersectionRec &iRec) const {

}
