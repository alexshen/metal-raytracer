//
//  scene_types.h
//  metal-raytracer
//
//  Created by ashen on 2020/2/10.
//  Copyright © 2020 ashen. All rights reserved.
//

#ifndef SCENE_TYPES_H
#define SCENE_TYPES_H

#include "metal_bridge.h"

enum MaterialType : int
{
    Diffuse,
    Metal,
    Dielectric
};

struct Node
{
    math::packed_float3 min;
    math::packed_float3 max;
    
    int left; // left node index
    int right; // right node index
    int firstObjIndex;
    int numObj;
};

struct Sphere
{
    math::packed_float3 center;
    float radius;
};

struct Material
{
    math::packed_float3 albedo;
    MaterialType type;
    float prop;
};

#endif /* SCENE_TYPES_H */
