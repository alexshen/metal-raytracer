//
//  RGBA16Image.m
//  metal-raytracer
//
//  Created by ashen on 2020/2/12.
//  Copyright © 2020 ashen. All rights reserved.
//

#import "RGBA16Image.h"
#include "color.h"
#include <cstddef>

@interface RGBA16Image ()
@property (nonatomic) NSUInteger width;
@property (nonatomic) NSUInteger height;
@end

@implementation RGBA16Image
{
    id<MTLTexture> _colorMaps[2];
    std::uint32_t* _colorMapPixels[2];
}

- (nullable instancetype)initWith:(id<MTLDevice>)device width:(NSUInteger)width height:(NSUInteger)height
{
    if (self = [super init]) {
        _width = width;
        _height = height;
        
        MTLTextureDescriptor* sceneTexDesc = [MTLTextureDescriptor
                                              texture2DDescriptorWithPixelFormat:MTLPixelFormatR32Uint
                                              width:width height:height
                                              mipmapped:NO];
        sceneTexDesc.usage = MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite;
        for (int i = 0; i < 2; ++i) {
            _colorMaps[i] = [device newTextureWithDescriptor:sceneTexDesc];
            _colorMapPixels[i] = new uint32_t[width * height]{};
        }
        [self update];
    }
    return self;
}

- (void)dealloc
{
    for (int i = 0; i < 2; ++i) {
        delete[] _colorMapPixels[i];
    }
}

- (void)update
{
    for (int i = 0; i < 2; ++i) {
        [_colorMaps[i] replaceRegion:MTLRegionMake2D(0, 0, self.width, self.height)
                         mipmapLevel:0
                           withBytes:_colorMapPixels[i]
                         bytesPerRow:self.width * sizeof(std::uint32_t)];
    }
}

- (glm::vec4)colorAt:(glm::uvec2)pos
{
    auto index = pos.x + pos.y * self.width;
    NSAssert(0 <= index && index < self.width * self.height, @"pos out of bounds");
    return BGRA16::decode(glm::uvec2(_colorMapPixels[0][index],
                                     _colorMapPixels[1][index]));
}

- (void)setColor:(glm::vec4)color at:(glm::uvec2)pos
{
    auto index = pos.x + pos.y * self.width;
    NSAssert(0 <= index && index < self.width * self.height, @"pos out of bounds");
    auto encodedColor = BGRA16::encode(color);
    for (int i = 0; i < 2; ++i) {
        _colorMapPixels[i][index] = encodedColor[i];
    }
}

- (void)setTextureFor:(id<MTLComputeCommandEncoder>)encoder at:(NSUInteger)index
{
    [encoder setTextures:_colorMaps withRange:NSMakeRange(index, 2)];
}

- (void)setVertexTextureFor:(id<MTLRenderCommandEncoder>)encoder at:(NSUInteger)index
{
    [encoder setVertexTextures:_colorMaps withRange:NSMakeRange(index, 2)];
}

- (void)setFragmentTextureFor:(id<MTLRenderCommandEncoder>)encoder at:(NSUInteger)index
{
    [encoder setFragmentTextures:_colorMaps withRange:NSMakeRange(index, 2)];
}

- (void)reset
{
    for (int i = 0; i < 2; ++i) {
        memset(_colorMapPixels[i], 0, sizeof(std::uint32_t) * self.width * self.height);
    }
    [self update];
}

@end
