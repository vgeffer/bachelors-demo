#pragma once
#include "../utils/progress_bar.hpp"
#include "../utils/progress_counter.hpp"
#include "../lib/glpix.hpp"
#include "include/rtutils.h"
#include <CL/cl_platform.h>
#include <cmath>
#include <openvdb/Types.h>
#include <ostream>
#include <queue>
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
                    iter = leaf.beginValueOn();
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
                                .rsquared = SPHERE_RADIUS_SQR, .cbegin = -1, .cend = -1,
                                .count = 1, .alpha = *iter
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
            int child_count = m_tree.size();

            /* Measure time */
            std::chrono::system_clock::time_point tp_start = std::chrono::system_clock::now(), 
                                                  tp_end;

            m_tree.emplace(m_tree.begin()); /* Create empty space at the beginning for the root node */
            m_bvh_build_counter = progress_counter(" nodes built", "Building BVH: ");
            
            sphere root = (sphere) { .cbegin = 1, .cend = child_count + 1 };
            fit(root);
            subdivide(root);

            m_tree[0] = root;
            m_bvh_build_counter.end_work();            
        
            tp_end = std::chrono::system_clock::now();
            std::cout << "Building BVH took: " << std::chrono::duration_cast<std::chrono::milliseconds>(tp_end - tp_start).count() / 1000.0f << "s" << std::endl;

            /* Debug print tree */ 
            int current_depth = 0, count = 0, leafs = 0;
            float node_ccount = 0;
            std::queue<std::pair<int, int>> queue;
            queue.emplace(0, 0);


            while (!queue.empty()) {
                auto [node, depth] = queue.front();
                queue.pop();

                if (current_depth != depth) {
                    std::cerr << "At depth: " << current_depth << " - nodes: " << count - leafs << "(" << node_ccount / (count - leafs) << ")" << ", leafs: " << leafs << std::endl;
                    current_depth = depth;
                    count = leafs = 0;
                    node_ccount = 0;
                }
                count++;
                    
                if (m_tree[node].cbegin < 0) leafs++;
                else node_ccount += m_tree[node].cend - m_tree[node].cbegin;
                
                for (int i = m_tree[node].cbegin; i < m_tree[node].cend; i++) {
                    queue.emplace(i, depth + 1);
                }
            }
        }

        bool is_valid() const noexcept { return m_tree.size() > 0; } 
        const std::vector<sphere>& tree() const noexcept { return  m_tree; };

        void refit();

    private:
        void subdivide(sphere& bb);
        void fit(sphere& bb);

        cl_float3 size_along_axies(const sphere& bb);

        progress_counter m_bvh_build_counter;
        std::vector<sphere> m_tree;
};


