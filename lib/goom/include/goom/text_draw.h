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

  void setFontSize(int val);
  void setOutlineWidth(float val);
  void setFont(const std::string& filename);

  using FontColorFunc = std::function<Pixel(wchar_t ch, float x, float y, float width, float height)>;
  void setFontColorFunc(const FontColorFunc&);
  void setOutlineFontColorFunc(const FontColorFunc&);

  class DrawStringInfo
  {
  public:
    virtual ~DrawStringInfo() noexcept = default;
    [[nodiscard]] virtual uint32_t getTotalWidth() const = 0;
    [[nodiscard]] virtual uint32_t getTotalHeight() const = 0;
  };
  std::unique_ptr<DrawStringInfo> getDrawStringInfo(const std::string&);

  void draw(const std::string&, int xPen, int yPen, int& xNext, int& yNext, PixelBuffer&);

private:
  class TextDrawImpl;
  std::unique_ptr<TextDrawImpl> textDrawImpl;
};

} // namespace goom
#endif
