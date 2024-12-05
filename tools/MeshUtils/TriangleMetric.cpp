/* This file is part of PyMesh. Copyright (c) 2015 by Qingnan Zhou */
#include "TriangleMetric.h"
#include <cassert>
#include <iostream>
#include <limits>

using namespace PyMesh;

namespace TriangleMetricHelper {
    Real operator*(const Vector& v1, const Vector& v2) {
        return v1.dot(v2);
    }
    Vector operator^(const Vector& v1, const Vector& v2) {
        return v1.cross(v2);
    }
}
using namespace TriangleMetricHelper;

const Real TriangleMetric::epsilon = 1e-8;
bool TriangleMetric::use_lin_canny = true;
void TriangleMetric::setUseLinCanny(bool v) {
	use_lin_canny = v;
}
bool TriangleMetric::getUseLinCanny() {
	return use_lin_canny;
}

TriangleMetric::TriangleMetric() {
	m_ft_types[0] = m_ft_types[1] = VERTEX;
	m_ft_idx[0] = m_ft_idx[1] = 0;
	//std::cout.precision(10);
}

void TriangleMetric::update(const Point& p1, const Point& p2, const Point& p3,
				const Point& p4, const Point& p5, const Point& p6) {
	m_tris[0][0] = p1;
	m_tris[0][1] = p2;
	m_tris[0][2] = p3;
	m_tris[1][0] = p4;
	m_tris[1][1] = p5;
	m_tris[1][2] = p6;
}

Real TriangleMetric::runLinCanny(Point& p1, Point& p2) {
	bool done = false;
	Real min_dist = std::numeric_limits<Real>::max() - 1;
	Point q1, q2;
	int count = 0;
	FeatureType temp_ft_types[2];
	int temp_ft_idx[2];
	FeatureType best_ft_types[2];
	int best_ft_idx[2];
	//std::cout << "================================" << std::endl;
	do {
		for (int i=0; i<2; i++) {
			temp_ft_types[i] = m_ft_types[i];
			temp_ft_idx[i] = m_ft_idx[i];
		}
		switch (m_ft_types[0]) {
			case VERTEX:
				switch (m_ft_types[1]) {
					case VERTEX:
						done = VVCase(0, 1, q1, q2);
						break;
					case EDGE:
						done = VECase(0, 1, q1, q2);
						break;
					case FACE:
						done = VFCase(0, 1, q1, q2);
						break;
				}
				break;
			case EDGE:
				switch (m_ft_types[1]) {
					case VERTEX:
						done = VECase(1, 0, q2, q1);
						break;
					case EDGE:
						done = EECase(0, 1, q1, q2);
						break;
					case FACE:
						done = EFCase(0, 1, q1, q2);
						break;
				}
				break;
			case FACE:
				switch (m_ft_types[1]) {
					case VERTEX:
						done = VFCase(1, 0, q2, q1);
						break;
					case EDGE:
						done = EFCase(1, 0, q2, q1);
						break;
					case FACE:
						done = FFCase(0, 1, q1, q2);
						break;
				}
				break;
		}

		Real dist = (q1 - q2).squaredNorm();
		if (dist != dist) {
			print();
			std::cout << "Warning: dist is NAN." << std::endl;
			std::cout << "Bail out using old code." << std::endl;
			return oldCodeWrapper(p1, p2);
		}
		//print();
		//std::cout << "\tdist: " << dist << " min_dist: " << min_dist
		//	<< " gap: " << dist - min_dist << std::endl;
		//std::cout << "\tft1: " << temp_ft_types[0]
		//	<< "\tft1 idx: " << temp_ft_idx[0] << std::endl;
		//std::cout << "\tft2: " << temp_ft_types[1]
		//	<< "\tft2 idx: " << temp_ft_idx[1] << std::endl;
		if (dist <= min_dist + epsilon) {
			min_dist = dist;
			p1 = q1;
			p2 = q2;
			for (int i=0; i<2; i++) {
				best_ft_types[i] = temp_ft_types[i];
				best_ft_idx[i] = temp_ft_idx[i];
			}
		} else {
			// Min distance should be monotonically decreasing.
			// If not, we probably entered a loop looking for closest
			// features. Just return the closest dist so far as an
			// approximation.
			Real diff = (dist - min_dist);
			if (diff > epsilon) {
				print();
				std::cout << "\tft1: " << temp_ft_types[0]
					<< "\tft1 idx: " << temp_ft_idx[0] << std::endl;
				std::cout << "\tft2: " << temp_ft_types[1]
					<< "\tft2 idx: " << temp_ft_idx[1] << std::endl;
				std::cout << "\tdist: " << dist << " min_dist: " << min_dist
					<< " gap: " << dist - min_dist << std::endl;
				std::cout
					<< "Warning: using approximation for triangle distances. "
					<< diff << std::endl;

				std::cout << "Bail out using old code." << std::endl;
				return oldCodeWrapper(p1, p2);
			}
			break;
		}
		count++;
		if (count > 100) {
			print();
			std::cout << "Warning: loop detected.  dist = "
				<< dist << std::endl;
			std::cout << "Bail out using old code." << std::endl;
			return oldCodeWrapper(p1, p2);
		}
	} while (!done);

	return min_dist;
}

