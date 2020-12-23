#include "text_draw.h"

#include "goom_graphic.h"
#include "goomutils/colorutils.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_STROKER_H

#include <format>
#include <fstream>

namespace goom
{

class TextDraw::TextDrawImpl
{
public:
  TextDrawImpl(int screenW, int screenH) noexcept;
  ~TextDrawImpl() noexcept;
  TextDrawImpl(const TextDrawImpl&) noexcept = delete;
  TextDrawImpl& operator=(const TextDrawImpl&) = delete;

  void setFont(const std::string& filename);
  void setFontSize(int val);
  void setOutlineWidth(float val);

  void draw(const std::string&, int xPen, int yPen, int& xNext, int& yNext, PixelBuffer&);

private:
  const int screenWidth;
  const int screenHeight;
  FT_Library library{};
  int fontSize = 100;
  uint32_t horizontalResolution = 90;
  uint32_t verticalResolution = 90;
  float outlineWidth = 3;
  std::string fontFilename{};
  std::vector<unsigned char> fontBuffer{};
  Pixel fontColor{{.r = 100, .g = 90, .b = 150, .a = 100}};
  Pixel outlineColor{{.r = 255, .g = 255, .b = 255, .a = 255}};
  void writeGlyph(wchar_t, FT_Face&, int xPen, int yPen, int& advance, PixelBuffer&) const;
  void writeGlyphSimple(wchar_t, FT_Face&, int xPen, int yPen, int& advance, PixelBuffer&) const;
  void drawBitmap(FT_Bitmap*, FT_Int x, FT_Int y, PixelBuffer&) const;

