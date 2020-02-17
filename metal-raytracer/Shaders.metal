//
//  Shaders.metal
//  metal-raytracer
//
//  Created by ashen on 2020/2/10.
//  Copyright © 2020 ashen. All rights reserved.
//

// File for Metal kernel and shader functions

#include <metal_stdlib>
#include <simd/simd.h>

// Including header shared between this Metal shader code and Swift/C code executing Metal API commands
#include "ShaderTypes.h"
#include "tracer.cpp"

using namespace metal;

constant bool g_bruteForce [[function_constant(ConstantIndexBruteForce)]];
constant bool g_debugBVHHit [[function_constant(ConstantIndexDebugBVHHit)]];

template<access acc>
struct SceneTexture
{
    texture2d<uint, acc> bgTex [[texture(TextureIndexSceneTex)]];
    texture2d<uint, acc> raTex [[texture(TextureIndexSceneTex + 1)]];
    
    float4 read(uint2 pos)
    {
        pos = clamp(pos, uint2(0), size() - 1);
        return BGRA16::decode(uint2(bgTex.read(pos).r, raTex.read(pos).r));
    }
    
    void write(float4 color, uint2 pos)
    {
        uint2 bgra = BGRA16::encode(color);
        bgTex.write(bgra[0], pos);
        raTex.write(bgra[1], pos);
    }
    
    float4 sample(sampler s, float2 uv)
    {
        return BGRA16::decode(uint2(bgTex.sample(s, uv).r, raTex.sample(s, uv).r));
    }
    
    uint2 size() const
    {
        return uint2(bgTex.get_width(), bgTex.get_height());
    }
};



kernel void raytrace(constant Node* nodes [[buffer(BufferIndexNode)]],
                     constant Sphere* spheres [[buffer(BufferIndexSphere)]],
                     constant Material* materials [[buffer(BufferIndexMaterial)]],
                     constant SceneUniform& sceneUniform [[buffer(BufferIndexSceneUniform)]],
                     SceneTexture<access::read_write> tex,
                     uint2 threadPos [[thread_position_in_grid]])
{
    tracer::Random random(sceneUniform.seed);
    tracer::Scene scene(nodes, spheres, materials, sceneUniform.numSpheres);
    tracer::Camera camera(sceneUniform.cameraPos, sceneUniform.cameraLookAt, float3(0, 1, 0),
                          sceneUniform.fovY, sceneUniform.focalLength,
                          sceneUniform.screenSize);
    tracer::RayTracer tracer(random, camera, scene, sceneUniform.backgroundColor);
    float3 color = float3(0);
    int iterEnd = min(sceneUniform.numSamples, sceneUniform.iterStart + sceneUniform.iterNum);
    for (int i = sceneUniform.iterStart; i < iterEnd; ++i) {
        float2 samplePos = float2(threadPos) + random.inUnitRect();
        if (g_debugBVHHit) {
            color += debugTrace(scene, camera, samplePos);
        } else if (g_bruteForce) {
            color += tracer.trace<true>(samplePos);
        } else {
            color += tracer.trace<false>(samplePos);
        }
    }

    color /= sceneUniform.numSamples;
    tex.write(tex.read(threadPos) + float4(color, 0.0f), threadPos);
}

struct QuadVertexOutput
{
    float4 position [[ position ]];
    float2 uv;
};

vertex QuadVertexOutput quadVertex(uint vertexId [[vertex_id]])
{
    constexpr float2 vertices[] = {
        { -1,  1 },
        { -1, -1 },
        {  1,  1 },
        {  1, -1 }
    };
    QuadVertexOutput output;
    output.position = float4(vertices[vertexId], 0, 1);
    output.uv = output.position.xy * 0.5 + 0.5;
    output.uv.y = 1.0 - output.uv.y;
    return output;
}

fragment float4 texturedQuadCustomFilterFrag(QuadVertexOutput in [[stage_in]], SceneTexture<access::read> tex)
{
    float2 fragCoord = in.uv * float2(tex.size()) - 0.5;
    float4 c00 = tex.read(uint2(fragCoord));
    float4 c01 = tex.read(uint2(fragCoord) + uint2(0, 1));
    float4 c10 = tex.read(uint2(fragCoord) + uint2(1, 0));
    float4 c11 = tex.read(uint2(fragCoord) + uint2(1, 1));
    float2 uv = fract(fragCoord.xy);
    
    return mix(mix(c00, c10, uv.x), mix(c01, c11, uv.x), uv.y);
}
    
fragment float4 texturedQuadFrag(QuadVertexOutput in [[stage_in]], SceneTexture<access::sample> tex)
{
    constexpr sampler sceneTexSampler(coord::normalized, filter::linear, address::clamp_to_edge);
    return tex.sample(sceneTexSampler, in.uv);
}