bool TriangleMetric::VVCase(int tri1, int tri2, Point& p1, Point& p2) {
	Point& v1 = m_tris[tri1][m_ft_idx[tri1]];
	Point& v2 = m_tris[tri2][m_ft_idx[tri2]];

	p1 = v1;
	p2 = v2;
	if (!vertexConeTest(v1, tri2)) {
		return false;
	}
	if (!vertexConeTest(v2, tri1)) {
		return false;
	}
	return true;
}

bool TriangleMetric::VECase(int tri1, int tri2, Point& p1, Point& p2) {
	Point& v1 = m_tris[tri1][m_ft_idx[tri1]];
	Point& e1 = m_tris[tri2][m_ft_idx[tri2]];
	Point& e2 = m_tris[tri2][(m_ft_idx[tri2]+1)%3];

	getClosestVEPts(v1, e1, e2, p1, p2);

	if (!edgeConeTest(v1, tri2)) {
		return false;
	}

	if (!vertexConeTest(p2, tri1)) {
		return false;
	}
	return true;
}

bool TriangleMetric::VFCase(int tri1, int tri2, Point& p1, Point& p2) {
	Point& v1 = m_tris[tri1][m_ft_idx[tri1]];
	Point& f1 = m_tris[tri2][0];
	Point& f2 = m_tris[tri2][1];
	Point& f3 = m_tris[tri2][2];

	getClosestVFPts(v1, f1, f2, f3, p1, p2);

	if (!faceConeTest(v1, tri2)) {
		return false;
	}

	if (!vertexConeTest(p2, tri1)) {
		return false;
	}
	return true;
}

bool TriangleMetric::EECase(int tri1, int tri2, Point& p1, Point& p2) {
	Point& e1 = m_tris[tri1][m_ft_idx[tri1]];
	Point& e2 = m_tris[tri1][(m_ft_idx[tri1]+1)%3];
	Point& e3 = m_tris[tri2][m_ft_idx[tri2]];
	Point& e4 = m_tris[tri2][(m_ft_idx[tri2]+1)%3];

	getClosestEEPts(e1, e2, e3, e4, p1, p2);

	if (!edgeConeTest(p1, tri2)) {
		return false;
	}

	if (!edgeConeTest(p2, tri1)) {
		return false;
	}
	return true;
}

bool TriangleMetric::EFCase(int tri1, int tri2, Point& p1, Point& p2) {
	Point& e1 = m_tris[tri1][m_ft_idx[tri1]];
	Point& e2 = m_tris[tri1][(m_ft_idx[tri1]+1)%3];
	Point& f1 = m_tris[tri2][0];
	Point& f2 = m_tris[tri2][1];
	Point& f3 = m_tris[tri2][2];

	getClosestEFPts(e1, e2, f1, f2, f3, p1, p2);

	if (!faceConeTest(p1, tri2)) {
		return false;
	}

	if (!edgeConeTest(p2, tri1)) {
		return false;
	}
	return true;
}

