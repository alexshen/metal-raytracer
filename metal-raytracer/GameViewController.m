//
//  GameViewController.m
//  metal-raytracer
//
//  Created by ashen on 2020/2/10.
//  Copyright © 2020 ashen. All rights reserved.
//

#import "GameViewController.h"
#import "Renderer.h"

static char kProgressChanged;

@interface GameViewController ()
@property (nonatomic) Renderer* renderer;
@end

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

    self.renderer = [[Renderer alloc] initWithMetalKitView:_view];
    [self.renderer mtkView:_view drawableSizeWillChange:_view.bounds.size];
    _view.delegate = self.renderer;
    
    _renderStartTime = mach_absolute_time() * _machTimeToSecs;

    [self.renderer addObserver:self forKeyPath:@"progress" options:0 context:&kProgressChanged];
}

- (void)dealloc
{
    [self.renderer removeObserver:self forKeyPath:@"progress"];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context
{
    if (context == &kProgressChanged) {
        if (self.renderer.progress == 0.0f) {
            _renderStartTime = mach_absolute_time() * _machTimeToSecs;
        }
        self.renderTimeText.stringValue = [NSString stringWithFormat:@"Render Time: %.2lf",
                                                mach_absolute_time() * _machTimeToSecs - _renderStartTime];
        [self.renderTimeText sizeToFit];
    } else {
        [super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
    }
}
@end
