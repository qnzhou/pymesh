/* This file is part of PyMesh. Copyright (c) 2018 by Qingnan Zhou */
#include "SnapRounding2.h"

#include <list>
#include <vector>
#include <set>

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Snap_rounding_traits_2.h>
#include <CGAL/Snap_rounding_2.h>

#ifdef WITH_CGAL
using namespace PyMesh;

void SnapRounding2::run(Float pixel_size, bool iterative) {
    using Kernel = CGAL::Exact_predicates_exact_constructions_kernel;
    using Traits = CGAL::Snap_rounding_traits_2<Kernel>;
    using Segment_2 = Kernel::Segment_2;
    using Point_2 = Kernel::Point_2;
    using Segment_list_2 = std::vector<Segment_2>;
    using Polyline_2 = std::list<Point_2>;
    using Polyline_list_2 = std::list<Polyline_2>;

    const size_t num_pts = m_points.rows();

    std::vector<Point_2> points;
    points.reserve(num_pts);
    for (size_t i=0; i<num_pts; i++) {
        points.emplace_back(m_points(i, 0), m_points(i, 1));
    }

    const size_t num_segments = m_segments.rows();
    std::vector<Segment_2> segments;
    segments.reserve(num_segments);
    for (size_t i=0; i<num_segments; i++) {
        Segment_2 s(points[m_segments(i, 0)], points[m_segments(i, 1)]);
        if (s.is_degenerate()) {
            // Warning: Degenearte segment will trigger CGAL assertion:
            //   RuntimeError: CGAL ERROR: precondition violation!
            //   Expr: ! is_degen
            //   File: /.../CGAL/Arr_segment_traits_2.h
            //   Line: 122
            //
            // Tested on CGAL 4.12.1.
            continue;
        }
        segments.push_back(s);
    }

    Polyline_list_2 output_list;
    CGAL::snap_rounding_2<Traits,Segment_list_2::const_iterator,Polyline_list_2>
        (segments.begin(), segments.end(),
         output_list,
         pixel_size,
         iterative,  // Whether to use iterative snap rounding.
         false,      // Whether to use int in output.
         1);         // Number of kd trees to use (heuristic for acceleration)

    size_t num_vertices = 0;
    size_t num_edges = 0;
    for (const auto& polyline : output_list) {
        assert(polyline.size() >= 1);
        num_vertices += polyline.size();
        num_edges += polyline.size() - 1;
    }

    m_vertices.resize(num_vertices, 2);
    m_edges.resize(num_edges, 2);

    size_t v_count = 0, e_count=0;
    for (const auto& polyline : output_list) {
        bool at_beginning = true;
        for (const auto& p : polyline) {
            m_vertices.row(v_count) <<
                CGAL::to_double(p.x()), CGAL::to_double(p.y());
            if (!at_beginning) {
                m_edges.row(e_count) << v_count-1, v_count;
                e_count++;
            }
            v_count++;
            at_beginning = false;
        }
    }
}

#endif