bool TriangleMetric::FFCase(int tri1, int tri2, Point& p1, Point& p2) {
	// This is a bit expensive.
	// We need to check all of the following distance pairs
	//   * Vertex - Face plane dist (6 pairs)
	//   * Edge - Edge dist (9 pairs)
	std::cout << "Warning: slow" << std::endl;
	Real dist = std::numeric_limits<Real>::max();
	Real local_dist;
	Point& v1 = m_tris[tri1][0];
	Point& v2 = m_tris[tri1][1];
	Point& v3 = m_tris[tri1][2];
	Point& v4 = m_tris[tri2][0];
	Point& v5 = m_tris[tri2][1];
	Point& v6 = m_tris[tri2][2];
	Point c1,c2;

	getClosestVFPts(v1, v4, v5, v6, c1, c2);
	local_dist = (c1 - c2).squaredNorm();
	if (local_dist < dist) {
		dist = local_dist;
		p1 = c1;
		p2 = c2;
		m_ft_types[tri1] = VERTEX;
		m_ft_idx[tri1] = 0;
		m_ft_types[tri2] = FACE;
		m_ft_idx[tri2] = 0;
	}

	getClosestVFPts(v2, v4, v5, v6, c1, c2);
	local_dist = (c1 - c2).squaredNorm();
	if (local_dist < dist) {
		dist = local_dist;
		p1 = c1;
		p2 = c2;
		m_ft_types[tri1] = VERTEX;
		m_ft_idx[tri1] = 1;
		m_ft_types[tri2] = FACE;
		m_ft_idx[tri2] = 0;
	}

	getClosestVFPts(v3, v4, v5, v6, c1, c2);
	local_dist = (c1 - c2).squaredNorm();
	if (local_dist < dist) {
		dist = local_dist;
		p1 = c1;
		p2 = c2;
		m_ft_types[tri1] = VERTEX;
		m_ft_idx[tri1] = 2;
		m_ft_types[tri2] = FACE;
		m_ft_idx[tri2] = 0;
	}

	getClosestVFPts(v4, v1, v2, v3, c2, c1);
	local_dist = (c1 - c2).squaredNorm();
	if (local_dist < dist) {
		dist = local_dist;
		p1 = c1;
		p2 = c2;
		m_ft_types[tri1] = FACE;
		m_ft_idx[tri1] = 0;
		m_ft_types[tri2] = VERTEX;
		m_ft_idx[tri2] = 0;
	}

	getClosestVFPts(v5, v1, v2, v3, c2, c1);
	local_dist = (c1 - c2).squaredNorm();
	if (local_dist < dist) {
		dist = local_dist;
		p1 = c1;
		p2 = c2;
		m_ft_types[tri1] = FACE;
		m_ft_idx[tri1] = 0;
		m_ft_types[tri2] = VERTEX;
		m_ft_idx[tri2] = 1;
	}

	getClosestVFPts(v6, v1, v2, v3, c2, c1);
	local_dist = (c1 - c2).squaredNorm();
	if (local_dist < dist) {
		dist = local_dist;
		p1 = c1;
		p2 = c2;
		m_ft_types[tri1] = FACE;
		m_ft_idx[tri1] = 0;
		m_ft_types[tri2] = VERTEX;
		m_ft_idx[tri2] = 2;
	}

	getClosestEEPts(v1, v2, v4, v5, c1, c2);
	local_dist = (c1 - c2).squaredNorm();
	if (local_dist < dist) {
		dist = local_dist;
		p1 = c1;
		p2 = c2;
		m_ft_types[tri1] = EDGE;
		m_ft_idx[tri1] = 0;
		m_ft_types[tri2] = EDGE;
		m_ft_idx[tri2] = 0;
	}

	getClosestEEPts(v1, v2, v5, v6, c1, c2);
	local_dist = (c1 - c2).squaredNorm();
	if (local_dist < dist) {
		dist = local_dist;
		p1 = c1;
		p2 = c2;
		m_ft_types[tri1] = EDGE;
		m_ft_idx[tri1] = 0;
		m_ft_types[tri2] = EDGE;
		m_ft_idx[tri2] = 1;
	}

	getClosestEEPts(v1, v2, v6, v4, c1, c2);
	local_dist = (c1 - c2).squaredNorm();
	if (local_dist < dist) {
		dist = local_dist;
		p1 = c1;
		p2 = c2;
		m_ft_types[tri1] = EDGE;
		m_ft_idx[tri1] = 0;
		m_ft_types[tri2] = EDGE;
		m_ft_idx[tri2] = 2;
	}

	getClosestEEPts(v2, v3, v4, v5, c1, c2);
	local_dist = (c1 - c2).squaredNorm();
	if (local_dist < dist) {
		dist = local_dist;
		p1 = c1;
		p2 = c2;
		m_ft_types[tri1] = EDGE;
		m_ft_idx[tri1] = 1;
		m_ft_types[tri2] = EDGE;
		m_ft_idx[tri2] = 0;
	}

	getClosestEEPts(v2, v3, v5, v6, c1, c2);
	local_dist = (c1 - c2).squaredNorm();
	if (local_dist < dist) {
		dist = local_dist;
		p1 = c1;
		p2 = c2;
		m_ft_types[tri1] = EDGE;
		m_ft_idx[tri1] = 1;
		m_ft_types[tri2] = EDGE;
		m_ft_idx[tri2] = 1;
	}

	getClosestEEPts(v2, v3, v6, v4, c1, c2);
	local_dist = (c1 - c2).squaredNorm();
	if (local_dist < dist) {
		dist = local_dist;
		p1 = c1;
		p2 = c2;
		m_ft_types[tri1] = EDGE;
		m_ft_idx[tri1] = 1;
		m_ft_types[tri2] = EDGE;
		m_ft_idx[tri2] = 2;
	}

	getClosestEEPts(v3, v1, v4, v5, c1, c2);
	local_dist = (c1 - c2).squaredNorm();
	if (local_dist < dist) {
		dist = local_dist;
		p1 = c1;
		p2 = c2;
		m_ft_types[tri1] = EDGE;
		m_ft_idx[tri1] = 2;
		m_ft_types[tri2] = EDGE;
		m_ft_idx[tri2] = 0;
	}

	getClosestEEPts(v3, v1, v5, v6, c1, c2);
	local_dist = (c1 - c2).squaredNorm();
	if (local_dist < dist) {
		dist = local_dist;
		p1 = c1;
		p2 = c2;
		m_ft_types[tri1] = EDGE;
		m_ft_idx[tri1] = 2;
		m_ft_types[tri2] = EDGE;
		m_ft_idx[tri2] = 1;
	}

	getClosestEEPts(v3, v1, v6, v4, c1, c2);
	local_dist = (c1 - c2).squaredNorm();
	if (local_dist < dist) {
		dist = local_dist;
		p1 = c1;
		p2 = c2;
		m_ft_types[tri1] = EDGE;
		m_ft_idx[tri1] = 2;
		m_ft_types[tri2] = EDGE;
		m_ft_idx[tri2] = 2;
	}
	return false;
}

void TriangleMetric::getClosestVEPts(const Point& v,
		const Point& e1, const Point& e2,
		Point& p1, Point& p2) const {
	Vector u1 = v - e1;
	Vector u2 = e2 - e1;
	Real dot = u1 * u2;
	if (dot <= 0) {
		p1 = v;
		p2 = e1;
		return;
	}

	Real u2_l2_sq = u2.squaredNorm();
	if (dot >= u2_l2_sq) {
		p1 = v;
		p2 = e2;
		return;
	}

	p1 = v;
	p2 = e1 + u2 * dot / u2_l2_sq;
}

