//
//  RGBA16Image.h
//  metal-raytracer
//
//  Created by ashen on 2020/2/12.
//  Copyright © 2020 ashen. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <glm/glm.hpp>

NS_ASSUME_NONNULL_BEGIN

@interface RGBA16Image : NSObject

- (nullable instancetype)init NS_UNAVAILABLE;
- (nullable instancetype)initWith:(id<MTLDevice>)device width:(NSUInteger)width height:(NSUInteger)height NS_DESIGNATED_INITIALIZER;

@property (nonatomic, readonly) NSUInteger width;
@property (nonatomic, readonly) NSUInteger height;

- (glm::vec4)colorAt:(glm::uvec2)pos;
- (void)setColor:(glm::vec4)color at:(glm::uvec2)pos;
- (void)update;
- (void)setTextureFor:(id<MTLComputeCommandEncoder>)encoder at:(NSUInteger)index;
- (void)setVertexTextureFor:(id<MTLRenderCommandEncoder>)encoder at:(NSUInteger)index;
- (void)setFragmentTextureFor:(id<MTLRenderCommandEncoder>)encoder at:(NSUInteger)index;
- (void)reset;

@end

NS_ASSUME_NONNULL_END
