#ifndef VISUALIZATION_GOOM_LIB_GOOMUTILS_BITMAPS_H
#define VISUALIZATION_GOOM_LIB_GOOMUTILS_BITMAPS_H

#include "goom/goom_draw.h"
#include "goom/goom_graphic.h"

#include <memory>
#include <utility>

namespace GOOM::UTILS
{

class IBitmap
{
public:
  IBitmap() noexcept = default;
  virtual ~IBitmap() noexcept = default;
  IBitmap(size_t width, size_t height) noexcept;
  IBitmap(const IBitmap&) noexcept = default;
  IBitmap(IBitmap&&) noexcept = default;
  auto operator=(const IBitmap&) -> IBitmap& = default;
  auto operator=(IBitmap&&) -> IBitmap& = default;

  [[nodiscard]] auto GetBitmap() const -> const GoomDraw::BitmapType&;

  void Resize(size_t width, size_t height);
  virtual void Load() = 0;

  [[nodiscard]] auto GetWidth() const -> size_t;
  [[nodiscard]] auto GetHeight() const -> size_t;
  auto operator()(size_t x, size_t y) const -> const Pixel&;

  //protected:
public:
  auto operator()(size_t x, size_t y) -> Pixel&;

private:
  GoomDraw::BitmapType m_bitmap{};
};

inline auto IBitmap::GetBitmap() const -> const GoomDraw::BitmapType&
{
  return m_bitmap;
}

inline auto IBitmap::GetWidth() const -> size_t
{
  if (GetHeight() == 0)
  {
    return 0;
  }
  return m_bitmap[0].size();
}

inline auto IBitmap::GetHeight() const -> size_t
{
  return m_bitmap.size();
}

inline auto IBitmap::operator()(const size_t x, const size_t y) const -> const Pixel&
{
  return m_bitmap[y][x];
}

inline auto IBitmap::operator()(const size_t x, const size_t y) -> Pixel&
{
  return m_bitmap[y][x];
}

class RectangleBitmap : public IBitmap
{
public:
  RectangleBitmap() noexcept = default;
  RectangleBitmap(size_t width, size_t height) noexcept;
  void Load() override;
};

inline RectangleBitmap::RectangleBitmap(size_t width, size_t height) noexcept
  : IBitmap(width, height)
{
}

inline void RectangleBitmap::Load()
{
  for (size_t y = 0; y < GetHeight(); ++y)
  {
    for (size_t x = 0; x < GetWidth(); ++x)
    {
      (*this)(x, y) = Pixel{0xffffffffU};
    }
  }
}

} // namespace GOOM::UTILS
#endif
