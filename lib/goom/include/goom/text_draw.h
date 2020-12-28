#ifndef _TEXT_DRAW_H
#define _TEXT_DRAW_H

#include <functional>
#include <memory>
#include <string>

namespace goom
{

class Pixel;
class PixelBuffer;

class TextDraw
{
public:
  TextDraw(uint32_t screenWidth, uint32_t screenHeight) noexcept;
  ~TextDraw() noexcept;

  enum class TextAlignment
  {
    left,
    center,
    right
  };
  void setAlignment(TextAlignment);

  void setFontSize(int val);
  void setOutlineWidth(float val);
  void setCharSpacing(float val);
  void setFont(const std::string& filename);
  void setText(const std::string&);

  using FontColorFunc =
      std::function<Pixel(size_t textIndexOfChar, int x, int y, int width, int height)>;
  void setFontColorFunc(const FontColorFunc&);
  void setOutlineFontColorFunc(const FontColorFunc&);

  void prepare();

  struct Rect
  {
    int xmin{};
    int xmax{};
    int ymin{};
    int ymax{};
    [[nodiscard]] int width() const { return xmax - xmin + 1; }
    [[nodiscard]] int height() const { return ymax - ymin + 1; }
  };
  [[nodiscard]] Rect getPreparedTextBoundingRect() const;
  [[nodiscard]] int getBearingX() const;
  [[nodiscard]] int getBearingY() const;

  void draw(int xPen, int yPen, int& xNext, int& yNext, PixelBuffer&);

private:
  class TextDrawImpl;
  std::unique_ptr<TextDrawImpl> textDrawImpl;
};

} // namespace goom
#endif
