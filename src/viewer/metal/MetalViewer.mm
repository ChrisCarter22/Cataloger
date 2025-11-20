#include "viewer/ViewerContract.h"

#if defined(__APPLE__)

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <cstring>
#include <iostream>

namespace cataloger::viewer {

namespace {

id<MTLTexture> HandleToTexture(const TextureHandle& handle) {
  return (__bridge id<MTLTexture>)handle.native_handle;
}

}  // namespace

class MetalViewer : public Viewer {
public:
  MetalViewer()
      : device_(MTLCreateSystemDefaultDevice()),
        queue_(device_ ? [device_ newCommandQueue] : nil),
        layer_([CAMetalLayer layer]) {
    if (layer_) {
      layer_.device = device_;
      layer_.pixelFormat = MTLPixelFormatBGRA8Unorm;
      layer_.maximumDrawableCount = 3;
      layer_.framebufferOnly = YES;
    }
  }

  bool beginFrame(FrameContext& context) override {
    if (!device_ || !queue_ || !layer_) {
      return false;
    }
    if (context.texture.backend != Backend::kMetal ||
        context.texture.native_handle == nullptr) {
      return false;
    }

    layer_.drawableSize = CGSizeMake(context.width * context.scale,
                                     context.height * context.scale);

    drawable_ = [layer_ nextDrawable];
    if (!drawable_) {
      return false;
    }
    return true;
  }

  bool submitFrame(const FrameContext& context, std::string& error) override {
    if (!drawable_) {
      error = "No drawable available";
      return false;
    }

    id<MTLTexture> source = HandleToTexture(context.texture);
    if (!source) {
      error = "Invalid Metal texture handle";
      return false;
    }

    const auto expected_width = static_cast<NSUInteger>(context.width);
    const auto expected_height = static_cast<NSUInteger>(context.height);
    if ([source width] != expected_width || [source height] != expected_height) {
      error = "Metal texture dimensions do not match FrameContext";
      return false;
    }

    id<MTLCommandBuffer> cmd = [queue_ commandBuffer];
    if (!cmd) {
      error = "Failed to create Metal command buffer";
      return false;
    }

    MTLOrigin origin = {0, 0, 0};
    MTLSize size = {static_cast<NSUInteger>(context.width),
                   static_cast<NSUInteger>(context.height),
                   1};

    id<MTLBlitCommandEncoder> blit = [cmd blitCommandEncoder];
    if (!blit) {
      error = "Failed to create Metal blit encoder";
      return false;
    }

    [blit copyFromTexture:source
               sourceSlice:0
               sourceLevel:0
              sourceOrigin:origin
                sourceSize:size
                 toTexture:drawable_.texture
          destinationSlice:0
          destinationLevel:0
         destinationOrigin:origin];
    [blit endEncoding];

    pending_command_ = cmd;
    return true;
  }

  bool present(std::string& error) override {
    if (!pending_command_ || !drawable_) {
      error = "No command buffer to present";
      return false;
    }

    [pending_command_ presentDrawable:drawable_];
    [pending_command_ commit];
    [pending_command_ waitUntilScheduled];

    if (pending_command_.error) {
      error = [[pending_command_.error description] UTF8String];
      pending_command_ = nil;
      drawable_ = nil;
      return false;
    }

    pending_command_ = nil;
    drawable_ = nil;
    return true;
  }

private:
  id<MTLDevice> device_{nil};
  id<MTLCommandQueue> queue_{nil};
  CAMetalLayer* layer_{nil};
  id<CAMetalDrawable> drawable_{nil};
  id<MTLCommandBuffer> pending_command_{nil};
};

std::unique_ptr<Viewer> CreateMetalViewer() {
  return std::make_unique<MetalViewer>();
}

}  // namespace cataloger::viewer

#endif  // __APPLE__
