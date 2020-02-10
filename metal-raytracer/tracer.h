//
//  tracer.h
//  metal-raytracer
//
//  Created by ashen on 2020/2/12.
//  Copyright © 2020 ashen. All rights reserved.
//

#ifndef TRACER_H
#define TRACER_H

#include "metal_bridge.h"
#include "scene_types.h"

namespace tracer
{

struct AABB
{
    math::packed_float3 min;
    math::packed_float3 max;
};

struct HitRecord
{
    math::float3 pt;
    math::float3 normal;
    Material material;
};

class Random
{
public:
    Random(uint seed);
    float next();
    math::float2 inUnitRect();
    math::float3 inUnitSphere();
private:
    uint m_seed;
};

struct Ray
{
    math::float3 origin;
    math::float3 dir;
};

bool intersect(Ray r, AABB volume, float tmin, float tmax);
float intersectSphere(Sphere sphere, Ray ray, float tmin, float tmax);

class Camera
{
public:
    Camera(math::float3 pos, math::float3 lookAt, math::float3 up,
           float fovY, float focalLength, math::float2 screenSize);
    Ray getRay(math::float2 samplePos) const;
    math::float3 getRayDir(math::float2 samplePos) const;
private:
    math::float3 m_pos;
    math::float3 m_lookDir;
    math::float3 m_right;
    math::float3 m_up;
    float m_focalLength;
    math::float2 m_halfImageSize;
    math::float2 m_screenSize;
};

constant constexpr int MaxHits = 128;

class Scene
{
public:
    Scene(constant Node* nodes, constant Sphere* spheres, constant Material* materials, int numSpheres);

    template<bool bruteForce>
    bool hit(Ray ray, float tmin, float tmax, thread HitRecord& rec) const
    {
        int sphereIndex = -1;
        float minT = INFINITY;
        if (bruteForce) {
            for (int i = 0; i < m_numSpheres; ++i) {
                float t = intersectSphere(getSphere(i), ray, tmin, tmax);
                if (t != -1 && minT > t) {
                    minT = t;
                    sphereIndex = i;
                }
            }
        } else {
            int hitNodes[MaxHits];
            int numHitNodes = findPossibleHits(ray, tmin, tmax, hitNodes);
            for (int i = 0; i < numHitNodes; ++i) {
                constant Node& node = getNode(hitNodes[i]);
                for (int j = 0; j < node.numObj; ++j) {
                    float t = intersectSphere(getSphere(node.firstObjIndex + j), ray, tmin, tmax);
                    if (t != -1 && minT > t) {
                        minT = t;
                        sphereIndex = node.firstObjIndex + j;
                    }
                }
            }
        }
        if (sphereIndex != -1) {
            rec.pt = ray.origin + minT * ray.dir;
            rec.normal = math::normalize(rec.pt - getSphere(sphereIndex).center);
            rec.material = getMaterial(sphereIndex);
            return true;
        }
        return false;
    }
    
    int findPossibleHits(Ray ray, float tmin, float tmax, thread int hitNodes[MaxHits]) const;

    int numSpheres() const { return m_numSpheres; }
    constant Sphere& getSphere(int i) const { return m_spheres[i]; }
    constant Node& getNode(int i) const { return m_nodes[i]; }
    constant Material& getMaterial(int i) const { return m_materials[i]; }
private:
    constant Node* m_nodes;
    constant Sphere* m_spheres;
    constant Material* m_materials;
    int m_numSpheres;
};

inline float schlick(float cosine, float n)
{
    float r0 = (1 - n) / (1 + n);
    r0 *= r0;
    return r0 + (1.0f - r0) * math::pow(1.0f - cosine, 5);
}

class RayTracer
{
public:
    RayTracer(thread Random& random, thread const Camera& camera, thread const Scene& scene, math::float3 bgColor);

    template<bool bruteForce>
    math::float3 trace(math::float2 samplePos) const
    {
        const int MaxIter = 50;
        
        math::float3 color = math::float3(1);
        HitRecord rec;
        Ray ray = m_camera.getRay(samplePos);
        for (int i = 0; i < MaxIter; ++i) {
            if (m_scene.hit<bruteForce>(ray, 0.0001f, INFINITY, rec)) {
                math::float3 scatteredDir;
                math::float3 attenuation;
                bool scattered = false;
                switch (rec.material.type) {
                case MaterialType::Diffuse:
                    scattered = diffuseScatter(ray.dir, rec, attenuation, scatteredDir);
                    break;
                    
                case MaterialType::Metal:
                    scattered = metalScatter(ray.dir, rec, attenuation, scatteredDir);
                    break;
                    
                case MaterialType::Dielectric:
                    scattered = dielectricScatter(ray.dir, rec, attenuation, scatteredDir);
                    break;
                }
                if (scattered) {
                    ray.origin = rec.pt;
                    ray.dir = scatteredDir;
                    color *= attenuation;
                } else {
                    color = math::float3(0);
                    break;
                }
            } else {
                break;
            }
        }
        
        color *= getBackgroundColor(ray.dir);
        return color;
    }
private:
    bool diffuseScatter(math::float3 rayDir, thread const HitRecord& rec,
                        thread math::float3& attenuation, thread math::float3& scattered) const;

    bool metalScatter(math::float3 rayDir, thread const HitRecord& rec,
                      thread math::float3& attenuation, thread math::float3& scattered) const;

    bool dielectricScatter(math::float3 rayDir, thread const HitRecord& rec,
                           thread math::float3& attenuation, thread math::float3& scattered) const;

    math::float3 getBackgroundColor(math::float3 dir) const;

    thread Random& m_random;
    thread const Camera& m_camera;
    thread const Scene& m_scene;
    math::float3 m_bgColor;
};

inline math::float3 debugTrace(thread const Scene& scene, thread const Camera& camera,
                               math::float2 samplePos)
{
    int hits[MaxHits];
    int numNodes = scene.findPossibleHits(camera.getRay(samplePos), 0, INFINITY, hits);
    int numHitSpheres = 0;
    for (int i = 0; i < numNodes; ++i) {
        numHitSpheres += scene.getNode(hits[i]).numObj;
    }
    return math::float3(float(numHitSpheres) / 32.0f);
}

}

#endif /* TRACER_H */
