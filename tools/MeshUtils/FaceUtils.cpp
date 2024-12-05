/* This file is part of PyMesh. Copyright (c) 2017 by Qingnan Zhou */
#include "FaceUtils.h"

#include <Predicates/predicates.h>
#include <Core/Exception.h>

using namespace PyMesh;

bool FaceUtils::is_colinear_2D(
        const Vector2F& v0,
        const Vector2F& v1,
        const Vector2F& v2) {
    prdc_exactinit();
    double v0_xy[2] = {v0[0], v0[1]};
    double v1_xy[2] = {v1[0], v1[1]};
    double v2_xy[2] = {v2[0], v2[1]};
    return prdc_orient2d(v0_xy, v1_xy, v2_xy) == 0;
}

bool FaceUtils::is_colinear_3D(
        const Vector3F& v0,
        const Vector3F& v1,
        const Vector3F& v2) {
    prdc_exactinit();
    double v0_xy[2] = {v0[0], v0[1]};
    double v1_xy[2] = {v1[0], v1[1]};
    double v2_xy[2] = {v2[0], v2[1]};

    double v0_yz[2] = {v0[1], v0[2]};
    double v1_yz[2] = {v1[1], v1[2]};
    double v2_yz[2] = {v2[1], v2[2]};

    double v0_zx[2] = {v0[2], v0[0]};
    double v1_zx[2] = {v1[2], v1[0]};
    double v2_zx[2] = {v2[2], v2[0]};

    bool colinear_xy = prdc_orient2d(v0_xy, v1_xy, v2_xy) == 0;
    bool colinear_yz = prdc_orient2d(v0_yz, v1_yz, v2_yz) == 0;
    bool colinear_zx = prdc_orient2d(v0_zx, v1_zx, v2_zx) == 0;
    return colinear_xy && colinear_yz && colinear_zx;
}

std::vector<size_t> FaceUtils::get_degenerated_faces(
        const MatrixFr& vertices, const MatrixIr& faces) {
    std::vector<size_t> result;
    const size_t num_faces = faces.rows();
    const size_t dim = vertices.cols();
    prdc_exactinit();

    if (dim == 3) {
        for (size_t i=0; i<num_faces; i++) {
            const auto& f = faces.row(i);
            if (FaceUtils::is_colinear_3D(
                        vertices.row(f[0]),
                        vertices.row(f[1]),
                        vertices.row(f[2]))) {
                result.push_back(i);
            }
        }
    } else if (dim == 2) {
        for (size_t i=0; i<num_faces; i++) {
            const auto& f = faces.row(i);
            if (FaceUtils::is_colinear_2D(
                        vertices.row(f[0]),
                        vertices.row(f[1]),
                        vertices.row(f[2]))) {
                result.push_back(i);
            }
        }
    }
    return result;
}

VectorF FaceUtils::get_triangle_orientations(
        const MatrixFr& vertices, const MatrixIr& faces) {
    const size_t num_faces = faces.rows();
    const size_t dim = vertices.cols();
    if (dim != 2) {
        throw NotImplementedError(
                "Triangle orientation is only well-defined in 2D");
    }

    prdc_exactinit();

    VectorF results(num_faces);
    for (size_t i=0; i<num_faces; i++) {
        const auto& f = faces.row(i);
        double v0_xy[2] = {vertices(f[0], 0), vertices(f[0], 1)};
        double v1_xy[2] = {vertices(f[1], 0), vertices(f[1], 1)};
        double v2_xy[2] = {vertices(f[2], 0), vertices(f[2], 1)};
        results[i] = prdc_orient2d(v0_xy, v1_xy, v2_xy);
    }
    return results;
}
