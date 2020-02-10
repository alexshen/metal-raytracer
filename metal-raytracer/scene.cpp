#include "scene.h"
#include "utils.h"

#include <glm/glm.hpp>
#include <cassert>

namespace 
{

int flatten(const bvh_node* root, SceneBuffer& buffer)
{
    if (!root) {
        return -1;
    }

    int index = (int)buffer.nodes.size();
    buffer.nodes.emplace_back();
    Node curNode;
    curNode.min = root->get_aabb().min;
    curNode.max = root->get_aabb().max;
    curNode.firstObjIndex = (int)buffer.objects.size();
    curNode.numObj = root->num_objects();

    for (int i = 0; i < root->num_objects(); ++i) {
        auto obj = static_cast<const SphereObject*>(root->get_object(i));
        auto& target = buffer.objects.emplace_back();
        target.center = obj->center;
        target.radius = obj->radius;

        auto& mat = buffer.materials.emplace_back();
        mat.albedo = obj->albedo;
        mat.type = obj->type;
        mat.prop = obj->prop;
    }

    curNode.left = flatten(root->left(), buffer);
    curNode.right = flatten(root->right(), buffer);
    buffer.nodes[index] = curNode;

    return index;
}

} // anonymous namespace

Scene createScene()
{
    Scene scene;
    scene.objects.emplace_back( glm::vec3(0, -1000, 0), 1000.0f, Diffuse, glm::vec3(1) * 0.5f );

    const int x_count = 11, y_count = 11;
    for (int a = -x_count; a < x_count; ++a) {
        for (int b = -y_count; b < y_count; ++b) {
            float choose_mat = utils::random();
            glm::vec3 center(a + 0.9f * utils::random(), 0.2f, b + 0.9f * utils::random());
            if ((center - glm::vec3(4, 0.2f, 0)).length() > 0.9f) {
                SphereObject obj;
                obj.center = center;
                obj.radius = 0.2f;
                if (choose_mat < 0.8f) {
                    obj.albedo = glm::vec3(utils::random() * utils::random(),
                                           utils::random() * utils::random(),
                                           utils::random() * utils::random());
                    obj.type = Diffuse;
                } else if (choose_mat < 0.95f) {
                    obj.albedo = glm::vec3(0.5f * (1 + utils::random()),
                                           0.5f * (1 + utils::random()),
                                           0.5f * (1 + utils::random()));
                    obj.prop = 0.5f * utils::random();
                    obj.type = Metal;
                } else {
                    obj.type = Dielectric;
                    obj.prop = 1.5f;
                }
                scene.objects.push_back(obj);
            }
        }
    }

    scene.objects.emplace_back( glm::vec3(0, 1, 0), 1.0f, Dielectric, glm::vec3(0), 1.5f );
    scene.objects.emplace_back( glm::vec3(-4, 1, 0), 1.0f, Diffuse, glm::vec3(0.4, 0.2, 0.1) );
    scene.objects.emplace_back( glm::vec3(4, 1, 0), 1.0f, Metal, glm::vec3(0.7, 0.6, 0.5) );

    std::vector<object*> objects;
    for (auto& o : scene.objects) {
        objects.push_back(&o);
    }
    scene.root = std::make_unique<bvh_node>(objects.data(), objects.size());
    return scene;
}

SceneBuffer::SceneBuffer(const Scene& scene)
{
    flatten(scene.root.get(), *this);
}
