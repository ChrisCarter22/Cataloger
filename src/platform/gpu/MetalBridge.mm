#include "GpuBridge.h"

#if defined(__APPLE__)

#import <Metal/Metal.h>

#include <cstring>
#include <vector>

namespace cataloger::platform::gpu {

namespace {

std::vector<std::uint8_t> toBGRA(
    const cataloger::services::preview::PreviewImage& image) {
  if (image.width <= 0 || image.height <= 0) {
    return {};
  }
  const auto pixel_count = static_cast<std::size_t>(image.width) *
                           static_cast<std::size_t>(image.height);
  std::vector<std::uint8_t> bgra(pixel_count * 4, 255);
  const auto src = image.pixels.data();
  for (std::size_t i = 0, j = 0;
       j < pixel_count && (i + 2) < image.pixels.size();
       ++j, i += 3) {
    bgra[j * 4 + 0] = src[i + 2];  // B
    bgra[j * 4 + 1] = src[i + 1];  // G
    bgra[j * 4 + 2] = src[i + 0];  // R
  }
  return bgra;
}

class MetalBridge : public GpuBridge {
public:
  MetalBridge()
      : backend_(Backend::kMetal),
        device_(MTLCreateSystemDefaultDevice()),
        queue_(device_ ? [device_ newCommandQueue] : nil),
        last_texture_label_("nil") {}

  bool upload(const cataloger::services::preview::PreviewImage& image,
              std::string& error) override {
    @autoreleasepool {
      if (!device_ || !queue_) {
        error = "Metal device/queue unavailable.";
        backend_ = Backend::kStub;
        return false;
      }

      const auto bgra = toBGRA(image);
      if (bgra.empty()) {
        error = "Preview pixels missing or malformed.";
        backend_ = Backend::kStub;
        return false;
      }

      const NSUInteger bytes_per_row =
          static_cast<NSUInteger>(image.width) * 4;
      const NSUInteger byte_count = bytes_per_row *
                                    static_cast<NSUInteger>(image.height);
      id<MTLBuffer> staging =
          [device_ newBufferWithLength:byte_count options:MTLResourceStorageModeShared];
      if (!staging) {
        error = "Failed to allocate Metal staging buffer.";
        backend_ = Backend::kStub;
        return false;
      }
      std::memcpy([staging contents], bgra.data(), byte_count);

      MTLTextureDescriptor* descriptor =
          [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                                                             width:image.width
                                                            height:image.height
                                                         mipmapped:NO];
      descriptor.usage = MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget;
      id<MTLTexture> texture = [device_ newTextureWithDescriptor:descriptor];
      if (!texture) {
        error = "Failed to allocate Metal texture.";
        backend_ = Backend::kStub;
        return false;
      }

      id<MTLCommandBuffer> command = [queue_ commandBuffer];
      if (!command) {
        error = "Failed to create Metal command buffer.";
        backend_ = Backend::kStub;
        return false;
      }

      id<MTLBlitCommandEncoder> blit = [command blitCommandEncoder];
      if (!blit) {
        error = "Failed to acquire Metal blit encoder.";
        backend_ = Backend::kStub;
        return false;
      }

      MTLSize size =
          MTLSizeMake(static_cast<NSUInteger>(image.width),
                      static_cast<NSUInteger>(image.height),
                      1);
      [blit copyFromBuffer:staging
               sourceOffset:0
          sourceBytesPerRow:bytes_per_row
        sourceBytesPerImage:bytes_per_row *
                             static_cast<NSUInteger>(image.height)
                 sourceSize:size
                  toTexture:texture
           destinationSlice:0
           destinationLevel:0
          destinationOrigin:MTLOriginMake(0, 0, 0)];
      [blit endEncoding];

      [command commit];
      [command waitUntilCompleted];

      if (command.error) {
        error = [[command.error description] UTF8String];
        backend_ = Backend::kStub;
        return false;
      }

      last_texture_label_ = [texture label] ? [[texture label] UTF8String]
                                            : "MTLTexture";
      last_texture_ = texture;
      backend_ = Backend::kMetal;
      error.clear();
      return true;
    }
  }

  Backend backend() const noexcept override {
    return backend_;
  }

  std::string textureDebugLabel() const override {
    return last_texture_label_;
  }

private:
  Backend backend_;
  id<MTLDevice> device_;
  id<MTLCommandQueue> queue_;
  id<MTLTexture> last_texture_;
  std::string last_texture_label_;
};

}  // namespace

std::unique_ptr<GpuBridge> CreateMetalBridge() {
  return std::make_unique<MetalBridge>();
}

}  // namespace cataloger::platform::gpu

#endif  // __APPLE__
