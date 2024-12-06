/* This file is part of PyMesh. Copyright (c) 2015 by Qingnan Zhou */
#include <cassert>
#include <cmath>
#include <functional>
#include <vector>
#include <Core/Exception.h>
extern "C" {
#include "tribox3.h"
}
#include "TriBox2D.h"

using namespace PyMesh;

namespace HashGridImplementationHelper {
    template<typename T>
    std::vector<Eigen::Matrix<T, 2, 1> >
    get_surrounding_cells(const Eigen::Matrix<T, 2, 1>& p) {
        typedef Eigen::Matrix<T, 2, 1> Key;
        return {
            Key(p[0]-1, p[1]-1),
            Key(p[0]-1, p[1]  ),
            Key(p[0]-1, p[1]+1),
            Key(p[0]  , p[1]-1),
            Key(p[0]  , p[1]  ),
            Key(p[0]  , p[1]+1),
            Key(p[0]+1, p[1]-1),
            Key(p[0]+1, p[1]  ),
            Key(p[0]+1, p[1]+1)
        };
    }

    template<typename T>
    std::vector<Eigen::Matrix<T, 3, 1> >
    get_surrounding_cells(const Eigen::Matrix<T, 3, 1>& p) {
        typedef Eigen::Matrix<T, 3, 1> Key;
        return {
            Key(p[0]-1, p[1]-1, p[2]-1),
            Key(p[0]-1, p[1]  , p[2]-1),
            Key(p[0]-1, p[1]+1, p[2]-1),
            Key(p[0]  , p[1]-1, p[2]-1),
            Key(p[0]  , p[1]  , p[2]-1),
            Key(p[0]  , p[1]+1, p[2]-1),
            Key(p[0]+1, p[1]-1, p[2]-1),
            Key(p[0]+1, p[1]  , p[2]-1),
            Key(p[0]+1, p[1]+1, p[2]-1),
            Key(p[0]-1, p[1]-1, p[2]  ),
            Key(p[0]-1, p[1]  , p[2]  ),
            Key(p[0]-1, p[1]+1, p[2]  ),
            Key(p[0]  , p[1]-1, p[2]  ),
            Key(p[0]  , p[1]  , p[2]  ),
            Key(p[0]  , p[1]+1, p[2]  ),
            Key(p[0]+1, p[1]-1, p[2]  ),
            Key(p[0]+1, p[1]  , p[2]  ),
            Key(p[0]+1, p[1]+1, p[2]  ),
            Key(p[0]-1, p[1]-1, p[2]+1),
            Key(p[0]-1, p[1]  , p[2]+1),
            Key(p[0]-1, p[1]+1, p[2]+1),
            Key(p[0]  , p[1]-1, p[2]+1),
            Key(p[0]  , p[1]  , p[2]+1),
            Key(p[0]  , p[1]+1, p[2]+1),
            Key(p[0]+1, p[1]-1, p[2]+1),
            Key(p[0]+1, p[1]  , p[2]+1),
            Key(p[0]+1, p[1]+1, p[2]+1)
        };
    }
}
using namespace HashGridImplementationHelper;

template<typename Trait>
HashGridImplementation<Trait>::HashGridImplementation(Float cell_size)
    : HashGrid(cell_size) {
        m_hash_map = Trait::create_map();
}

template<typename Trait>
bool HashGridImplementation<Trait>::insert(int obj_id, const VectorF& coordinates) {
    HashKey key = convert_to_key(coordinates);
    return insert_key(obj_id, key);
}

