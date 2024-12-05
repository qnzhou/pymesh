/* This file is part of PyMesh. Copyright (c) 2015 by Qingnan Zhou */
#pragma once

#include <memory>
#include <limits>

#include <unordered_map>
#include <unordered_set>
#include <Core/EigenTypedef.h>

#include "HashKey.h"

namespace PyMesh {

/**
 * Typedefs for std hash types.
 * HashType: 1 google sparse
 *           2 google dense
 *           else std hash
 */
template<int DIM, int HashType>
struct HashMapTrait {
    static constexpr int dim = DIM;
    typedef VectorHashKey<int64_t, DIM> HashKey;

    struct HashMapFunc {
        int operator()(const HashKey& key) const {
            return key.hash();
        }
    };

    typedef std::unordered_set<int> HashItem;
    typedef std::unordered_map<HashKey, HashItem, HashMapFunc> HashMap;
    typedef std::shared_ptr<HashMap> Ptr;

    static Ptr create_map() { return std::make_shared<HashMap>(); }
    static HashItem get_default_item() { return HashItem(); }
};

}

#ifdef USE_SPARSEHASH
#include <sparsehash/sparse_hash_map>
#include <sparsehash/sparse_hash_set>
#include <sparsehash/dense_hash_map>
#include <sparsehash/dense_hash_set>

namespace PyMesh {

/**
 * Typedefs for google sparse hash.
 */
template<int DIM>
struct HashMapTrait<DIM, 1> {
    static constexpr int dim = DIM;
    typedef VectorHashKey<long, DIM> HashKey;

    struct HashMapFunc {
        int operator()(const HashKey& key) const {
            return key.hash();
        }
    };

    typedef google::sparse_hash_set<int> HashItem;
    typedef google::sparse_hash_map<HashKey, HashItem, HashMapFunc> HashMap;
    typedef std::shared_ptr<HashMap> Ptr;


    static Ptr create_map() {
        Ptr hash_map = std::make_shared<HashMap>();
        hash_map->set_deleted_key(HashKey(get_deleted_val()));
        return hash_map;
    }
    static HashItem get_default_item() {
        HashItem item;
        item.set_deleted_key(get_deleted_val());
        return item;
    }
    static int get_deleted_val() {
        return std::numeric_limits<int>::max();
    }
};

/**
 * Typedefs for google dense hash.
 */
template<int DIM>
struct HashMapTrait<DIM, 2> {
    static constexpr int dim = DIM;
    typedef VectorHashKey<long, DIM> HashKey;

    struct HashMapFunc {
        int operator()(const HashKey& key) const {
            return key.hash();
        }
    };

    typedef google::dense_hash_set<int> HashItem;
    typedef google::dense_hash_map<HashKey, HashItem, HashMapFunc> HashMap;
    typedef std::shared_ptr<HashMap> Ptr;

    static constexpr int DELETED_VAL = std::numeric_limits<int>::max();
    static constexpr int EMPTY_VAL = DELETED_VAL-1;

    static Ptr create_map() {
        Ptr hash_map = std::make_shared<HashMap>();
        hash_map->set_empty_key(HashKey(get_empty_val()));
        hash_map->set_deleted_key(HashKey(get_deleted_val()));
        return hash_map;
    }
    static HashItem get_default_item() {
        HashItem item;
        item.set_empty_key(get_empty_val());
        item.set_deleted_key(get_deleted_val());
        return item;
    }
    static int get_deleted_val() {
        return std::numeric_limits<int>::max();
    }
    static int get_empty_val() {
        return get_deleted_val() - 1;
    }
};

}

#endif //USE_SPARSEHASH
