#include "DegeneratedTriangleRemoval.h"

#include <cassert>
#include <iostream>
#include <limits>
#include <set>
#include <vector>

#include <Math/MatrixUtils.h>
#include <MeshUtils/FaceUtils.h>
#include <MeshUtils/IsolatedVertexRemoval.h>
#include <MeshUtils/ShortEdgeRemoval.h>
#include <Predicates/predicates.h>

using namespace PyMesh;

namespace DegeneratedTriangleRemovalHelper {
    const size_t INVALID = std::numeric_limits<size_t>::max();
}
using namespace DegeneratedTriangleRemovalHelper;

DegeneratedTriangleRemoval::DegeneratedTriangleRemoval(
        const MatrixFr& vertices, const MatrixIr& faces) :
m_vertices(vertices), m_faces(faces) {
    assert(m_faces.cols() == 3);
    prdc_exactinit();
    init_ori_face_indices();
}

void DegeneratedTriangleRemoval::run(size_t num_iterations) {
    size_t num_removed = 0;
    size_t count = 0;
    do {
        num_removed = 0;
        size_t e_removed = remove_zero_edges();
        size_t e_flipped = remove_line_faces();
        remove_isolated_vertices();
        count++;
        num_removed += (e_removed + e_flipped);
    } while (num_removed != 0 && count < num_iterations);

    if (num_removed != 0) {
        std::cerr << "Warning: Max number of iterations reached.  ";
        std::cerr << "Not all degenerated faces are removed." << std::endl;
    }
}

void DegeneratedTriangleRemoval::init_ori_face_indices() {
    const size_t num_faces = m_faces.rows();
    m_ori_face_indices.resize(num_faces);
    for (size_t i=0; i<num_faces; i++) {
        m_ori_face_indices[i] = i;
    }
}

size_t DegeneratedTriangleRemoval::remove_zero_edges() {
    ShortEdgeRemoval remover(m_vertices, m_faces);
    const size_t num_removed = remover.run(std::numeric_limits<Float>::epsilon());
    m_vertices = remover.get_vertices();
    m_faces = remover.get_faces();

    auto sources = remover.get_face_indices();
    const size_t num_faces = m_faces.rows();
    assert(num_faces == sources.rows());
    assert(sources.size() == 0 || sources.minCoeff() >= 0);
    assert(sources.size() == 0 ||
            sources.maxCoeff() < m_ori_face_indices.size());

    VectorI updated_ori_face_indices(num_faces);
    for (size_t i=0; i<num_faces; i++) {
        updated_ori_face_indices[i] = m_ori_face_indices[sources[i]];
    }
    m_ori_face_indices = updated_ori_face_indices;

    return num_removed;
}

size_t DegeneratedTriangleRemoval::remove_line_faces() {
    init_edge_map();
    // loop through faces, check if a face is a colinear.
    // If so, flip the longest edge.

    auto comp = [&](const Duplet& e1, const Duplet& e2) -> bool{
        // Return true if e1 is longer than e2.
        auto v1 = m_vertices.row(e1.get_ori_data()[0]);
        auto v2 = m_vertices.row(e1.get_ori_data()[1]);
        auto v3 = m_vertices.row(e2.get_ori_data()[0]);
        auto v4 = m_vertices.row(e2.get_ori_data()[1]);
        return (v1-v2).squaredNorm() > (v3-v4).squaredNorm();
    };

    const size_t num_faces = m_faces.rows();
    std::set<Duplet, decltype(comp)> edges_to_remove(comp);
    std::vector<size_t> longest_edges(num_faces, INVALID);
    for (size_t fi=0; fi<num_faces; fi++) {
        if (!is_degenerated(fi)) continue;
        const auto& face = m_faces.row(fi);
        const auto fi_opp_v = find_longest_edge(fi);
        const auto vi_0 = face[(fi_opp_v+1)%3];
        const auto vi_1 = face[(fi_opp_v+2)%3];
        edges_to_remove.insert(Duplet{vi_0, vi_1});
        longest_edges[fi] = fi_opp_v;
    }

    std::vector<VectorI> new_faces;
    std::vector<VectorI::Scalar> new_ori_face_indices;
    std::vector<bool> is_valid(num_faces, true);
    size_t count = 0;

    for (auto& edge : edges_to_remove) {
        auto& neighboring_faces = m_edge_map[edge];
        bool all_valid = true;
        for (auto fj : neighboring_faces) { all_valid &= is_valid[fj]; }
        if (!all_valid) continue;

        const size_t vi_0 = edge.get_ori_data()[0];
        const size_t vi_1 = edge.get_ori_data()[1];
        size_t vi_opp = INVALID;
        size_t fi = INVALID;
        for (auto fj : neighboring_faces) {
            if (longest_edges[fj] == INVALID) continue;
            const auto neighbor_face = m_faces.row(fj);
            const size_t vj_opp = neighbor_face[longest_edges[fj]];
            if (vj_opp != vi_0 && vj_opp != vi_1) {
                fi = fj;
                vi_opp = vj_opp;
                break;
            }
        }
        assert(vi_opp != INVALID);
        assert(fi != INVALID);

        for (auto fj : neighboring_faces) {
            is_valid[fj] = false;
            if (fj == fi) continue;
            const auto neighbor_face = m_faces.row(fj);
            size_t fj_opp_v = INVALID;
            for (size_t i=0; i<3; i++) {
                if (neighbor_face[i] != vi_0 && neighbor_face[i] != vi_1) {
                    fj_opp_v = i;
                    break;
                }
            }
            assert(fj_opp_v != INVALID);
            const size_t vj_opp = neighbor_face[fj_opp_v];
            const size_t vj_0 = neighbor_face[(fj_opp_v+1)%3];
            const size_t vj_1 = neighbor_face[(fj_opp_v+2)%3];
            new_faces.push_back(Vector3I(vi_opp, vj_opp, vj_0));
            new_faces.push_back(Vector3I(vi_opp, vj_1, vj_opp));
            new_ori_face_indices.push_back(m_ori_face_indices[fj]);
            new_ori_face_indices.push_back(m_ori_face_indices[fj]);
        }
        count++;
    }

    // Add untouched faces.
    for (size_t fi=0; fi<num_faces; fi++) {
        if (is_valid[fi]) {
            new_faces.push_back(m_faces.row(fi));
            new_ori_face_indices.push_back(m_ori_face_indices[fi]);
        }
    }
    if (!new_faces.empty()) {
        m_faces = MatrixUtils::rowstack(new_faces);
        m_ori_face_indices = MatrixUtils::std2eigen(new_ori_face_indices);
    } else {
        m_faces = MatrixIr(0, 3);
        m_ori_face_indices.resize(0);
    }
    assert(m_faces.rows() == new_faces.size());
    return count;
}