template<typename Trait>
bool HashGridImplementation<Trait>::insert_bbox(int obj_id, const MatrixF& shape) {
    assert(shape.cols() == 3 || shape.cols() == 2);
    VectorF bbox_min = shape.colwise().minCoeff();
    VectorF bbox_max = shape.colwise().maxCoeff();
    HashKey min_key = convert_to_key(bbox_min);
    HashKey max_key = convert_to_key(bbox_max);

    bool success = true;
    if (Trait::dim == 3) {
        for (typename HashKey::ValueType x=min_key[0]; x<=max_key[0]; x+=1) {
            for (typename HashKey::ValueType y=min_key[1]; y<=max_key[1]; y+=1) {
                for (typename HashKey::ValueType z=min_key[2]; z<=max_key[2]; z+=1) {
                    HashKey cur_key({x, y, z});
                    bool r = insert_key(obj_id, cur_key);
                    success &= r;
                }
            }
        }
    } else if (Trait::dim == 2) {
        for (typename HashKey::ValueType x=min_key[0]; x<=max_key[0]; x+=1) {
            for (typename HashKey::ValueType y=min_key[1]; y<=max_key[1]; y+=1) {
                HashKey cur_key({x, y});
                bool r = insert_key(obj_id, cur_key);
                success &= r;
            }
        }
    } else {
        throw NotImplementedError("Only 2D and 3D are supported in HashGrid.");
    }
    return success;
}

template<typename Trait>
bool HashGridImplementation<Trait>::insert_triangle(int obj_id, const MatrixFr& shape) {
    assert(shape.cols() == 3 || shape.cols() == 2);
    const Float EPS = 1e-6;
    VectorF bbox_min = shape.colwise().minCoeff();
    VectorF bbox_max = shape.colwise().maxCoeff();
    bbox_min.array() -= EPS;
    bbox_max.array() += EPS;
    HashKey min_key = convert_to_key(bbox_min);
    HashKey max_key = convert_to_key(bbox_max);

    bool success = true;
    if (Trait::dim == 3) {
        const Float tri[3][3] = {
            {shape(0,0), shape(0,1), shape(0,2)},
            {shape(1,0), shape(1,1), shape(1,2)},
            {shape(2,0), shape(2,1), shape(2,2)}
        };

        Vector3F cell_sizes = Vector3F::Ones() * m_cell_size * 0.5;
        for (typename HashKey::ValueType x=min_key[0]; x<=max_key[0]; x+=1) {
            for (typename HashKey::ValueType y=min_key[1]; y<=max_key[1]; y+=1) {
                for (typename HashKey::ValueType z=min_key[2]; z<=max_key[2]; z+=1) {
                    HashKey cur_key({x, y, z});
                    VectorF grid_pt = convert_to_grid_point(cur_key);
                    int does_overlap = triBoxOverlap(
                            grid_pt.data(),
                            cell_sizes.data(),
                            tri);
                    if (does_overlap == 1) {
                        bool r = insert_key(obj_id, cur_key);
                        success &= r;
                    }
                }
            }
        }
    } else if (Trait::dim == 2) {
        const Float tri[3][2] = {
            {shape(0,0), shape(0,1)},
            {shape(1,0), shape(1,1)},
            {shape(2,0), shape(2,1)}
        };

        Vector2F cell_sizes = Vector2F::Ones() * m_cell_size * 0.5;
        for (typename HashKey::ValueType x=min_key[0]; x<=max_key[0]; x+=1) {
            for (typename HashKey::ValueType y=min_key[1]; y<=max_key[1]; y+=1) {
                HashKey cur_key({x, y});
                VectorF grid_pt = convert_to_grid_point(cur_key);
                int does_overlap = TriBox2D::triBoxOverlap(
                        grid_pt.data(),
                        cell_sizes.data(),
                        tri);
                if (does_overlap == 1) {
                    bool r = insert_key(obj_id, cur_key);
                    success &= r;
                }
            }
        }
    } else {
        throw NotImplementedError("Only 2D and 3D are supported in HashGrid.");
    }
    return success;
}

template<typename Trait>
bool HashGridImplementation<Trait>::insert_multiple_triangles(
        const VectorI& obj_ids, const MatrixFr& shape) {
    if (shape.rows() % 3 != 0) {
        std::stringstream err_msg;
        err_msg << "Expect number of vertices to be multiples of 3" << std::endl;
        err_msg << "but get " << shape.rows() << " vertices instead.";
        throw RuntimeError(err_msg.str());
    }
    if (obj_ids.size() * 3 != shape.rows()) {
        std::stringstream err_msg;
        err_msg << "Number of ids " << obj_ids.size()
            << " does not match the number of triangles "
            << shape.rows() / 3;
        throw RuntimeError(err_msg.str());
    }

    bool success = true;
    const size_t dim = shape.cols();
    const size_t num_faces = obj_ids.size();
    for (size_t i=0; i<num_faces; i++) {
        success &= insert_triangle(obj_ids[i], shape.block(i*3, 0, 3, dim));
    }
    return success;
}

