#ifndef OBJECT_H
#define OBJECT_H

#include "aabb.h"

class object
{
public:
    virtual ~object() = default;
    // return the aabb in xz plane
    virtual aabb3 get_aabb() const = 0;
};


#endif /* OBJECT_H */
