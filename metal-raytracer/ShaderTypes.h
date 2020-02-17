//
//  ShaderTypes.h
//  metal-raytracer
//
//  Created by ashen on 2020/2/10.
//  Copyright © 2020 ashen. All rights reserved.
//

//
//  Header containing types and enum constants shared between Metal shaders and Swift/ObjC source
//
#ifndef SHADERTYPES_H
#define SHADERTYPES_H

#include "metal_bridge.h"

enum BufferIndex
{
    BufferIndexNode = 0,
    BufferIndexSphere = 1,
    BufferIndexMaterial = 2,
    BufferIndexSceneUniform = 3
};

enum ConstantIndex
{
    ConstantIndexBruteForce = 0,
    ConstantIndexDebugBVHHit = 1,
    ConstantIndexCustomBilinearFilter = 2,
};

enum TextureIndex
{
    TextureIndexSceneTex = 0,
    // 1 is reserved for the channels for the second texture
};

struct SceneUniform
{
    math::packed_float3 cameraPos;
    math::packed_float3 cameraLookAt;
    float focalLength;
    float fovY;
    math::packed_float2 screenSize;
    math::packed_float3 backgroundColor;
    int numSamples;
    int numSpheres;
    int iterNum;
    int iterStart;
    uint seed;
};

#include "tracer.h"
#include "color.h"

#endif /* SHADERTYPES_H */