template<typename Trait>
bool HashGridImplementation<Trait>::insert_batch(int obj_id, const MatrixFr& points) {
    size_t num_pts = points.rows();
    bool success = true;
    for (size_t i=0; i<num_pts; i++) {
        bool r = insert(obj_id, points.row(i));
        success &= r;
    }
    return success;
}

template<typename Trait>
bool HashGridImplementation<Trait>::insert_multiple(const VectorI& obj_ids, const MatrixFr& points) {
    size_t num_pts = points.rows();
    if (obj_ids.size() != num_pts) {
        std::stringstream err_msg;
        err_msg << "Number of object IDs does not match number of points: "
            << obj_ids.size() << " != " << num_pts;
        throw RuntimeError(err_msg.str());
    }
    bool success = true;
    for (size_t i=0; i<num_pts; i++) {
        bool r = insert(obj_ids[i], points.row(i));
        success &= r;
    }
    return success;
}

template<typename Trait>
bool HashGridImplementation<Trait>::remove(int obj_id, const VectorF& coordinates) {
    HashKey key = convert_to_key(coordinates);
    typename HashMap::iterator itr = m_hash_map->find(key);
    if (itr == m_hash_map->end()) return false;

    std::size_t num_erased = itr->second.erase(obj_id);

    if (itr->second.empty()) {
        m_hash_map->erase(key);
    }
    return num_erased != 0;
}

template<typename Trait>
bool HashGridImplementation<Trait>::occupied(int obj_id, const VectorF& coordinates) const {
    HashKey key = convert_to_key(coordinates);

    typename HashMap::const_iterator itr = m_hash_map->find(key);
    if (itr == m_hash_map->end()) return false;

    typename HashItem::const_iterator i = itr->second.find(obj_id);
    return i!=itr->second.end();
}

template<typename Trait>
VectorI HashGridImplementation<Trait>::get_items_near_point(const VectorF& coordinates) {
    HashItem nearby_set = Trait::get_default_item();
    HashKey center_key = convert_to_key(coordinates);

    const auto surrounding_cells = get_surrounding_cells(center_key.get_raw_data());
    for (const auto& cell_idx : surrounding_cells) {
        HashKey key(cell_idx);
        typename HashMap::const_iterator itr = m_hash_map->find(key);
        if (itr != m_hash_map->end()) {
            nearby_set.insert(itr->second.begin(), itr->second.end());
        }
    }

    VectorI result(nearby_set.size());
    std::copy(nearby_set.begin(), nearby_set.end(), result.data());
    return result;
}

template<typename Trait>
MatrixFr HashGridImplementation<Trait>::get_occupied_cell_centers() const {
    const size_t dim = Trait::dim;
    MatrixFr centers(size(), dim);
    size_t count = 0;
    for (auto itr = m_hash_map->begin(); itr != m_hash_map->end(); itr++) {
        centers.row(count) = convert_to_grid_point(itr->first);
        count++;
    }
    return centers;
}

template<typename Trait>
typename HashGridImplementation<Trait>::HashKey HashGridImplementation<Trait>::convert_to_key(const VectorF& value) const {
    return typename HashKey::VectorType(
            (value / m_cell_size).unaryExpr(
                std::ptr_fun<Float,Float>(std::round)
                ).template cast<int64_t>());
}

template <typename Trait>
VectorF HashGridImplementation<Trait>::convert_to_grid_point(
        const typename HashGridImplementation<Trait>::HashKey& key) const {
    return key.get_raw_data().template cast<Float>() * m_cell_size;
}

template<typename Trait>
bool HashGridImplementation<Trait>::insert_key(int obj_id, HashGridImplementation<Trait>::HashKey& key) {
    typename HashMap::iterator itr = m_hash_map->find(key);
    if (itr == m_hash_map->end()) {
        HashItem item = Trait::get_default_item();
        item.insert(obj_id);
        typename HashMap::value_type data(key, item);
        return m_hash_map->insert(data).second;
    } else {
        return itr->second.insert(obj_id).second;
    }
}