  static constexpr int toStdPixelCoord(int freeTypeCoord);
  static constexpr int toFreeTypeCoord(int stdPixelCoord);
  static constexpr int toFreeTypeCoord(float stdPixelCoord);
  struct Vec2;
  struct Rect;
  struct Span;
  using Spans = std::vector<Span>;
  Spans getSpans(FT_Face&) const;
  Spans getOutlineSpans(FT_Face&) const;
  static Rect getBoundingRect(const Spans& outlineSpans, const Spans& spans);
  void RenderSpans(FT_Outline*, Spans*) const;
  static void RasterCallback(int y, int count, const FT_Span* spans, void* user);
  void writeSpansToImage(
      const Spans&, const Rect&, int xPen, int yPen, const Pixel& color, PixelBuffer&) const;
};

TextDraw::TextDraw(const int screenW, const int screenH) noexcept
  : textDrawImpl{new TextDrawImpl{screenW, screenH}}
{
}

TextDraw::~TextDraw() noexcept
{
}

void TextDraw::setFont(const std::string& filename)
{
  textDrawImpl->setFont(filename);
}

void TextDraw::setFontSize(int val)
{
  textDrawImpl->setFontSize(val);
}

void TextDraw::setOutlineWidth(float val)
{
  textDrawImpl->setOutlineWidth(val);
}

void TextDraw::draw(const std::string& str,
                    const int xPen,
                    const int yPen,
                    int& xNext,
                    int& yNext,
                    PixelBuffer& buffer)
{
  textDrawImpl->draw(str, xPen, yPen, xNext, yNext, buffer);
}

TextDraw::TextDrawImpl::TextDrawImpl(const int screenW, const int screenH) noexcept
  : screenWidth{screenW}, screenHeight{screenH}
{
  FT_Init_FreeType(&library);
}

TextDraw::TextDrawImpl::~TextDrawImpl() noexcept
{
  FT_Done_FreeType(library);
}

void TextDraw::TextDrawImpl::setFont(const std::string& filename)
{
  fontFilename = filename;

  std::ifstream fontFile(fontFilename, std::ios::binary);
  if (!fontFile)
  {
    throw std::runtime_error(std20::format("Could not open font file \"{}\".", fontFilename));
  }

  // Read the entire file to a memory buffer.
  fontFile.seekg(0, std::ios::end);
  std::fstream::pos_type fontFileSize = fontFile.tellg();
  fontFile.seekg(0);
  fontBuffer.resize(static_cast<size_t>(fontFileSize));
  fontFile.read((char*)fontBuffer.data(), fontFileSize);
}

void TextDraw::TextDrawImpl::setFontSize(int val)
{
  fontSize = val;
}

void TextDraw::TextDrawImpl::setOutlineWidth(float val)
{
  outlineWidth = val;
}

constexpr int TextDraw::TextDrawImpl::toStdPixelCoord(const int freeTypeCoord)
{
  return freeTypeCoord >> 6;
}

constexpr int TextDraw::TextDrawImpl::toFreeTypeCoord(const int stdPixelCoord)
{
  return stdPixelCoord << 6;
}

constexpr int TextDraw::TextDrawImpl::toFreeTypeCoord(const float stdPixelCoord)
{
  return static_cast<int>(std::lround(stdPixelCoord * 64.0));
}

void TextDraw::TextDrawImpl::draw(const std::string& str,
                                  const int xPen,
                                  const int yPen,
                                  int& xNext,
                                  int& yNext,
                                  PixelBuffer& buffer)
{
  // Create a face from a memory buffer.  Be sure not to delete the memory buffer
  // until you are done using that font as FreeType will reference it directly.
  FT_Face face;
  FT_New_Memory_Face(library, fontBuffer.data(), static_cast<FT_Long>(fontBuffer.size()), 0, &face);

  // Dump out a single glyph.
  xNext = xPen;
  yNext = yPen;
  int advance = 0;

  for (const auto& c : str)
  {
    xNext += advance;
    writeGlyph(c, face, xNext, yNext + 200, advance, buffer);
    writeGlyphSimple(c, face, xNext, yNext, advance, buffer);
  }
}

void TextDraw::TextDrawImpl::drawBitmap(FT_Bitmap* bitmap,
                                        const FT_Int x,
                                        const FT_Int y,
                                        PixelBuffer& buffer) const
{
  const auto xMax = static_cast<uint32_t>(x) + bitmap->width;
  const auto yMax = static_cast<uint32_t>(y) + bitmap->rows;

  // for simplicity, we assume that `bitmap->pixel_mode'
  // is `FT_PIXEL_MODE_GRAY' (i.e., not a bitmap font)

  for (int i = x, p = 0; i < static_cast<int>(xMax); i++, p++)
  {
    for (int j = y, q = 0; j < static_cast<int>(yMax); j++, q++)
    {
      if (i < 0 || j < 0 || i >= screenWidth || j >= screenHeight)
      {
        continue;
      }

      const int color =
          bitmap->buffer[static_cast<size_t>(q * static_cast<int>(bitmap->width) + p)];
      if (color == 0)
      {
        buffer(static_cast<size_t>(i), static_cast<size_t>(j)) = Pixel{0U};
      }
      else
      {
        buffer(static_cast<size_t>(i), static_cast<size_t>(j)) = fontColor;
      }
    }
  }
}

void TextDraw::TextDrawImpl::writeGlyphSimple(const wchar_t ch,
                                              FT_Face& face,
                                              const int xPen,
                                              const int yPen,
                                              int& advance,
                                              PixelBuffer& buff) const
{
  // Set the size to use.
  if (FT_Set_Char_Size(face, toFreeTypeCoord(fontSize), toFreeTypeCoord(fontSize),
                       horizontalResolution, verticalResolution) != 0)
  {
    throw std::runtime_error(std20::format("Could not set font size to {}.", fontSize));
  }

  const float angle = (25.0F / 360.0F) * 3.14159F * 2; /* use 25 degrees     */

  FT_GlyphSlot slot = face->glyph;

  FT_Matrix matrix;
  matrix.xx = (FT_Fixed)(+std::cos(angle) * 0x10000L);
  matrix.xy = (FT_Fixed)(-std::sin(angle) * 0x10000L);
  matrix.yx = (FT_Fixed)(+std::sin(angle) * 0x10000L);
  matrix.yy = (FT_Fixed)(+std::cos(angle) * 0x10000L);

  FT_Vector pen{toFreeTypeCoord(xPen), toFreeTypeCoord(yPen)};
  FT_Set_Transform(face, &matrix, &pen);

  // load glyph image into the slot (erase previous one)
  if (FT_Load_Char(face, static_cast<FT_ULong>(ch), FT_LOAD_RENDER))
  {
    throw std::runtime_error(std20::format("Could not load face for '{}'.", static_cast<char>(ch)));
  }

  // now, draw to our target surface (convert position)
  drawBitmap(&slot->bitmap, slot->bitmap_left, screenHeight - slot->bitmap_top, buff);

  advance = toStdPixelCoord(slot->advance.x);
}

// A horizontal pixel span generated by the FreeType renderer.
struct TextDraw::TextDrawImpl::Vec2
{
  Vec2(const int a, const int b) : x{static_cast<float>(a)}, y{static_cast<float>(b)} {}
  float x;
  float y;
};

struct TextDraw::TextDrawImpl::Rect
{
  Rect(const int left, const int top, const int right, const int bottom)
    : xmin{static_cast<float>(left)},
      xmax{static_cast<float>(right)},
      ymin{static_cast<float>(top)},
      ymax{static_cast<float>(bottom)}
  {
  }

