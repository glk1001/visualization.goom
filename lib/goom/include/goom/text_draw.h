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
  TextDraw(int screenWidth, int screenHeight) noexcept;
  ~TextDraw() noexcept;

  void setFont(const std::string& filename);
  void setFontSize(int val);
  void setOutlineWidth(float val);

  using FontColorFunc = std::function<Pixel(wchar_t ch, float x, float y, float width, float height)>;
  void setFontColorFunc(const FontColorFunc&);
  void setOutlineFontColorFunc(const FontColorFunc&);

  void draw(const std::string&, int xPen, int yPen, int& xNext, int& yNext, PixelBuffer&);

private:
  class TextDrawImpl;
  std::unique_ptr<TextDrawImpl> textDrawImpl;
};

} // namespace goom
#endif