void TriangleMetric::getClosestVFPts(const Point& v,
		const Point& f1, const Point& f2, const Point& f3,
		Point& p1, Point& p2) const {
	Vector ab = f2 - f1;
	Vector ac = f3 - f1;
	Vector ap = v  - f1;

	Real d1 = ab * ap;
	Real d2 = ac * ap;
	if ((d1 <= 0.0f) && (d2 <= 0.0f)) {
		p1 = v;
		p2 = f1;
		return;
	}

	Vector bp = v - f2;
	Real d3 = ab * bp;
	Real d4 = ac * bp;
	if ((d3 >= 0.0f) && (d4 <= d3)) {
		p1 = v;
		p2 = f2;
		return;
	}

	Real vc = d1*d4 - d3*d2;
	if ((vc <= 0.0f) && (d1 >= 0.0f) && (d3 <= 0.0f)) {
		Real w = d1 / (d1 - d3);
		p1 = v;
		p2 = f1 + ab * w;
		return;
	}

	Vector cp = v - f3;
	Real d5 = ab * cp;
	Real d6 = ac * cp;
	if ((d6 >= 0.0f) && (d5 <= d6)) {
		p1 = v;
		p2 = f3;
		return;
	}

	Real vb = d5*d2 - d1*d6;
	if ((vb <= 0.0f) && (d2 >= 0.0f) && (d6 <= 0.0f)) {
		Real w = d2 / (d2 - d6);
		p1 = v;
		p2 = f1 + ac * w;
		return;
	}

	Real va = d3*d6 - d5*d4;
	if ((va <= 0.0f) && ((d4-d3) >= 0.0f) && ((d5-d6) >= 0.0f)) {
		Real w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
		p1 = v;
		p2 = f2*(1-w) + f3*w;
		return;
	}

	Real denom = 1.0f / (va + vb + vc);
	Real w2 = vb * denom;
	Real w3 = vc * denom;
	Real w1 = 1 - w2 - w3;
	p1 = v;
	p2 = f1*w1 + f2*w2 + f3*w3;
}

void TriangleMetric::getClosestEEPts(const Point& e1, const Point& e2,
		const Point& e3, const Point& e4,
		Point& p1, Point& p2) const {
	//const Real epsilon = 1e-8;
	Real s = -1;
	Real t = -1;
	Vector d1 = e2 - e1;
	Vector d2 = e4 - e3;
	Real   a  = d1.squaredNorm();
	Real   e  = d2.squaredNorm();
	if (a <= epsilon) {
		s = 0;
	}
	if (e <= epsilon) {
		t = 0;
	}

	if (s < -epsilon || t < -epsilon) {
		Vector r = e1 - e3;

		Real f = d2 * r;
		Real c = d1 * r;
		Real b = d1 * d2;

		Real denom = a * e - b * b;
		if (denom < epsilon) {
			// Handle parallel case.
			Real s3 = 0;
			Real s4 = 1;
			Real s1 = r * d2 / e;
			Real s2 = ((r + d1).dot(d2)) / e;
			Real s_left;
			Real s_right;
			if (b > 0) {
				s_left = std::max(s1, s3);
				s_right = std::min(s2, s4);
			} else {
				s_left = std::max(s2, s3);
				s_right = std::min(s1, s4);
			}
			if (s_left > s_right) {
				if (b > 0) {
					s = (s1 < s3) ? 1 : 0;
					t = (s1 < s3) ? 0 : 1;
				} else {
					s = (s2 < s3) ? 0 : 1;
					t = (s2 < s3) ? 0 : 1;
				}
			} else {
				Real l;
				l = s_left;
				Real sl = (l - s1) / (s2 - s1);
				Real tl = l;
				Real dist_left = (r + d1 * sl - d2 * tl).squaredNorm();

				l = s_right;
				Real sr = (l - s1) / (s2 - s1);
				Real tr = l;
				Real dist_right = (r + d1 * sr - d2 * tr).squaredNorm();

				if (dist_left < dist_right) {
					s = sl;
					t = tl;
				} else {
					s = sr;
					t = tr;
				}
			}
		} else {
			s = (b * f - c * e) / denom;
			s = std::max(s, Real(0));
			s = std::min(s, Real(1));

			Real tnom = b * s + f;
			if (tnom < epsilon) {
				t = Real(0);
				s = -c / a;
			} else if (tnom > e) {
				t = Real(1);
				s = (b - c) / a;
			} else {
				t = tnom / e;
			}
			s = std::max(s, Real(0));
			s = std::min(s, Real(1));
		}
	}
	p1 = e1 + d1 * s;
	p2 = e3 + d2 * t;
}

void TriangleMetric::getClosestEFPts(const Point& e1, const Point& e2,
		const Point& f1, const Point& f2, const Point& f3,
		Point& p1, Point& p2) const {
	// The closest point pairs could be captured in 2 cases:
	//   p1 is a end pt of the edge, and p2 is an interior pt of the triangle.
	//   p1 and p2 are closest pts between the edge and an edge of triangle.
	//
	// The case *NOT* handled here when the edge penetrate the triangle,
	// in which case the closest pts should be somewhere in the middle of
	// the edge and interior of the triangle.
	Real dist = std::numeric_limits<Real>::max();
	Real local_dist;
	Point c1, c2;

	// Check e1 to face plane.
	getClosestVFPts(e1, f1, f2, f3, c1, c2);
	local_dist = (c1 - c2).squaredNorm();
	if (local_dist < dist) {
		dist = local_dist;
		p1 = c1;
		p2 = c2;
	}

	// Check e2 to face plane.
	getClosestVFPts(e2, f1, f2, f3, c1, c2);
	local_dist = (c1 - c2).squaredNorm();
	if (local_dist < dist) {
		dist = local_dist;
		p1 = c1;
		p2 = c2;
	}

	// Check e1 e2 to f1 f2.
	getClosestEEPts(e1, e2, f1, f2, c1, c2);
	local_dist = (c1 - c2).squaredNorm();
	if (local_dist < dist) {
		dist = local_dist;
		p1 = c1;
		p2 = c2;
	}

	// Check e1 e2 to f2 f3.
	getClosestEEPts(e1, e2, f2, f3, c1, c2);
	local_dist = (c1 - c2).squaredNorm();
	if (local_dist < dist) {
		dist = local_dist;
		p1 = c1;
		p2 = c2;
	}

	// Check e1 e2 to f3 f1.
	getClosestEEPts(e1, e2, f3, f1, c1, c2);
	local_dist = (c1 - c2).squaredNorm();
	if (local_dist < dist) {
		dist = local_dist;
		p1 = c1;
		p2 = c2;
	}
}

