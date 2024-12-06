/* This file is part of PyMesh. Copyright (c) 2015 by Qingnan Zhou */
#pragma once

#include <Core/EigenTypedef.h>

namespace PyMesh {

typedef Vector3F Point;
typedef Vector3F Vector;
typedef Float    Real;

/**
 * Triangle Metric keeps track of distance between 2 trinagles
 * By default Lin-Canny algorithm is used.
 * http://www.cs.berkeley.edu/~jfc/mirtich/collDet.html
 */
class TriangleMetric {
    public:
        TriangleMetric();

    public:
        void update(const Point& p1, const Point& p2, const Point& p3,
                const Point& p4, const Point& p5, const Point& p6);

        template<typename Derived>
        void update(const Eigen::MatrixBase<Derived>& tri1,
                const Eigen::MatrixBase<Derived>& tri2) {
            update( tri1.row(0), tri1.row(1), tri1.row(2),
                    tri2.row(0), tri2.row(1), tri2.row(2));
        }

        Real getClosestPts(Point& p1, Point& p2) {
            if (getUseLinCanny()) {
                return runLinCanny(p1, p2);
            } else {
                return oldCodeWrapper(p1, p2);
            }
        }
        Real getDistance() {
            Point p1,p2;
            return getClosestPts(p1, p2);
        }

    private:
        Real runLinCanny(Point& p1, Point& p2);
        bool VVCase(int tri1, int tri2, Point& p1, Point& p2);
        bool VECase(int tri1, int tri2, Point& p1, Point& p2);
        bool VFCase(int tri1, int tri2, Point& p1, Point& p2);
        bool EECase(int tri1, int tri2, Point& p1, Point& p2);
        bool EFCase(int tri1, int tri2, Point& p1, Point& p2);
        bool FFCase(int tri1, int tri2, Point& p1, Point& p2);

        void getClosestVEPts(const Point& v,
                const Point& e1, const Point& e2,
                Point& p1, Point& p2) const;
        void getClosestVFPts(const Point& v,
                const Point& f1, const Point& f2, const Point& f3,
                Point& p1, Point& p2) const;
        void getClosestEEPts(const Point& e1, const Point& e2,
                const Point& e3, const Point& e4,
                Point& p1, Point& p2) const;
        void getClosestEFPts(const Point& e1, const Point& e2,
                const Point& f1, const Point& f2, const Point& f3,
                Point& p1, Point& p2) const;

        bool vertexConeTest(Point& p, int tri_idx);
        bool edgeConeTest(Point& p, int tri_idx);
        bool faceConeTest(Point& p, int tri_idx);

        void print() const;

    private:
        Real oldCodeWrapper(Point& p1, Point& p2);
        // Old distance code.
        Real getClosestPointsOld(Real &u1, Real &v1, Real &w1,
                Real &u2, Real &v2, Real &w2);

        Real getVertexTriangleDistance(Point &p,
                Point &a, Point &b, Point &c,
                Real &t1, Real &t2, Real &t3);

        Real getEdgeEdgeDistance(Point &p1, Point &q1,
                Point &p2, Point &q2, Real &s, Real &t);

    public:
        static void test();

    private:
        enum FeatureType {
            VERTEX,
            EDGE,
            FACE
        };

        // Triangle coordinates.
        Point m_tris[2][3];

        FeatureType m_ft_types[2];
        int m_ft_idx[2];

    public:
        static void setUseLinCanny(bool v);
        static bool getUseLinCanny();

    private:
        static const Real epsilon;
        static bool use_lin_canny;
};

}