  void Include(const Vec2& r)
  {
    xmin = std::min(xmin, r.x);
    ymin = std::min(ymin, r.y);
    xmax = std::max(xmax, r.x);
    ymax = std::max(ymax, r.y);
  }

  [[maybe_unused]] [[nodiscard]] float Width() const { return xmax - xmin + 1; }
  [[maybe_unused]] [[nodiscard]] float Height() const { return ymax - ymin + 1; }

  [[nodiscard]] int xMin() const { return static_cast<int>(std::lround(xmin)); }
  [[nodiscard]] int yMin() const { return static_cast<int>(std::lround(ymin)); }

private:
  float xmin;
  float xmax;
  float ymin;
  float ymax;
};

struct TextDraw::TextDrawImpl::Span
{
  Span(const int _x, const int _y, const int _width, const int _coverage)
    : x{_x}, y{_y}, width{_width}, coverage{_coverage}
  {
  }

  int x;
  int y;
  int width;
  int coverage;
};

// Render the specified character as a colored glyph with a colored outline.
void TextDraw::TextDrawImpl::writeGlyph(const wchar_t ch,
                                        FT_Face& face,
                                        const int xPen,
                                        const int yPen,
                                        int& advance,
                                        PixelBuffer& buff) const
{
  // Set the size to use.
  if (FT_Set_Char_Size(face, toFreeTypeCoord(fontSize), toFreeTypeCoord(fontSize),
                       horizontalResolution, verticalResolution) != 0)
  {
    throw std::runtime_error(std20::format("Could set font size to {}.", fontSize));
  }

  // Load the glyph we are looking for.
  const FT_UInt gindex = FT_Get_Char_Index(face, static_cast<FT_ULong>(ch));
  if (FT_Load_Glyph(face, gindex, FT_LOAD_NO_BITMAP) != 0)
  {
    throw std::runtime_error(std20::format("Could load font for glyph index {}.", gindex));
  }

  // Need an outline for this to work.
  if (face->glyph->format != FT_GLYPH_FORMAT_OUTLINE)
  {
    throw std::runtime_error(std20::format("Not a correct font format: {}.", face->glyph->format));
  }

  // Render the basic glyph to a span list.
  const Spans spans = getSpans(face);
  if (spans.empty())
  {
    return;
  }

  // Next we need the spans for the outline.
  const Spans outlineSpans = getOutlineSpans(face);

  // Now we need to put it all together.

  // Figure out what the bounding rect is for both the span lists.
  const Rect rect = getBoundingRect(outlineSpans, spans);

  // Loop over the outline spans and just draw them into the image.
  writeSpansToImage(outlineSpans, rect, xPen, yPen, outlineColor, buff);

  // Then loop over the regular glyph spans and blend them into the image.
  writeSpansToImage(spans, rect, xPen, yPen, fontColor, buff);

#if 0
  // This is unused in this test but you would need this to draw
  // more than one glyph.
  float bearingX = face->glyph->metrics.horiBearingX >> 6;
  float bearingY = face->glyph->metrics.horiBearingY >> 6;
  float advance = face->glyph->advance.x >> 6;
#endif

  advance = toStdPixelCoord(face->glyph->advance.x);
}

// Each time the renderer calls us back we just push another span entry on our list.
void TextDraw::TextDrawImpl::RasterCallback(const int y,
                                            const int count,
                                            const FT_Span* const spans,
                                            void* const user)
{
  auto* userSpans = static_cast<Spans*>(user);
  for (int i = 0; i < count; ++i)
  {
    userSpans->push_back(Span(spans[i].x, y, spans[i].len, spans[i].coverage));
  }
}

// Set up the raster parameters and render the outline.
void TextDraw::TextDrawImpl::RenderSpans(FT_Outline* const outline, Spans* const spans) const
{
  FT_Raster_Params params;
  memset(&params, 0, sizeof(params));
  params.flags = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT;
  params.gray_spans = RasterCallback;
  params.user = spans;

  FT_Outline_Render(library, outline, &params);
}

TextDraw::TextDrawImpl::Rect TextDraw::TextDrawImpl::getBoundingRect(const Spans& outlineSpans,
                                                                     const Spans& spans)
{
  Rect rect{spans.front().x, spans.front().y, spans.front().x, spans.front().y};

  for (const auto& s : spans)
  {
    rect.Include(Vec2{s.x, s.y});
    rect.Include(Vec2{s.x + s.width - 1, s.y});
  }
  for (const auto& s : outlineSpans)
  {
    rect.Include(Vec2{s.x, s.y});
    rect.Include(Vec2{s.x + s.width - 1, s.y});
  }

  return rect;
}

void TextDraw::TextDrawImpl::writeSpansToImage(const Spans& spans,
                                               const Rect& rect,
                                               const int xPen,
                                               const int yPen,
                                               const Pixel& color,
                                               PixelBuffer& buff) const
{
  for (const auto& s : spans)
  {
    const int yPos = screenHeight - (yPen + s.y - rect.yMin());
    if (yPos < 0 || yPos >= screenHeight)
    {
      continue;
    }
    const Pixel srceColor{
        {.r = color.r(), .g = color.g(), .b = color.b(), .a = static_cast<uint8_t>(s.coverage)}};

    const int xPos0 = xPen + s.x - rect.xMin();
    for (int w = 0; w < s.width; ++w)
    {
      const int xPos = xPos0 + w;
      if (xPos < 0 || xPos >= screenWidth)
      {
        continue;
      }

      Pixel& destColor = buff(static_cast<size_t>(xPos), static_cast<size_t>(yPos));
      destColor = getColorBlend(srceColor, destColor);
    }
  }
}

TextDraw::TextDrawImpl::Spans TextDraw::TextDrawImpl::getSpans(FT_Face& face) const
{
  Spans spans{};

  RenderSpans(&face->glyph->outline, &spans);

  return spans;
}

TextDraw::TextDrawImpl::Spans TextDraw::TextDrawImpl::getOutlineSpans(FT_Face& face) const
{
  // Set up a stroker.
  FT_Stroker stroker{};
  FT_Stroker_New(library, &stroker);
  FT_Stroker_Set(stroker, toFreeTypeCoord(outlineWidth), FT_STROKER_LINECAP_ROUND,
                 FT_STROKER_LINEJOIN_ROUND, 0);

  FT_Glyph glyph{};
  if (FT_Get_Glyph(face->glyph, &glyph) != 0)
  {
    throw std::runtime_error("Could not get glyph for outline spans.");
  }

  // Next we need the spans for the outline.
  FT_Glyph_StrokeBorder(&glyph, stroker, false, true);

  // Again, this needs to be an outline to work.
  if (glyph->format != FT_GLYPH_FORMAT_OUTLINE)
  {
    throw std::runtime_error("Glyph does not have outline format.");
  }

  // Render the outline spans to the span list
  FT_Outline* outline = &reinterpret_cast<FT_OutlineGlyph>(glyph)->outline;
  Spans outlineSpans{};
  RenderSpans(outline, &outlineSpans);
  if (outlineSpans.empty())
  {
    throw std::runtime_error("Rendered outline spans are empty.");
  }

  // Clean up afterwards.
  FT_Stroker_Done(stroker);
  FT_Done_Glyph(glyph);

  return outlineSpans;
}

} // namespace goom