void DegeneratedTriangleRemoval::remove_isolated_vertices() {
    IsolatedVertexRemoval remover(m_vertices, m_faces);
    remover.run();
    m_vertices = remover.get_vertices();
    m_faces = remover.get_faces();
}

void DegeneratedTriangleRemoval::init_edge_map() {
    m_edge_map.clear();
    const size_t num_faces = m_faces.rows();
    assert(m_faces.cols() == 3);
    for (size_t i=0; i<num_faces; i++) {
        const auto& f = m_faces.row(i);
        m_edge_map.insert({f[0], f[1]}, i);
        m_edge_map.insert({f[1], f[2]}, i);
        m_edge_map.insert({f[2], f[0]}, i);
    }
}

bool DegeneratedTriangleRemoval::is_degenerated(size_t i) const {
    const auto& f = m_faces.row(i);
    const auto& v0 = m_vertices.row(f[0]);
    const auto& v1 = m_vertices.row(f[1]);
    const auto& v2 = m_vertices.row(f[2]);

    if (m_vertices.cols() == 3) {
        return FaceUtils::is_colinear_3D(v0, v1, v2);
    } else {
        return FaceUtils::is_colinear_2D(v0, v1, v2);
    }
}

size_t DegeneratedTriangleRemoval::find_longest_edge(size_t fi) const {
    // This algorithm only works if all vertices of this face is colinear.
    const auto& f = m_faces.row(fi);
    const auto& v0 = m_vertices.row(f[0]);
    const auto& v1 = m_vertices.row(f[1]);
    const auto& v2 = m_vertices.row(f[2]);
    size_t i = 0;
    if (v0[0] != v1[0] || v0[0] != v2[0] || v1[0] != v2[0]) {
        i = 0;
    } else if (v0[1] != v1[1] || v0[1] != v2[1] || v1[1] != v2[1]) {
        i = 1;
    } else if (v0[2] != v1[2] || v0[2] != v2[2] || v1[2] != v2[2]) {
        i = 2;
    } else {
        // The triangle degenerates to a point, which should be removed prior to
        // calling this function..
        std::cerr << f[0] << ": " << v0 << std::endl;
        std::cerr << f[1] << ": " << v1 << std::endl;
        std::cerr << f[2] << ": " << v2 << std::endl;
        throw RuntimeError("Triangle degenerates to a point, report this bug");
    }

    if (v0[i] < v1[i] && v0[i] > v2[i]) return 0;
    if (v0[i] > v1[i] && v0[i] < v2[i]) return 0;
    if (v1[i] < v0[i] && v1[i] > v2[i]) return 1;
    if (v1[i] > v0[i] && v1[i] < v2[i]) return 1;
    if (v2[i] < v0[i] && v2[i] > v1[i]) return 2;
    if (v2[i] > v0[i] && v2[i] < v1[i]) return 2;

    // If the execution reached there, two of the vertices must have the same
    // coordinates, which should have been removed prior to calling this mehtod.
    std::cerr << f[0] << ": " << v0 << std::endl;
    std::cerr << f[1] << ": " << v1 << std::endl;
    std::cerr << f[2] << ": " << v2 << std::endl;
    std::cerr << "|v0 - v1| = " << (v0-v1).norm() << std::endl;
    std::cerr << "|v1 - v2| = " << (v1-v2).norm() << std::endl;
    std::cerr << "|v2 - v0| = " << (v2-v0).norm() << std::endl;
    throw RuntimeError("Triangle contains an zero edge, report this bug");
}

