#pragma once

#include "PreviewTypes.h"

namespace cataloger::services::preview {

class PreviewExtractor {
public:
  PreviewImage extract(const PreviewDescriptor& descriptor) const;

private:
  static PreviewImage readPreviewBytes(const PreviewDescriptor& descriptor);
};

}  // namespace cataloger::services::preview
