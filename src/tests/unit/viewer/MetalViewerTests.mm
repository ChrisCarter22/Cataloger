#include <gtest/gtest.h>

#if defined(__APPLE__)

#import <Metal/Metal.h>

#include "viewer/ViewerContract.h"

using cataloger::viewer::Backend;
using cataloger::viewer::CreateMetalViewer;
using cataloger::viewer::FrameContext;

TEST(MetalViewerTests, CanBeginSubmitAndPresent) {
  id<MTLDevice> device = MTLCreateSystemDefaultDevice();
  ASSERT_NE(device, nil);

  const std::uint32_t width = 64;
  const std::uint32_t height = 64;
  MTLTextureDescriptor* desc =
      [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                         width:width
                                                        height:height
                                                     mipmapped:NO];
  desc.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
  id<MTLTexture> texture = [device newTextureWithDescriptor:desc];
  ASSERT_NE(texture, nil);

  auto viewer = CreateMetalViewer();
  ASSERT_NE(viewer, nullptr);

  FrameContext context;
  context.width = width;
  context.height = height;
  context.scale = 1.0;
  context.vsync = true;
  context.color_space = "sRGB IEC61966-2.1";
  context.texture.backend = Backend::kMetal;
  context.texture.native_handle = (__bridge void*)texture;

  EXPECT_TRUE(viewer->beginFrame(context));
  std::string error;
  EXPECT_TRUE(viewer->submitFrame(context, error)) << error;
  EXPECT_TRUE(error.empty());
  EXPECT_TRUE(viewer->present(error)) << error;
}

#endif  // __APPLE__

