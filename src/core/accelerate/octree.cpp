#include "octree.h"

std::vector<AABB3f> OcNode::getSubBounds() const {
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




bool OcNode::rayIntersect(const Ray3f &ray, RayIntersectionRec &iRec) const {

}