bool TriangleMetric::vertexConeTest(Point& p, int tri_idx) {
	Point& v = m_tris[tri_idx][m_ft_idx[tri_idx]];
	Vector dir = (p - v);

	Vector n1 = (m_tris[tri_idx][(m_ft_idx[tri_idx]+1)%3] - v);
	if (dir * n1 > epsilon) {
		m_ft_types[tri_idx] = EDGE;
		// no change in m_ft_idx[tri_idx]
		return false;
	}

	Vector n2 = (m_tris[tri_idx][(m_ft_idx[tri_idx]+2)%3] - v);
	if (dir * n2 > epsilon) {
		m_ft_types[tri_idx] = EDGE;
		m_ft_idx[tri_idx] = (m_ft_idx[tri_idx] + 2) % 3;
		return false;
	}
	return true;
}

bool TriangleMetric::edgeConeTest(Point& p, int tri_idx) {
	Point& e1 = m_tris[tri_idx][m_ft_idx[tri_idx]];
	Point& e2 = m_tris[tri_idx][(m_ft_idx[tri_idx]+1)%3];
	Point& o  = m_tris[tri_idx][(m_ft_idx[tri_idx]+2)%3];

	Vector n1 = e2 - e1;
	Vector v1 = p  - e1;
	if (n1 * v1 < -epsilon) {
		m_ft_types[tri_idx] = VERTEX;
		// no change in m_ft_idx[tri_idx]
		return false;
	}

	Vector n2 = e1 - e2;
	Vector v2 = p  - e2;
	if (n2 * v2 < -epsilon) {
		m_ft_types[tri_idx] = VERTEX;
		m_ft_idx[tri_idx] = (m_ft_idx[tri_idx] + 1) % 3;
		return false;
	}

	Vector n = (e1 - o) ^ (e2 - o);
	Vector n3 = n1 ^ n;
	n3.normalize();
	if (n3 * v1 < -epsilon) {
		m_ft_types[tri_idx] = FACE;
		// no change in m_ft_idx[tri_idx]
		return false;
	}
	return true;
}

bool TriangleMetric::faceConeTest(Point& p, int tri_idx) {
	//m_ft_types[tri_idx] = VERTEX;
	//m_ft_idx[tri_idx] = 0;
	//if (vertexConeTest(p, tri_idx)) {
	//	return false;
	//}
	//m_ft_types[tri_idx] = VERTEX;
	//m_ft_idx[tri_idx] = 1;
	//if (vertexConeTest(p, tri_idx)) {
	//	return false;
	//}
	//m_ft_types[tri_idx] = VERTEX;
	//m_ft_idx[tri_idx] = 2;
	//if (vertexConeTest(p, tri_idx)) {
	//	return false;
	//}

	//m_ft_types[tri_idx] = EDGE;
	//m_ft_idx[tri_idx] = 0;
	//if (edgeConeTest(p, tri_idx)) {
	//	return false;
	//}
	//m_ft_types[tri_idx] = EDGE;
	//m_ft_idx[tri_idx] = 1;
	//if (edgeConeTest(p, tri_idx)) {
	//	return false;
	//}
	//m_ft_types[tri_idx] = EDGE;
	//m_ft_idx[tri_idx] = 2;
	//if (edgeConeTest(p, tri_idx)) {
	//	return false;
	//}

	//m_ft_types[tri_idx] = FACE;
	//m_ft_idx[tri_idx] = 0;
	//return true;



	Point& f1 = m_tris[tri_idx][0];
	Point& f2 = m_tris[tri_idx][1];
	Point& f3 = m_tris[tri_idx][2];
	Vector e1 = f2 - f1;
	Vector e2 = f3 - f2;
	Vector e3 = f1 - f3;

	// Check vertex cones.
	Vector v = p - f1;
	if (v * e1 < epsilon && v * e3 > -epsilon) {
		m_ft_types[tri_idx] = VERTEX;
		m_ft_idx[tri_idx] = 0;
		return false;
	}
	v = p - f2;
	if (v * e2 < epsilon && v * e1 > -epsilon) {
		m_ft_types[tri_idx] = VERTEX;
		m_ft_idx[tri_idx] = 1;
		return false;
	}
	v = p - f3;
	if (v * e3 < epsilon && v * e2 > -epsilon) {
		m_ft_types[tri_idx] = VERTEX;
		m_ft_idx[tri_idx] = 2;
		return false;
	}

	// Check edge cones.
	Vector n = (f2 - f1) ^ (f3 - f1);
	Real min_dist = 0;
	Real dist;
	int min_idx = 0;

	Vector n1 = n ^ (f2 - f1);
	n1.normalize();
	Vector v1 = p - f1;
	dist = n1 * v1;
	if (dist < min_dist) {
		min_dist = dist;
		min_idx = 0;
	}

	Vector n2 = n ^ (f3 - f2);
	n2.normalize();
	Vector v2 = p - f2;
	dist = n2 * v2;
	if (dist < min_dist) {
		min_dist = dist;
		min_idx = 1;
	}

	Vector n3 = n ^ (f1 - f3);
	n3.normalize();
	Vector v3 = p - f3;
	dist = n3 * v3;
	if (dist < min_dist) {
		min_dist = dist;
		min_idx = 2;
	}

	if (min_dist >= -epsilon) {
		return true;
	} else {
		m_ft_types[tri_idx] = EDGE;
		m_ft_idx[tri_idx] = min_idx;
		return false;
	}
}

