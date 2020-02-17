//
//  Renderer.m
//  metal-raytracer
//
//  Created by ashen on 2020/2/10.
//  Copyright © 2020 ashen. All rights reserved.
//

#import "Renderer.h"
#import "ShaderTypes.h"
#import "RGBA16Image.h"

#include "scene.h"
#include <glm/glm.hpp>
#include <atomic>
#include <cstddef>

#define DEBUG_SHADER 0
#if DEBUG_SHADER
constexpr int ScreenSizeX = 8;
constexpr int ScreenSizeY = 8;
#endif

inline static glm::uvec2 CGSizeToVec2(CGSize size)
{
    return glm::uvec2(size.width, size.height);
}

@interface Renderer ()
@property (nonatomic) int curIter;
@end

@implementation Renderer
{
    int _iterNum;
    bool _textureDirty;
    std::atomic<bool> _softwareRenderFinished;

    id<MTLDevice> _device;
    id<MTLLibrary> _library;
    id<MTLCommandQueue> _commandQueue;

    id<MTLComputePipelineState> _rayTracingPipelineStates[2][2];
    id<MTLBuffer> _nodesBuffer;
    id<MTLBuffer> _spheresBuffer;
    id<MTLBuffer> _materialsBuffer;
    SceneUniform _sceneUniform;

    id<MTLRenderPipelineState> _quadPipelineStates[2];
    RGBA16Image* _sceneImage;

    SceneBuffer* _sceneBuffer;
}

- (void)dealloc
{
    delete _sceneBuffer;
}

- (instancetype)initWithMetalKitView:(nonnull MTKView *)view
{
    self = [super init];
    if (self) {
        _iterNum = 1;
        _hardwareRendering = YES;
        _softwareRenderFinished = true;
        [self _loadMetalWithView:view];
    }

    return self;
}

- (void)_loadMetalWithView:(nonnull MTKView *)view;
{
    /// Load Metal state objects and initalize renderer dependent view properties
    _device = view.device;

    view.depthStencilPixelFormat = MTLPixelFormatInvalid;
    view.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
    view.sampleCount = 1;

    _library = [_device newDefaultLibrary];
    [self _initSceneWithView:view];
    [self _configRaytracePipelineStates];
    [self _configQuadPipelineStateWithView:view];

#if DEBUG_SHADER
    auto size = glm::uvec2(ScreenSizeX, ScreenSizeY);
#else
    auto size = CGSizeToVec2(view.drawableSize);
#endif
    _sceneImage = [[RGBA16Image alloc] initWith:_device width:size.x height:size.y];

    _commandQueue = [_device newCommandQueue];
}

- (void)_configRaytracePipelineStates
{
    MTLFunctionConstantValues* constValues = [MTLFunctionConstantValues new];
    for (int i = 0; i < 2; ++i) {
        BOOL bruteForce = i != 0;
        for (int j = 0; j < 2; ++j) {
            BOOL debugBVHHit = j != 0;
            [constValues setConstantValue:&bruteForce type:MTLDataTypeBool atIndex:ConstantIndexBruteForce];
            [constValues setConstantValue:&debugBVHHit type:MTLDataTypeBool atIndex:ConstantIndexDebugBVHHit];
    
            NSError* error;
            id<MTLFunction> raytraceFunc = [_library newFunctionWithName:@"raytrace"
                                                          constantValues:constValues
                                                                   error:&error];
            if (!raytraceFunc) {
                NSLog(@"%@", error);
                return;
            }
    
            _rayTracingPipelineStates[i][j] = [_device newComputePipelineStateWithFunction:raytraceFunc error:&error];
            if (!_rayTracingPipelineStates[i][j]) {
                NSLog(@"%@", error);
                return;
            }
        }
    }
}

- (void)_configQuadPipelineStateWithView:(MTKView*)view
{
    id<MTLFunction> quadVertex = [_library newFunctionWithName:@"quadVertex"];
    id<MTLFunction> frags[] = {
        [_library newFunctionWithName:@"texturedQuadCustomFilterFrag"],
        [_library newFunctionWithName:@"texturedQuadFrag"],
    };

    MTLRenderPipelineDescriptor* desc = [MTLRenderPipelineDescriptor new];
    desc.colorAttachments[0].pixelFormat = view.colorPixelFormat;
    for (int i = 0; i < 2; ++i) {
        desc.vertexFunction = quadVertex;
        desc.fragmentFunction = frags[i];

        NSError* error;
        _quadPipelineStates[i] = [_device newRenderPipelineStateWithDescriptor:desc error:&error];
        if (!_quadPipelineStates[i]) {
            NSLog(@"%@", error);
            return;
        }
    }
}

