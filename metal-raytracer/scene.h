#ifndef SCENE_H
#define SCENE_H

#pragma once

#include "bvh_node.h"
#include "sphere_object.h"
#include "scene_types.h"
#include <vector>

struct Scene
{
    std::vector<SphereObject> objects;
    std::unique_ptr<bvh_node> root;
};

struct SceneBuffer
{
    SceneBuffer(const Scene& scene);

    std::vector<Node> nodes;
    std::vector<Sphere> objects;
    std::vector<Material> materials;
};

Scene createScene();

#endif // SCENE_H
