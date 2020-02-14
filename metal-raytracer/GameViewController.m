//
//  GameViewController.m
//  metal-raytracer
//
//  Created by ashen on 2020/2/10.
//  Copyright © 2020 ashen. All rights reserved.
//

#import "GameViewController.h"
#import "Renderer.h"

static char kProgressChanged = 0;
static char kNumSamplesChanged = 0;

@implementation GameViewController
{
    MTKView *_view;
    double _machTimeToSecs;
    double _renderStartTime;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    mach_timebase_info_data_t timebase;
    mach_timebase_info(&timebase);
    _machTimeToSecs = (double)timebase.numer / timebase.denom * 1e-9;
    
    _view = (MTKView *)self.view;
    _view.device = MTLCreateSystemDefaultDevice();

    if(!_view.device) {
        NSLog(@"Metal is not supported on this device");
        self.view = [[NSView alloc] initWithFrame:self.view.frame];
        return;
    }

    _renderer = [[Renderer alloc] initWithMetalKitView:_view];
    [_renderer mtkView:_view drawableSizeWillChange:_view.bounds.size];
    _view.delegate = _renderer;
    
    self.numSamplesText.intValue = self.renderer.numSamples;
    self.numSamplesSlider.intValue = self.renderer.numSamples;
    self.debugBVHToggle.state = self.renderer.debugBVHHit;
    self.hardwareRenderingToggle.state = self.renderer.hardwareRendering;
    self.bruteForceToggle.state = self.renderer.bruteForce;
    self.renderTimeText.hidden = YES;
    
    [self.renderer addObserver:self forKeyPath:@"progress" options:0 context:&kProgressChanged];
    [self.renderer addObserver:self forKeyPath:@"numSamples" options:0 context:&kNumSamplesChanged];
    
    _renderStartTime = mach_absolute_time() * _machTimeToSecs;
}

- (void)dealloc
{
    [self.renderer removeObserver:self forKeyPath:@"progress"];
    [self.renderer removeObserver:self forKeyPath:@"numSamples"];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context
{
    if (context == &kProgressChanged) {
        self.renderProgressIndicator.doubleValue = self.renderer.progress;
        if (self.renderer.progress == 0.0f) {
            _renderStartTime = mach_absolute_time() * _machTimeToSecs;
        }
        self.renderTimeText.hidden = NO;
        self.renderTimeText.stringValue = [NSString stringWithFormat:@"Render Time: %.2lf", mach_absolute_time() * _machTimeToSecs - _renderStartTime];
        [self.renderTimeText sizeToFit];
    } else if (context == &kNumSamplesChanged) {
        self.numSamplesText.intValue = self.renderer.numSamples;
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}
@end