void TriangleMetric::print() const {
	std::cout << "tri1: " << std::endl;
	std::cout << "\tv "
		<< m_tris[0][0][0] << " "
		<< m_tris[0][0][1] << " "
		<< m_tris[0][0][2] << std::endl;
	std::cout << "\tv "
		<< m_tris[0][1][0] << " "
		<< m_tris[0][1][1] << " "
		<< m_tris[0][1][2] << std::endl;
	std::cout << "\tv "
		<< m_tris[0][2][0] << " "
		<< m_tris[0][2][1] << " "
		<< m_tris[0][2][2] << std::endl;
	std::cout << "\tFt:" << m_ft_types[0]
		<< "\tFt_idx:" << m_ft_idx[0] << std::endl;

	std::cout << "tri2: " << std::endl;
	std::cout << "\tv "
		<< m_tris[1][0][0] << " "
		<< m_tris[1][0][1] << " "
		<< m_tris[1][0][2] << std::endl;
	std::cout << "\tv "
		<< m_tris[1][1][0] << " "
		<< m_tris[1][1][1] << " "
		<< m_tris[1][1][2] << std::endl;
	std::cout << "\tv "
		<< m_tris[1][2][0] << " "
		<< m_tris[1][2][1] << " "
		<< m_tris[1][2][2] << std::endl;
	std::cout << "\tFt:" << m_ft_types[1]
		<< "\tFt_idx:" << m_ft_idx[1] << std::endl;
}

Real TriangleMetric::oldCodeWrapper(Point& p1, Point& p2) {
	Real u1, v1, w1, u2, v2, w2;
	Real w = getClosestPointsOld(u1, v1, w1, u2, v2, w2); 
	p1 = m_tris[0][0] * u1 + m_tris[0][1] * v1 + m_tris[0][2] * w1;
	p2 = m_tris[1][0] * u2 + m_tris[1][1] * v2 + m_tris[1][2] * w2;
	return w*w;
}

Real TriangleMetric::getClosestPointsOld(Real& u1, Real& v1, Real& w1,
		Real& u2, Real& v2, Real& w2) {
	Real minDist = std::numeric_limits<Real>::max();

	// Check each vertex-triangle pair
	//
	for (size_t i=0; i<3; ++i)
	{
		Real t1, t2, t3;
		Real d = getVertexTriangleDistance( m_tris[0][i],
				m_tris[1][0], m_tris[1][1], m_tris[1][2], t1, t2, t3);
		if (d < minDist)
		{
			minDist = d;

			if (i==0)	   { u1 = 1.0; v1 = 0.0; w1 = 0.0; }
			else if (i==1) { u1 = 0.0; v1 = 1.0; w1 = 0.0; }
			else if (i==2) { u1 = 0.0; v1 = 0.0; w1 = 1.0; }

			u2 = t1;
			v2 = t2;
			w2 = t3;
		}
		
		d = getVertexTriangleDistance( m_tris[1][i],
				m_tris[0][0], m_tris[0][1], m_tris[0][2], t1, t2, t3);
		if (d < minDist)
		{
			minDist = d;

			if (i==0)	   { u2 = 1.0; v2 = 0.0; w2 = 0.0; }
			else if (i==1) { u2 = 0.0; v2 = 1.0; w2 = 0.0; }
			else if (i==2) { u2 = 0.0; v2 = 0.0; w2 = 1.0; }

			u1 = t1;
			v1 = t2;
			w1 = t3;
		}
	}

	// Check each edge-edge pair
	//
	for (size_t i=0; i<3; ++i)
	{
		for (size_t j=0; j<3; ++j)
		{
			Real s, t;
			Real d = getEdgeEdgeDistance(m_tris[0][i], m_tris[0][(i+1)%3],
					m_tris[1][j], m_tris[1][(j+1)%3], s, t);
			if (d < minDist)
			{
				minDist = d;

				if (i==0)      { u1 = (1.0-s); v1 = s; w1 = 0.0; }
				else if (i==1) { u1 = 0.0; v1 = (1.0-s); w1 = s; }
				else if (i==2) { u1 = s; v1 = 0.0; w1 = (1.0-s); }
				
				if (j==0)      { u2 = (1.0-t); v2 = t; w2 = 0.0; }
				else if (j==1) { u2 = 0.0; v2 = (1.0-t); w2 = t; }
				else if (j==2) { u2 = t; v2 = 0.0; w2 = (1.0-t); }
			}
		}
	}

	return sqrt(minDist);
}

