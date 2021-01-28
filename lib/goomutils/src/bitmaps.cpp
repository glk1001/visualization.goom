#include "bitmaps.h"

namespace GOOM::UTILS
{

IBitmap::IBitmap(const size_t width, const size_t height) noexcept
{
  Resize(width, height);
}

void IBitmap::Resize(const size_t width, const size_t height)
{
  m_bitmap.resize(height);
  for (auto& b : m_bitmap)
  {
    b.resize(width);
  }
}

} // namespace GOOM::UTILS
