#include "bvh_node.h"
#include "object.h"
#include "utils.h"

#include <iterator>
#include <algorithm>
#include <limits>
#include <numeric>

using namespace std;

bvh_node::bvh_node(object** objs, int n)
{
    // find the aabb of the current node
    m_volume = accumulate(objs, objs + n, aabb3::empty(),
                          [](const auto& a, auto b) { return a.expand(b->get_aabb()); });

    if (n <= max_objects) {
        m_objects.assign(objs, objs + n);
    } else {
        // use SAH to build the BVH
        float min_cost = numeric_limits<float>::max();
        vector<object*> permutation(n);
        int left_obj_num = 0;

        vector<float> left_areas(n);
        vector<float> right_areas(n);

        // for each axis
        for (int i = 0; i < 3; ++i) {
            sort(objs, objs + n, [=](auto const& lhs, auto rhs) {
                    return lhs->get_aabb().center()[i] < rhs->get_aabb().center()[i];
                 });

            // calculate the area for all possible partitions
            auto bb = aabb3::empty();
            transform(objs, objs + n, left_areas.begin(), [&](auto p) {
                        bb.expand(p->get_aabb());
                        return bb.area();
                      });

            bb = aabb3::empty();
            transform(make_reverse_iterator(objs + n), make_reverse_iterator(objs),
                      right_areas.begin(), [&](auto p) {
                        bb.expand(p->get_aabb());
                        return bb.area();
                      });

            bool need_update = false;
            auto start = objs;
            float step = (m_volume.max[i] - m_volume.min[i]) / bin_num;
            for (int j = 1; j < bin_num && start != objs + n; ++j) {
                auto it = find_if(start, objs + n, [=](auto p) {
                                        return p->get_aabb().center()[i] >= j * step;
                                    });
                float cost = 0.0f;
                auto left_num = it - objs;
                if (left_num > 0) {
                    cost += left_areas[left_num - 1] * left_num;
                }
                auto right_num = n - left_num;
                if (right_num > 0) {
                    cost += right_areas[right_num - 1] * right_num;
                }
                if (cost < min_cost) {
                    min_cost = cost;
                    left_obj_num = (int)left_num;
                    need_update = true;
                }

                start = it;
            }

            if (need_update) {
                copy(objs, objs + n, permutation.begin());
            }
        }

        if (left_obj_num > 0 && left_obj_num < n) {
            m_left = make_unique<bvh_node>(permutation.data(), left_obj_num);
            m_right = make_unique<bvh_node>(permutation.data() + left_obj_num, n - left_obj_num);
        } else {
			// fall back to random axis partition
			auto axis = (int)(2.0f * utils::random() + 1);
			sort(objs, objs + n, [axis](auto lhs, auto rhs) {
				return lhs->get_aabb().center()[axis] < rhs->get_aabb().center()[axis];
			});
			m_left = make_unique<bvh_node>(objs, n / 2);
			m_right = make_unique<bvh_node>(objs + n / 2, n - n / 2);
        }
    }
}