Real TriangleMetric::getVertexTriangleDistance(Point& p,
		Point& a, Point& b, Point& c,
		Real& t1, Real& t2, Real& t3) {
	Real ab[3], ac[3], ap[3], bp[3];

	ab[0] = b[0] - a[0];
	ab[1] = b[1] - a[1];
	ab[2] = b[2] - a[2];

	ac[0] = c[0] - a[0];
	ac[1] = c[1] - a[1];
	ac[2] = c[2] - a[2];

	ap[0] = p[0] - a[0];
	ap[1] = p[1] - a[1];
	ap[2] = p[2] - a[2];

	Real d1 = ab[0]*ap[0] + ab[1]*ap[1] + ab[2]*ap[2];
	Real d2 = ac[0]*ap[0] + ac[1]*ap[1] + ac[2]*ap[2];

	if ((d1 <= 0.0f) && (d2 <= 0.0f))
	{
		t1 = 1.0f;
		t2 = 0.0f;
		t3 = 0.0f;

		return ((p[0]-a[0])*(p[0]-a[0]) + (p[1]-a[1])*(p[1]-a[1]) + (p[2]-a[2])*(p[2]-a[2]));
	}

	bp[0] = p[0] - b[0];
	bp[1] = p[1] - b[1];
	bp[2] = p[2] - b[2];

	Real d3 = ab[0]*bp[0] + ab[1]*bp[1] + ab[2]*bp[2];
	Real d4 = ac[0]*bp[0] + ac[1]*bp[1] + ac[2]*bp[2];

	if ((d3 >= 0.0f) && (d4 <= d3))
	{
		t1 = 0.0f;
		t2 = 1.0f;
		t3 = 0.0f;

		return ((p[0]-b[0])*(p[0]-b[0]) + (p[1]-b[1])*(p[1]-b[1]) + (p[2]-b[2])*(p[2]-b[2]));
	}

	Real vc = d1*d4 - d3*d2;

	if ((vc <= 0.0f) && (d1 >= 0.0f) && (d3 <= 0.0f))
	{
		Real v = d1 / (d1 - d3);
		
		t1 = 1-v;
		t2 = v;
		t3 = 0;

		Real vec[3];
		vec[0] = p[0] - (a[0]+v*ab[0]);
		vec[1] = p[1] - (a[1]+v*ab[1]);
		vec[2] = p[2] - (a[2]+v*ab[2]);

		return (vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2]);
	}

	Real cp[3];
	cp[0] = p[0] - c[0];
	cp[1] = p[1] - c[1];
	cp[2] = p[2] - c[2];
	
	Real d5 = ab[0]*cp[0] + ab[1]*cp[1] + ab[2]*cp[2];
	Real d6 = ac[0]*cp[0] + ac[1]*cp[1] + ac[2]*cp[2];

	if ((d6 >= 0.0f) && (d5 <= d6))
	{
		t1 = 0;
		t2 = 0;
		t3 = 1;

		return ((p[0]-c[0])*(p[0]-c[0]) + (p[1]-c[1])*(p[1]-c[1]) + (p[2]-c[2])*(p[2]-c[2]));
	}

	Real vb = d5*d2 - d1*d6;

	if ((vb <= 0.0f) && (d2 >= 0.0f) && (d6 <= 0.0f))
	{
		Real w = d2 / (d2 - d6);

		t1 = 1-w;
		t2 = 0;
		t3 = w;

		Real vec[3];
		vec[0] = p[0] - (a[0]+w*ac[0]);
		vec[1] = p[1] - (a[1]+w*ac[1]);
		vec[2] = p[2] - (a[2]+w*ac[2]);

		return (vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2]);
	}

	Real va = d3*d6 - d5*d4;

	if ((va <= 0.0f) && ((d4-d3) >= 0.0f) && ((d5-d6) >= 0.0f))
	{
		Real w = (d4 - d3) / ((d4 - d3) + (d5 - d6));

		t1 = 0;
		t2 = 1-w;
		t3 = w;

		Real vec[3];
		vec[0] = p[0] - (b[0]+w*(c[0]-b[0]));
		vec[1] = p[1] - (b[1]+w*(c[1]-b[1]));
		vec[2] = p[2] - (b[2]+w*(c[2]-b[2]));

		return (vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2]);
	}

	Real denom = 1.0f / (va + vb + vc);
	Real v = vb * denom;
	Real w = vc * denom;
	Real u = 1.0 - v - w;

	t1 = u;
	t2 = v;
	t3 = w;

	Real vec[3];
	vec[0] = p[0] - (u*a[0] + v*b[0] + w*c[0]);
	vec[1] = p[1] - (u*a[1] + v*b[1] + w*c[1]);
	vec[2] = p[2] - (u*a[2] + v*b[2] + w*c[2]);

	return (vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2]);
}

