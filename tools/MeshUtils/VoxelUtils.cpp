/* This file is part of PyMesh. Copyright (c) 2017 by Qingnan Zhou */
#include "VoxelUtils.h"

#include <Core/Exception.h>
#include <Predicates/predicates.h>
#include <Misc/MultipletMap.h>

using namespace PyMesh;

VectorF VoxelUtils::get_tet_orientations(
        const MatrixFr& vertices,
        const MatrixIr& voxels) {
    const size_t dim = vertices.cols();
    const size_t vertex_per_voxel = voxels.cols();
    const size_t num_tets = voxels.rows();
    if (dim != 3) {
        throw RuntimeError("Degenerate tets is only defined for 3D meshes.");
    }
    if (vertex_per_voxel != 4) {
        throw RuntimeError("Degenerate tet expect a tet mesh.");
    }

    prdc_exactinit();
    VectorF results(num_tets);
    results.setZero();
    for (size_t i=0; i<num_tets; i++) {
        const Vector4I tet = voxels.row(i);
        Float a[3]{
            vertices(tet[0], 0),
            vertices(tet[0], 1),
            vertices(tet[0], 2)};
        Float b[3]{
            vertices(tet[1], 0),
            vertices(tet[1], 1),
            vertices(tet[1], 2)};
        Float c[3]{
            vertices(tet[2], 0),
            vertices(tet[2], 1),
            vertices(tet[2], 2)};
        Float d[3]{
            vertices(tet[3], 0),
            vertices(tet[3], 1),
            vertices(tet[3], 2)};

        // NOTE:
        //
        // orient3d(a,b,c,d) returns positive if d is below the plane defined by
        // triangle (a, b, c).  The tet vertex ordering used by MSH format is
        // the opposite (i.e. d is above the triangle (a,b,c)), so this check
        // switch the ordering of a and b to return positive if tet is not
        // inverted.
        results[i] = prdc_orient3d(b, a, c, d);
    }
    return results;
}

VectorF VoxelUtils::is_delaunay(
        const MatrixFr& vertices,
        const MatrixIr& tets) {
    const size_t num_tets = tets.rows();
    VectorF result(num_tets);
    result.setConstant(1);

    MultipletMap<Triplet, size_t> adj_list;
    for (size_t i=0; i<num_tets; i++) {
        adj_list.insert({tets(i,0), tets(i,1), tets(i,2)}, i);
        adj_list.insert({tets(i,1), tets(i,2), tets(i,3)}, i);
        adj_list.insert({tets(i,2), tets(i,3), tets(i,0)}, i);
        adj_list.insert({tets(i,3), tets(i,0), tets(i,1)}, i);
    }

    constexpr int INVALID = std::numeric_limits<int>::max();
    auto get_opposite_vertex = [&tets, INVALID](size_t index, const Triplet& f) {
        for (size_t i=0; i<4; i++) {
            const int val = tets(index, i);
            if (val != f.get_data()[0] &&
                val != f.get_data()[1] &&
                val != f.get_data()[2]) {
                return val;
            }
        }
        // Tet is topologically degenerate.
        return INVALID;
    };

    for (size_t i=0; i<num_tets; i++) {
        const int i0 = tets(i,0);
        const int i1 = tets(i,1);
        const int i2 = tets(i,2);
        const int i3 = tets(i,3);
        const Vector3F v0 = vertices.row(i0);
        const Vector3F v1 = vertices.row(i1);
        const Vector3F v2 = vertices.row(i2);
        const Vector3F v3 = vertices.row(i3);

        std::vector<Triplet> faces = {
            {i0, i1, i2},
            {i1, i2, i3},
            {i2, i3, i0},
            {i3, i0, i1}
        };

        for (const auto& f : faces) {
            if (result[i] < 0) break;
            const auto& adj_tets = adj_list[f];
            for (auto j : adj_tets) {
                if (i==j) continue;
                if (result[i] < 0) break;
                const int oppo = get_opposite_vertex(j, f);
                const Vector3F vo = vertices.row(oppo);
                // Note that the orientation of the sphere/tet is different from
                // the orientation defined by the MSH format.  Swapping v0 and
                // v1 to ensure consistency.
                auto r = prdc_insphere(
                        const_cast<double*>(v1.data()),
                        const_cast<double*>(v0.data()),
                        const_cast<double*>(v2.data()),
                        const_cast<double*>(v3.data()),
                        const_cast<double*>(vo.data()));
                if (r > 0) {
                    result[i] = -1;
                } else if (r == 0) {
                    result[i] = 0;
                }
            }
        }
    }
    return result;
}

