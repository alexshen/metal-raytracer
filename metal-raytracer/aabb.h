#ifndef AABB_H
#define AABB_H

#include <glm/glm.hpp>
#include <algorithm>
#include <cfloat>

template<typename T>
struct basic_aabb
{
    basic_aabb() = default;
    basic_aabb(const T& min, const T& max)
        : min(min)
        , max(max)
    {
    }
    
    basic_aabb(const T& center, float radius)
        : min(center - T(1.0) * radius)
        , max(center + T(1.0) * radius)
    {
    }
    
    T center() const { return (min + max) * 0.5f; }
    T extent() const { return (max - min) * 0.5f; }

    float area() const
    {
        auto size = max - min;
        float total = 1.0f;
        for (int i = 0; i < T::length(); ++i) {
            total *= size[i];
        }
        return total;
    }

    bool overlap(const basic_aabb& rhs) const
    {
        for (int i = 0; i < T::length(); ++i) {
            if (!overlap(rhs, i)) {
                return false;
            }
        }
        return true;
    }

    bool overlap(const basic_aabb& rhs, int axis) const
    {
        float s = std::max(min[axis], rhs.min[axis]);
        float e = std::min(max[axis], rhs.max[axis]);
        return s < e;
    }

    basic_aabb& expand(const basic_aabb& rhs)
    {
        for (int i = 0; i < T::length(); ++i) {
            min[i] = std::min(min[i], rhs.min[i]);
            max[i] = std::max(max[i], rhs.max[i]);
        }
        return *this;
    }

    basic_aabb expand(basic_aabb rhs) const
    {
        rhs.expand(*this);
        return rhs;
    }

    basic_aabb& translate(const T& offset)
    {
        min += offset;
        max += offset;

        return *this;
    }

    // the empty basic_aabb for iterative expansion
    static basic_aabb empty()
    {
        return { T(1.0) * FLT_MAX, T(1.0) * -FLT_MAX };
    }

    T min;
    T max;
};

using aabb2 = basic_aabb<glm::vec2>;
using aabb3 = basic_aabb<glm::vec3>;

#endif /* AABB_H */
