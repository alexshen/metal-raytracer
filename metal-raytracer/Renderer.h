//
//  Renderer.h
//  metal-raytracer
//
//  Created by ashen on 2020/2/10.
//  Copyright © 2020 ashen. All rights reserved.
//

#import <MetalKit/MetalKit.h>

// Our platform independent renderer class.   Implements the MTKViewDelegate protocol which
//   allows it to accept per-frame update and drawable resize callbacks.
@interface Renderer : NSObject <MTKViewDelegate>

- (instancetype)initWithMetalKitView:(nonnull MTKView *)view;

@property (nonatomic) BOOL bruteForce;
@property (nonatomic) BOOL debugBVHHit;
@property (nonatomic) int numSamples;
@property (nonatomic) BOOL hardwareRendering;
@property (nonatomic, readonly) float progress;

@end