- (void)_initSceneWithView:(MTKView*)view
{
    _sceneBuffer = new SceneBuffer(createScene());
    auto& sceneBuf = *_sceneBuffer;

    _nodesBuffer = [_device newBufferWithBytes:sceneBuf.nodes.data()
                                        length:sizeof(Node) * sceneBuf.nodes.size()
                                       options:MTLResourceStorageModeManaged];
    _spheresBuffer = [_device newBufferWithBytes:sceneBuf.objects.data()
                                          length:sizeof(Sphere) * sceneBuf.objects.size()
                                         options:MTLResourceStorageModeManaged];
    _materialsBuffer = [_device newBufferWithBytes:sceneBuf.materials.data()
                                            length:sizeof(Material) * sceneBuf.materials.size()
                                           options:MTLResourceStorageModeManaged];
    _sceneUniform.cameraPos = glm::vec3(13, 2, 3);
    _sceneUniform.cameraLookAt = glm::vec3(0);
    _sceneUniform.focalLength = 1.0f;
    _sceneUniform.fovY = glm::radians(60.0f);
#if DEBUG_SHADER
    _sceneUniform.screenSize = glm::vec2(ScreenSizeX, ScreenSizeY);
#else
    _sceneUniform.screenSize = CGSizeToVec2(view.drawableSize);
#endif
    _sceneUniform.backgroundColor = glm::vec3(0.5f, 0.7f, 1.0f);
    _sceneUniform.numSamples = 100;
    _sceneUniform.numSpheres = static_cast<int>(sceneBuf.objects.size());
    _sceneUniform.iterStart = 0;
    _sceneUniform.iterNum = _iterNum;
}

