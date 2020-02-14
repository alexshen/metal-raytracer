//
//  GameViewController.h
//  metal-raytracer
//
//  Created by ashen on 2020/2/10.
//  Copyright © 2020 ashen. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import "Renderer.h"

// Our macOS view controller.
@interface GameViewController : NSViewController

@property (nonatomic, readonly) Renderer* renderer;

@property (weak) IBOutlet NSTextField *numSamplesText;
@property (weak) IBOutlet NSButton *debugBVHToggle;
@property (weak) IBOutlet NSButton *hardwareRenderingToggle;
@property (weak) IBOutlet NSButton *bruteForceToggle;
@property (weak) IBOutlet NSSlider *numSamplesSlider;
@property (weak) IBOutlet NSProgressIndicator *renderProgressIndicator;
@property (weak) IBOutlet NSTextField *renderTimeText;

@end
