/* This file is part of PyMesh. Copyright (c) 2015 by Qingnan Zhou */
#include "MeshUtils.h"
#include <cassert>
#include <map>
#include <vector>

#include <Core/Exception.h>
#include <Misc/Multiplet.h>

using namespace PyMesh;
using namespace MeshUtils;

MatrixI MeshUtils::extract_exterior_faces(const MatrixI& voxels) {
    if (voxels.cols() != 4) {
        throw NotImplementedError("Only tet mesh is supported");
    }

    typedef std::map<Triplet, int> FaceCounter;
    FaceCounter face_counter;
    const size_t num_voxels = voxels.rows();
    for (size_t i=0; i<num_voxels; i++) {
        const VectorI& voxel = voxels.row(i);
        Triplet voxel_faces[4] = {
            {voxel[0], voxel[2], voxel[1]},
            {voxel[0], voxel[1], voxel[3]},
            {voxel[0], voxel[3], voxel[2]},
            {voxel[1], voxel[2], voxel[3]}
        };
        for (size_t j=0; j<4; j++) {
            if (face_counter.find(voxel_faces[j]) == face_counter.end()) {
                face_counter[voxel_faces[j]] = 1;
            } else {
                face_counter[voxel_faces[j]] += 1;
            }
        }
    }

    std::vector<const Triplet*> vertex_buffer;
    vertex_buffer.reserve(face_counter.size());
    for (FaceCounter::const_iterator itr = face_counter.begin();
            itr!=face_counter.end(); itr++) {
        assert(itr->second == 1 || itr->second == 2);
        if (itr->second == 1) {
            vertex_buffer.push_back(&itr->first);
        }
    }

    const size_t num_faces = vertex_buffer.size();
    MatrixI result(num_faces, 3);
    for (size_t i=0; i<num_faces; i++) {
        const Triplet* face = vertex_buffer.at(i);
        result.row(i) = face->get_ori_data();
    }
    return result;
}

