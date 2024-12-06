/* This file is part of PyMesh. Copyright (c) 2015 by Qingnan Zhou */
#ifdef WITH_CLIPPER
#include "ClipperEngine.h"
#include <Core/Exception.h>
#include <MeshUtils/Boundary.h>
#include <MeshUtils/DuplicatedVertexRemoval.h>
#include <Triangle/TriangleWrapper.h>

#include <cassert>
#include <iostream>
#include <list>
#include <limits>
#include <vector>
#include <unordered_map>
#include <unordered_set>

using namespace PyMesh;

namespace ClipperEngineHelper {
    typedef std::list<size_t> Loop;
    typedef std::list<Loop> Loops;

    /**
     * Extract boundary loops from an array of boudnary segments.
     * Assuming boundary segments are orientated consistently.
     */
    Loops extract_loops(const MatrixIr& boundaries) {
        assert(boundaries.cols() == 2);
        const size_t num_boundaries = boundaries.rows();
        std::unordered_map<size_t, std::list<MatrixIr::Scalar> > adjacencies;

        for (size_t i=0; i<num_boundaries; i++) {
            const VectorI& edge = boundaries.row(i);
            auto itr = adjacencies.find(edge[0]);
            if (itr != adjacencies.end()) {
                itr->second.push_back(edge[1]);
            } else {
                adjacencies.insert({(size_t)edge[0], {edge[1]}});
            }
        }

        Loops loops;
        std::unordered_set<size_t> visited;
        for (const auto& itr : adjacencies) {
            auto i = itr.first;
            if (itr.second.empty()) continue; // Vertex not part of loop.

            Loop loop;
            loop.push_back(i);
            visited.insert(i);
            do {
                auto& neighbors = adjacencies[loop.back()];
                if (neighbors.empty()) {
                    throw RuntimeError("Open loop detected.");
                }
                int next = neighbors.front();
                neighbors.pop_front();
                loop.push_back(next);
                visited.insert(next);
            } while (loop.back() != loop.front());
            loop.pop_back();
            if (loop.size() > 0) loops.push_back(loop);
        }
        return loops;
    }

    class PointConvert {
        public:
            static ClipperLib::IntPoint point_to_int_point(const Vector2F& p) {
                ClipperLib::IntPoint r;
                r.X = (ClipperLib::cInt)std::round(p.x() * FACTOR);
                r.Y = (ClipperLib::cInt)std::round(p.y() * FACTOR);
                return r;
            }

            static Vector2F int_point_to_point(const ClipperLib::IntPoint& p) {
                return Vector2F(p.X, p.Y) / FACTOR;
            }

            static const int FACTOR;
    };
    const int PointConvert::FACTOR = 1<<23;

    ClipperLib::Path loop_to_path(const MatrixFr& vertices, const Loop& loop) {
        ClipperLib::Path path;
        for (auto i : loop) {
            path.push_back(
                    PointConvert::point_to_int_point(
                        vertices.row(i)));
        }
        return path;
    }

    Loop path_to_loop(const ClipperLib::Path& path,
            std::list<Vector2F>& vertices) {
        const size_t base_index = vertices.size();
        const size_t num_segments = path.size();
        for (auto p : path) {
            Vector2F v = PointConvert::int_point_to_point(p);
            vertices.push_back(v);
        }

        Loop loop;
        for (size_t i=0; i<num_segments; i++) {
            loop.push_back(i + base_index);
        }
        return loop;
    }

    void remove_duplicated_vertices(MatrixFr& vertices, MatrixIr& segments) {
        DuplicatedVertexRemoval remover(vertices, segments);
        const size_t num_removed = remover.run(std::numeric_limits<Float>::min());
        assert(num_removed < vertices.rows());
        vertices = remover.get_vertices();
        segments = remover.get_faces();
    }
}

using namespace ClipperEngineHelper;

void ClipperEngine::compute_union() {
    clip(ClipperLib::ctUnion);
}

