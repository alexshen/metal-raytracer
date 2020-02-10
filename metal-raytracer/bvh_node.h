#ifndef BVH_NODE_H
#define BVH_NODE_H

#include "aabb.h"

#include <memory>
#include <vector>
#include <utility>

class object;

class bvh_node
{
public:
    bvh_node(object** objs, int n);

    const aabb3& get_aabb() const { return m_volume; }
    bool is_leaf() const { return !m_left; }

    const bvh_node* left() const { return m_left.get(); }
    const bvh_node* right() const { return m_right.get(); }

    int num_objects() const { return (int)m_objects.size(); }
    object* get_object(int i) const
    {
        assert(0 <= i && i < m_objects.size());
        return m_objects[i];
    }
private:
    static constexpr int max_objects = 2;
    static constexpr int bin_num = 1024;

    std::unique_ptr<bvh_node> m_left;
    std::unique_ptr<bvh_node> m_right;
    std::vector<object*> m_objects;

    aabb3 m_volume;
};

template<typename F>
void preorder_visit(const bvh_node* root, F&& f)
{
    f(root);
    if (!root) {
        return;
    }
    preorder_visit(root->left(), std::forward<F>(f));
    preorder_visit(root->right(), std::forward<F>(f));
}

#endif // BVH_NODE_H
