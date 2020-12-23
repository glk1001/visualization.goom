#ifndef _TEXT_DRAW_H
#define _TEXT_DRAW_H

#include <memory>
#include <string>

namespace goom
{

class PixelBuffer;

class TextDraw
{
public:
  TextDraw(int screenWidth, int screenHeight) noexcept;
  ~TextDraw() noexcept;

  void setFont(const std::string& filename);
  void setFontSize(int val);
  void setOutlineWidth(float val);

  void draw(const std::string&, int xPen, int yPen, int& xNext, int& yNext, PixelBuffer&);

private:
  class TextDrawImpl;
  std::unique_ptr<TextDrawImpl> textDrawImpl;
};

} // namespace goom
#endif
