//
//  tracer.cpp
//  metal-raytracer
//
//  Created by ashen on 2020/2/12.
//  Copyright © 2020 ashen. All rights reserved.
//

#include "tracer.h"

namespace tracer
{

Random::Random(uint seed)
    : m_seed(seed)
{}

float Random::next()
{
    // https://software.intel.com/en-us/articles/fast-random-number-generator-on-the-intel-pentiumr-4-processor
    m_seed = 214013*m_seed+2531011;
    return ((m_seed>>16)&0x7FFF) / float(0x8000);
}

math::float2 Random::inUnitRect()
{
    return math::float2(next(), next());
}

math::float3 Random::inUnitSphere()
{
    math::float3 res;
    do {
        res = math::float3(next(), next(), next());
        res = res * 2.0f - 1.0f;
    } while (math::dot(res, res) >= 1.0f);
    return res;
}

bool intersect(Ray r, AABB volume, float tmin, float tmax)
{
    for (int i = 0; i < 3; ++i) {
        float t0 = (volume.min[i] - r.origin[i]) / r.dir[i];
        float t1 = (volume.max[i] - r.origin[i]) / r.dir[i];
        if (r.dir[i] < 0) {
            float tmp = t0;
            t0 = t1;
            t1 = tmp;
        }
        tmin = math::max(tmin, t0);
        tmax = math::min(tmax, t1);
    }
    return tmin < tmax;
}

float intersectSphere(Sphere sphere, Ray ray, float tmin, float tmax)
{
    math::float3 oc = ray.origin - sphere.center;
    float a = math::dot(ray.dir, ray.dir);
    float b = math::dot(oc, ray.dir);
    float c = math::dot(oc, oc) - sphere.radius * sphere.radius;
    float discriminant = b*b - a*c;
    if (discriminant < 0) {
        return -1;
    }
    float sqrtDisr = math::sqrt(discriminant);
    float t = (-b - sqrtDisr) / a;
    if (tmin <= t && t <= tmax) {
        return t;
    }
    t = (-b + sqrtDisr) / a;
    if (tmin <= t && t <= tmax) {
        return t;
    }
    return -1;
}

Camera::Camera(math::float3 pos, math::float3 lookAt, math::float3 up,
               float fovY, float focalLength, math::float2 screenSize)
    : m_pos(pos)
    , m_focalLength(focalLength)
    , m_screenSize(screenSize)
{
    m_lookDir = math::normalize(lookAt - pos);
    m_right = math::normalize(math::cross(m_lookDir, up));
    m_up = math::cross(m_right, m_lookDir);
    m_halfImageSize.y = math::tan(fovY / 2.0f) * focalLength;
    m_halfImageSize.x = m_halfImageSize.y * screenSize.x / screenSize.y;
}

Ray Camera::getRay(math::float2 samplePos) const
{
    return { m_pos, getRayDir(samplePos) };
}

math::float3 Camera::getRayDir(math::float2 samplePos) const
{
    math::float2 p = samplePos / m_screenSize;
    p = p * 2.0f - 1.0f;
    p.y = -p.y;
    math::float3 dir = m_lookDir * m_focalLength +
                        p.x * m_right * m_halfImageSize.x +
                        p.y * m_up * m_halfImageSize.y;
    return math::normalize(dir);
}

Scene::Scene(constant Node* nodes, constant Sphere* spheres, constant Material* materials, int numSpheres)
    : m_nodes(nodes)
    , m_spheres(spheres)
    , m_materials(materials)
    , m_numSpheres(numSpheres)
{ }

int Scene::findPossibleHits(Ray ray, float tmin, float tmax, thread int hitNodes[MaxHits]) const
{
    constexpr int MaxStackSize = 64;
    int num = 0;
    int stack[MaxStackSize];
    stack[0] = 0;
    int i = 1;
    
    while (i > 0) {
        --i;
        constant Node& node = m_nodes[stack[i]];
        if (intersect(ray, { node.min, node.max }, tmin, tmax)) {
            // leaf node
            if (node.left == -1) {
                MB_ASSERT(num < MaxHits);
                hitNodes[num++] = stack[i];
            } else {
                MB_ASSERT(i + 1 < MaxStackSize);
                stack[i++] = node.right;
                stack[i++] = node.left;
            }
        }
    }
    return num;
}

RayTracer::RayTracer(thread Random& random, thread const Camera& camera, thread const Scene& scene, math::float3 bgColor)
    : m_random(random)
    , m_camera(camera)
    , m_scene(scene)
    , m_bgColor(bgColor)
{}

bool RayTracer::diffuseScatter(math::float3 rayDir, thread const HitRecord& rec,
                               thread math::float3& attenuation, thread math::float3& scattered) const
{
    scattered = math::normalize(rec.normal + m_random.inUnitSphere());
    attenuation = rec.material.albedo;
    return true;
}

bool RayTracer::metalScatter(math::float3 rayDir, thread const HitRecord& rec,
                             thread math::float3& attenuation, thread math::float3& scattered) const
{
    scattered = rec.material.prop * m_random.inUnitSphere() + math::reflect(rayDir, rec.normal);
    scattered = math::normalize(scattered);
    attenuation = rec.material.albedo;
    return math::dot(rec.normal, scattered) > 0;
}

bool RayTracer::dielectricScatter(math::float3 rayDir, thread const HitRecord& rec,
                                  thread math::float3& attenuation, thread math::float3& scattered) const
{
    math::float3 uin = math::normalize(rayDir);
    attenuation = math::float3(1);
    
    float index = rec.material.prop;
    float ni_over_nt;
    float cosine = math::dot(uin, rec.normal);
    math::float3 normal;
    if (cosine > 0) {
        ni_over_nt = index;
        normal = -rec.normal;
    } else {
        ni_over_nt = 1.0f / index;
        normal = rec.normal;
    }
    
    float reflect_prob;
    math::float3 refracted = math::refract(uin, normal, ni_over_nt);
    // if > 0, then we assume that light travels from denser material to air
    if (cosine > 0) {
        cosine = math::sqrt(1.0f - index * index * (1.0f - cosine * cosine));
    } else {
        cosine = -cosine;
    }
    reflect_prob = schlick(cosine, index);
    
    if (m_random.next() < reflect_prob) {
        scattered = math::reflect(uin, normal);
    } else {
        scattered = refracted;
    }
    return true;
}

math::float3 RayTracer::getBackgroundColor(math::float3 dir) const
{
    float t = (dir.y + 1.0) * 0.5;
    return math::mix(math::float3(1), m_bgColor, t);
}

}
