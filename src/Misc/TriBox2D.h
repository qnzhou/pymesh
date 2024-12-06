/* This file is part of PyMesh. Copyright (c) 2015 by Qingnan Zhou */
#pragma once
#include <Core/EigenTypedef.h>
#include <iostream>

namespace PyMesh {
namespace TriBox2D {

inline bool get_orientation(const Float triverts[3][2]) {
    Float r =
        (triverts[1][0] - triverts[0][0]) *
        (triverts[2][1] - triverts[0][1]) -
        (triverts[1][1] - triverts[0][1]) *
        (triverts[2][0] - triverts[0][0]);
    return r >= 0;
}

inline bool outside_halfplane(const Float target[2], const Float p[2], const Float n[2],
        bool positive_orientation) {
    const Float v[2] = {target[0] - p[0], target[1] - p[1]};
    if (positive_orientation)
        return (v[0] * n[0] + v[1] * n[1]) > 0;
    else
        return (v[0] * n[0] + v[1] * n[1]) < 0;
}

inline int triBoxOverlap(const Float boxcenter[2], const Float boxhalfsize[2], const Float triverts[3][2]) {
    // Check all possible separating lines:
    // * 4 edges of the box
    // * 3 edges of the triangle
    // If all 7 lines failed to separate the two shapes, they must overlap.

    const Float center_v1[2] = {triverts[0][0] - boxcenter[0], triverts[0][1] - boxcenter[1]};
    const Float center_v2[2] = {triverts[1][0] - boxcenter[0], triverts[1][1] - boxcenter[1]};
    const Float center_v3[2] = {triverts[2][0] - boxcenter[0], triverts[2][1] - boxcenter[1]};

    if (center_v1[0] > boxhalfsize[0] && center_v2[0] > boxhalfsize[0] && center_v3[0] > boxhalfsize[0]) {
        return 0;
    }
    if (center_v1[0] < -boxhalfsize[0] && center_v2[0] < -boxhalfsize[0] && center_v3[0] < -boxhalfsize[0]) {
        return 0;
    }
    if (center_v1[1] > boxhalfsize[1] && center_v2[1] > boxhalfsize[1] && center_v3[1] > boxhalfsize[1]) {
        return 0;
    }
    if (center_v1[1] < -boxhalfsize[1] && center_v2[1] < -boxhalfsize[1] && center_v3[1] < -boxhalfsize[1]) {
        return 0;
    }

    const Float p1[2] = {boxcenter[0]+boxhalfsize[0], boxcenter[1]+boxhalfsize[1]};
    const Float p2[2] = {boxcenter[0]-boxhalfsize[0], boxcenter[1]+boxhalfsize[1]};
    const Float p3[2] = {boxcenter[0]-boxhalfsize[0], boxcenter[1]-boxhalfsize[1]};
    const Float p4[2] = {boxcenter[0]+boxhalfsize[0], boxcenter[1]-boxhalfsize[1]};

    const Float edge_1[2] = {triverts[2][0] - triverts[1][0], triverts[2][1] - triverts[1][1]};
    const Float edge_2[2] = {triverts[0][0] - triverts[2][0], triverts[0][1] - triverts[2][1]};
    const Float edge_3[2] = {triverts[1][0] - triverts[0][0], triverts[1][1] - triverts[0][1]};

    const Float n1[2] = {edge_1[1], -edge_1[0]};
    const Float n2[2] = {edge_2[1], -edge_2[0]};
    const Float n3[2] = {edge_3[1], -edge_3[0]};

    bool ori = get_orientation(triverts);

    if ( outside_halfplane(p1, triverts[1], n1, ori) &&
         outside_halfplane(p2, triverts[1], n1, ori) &&
         outside_halfplane(p3, triverts[1], n1, ori) &&
         outside_halfplane(p4, triverts[1], n1, ori) ) {
        return 0;
    }

    if ( outside_halfplane(p1, triverts[2], n2, ori) &&
         outside_halfplane(p2, triverts[2], n2, ori) &&
         outside_halfplane(p3, triverts[2], n2, ori) &&
         outside_halfplane(p4, triverts[2], n2, ori) ) {
        return 0;
    }

    if ( outside_halfplane(p1, triverts[0], n3, ori) &&
         outside_halfplane(p2, triverts[0], n3, ori) &&
         outside_halfplane(p3, triverts[0], n3, ori) &&
         outside_halfplane(p4, triverts[0], n3, ori) ) {
        return 0;
    }

    return 1;
}

}
}