- (void)drawInMTKView:(nonnull MTKView *)view
{
    id <MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
    if (self.curIter < self.numSamples && _softwareRenderFinished) {
        if (_textureDirty) {
            [_sceneImage update];
            _textureDirty = false;
        }
        _sceneUniform.iterStart = self.curIter;
        _sceneUniform.seed = _sceneUniform.iterStart * 17 + _sceneUniform.iterNum;
        
        ++self.curIter;

        if (self.hardwareRendering) {
            [self _renderWithCommandBuffer:commandBuffer view:view];
        } else {
            [self _renderWithSoftware];
        }
        
        if (self.debugBVHHit) {
            // debug render finishes in one iteration
            self.curIter = self.numSamples;
        }
    }

    MTLRenderPassDescriptor* renderPassDescriptor = view.currentRenderPassDescriptor;

    if(renderPassDescriptor != nil) {
        id <MTLRenderCommandEncoder> renderEncoder =
            [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
        
        [renderEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
        [renderEncoder setCullMode:MTLCullModeBack];
        [renderEncoder setRenderPipelineState:_quadPipelineStates[self.hardwareFilter]];
        [_sceneImage setFragmentTextureFor:renderEncoder at:TextureIndexSceneTex];
        [renderEncoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];

        [renderEncoder endEncoding];
        [commandBuffer presentDrawable:view.currentDrawable];
    }

    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
}

- (void)_renderWithCommandBuffer:(id<MTLCommandBuffer>)commandBuffer view:(MTKView * _Nonnull)view
{
    id<MTLComputeCommandEncoder> rayTraceEncoder = [commandBuffer computeCommandEncoder];
    id<MTLComputePipelineState> raytracingState = _rayTracingPipelineStates[self.bruteForce][self.debugBVHHit];
    rayTraceEncoder.label = @"Ray Tracing Compute";
    [rayTraceEncoder setComputePipelineState:raytracingState];
    [_sceneImage setTextureFor:rayTraceEncoder at:TextureIndexSceneTex];
    [rayTraceEncoder setBuffer:_nodesBuffer offset:0 atIndex:BufferIndexNode];
    [rayTraceEncoder setBuffer:_spheresBuffer offset:0 atIndex:BufferIndexSphere];
    [rayTraceEncoder setBuffer:_materialsBuffer offset:0 atIndex:BufferIndexMaterial];
    [rayTraceEncoder setBytes:&_sceneUniform length:sizeof(SceneUniform) atIndex:BufferIndexSceneUniform];
    
    // calculate thread size
#if DEBUG_SHADER
    MTLSize threadsSize = MTLSizeMake(ScreenSizeX, ScreenSizeY, 1);
#else
    MTLSize threadsSize = MTLSizeMake(view.drawableSize.width, view.drawableSize.height, 1);
#endif
    MTLSize threadGroupSize;
    threadGroupSize.width = raytracingState.threadExecutionWidth;
    threadGroupSize.height = raytracingState.maxTotalThreadsPerThreadgroup / threadGroupSize.width;
    threadGroupSize.depth = 1;
    
    [rayTraceEncoder dispatchThreads:threadsSize threadsPerThreadgroup:threadGroupSize];
    [rayTraceEncoder endEncoding];
}

- (void)_renderWithSoftware
{
    tracer::Scene scene(_sceneBuffer->nodes.data(),
                        _sceneBuffer->objects.data(),
                        _sceneBuffer->materials.data(),
                        static_cast<int>(_sceneBuffer->objects.size()));
    tracer::Camera camera(_sceneUniform.cameraPos, _sceneUniform.cameraLookAt, math::float3(0, 1, 0),
                          _sceneUniform.fovY, _sceneUniform.focalLength,
                          _sceneUniform.screenSize);

    int iterEnd = math::min(_sceneUniform.numSamples, _sceneUniform.iterStart + _sceneUniform.iterNum);
    dispatch_queue_t taskQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_group_t group = dispatch_group_create();
    
    if (self.debugBVHHit) {
        tracer::Random random(self->_sceneUniform.seed);
        tracer::RayTracer tracer(random, camera, scene, self->_sceneUniform.backgroundColor);
        for (int i = 0; i < _sceneUniform.screenSize.y; ++i) {
            for (int j = 0; j < self->_sceneUniform.screenSize.x; ++j) {
                math::float3 color = tracer::debugTrace(scene, camera, math::float2(j, i));
                [self->_sceneImage setColor:math::float4(color, 0) at:math::uint2(j, i)];
            }
        }
    } else {
        for (int i = 0; i < _sceneUniform.screenSize.y; ++i) {
            for (int j = 0; j < _sceneUniform.screenSize.x; ++j) {
                math::uint2 threadPos(j, i);
                dispatch_group_async(group, taskQueue, ^{
                    tracer::Random random(self->_sceneUniform.seed);
                    tracer::RayTracer tracer(random, camera, scene, self->_sceneUniform.backgroundColor);
                    math::float3 color(0);
                    for (int i = self->_sceneUniform.iterStart; i < iterEnd; ++i) {
                        math::float2 samplePos = math::float2(threadPos) + random.inUnitRect();
                        if (self.bruteForce) {
                            color += tracer.trace<true>(samplePos);
                        } else {
                            color += tracer.trace<false>(samplePos);
                        }
                    }
                    math::float4 newColor = ([self->_sceneImage colorAt:threadPos] * (float)self->_sceneUniform.iterStart
                                             + math::float4(color, 0)) / (float)iterEnd;
                    [self->_sceneImage setColor:newColor at:threadPos];
                });
            }
        }
    }

    _softwareRenderFinished = false;
    dispatch_group_notify(group, taskQueue, ^{
        self->_textureDirty = true;
        self->_softwareRenderFinished = true;
    });
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size
{
    /// Respond to drawable size or orientation changes here
}

- (void)setBruteForce:(BOOL)bruteForce
{
    if (_bruteForce != bruteForce) {
        _bruteForce = bruteForce;
        [self _resetRender];
    }
}

- (void)setDebugBVHHit:(BOOL)debugBVHHit
{
    if (_debugBVHHit != debugBVHHit) {
        _debugBVHHit = debugBVHHit;
        [self _resetRender];
    }
}

- (int)numSamples
{
    return _sceneUniform.numSamples;
}

- (void)setNumSamples:(int)numSamples
{
    NSAssert(numSamples > 0, @"numSamples must be positive");
    if (_sceneUniform.numSamples != numSamples) {
        _sceneUniform.numSamples = numSamples;
        [self _resetRender];
    }
}

- (void)setHardwareRendering:(BOOL)hardwareRendering
{
    if (_hardwareRendering != hardwareRendering) {
        _hardwareRendering = hardwareRendering;
        [self _resetRender];
    }
}

- (void)setHardwareFilter:(BOOL)hardwareFilter {
    if (_hardwareFilter != hardwareFilter) {
        _hardwareFilter = hardwareFilter;
        [self _resetRender];
    }
}

- (void)_resetRender
{
    self.curIter = 0;
    [_sceneImage reset];
}

- (float)progress
{
    return float(_curIter) / self.numSamples;
}

- (void)setCurIter:(int)curIter
{
    if (_curIter != curIter) {
        [self willChangeValueForKey:@"progress"];
        _curIter = curIter;
        [self didChangeValueForKey:@"progress"];
    }
}
@end