void ClipperEngine::compute_intersection() {
    clip(ClipperLib::ctIntersection);
}

void ClipperEngine::compute_difference() {
    clip(ClipperLib::ctDifference);
}

void ClipperEngine::compute_symmetric_difference() {
    clip(ClipperLib::ctXor);
}

void ClipperEngine::clip(ClipperLib::ClipType type) {
    initialize_if_needed();
    ClipperLib::Paths out;
    ClipperLib::Clipper clipper(
            ClipperLib::ioStrictlySimple | ClipperLib::ioPreserveCollinear);
    clipper.AddPaths(m_loops_1, ClipperLib::ptSubject, true);
    clipper.AddPaths(m_loops_2, ClipperLib::ptClip, true);
    clipper.Execute(type, out, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
    extract_result(out);
}

void ClipperEngine::initialize_if_needed() {
    m_vertices = MatrixFr::Zero(0,2);
    m_faces = MatrixIr::Zero(0,3);
    if (m_vertices_1.cols() != 2) { m_vertices_1 = MatrixFr(m_vertices_1.leftCols(2)); }
    if (m_vertices_2.cols() != 2) { m_vertices_2 = MatrixFr(m_vertices_2.leftCols(2)); }
    m_loops_1.clear();
    m_loops_2.clear();
    extract_boundary_loops();
}

void ClipperEngine::extract_boundary_loops() {
    Boundary::Ptr bd_extractor_1 =
        Boundary::extract_surface_boundary_raw(m_vertices_1, m_faces_1);
    Boundary::Ptr bd_extractor_2 =
        Boundary::extract_surface_boundary_raw(m_vertices_2, m_faces_2);

    MatrixIr boundary_1 = bd_extractor_1->get_boundaries();
    MatrixIr boundary_2 = bd_extractor_2->get_boundaries();

    Loops loops_1 = extract_loops(boundary_1);
    Loops loops_2 = extract_loops(boundary_2);

    for (const auto& loop : loops_1) {
        m_loops_1.push_back(loop_to_path(m_vertices_1, loop));
    }
    for (const auto& loop : loops_2) {
        m_loops_2.push_back(loop_to_path(m_vertices_2, loop));
    }
}

void ClipperEngine::extract_result(const ClipperLib::Paths& paths) {
    std::list<Vector2F> vertices;
    Loops loops;
    size_t num_segments = 0;
    for (const auto& path : paths) {
        Loop loop = path_to_loop(path, vertices);
        loops.push_back(loop);
        num_segments += loop.size();
    }

    const size_t num_loop_vertices = vertices.size();
    MatrixFr loop_vertices(num_loop_vertices, 2);
    size_t count = 0;
    for (const auto& v : vertices) {
        loop_vertices.row(count) = v;
        count++;
    }

    MatrixIr segments(num_segments, 2);
    count = 0;
    for (auto loop : loops) {
        const size_t loop_size = loop.size();
        size_t i=0;
        for (auto v_index : loop) {
            const size_t curr_segment = count + i;
            const size_t next_segment = count + (i+1)%loop_size;
            segments.coeffRef(curr_segment, 1) = v_index;
            segments.coeffRef(next_segment, 0) = v_index;
            i++;
        }
        count+=loop_size;
    }

    if (num_segments > 0) {
        // Known issue: duplicated vertices from different loops cause triangle to crash.
        ::remove_duplicated_vertices(loop_vertices, segments);
        TriangleWrapper tri_wrapper;
        tri_wrapper.set_points(loop_vertices);
        tri_wrapper.set_segments(segments);
        tri_wrapper.set_split_boundary(false);
        tri_wrapper.set_auto_hole_detection(true);
        tri_wrapper.set_max_num_steiner_points(0);
        tri_wrapper.set_verbosity(0);
        tri_wrapper.run();

        m_vertices = tri_wrapper.get_vertices();
        m_faces = tri_wrapper.get_faces();
    }
}

#endif