Real TriangleMetric::getEdgeEdgeDistance(Point& p1, Point& q1,
		Point& p2, Point& q2, Real& s, Real& t) {
	Real d1[3], d2[3], r[3], a, e, f;
	Real c1[3], c2[3];

	d1[0] = q1[0] - p1[0];
	d1[1] = q1[1] - p1[1];
	d1[2] = q1[2] - p1[2];

	d2[0] = q2[0] - p2[0];
	d2[1] = q2[1] - p2[1];
	d2[2] = q2[2] - p2[2];

	r[0] = p1[0] - p2[0];
	r[1] = p1[1] - p2[1];
	r[2] = p1[2] - p2[2];

	a = d1[0]*d1[0] + d1[1]*d1[1] + d1[2]*d1[2];
	e = d2[0]*d2[0] + d2[1]*d2[1] + d2[2]*d2[2];
	f = d2[0]*r[0] + d2[1]*r[1] + d2[2]*r[2];

	// check if either or both segments degenerate into points
	//
	if ((a <= 1e-8) && (e <= 1e-8))
	{
		s = t = 0.0f;
		c1[0] = p1[0]; c1[1] = p1[1]; c1[2] = p1[2];
		c2[0] = p2[0]; c2[1] = p2[1]; c2[2] = p2[2];

		return ((c1[0]-c2[0])*(c1[0]-c2[0]) + (c1[1]-c2[1])*(c1[1]-c2[1]) + (c1[2]-c2[2])*(c1[2]-c2[2]));
	}

	if (a <= 1e-8)
	{
		// first segment degenerates into a point
		//
		s = 0.0f;
		t = f / e;
		if (t<0.0f) t = 0.0f;
		if (t>1.0f) t = 1.0f;
	}
	else
	{
		Real c = d1[0]*r[0] + d1[1]*r[1] + d1[2]*r[2];

		if (e <= 1e-8)
		{
			// second segment degenerates into a point
			//
			t = 0.0f;
			s = -c / a;
			if (s<0.0f) s = 0.0f;
			if (s>1.0f) s = 1.0f;
		}
		else
		{
			// nondegenerate case
			//
			Real b = d1[0]*d2[0] + d1[1]*d2[1] + d1[2]*d2[2];
			Real denom = a*e - b*b;

			if (denom != 0.0f)
			{
				s = (b*f - c*e) / denom;
				if (s<0.0f) s = 0.0f;
				if (s>1.0f) s = 1.0f;
			}
			else
				s = 0.0f;

			Real tnom = b*s + f;
			if (tnom < 0.0f)
			{
				t = 0.0f;
				s = -c / a;
				if (s<0.0f) s = 0.0f;
				if (s>1.0f) s = 1.0f;
			}
			else if (tnom > e)
			{
				t = 1.0f;
				s = (b - c) / a;
				if (s<0.0f) s = 0.0f;
				if (s>1.0f) s = 1.0f;
			}
			else
				t = tnom / e;
		}
	}

	c1[0] = p1[0] + d1[0] * s;
	c1[1] = p1[1] + d1[1] * s;
	c1[2] = p1[2] + d1[2] * s;

	c2[0] = p2[0] + d2[0] * t;
	c2[1] = p2[1] + d2[1] * t;
	c2[2] = p2[2] + d2[2] * t;

	return ((c1[0]-c2[0])*(c1[0]-c2[0]) + (c1[1]-c2[1])*(c1[1]-c2[1]) + (c1[2]-c2[2])*(c1[2]-c2[2]));
}

void TriangleMetric::test() {
	TriangleMetric tm;
	Point t1[3] = {
		Point(2.000098712, -0.04524066013, -0.9999982602),
		Point(1.999842166, -0.04465568953, 1.359489154e-05),
		Point(2.999878476, -0.04524056265, 1.530231913e-05)
	};
	Point t2[3] = {
		Point(3.000113436, -0.04465617663, -0.9999897823),
		Point(3.999907166, -0.02901589298, 2.439938101e-05),
		Point(4.000050746, -0.02771025116, -0.9999683076)
	};
	tm.update(t1[0], t1[1], t1[2], t2[0], t2[1], t2[2]);
	tm.m_ft_types[0] = EDGE;
	tm.m_ft_types[1] = EDGE;
	tm.m_ft_idx[0] = 2;
	tm.m_ft_idx[1] = 2;

	Point p1, p2;
	//Real d = tm.getClosestPts(p1, p2);
	std::cout << "p1: " << p1[0] << ", " << p1[1] << ", " << p1[2] << std::endl;
	std::cout << "p2: " << p2[0] << ", " << p2[1] << ", " << p2[2] << std::endl;
	std::cout << "dist is " << (p1 - p2).squaredNorm() << std::endl;
	tm.print();
	//bool done = tm.EFCase(1, 0, p2, p1);
	//std::cout << "done? " << done << std::endl;
	//std::cout << "p1: " << p1[0] << ", " << p1[1] << ", " << p1[2] << std::endl;
	//std::cout << "p2: " << p2[0] << ", " << p2[1] << ", " << p2[2] << std::endl;
	//std::cout << "dist is " << (p1 - p2).squaredNorm() << std::endl;
	//tm.print();

	//tm.getClosestVEPts(t1[0], t2[2], t2[0], p1, p2);
	//std::cout << "p1: " << p1[0] << ", " << p1[1] << ", " << p1[2] << std::endl;
	//std::cout << "p2: " << p2[0] << ", " << p2[1] << ", " << p2[2] << std::endl;
	//std::cout << "dist is " << (p1 - p2).squaredNorm() << std::endl;

	//tm.getClosestEEPts(t2[0], t2[1], t1[1], t1[2], p2, p1);
	//std::cout << "p1: " << p1[0] << ", " << p1[1] << ", " << p1[2] << std::endl;
	//std::cout << "p2: " << p2[0] << ", " << p2[1] << ", " << p2[2] << std::endl;
	//std::cout << "dist is " << (p1 - p2).squaredNorm() << std::endl;
}
