#pragma once
#include "../utils/progress_bar.hpp"
#include "../lib/glpix.hpp"
#include "include/rtutils.h"
#include <cmath>
#include <ostream>
#include <stdexcept>
#include <vector>

#if defined(PLATFORM_UNIX)
    #include <openvdb/openvdb.h>
#elif defined(PLATFORM_MACOS)
    #include <OpenVDB/openvdb.h>
    #include <OpenVDB/tools/NodeVisitor.h>
#endif

#define SPHERE_RADIUS_SQR 0.75 

inline float randf() { return (static_cast<float>(rand() % 65536) / 65536.0f); }

class splat_model {

    private:
        struct vdb_leaf_generator_op {
            public: 
                vdb_leaf_generator_op(std::vector<sphere>& leafs, progress_bar bar = progress_bar(), float epsilon = -INFINITY) 
                    : m_bar(bar), m_leafs(leafs), m_epsilon(epsilon) {}
                vdb_leaf_generator_op(const vdb_leaf_generator_op& other) 
                    : m_bar(other.m_bar), m_leafs(other.m_leafs), m_epsilon(other.m_epsilon) {}
                vdb_leaf_generator_op(const vdb_leaf_generator_op& other, tbb::split) 
                    : m_bar(other.m_bar), m_leafs(other.m_leafs), m_epsilon(other.m_epsilon) {}

                template <typename LeafNodeType>
                void operator()(LeafNodeType &leaf, size_t) {
        
                    typename LeafNodeType::ValueOnIter iter = leaf.beginValueOn();
                    for (; iter; ++iter) {
                        m_bar.increment_work();

                        if (*iter < m_epsilon)
                            continue; 
                        
                        /* Assume uniform size of r = sqrt(3)/2 */
                        auto coords = iter.getCoord();
                        m_leafs.emplace_back(
                            (sphere) {
                                .center = (cl_float4) { 
                                    .x = static_cast<cl_float>(coords.x()), .y = static_cast<cl_float>(coords.y()), 
                                    .z = static_cast<cl_float>(coords.z()), .w = 1.0f 
                                }, 
                                .rsquared = SPHERE_RADIUS_SQR*2, .children = {-1, -1, -1, -1},
                                .color = (cl_float4) { .x = randf(), .y = randf(), .z = randf(), .w = *iter }
                            });
                    }
                }

            void join(const vdb_leaf_generator_op &other) { throw new std::logic_error("Parallel building not yet implemented"); }

        private:
            progress_bar m_bar;
            std::vector<sphere>& m_leafs;
            const float m_epsilon = -INFINITY;
            int counter = 0;
    };

    public:
        splat_model() {}
        template<typename TreeType> splat_model(TreeType& vdb_tree) {

            openvdb::tree::LeafManager<TreeType> lm = openvdb::tree::LeafManager<TreeType>(vdb_tree); 
            m_tree.reserve(lm.activeLeafVoxelCount() / 2);

            progress_bar alpha_bar = progress_bar(lm.activeLeafVoxelCount(), "Generating leafs from VDB:");
            vdb_leaf_generator_op prune_op(m_tree, alpha_bar, RT_EPSILON);
            lm.reduce(prune_op, false);

            /* Build the rest of the tree */
            std::cout << "Built " << m_tree.size() << " leafs from " << lm.activeLeafVoxelCount() << " voxels" << std::endl;
        }

        bool is_valid() const noexcept { return m_tree.size() > 0; } 
        const std::vector<sphere>& tree() const noexcept { return  m_tree; };

        void refit();

    private:

        void fit(sphere& bb);

        std::vector<sphere> m_tree;
};


