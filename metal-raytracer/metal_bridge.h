//
//  types.h
//  metal-raytracer
//
//  Created by ashen on 2020/2/12.
//  Copyright © 2020 ashen. All rights reserved.
//

#ifndef METAL_BRIDGE_H
#define METAL_BRIDGE_H

#ifndef __METAL_VERSION__
#  include <glm/glm.hpp>
#  include <cmath>
#  define NS glm
#  define thread
#  define constant const
using uint = unsigned int;

#  include <cassert>
#  define MB_ASSERT(cond) assert(cond)
#else
#  define MB_ASSERT(cond) (void)0
#  define NS metal
#endif

namespace math
{
#ifndef __METAL_VERSION__
using packed_float2 = glm::vec2;
using packed_float3 = glm::vec3;
using float2 = glm::vec2;
using float3 = glm::vec3;
using float4 = glm::vec4;
using uint2 = glm::uvec2;
using uint3 = glm::uvec3;
using uint4 = glm::uvec4;
#else
using packed_float2 = metal::packed_float2;
using packed_float3 = metal::packed_float3;
using float2 = metal::float2;
using float3 = metal::float3;
using float4 = metal::float4;
using uint2 = metal::uint2;
using uint3 = metal::uint3;
using uint4 = metal::uint4;
#endif

template<typename T>
inline T normalize(T v) { return NS::normalize(v); }

template<typename T>
inline T pow(T v, float f) { return NS::pow(v, f); }

template<typename T>
inline T reflect(T in, T normal) { return NS::reflect(in, normal); }

template<typename T>
inline T refract(T in, T normal, float eta) { return NS::refract(in, normal, eta); }

template<typename T>
inline float dot(T a, T b) { return NS::dot(a, b); }

template<typename T>
inline T sqrt(T v) { return NS::sqrt(v); }

template<typename T>
inline T mix(T a, T b, float t) { return NS::mix(a, b, t); }

template<typename T>
inline T max(T a, T b) { return NS::max(a, b); }

template<typename T>
inline T min(T a, T b) { return NS::min(a, b); }

template<typename T>
inline T cross(T a, T b) { return NS::cross(a, b); }

template<typename T>
inline T tan(T a) { return NS::tan(a); }

template<typename T>
inline T saturate(T a)
{
#ifndef __METAL_VERSION__
    return glm::clamp(a, 0.0f, 1.0f);
#else
    return metal::saturate(a);
#endif
}

}

#undef NS

#endif /* METAL_BRIDGE_H */
