#ifndef VISUALIZATION_GOOM_LIB_GOOMUTILS_IMAGE_BITMAPS_H
#define VISUALIZATION_GOOM_LIB_GOOMUTILS_IMAGE_BITMAPS_H

#include "goom/goom_graphic.h"

#include <string>
#include <utility>

namespace GOOM::UTILS
{

class ImageBitmap : public PixelBuffer
{
public:
  ImageBitmap() noexcept = default;
  explicit ImageBitmap(std::string imageFilename);

  void SetFilename(std::string imageFilename);
  void Load();

private:
  std::string m_filename{};
};

inline ImageBitmap::ImageBitmap(std::string imageFilename)
  : PixelBuffer{}, m_filename{std::move(imageFilename)}
{
}

inline void ImageBitmap::SetFilename(std::string imageFilename)
{
  m_filename = std::move(imageFilename);
}


} // namespace GOOM::UTILS
#endif
