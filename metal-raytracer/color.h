//
//  color.h
//  metal-raytracer
//
//  Created by ashen on 2020/2/12.
//  Copyright © 2020 ashen. All rights reserved.
//

#ifndef COLOR_H
#define COLOR_H

#include "metal_bridge.h"

struct BGR10
{
    static constexpr constant uint ChannelBits = 10;
    static constexpr constant uint ChannelMax = (1 << ChannelBits) - 1;
    
    static math::float3 decode(uint color)
    {
        math::float3 res;
        res.b = color & ChannelMax;
        
        color >>= ChannelBits;
        res.g = color & ChannelMax;
        
        color >>= ChannelBits;
        res.r = color & ChannelMax;
        
        return res / (float)ChannelMax;
    }
    
    static uint encode(math::float3 color)
    {
        math::uint3 tmp = math::uint3(math::saturate(color) * (float)ChannelMax);
        uint res = tmp.r;
        res <<= ChannelBits;
        res |= tmp.g;
        res <<= ChannelBits;
        res |= tmp.b;
        return res;
    }
};

struct BGRA16
{
    static constant constexpr uint ChannelBits = 16;
    static constant constexpr uint ChannelMask = (1 << ChannelBits) - 1;
    
    // little endian
    // color[0] = bg
    // color[1] = ra
    static math::float4 decode(math::uint2 color)
    {
        math::float4 res;
        res.b = (color[0] & ChannelMask) / (float)ChannelMask;
        res.g = ((color[0] >> ChannelBits) & ChannelMask) / (float)ChannelMask;
        res.r = (color[1] & ChannelMask) / (float)ChannelMask;
        res.a = ((color[1] >> ChannelBits) & ChannelMask) / (float)ChannelMask;
        return res;
    }
    
    static math::uint2 encode(math::float4 color)
    {
        math::uint4 tmp = math::uint4(math::saturate(color) * float(ChannelMask));
        math::uint2 res;
        res[0] = (tmp.g << ChannelBits) | tmp.b;
        res[1] = (tmp.a << ChannelBits) | tmp.r;
        return res;
    }
};

#endif /* COLOR_H */
