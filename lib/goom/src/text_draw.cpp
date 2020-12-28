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
  TextDrawImpl(uint32_t screenW, uint32_t screenH) noexcept;
  ~TextDrawImpl() noexcept;
  TextDrawImpl(const TextDrawImpl&) noexcept = delete;
  TextDrawImpl& operator=(const TextDrawImpl&) = delete;

  void setAlignment(TextAlignment);
  void setFont(const std::string& filename);
  void setFontSize(int val);
  void setOutlineWidth(float val);
  void setCharSpacing(float val);
  void setText(const std::string&);

  void setFontColorFunc(const FontColorFunc&);
  void setOutlineFontColorFunc(const FontColorFunc&);

  void prepare();
  [[nodiscard]] Rect getPreparedTextBoundingRect() const;
  [[nodiscard]] int getBearingX() const;
  [[nodiscard]] int getBearingY() const;

  void draw(int xPen, int yPen, int& xNext, int& yNext, PixelBuffer&);

private:
  const int screenWidth;
  const int screenHeight;
  FT_Library library{};
  int fontSize = 100;
  uint32_t horizontalResolution = 90;
  uint32_t verticalResolution = 90;
  float outlineWidth = 3;
  float charSpacing = 0;
  std::string fontFilename{};
  std::vector<unsigned char> fontBuffer{};
  std::string theText{};
  TextAlignment textAlignment{TextAlignment::left};
  FT_Face face{};
  void setFaceFontSize();

  FontColorFunc getFontColor{};
  FontColorFunc getOutlineFontColor{};

  static constexpr int toStdPixelCoord(int freeTypeCoord);
  static constexpr int toFreeTypeCoord(int stdPixelCoord);
  static constexpr int toFreeTypeCoord(float stdPixelCoord);
  struct Vec2;
  struct Span;
  using SpanArray = std::vector<Span>;

  struct RectImpl : public Rect
  {
    RectImpl() noexcept = default;
    RectImpl(int left, int top, int right, int bottom) noexcept;
    RectImpl(const RectImpl&) noexcept = default;

    void Include(const Vec2&);
  };

  struct Spans
  {
    SpanArray stdSpans{};
    SpanArray outlineSpans{};
    size_t textIndexOfChar{};
    RectImpl rect{};
    int advance{};
    int bearingX{};
    int bearingY{};
  };

  std::vector<Spans> textSpans{};
  Rect textBoundingRect{};
  static RectImpl getBoundingRect(const SpanArray& stdSpans, const SpanArray& outlineSpans);
  [[nodiscard]] Spans getSpans(size_t textIndexOfChar) const;
  [[nodiscard]] SpanArray getStdSpans() const;
  [[nodiscard]] SpanArray getOutlineSpans() const;
  void RenderSpans(FT_Outline* outline, SpanArray* spans) const;
  static void RasterCallback(int y, int count, const FT_Span* spans, void* user);

  [[nodiscard]] int getStartXPen(int xPen) const;
  [[nodiscard]] int getStartYPen(int yPen) const;
  void writeGlyph(const Spans&, int xPen, int yPen, PixelBuffer& buff) const;
  void writeSpansToImage(const SpanArray&,
                         const RectImpl&,
                         int xPen,
                         int yPen,
                         size_t textIndexOfChar,
                         const FontColorFunc& getColor,
                         PixelBuffer&) const;
};

TextDraw::TextDraw(const uint32_t screenW, const uint32_t screenH) noexcept
  : textDrawImpl{new TextDrawImpl{screenW, screenH}}
{
}

TextDraw::~TextDraw() noexcept = default;

void TextDraw::setAlignment(const TextAlignment a)
{
  textDrawImpl->setAlignment(a);
}

void TextDraw::setFont(const std::string& filename)
{
  textDrawImpl->setFont(filename);
}

void TextDraw::setFontSize(const int val)
{
  textDrawImpl->setFontSize(val);
}

void TextDraw::setOutlineWidth(const float val)
{
  textDrawImpl->setOutlineWidth(val);
}

void TextDraw::setCharSpacing(const float val)
{
  textDrawImpl->setCharSpacing(val);
}

void TextDraw::setText(const std::string& val)
{
  textDrawImpl->setText(val);
}

void TextDraw::setFontColorFunc(const FontColorFunc& val)
{
  textDrawImpl->setFontColorFunc(val);
}

void TextDraw::setOutlineFontColorFunc(const FontColorFunc& val)
{
  textDrawImpl->setOutlineFontColorFunc(val);
}

void TextDraw::prepare()
{
  textDrawImpl->prepare();
}

TextDraw::Rect TextDraw::getPreparedTextBoundingRect() const
{
  return textDrawImpl->getPreparedTextBoundingRect();
}

int TextDraw::getBearingX() const
{
  return textDrawImpl->getBearingX();
}

int TextDraw::getBearingY() const
{
  return textDrawImpl->getBearingY();
}

void TextDraw::draw(const int xPen, const int yPen, int& xNext, int& yNext, PixelBuffer& buffer)
{
  textDrawImpl->draw(xPen, yPen, xNext, yNext, buffer);
}

TextDraw::TextDrawImpl::TextDrawImpl(const uint32_t screenW, const uint32_t screenH) noexcept
  : screenWidth{static_cast<int>(screenW)}, screenHeight{static_cast<int>(screenH)}
{
  FT_Init_FreeType(&library);
}

TextDraw::TextDrawImpl::~TextDrawImpl() noexcept
{
  FT_Done_FreeType(library);
}

void TextDraw::TextDrawImpl::setAlignment(const TextAlignment a)
{
  textAlignment = a;
}

void TextDraw::TextDrawImpl::setFont(const std::string& filename)
{
  fontFilename = filename;

  std::ifstream fontFile(fontFilename, std::ios::binary);
  if (!fontFile)
  {
    throw std::runtime_error(std20::format("Could not open font file \"{}\".", fontFilename));
  }

  fontFile.seekg(0, std::ios::end);
  std::fstream::pos_type fontFileSize = fontFile.tellg();
  fontFile.seekg(0);
  fontBuffer.resize(static_cast<size_t>(fontFileSize));
  fontFile.read((char*)fontBuffer.data(), fontFileSize);

  // Create a face from a memory buffer.  Be sure not to delete the memory buffer
  // until we are done using that font as FreeType will reference it directly.
  FT_New_Memory_Face(library, fontBuffer.data(), static_cast<FT_Long>(fontBuffer.size()), 0, &face);

  setFaceFontSize();
}

void TextDraw::TextDrawImpl::setFaceFontSize()
{
  if (FT_Set_Char_Size(face, toFreeTypeCoord(fontSize), toFreeTypeCoord(fontSize),
                       horizontalResolution, verticalResolution) != 0)
  {
    throw std::logic_error(std20::format("Could not set face font size to {}.", fontSize));
  }
}

void TextDraw::TextDrawImpl::setFontSize(int val)
{
  if (val <= 0)
  {
    throw std::logic_error(std20::format("Font size <= 0: {}.", val));
  }

  fontSize = val;
  if (face)
  {
    setFaceFontSize();
  }
}

void TextDraw::TextDrawImpl::setOutlineWidth(float val)
{
  if (val <= 0)
  {
    throw std::logic_error(std20::format("Outline width <= 0: {}.", val));
  }
  outlineWidth = val;
}

void TextDraw::TextDrawImpl::setCharSpacing(const float val)
{
  if (val <= 0)
  {
    throw std::logic_error(std20::format("Char spacing <= 0: {}.", val));
  }
  charSpacing = val;
}

void TextDraw::TextDrawImpl::setText(const std::string& str)
{
  if (str.empty())
  {
    throw std::logic_error("Text string is empty.");
  }

  theText = str;
}

void TextDraw::TextDrawImpl::setFontColorFunc(const FontColorFunc& val)
{
  getFontColor = val;
}

void TextDraw::TextDrawImpl::setOutlineFontColorFunc(const FontColorFunc& val)
{
  getOutlineFontColor = val;
}

void TextDraw::TextDrawImpl::prepare()
{
  if (!face)
  {
    throw std::logic_error("Font face has not been set.");
  }

  textSpans.resize(0);

  int xmax = 0;
  int ymin = 100000;
  int ymax = 0;
  for (size_t i = 0; i < theText.size(); i++)
  {
    // Load the glyph we are looking for.
    const FT_UInt gIndex = FT_Get_Char_Index(face, static_cast<FT_ULong>(theText[i]));
    if (FT_Load_Glyph(face, gIndex, FT_LOAD_NO_BITMAP) != 0)
    {
      throw std::runtime_error(std20::format("Could not load font for glyph index {}.", gIndex));
    }

    // Need an outline for this to work.
    if (face->glyph->format != FT_GLYPH_FORMAT_OUTLINE)
    {
      throw std::logic_error(std20::format("Not a correct font format: {}.", face->glyph->format));
    }

    const Spans spans = getSpans(i);
    textSpans.emplace_back(spans);

    xmax += spans.advance;
    ymin = std::min(ymin, spans.rect.ymin);
    ymax = std::max(ymax, spans.rect.ymax);
  }

  textBoundingRect.xmin = 0;
  textBoundingRect.xmax = xmax;
  textBoundingRect.ymin = ymin;
  textBoundingRect.ymax = ymax;
}

int TextDraw::TextDrawImpl::getStartXPen(const int xPen) const
{
  switch (textAlignment)
  {
    case TextAlignment::left:
      return xPen;
    case TextAlignment::center:
      return xPen - (getPreparedTextBoundingRect().xmax - getPreparedTextBoundingRect().xmin) / 2;
    case TextAlignment::right:
      return xPen - (getPreparedTextBoundingRect().xmax - getPreparedTextBoundingRect().xmin);
    default:
      throw std::logic_error("Unknown TextAlignment value.");
  }
}

int TextDraw::TextDrawImpl::getStartYPen(const int yPen) const
{
  return yPen;
}

void TextDraw::TextDrawImpl::draw(
    const int xPen, const int yPen, int& xNext, int& yNext, PixelBuffer& buffer)
{
  if (textSpans.empty())
  {
    throw std::logic_error("textSpans is empty.");
  }

  xNext = getStartXPen(xPen);
  yNext = getStartYPen(yPen);

  for (auto& s : textSpans)
  {
    writeGlyph(s, xNext, screenHeight - yNext, buffer);
    xNext += s.advance;
  }
}

// A horizontal pixel span generated by the FreeType renderer.
struct TextDraw::TextDrawImpl::Vec2
{
  Vec2(const int a, const int b) : x{a}, y{b} {}
  int x;
  int y;
};

TextDraw::TextDrawImpl::RectImpl::RectImpl(const int left,
                                           const int top,
                                           const int right,
                                           const int bottom) noexcept
{
  xmin = left;
  xmax = right;
  ymin = top;
  ymax = bottom;
}

void TextDraw::TextDrawImpl::RectImpl::Include(const Vec2& span)
{
  xmin = std::min(xmin, span.x);
  ymin = std::min(ymin, span.y);
  xmax = std::max(xmax, span.x);
  ymax = std::max(ymax, span.y);
}

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
void TextDraw::TextDrawImpl::writeGlyph(const Spans& spans,
                                        const int xPen,
                                        const int yPen,
                                        PixelBuffer& buff) const
{
  // Loop over the outline spans and just draw them into the image.
  writeSpansToImage(spans.outlineSpans, spans.rect, xPen, yPen, spans.textIndexOfChar,
                    getOutlineFontColor, buff);

  // Then loop over the regular glyph spans and blend them into the image.
  writeSpansToImage(spans.stdSpans, spans.rect, xPen, yPen, spans.textIndexOfChar, getFontColor,
                    buff);
}

void TextDraw::TextDrawImpl::writeSpansToImage(const SpanArray& spans,
                                               const RectImpl& rect,
                                               const int xPen,
                                               const int yPen,
                                               const size_t textIndexOfChar,
                                               const FontColorFunc& getColor,
                                               PixelBuffer& buff) const
{
  for (const auto& s : spans)
  {
    const int yPos = screenHeight - (yPen + s.y - rect.ymin);
    if (yPos < 0 || yPos >= screenHeight)
    {
      continue;
    }

    const int xPos0 = xPen + s.x - rect.xmin;
    const int xf0 = s.x - rect.xmin;
    const auto coverage = static_cast<uint8_t>(s.coverage);
    for (int w = 0; w < s.width; ++w)
    {
      const int xPos = xPos0 + w;
      if (xPos < 0 || xPos >= screenWidth)
      {
        continue;
      }

      const Pixel color = getColor(textIndexOfChar, xf0 + w, rect.height() - (s.y - rect.ymin),
                                   rect.width(), rect.height());
      const Pixel srceColor{{.r = color.r(), .g = color.g(), .b = color.b(), .a = coverage}};

      Pixel& destColor = buff(static_cast<size_t>(xPos), static_cast<size_t>(yPos));
      destColor = getColorBlend(srceColor, destColor);
    }
  }
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

// Each time the renderer calls us back we just push another span entry on our list.
void TextDraw::TextDrawImpl::RasterCallback(const int y,
                                            const int count,
                                            const FT_Span* const spans,
                                            void* const user)
{
  auto* userSpans = static_cast<SpanArray*>(user);
  for (int i = 0; i < count; ++i)
  {
    userSpans->push_back(Span{spans[i].x, y, spans[i].len, spans[i].coverage});
  }
}

// Set up the raster parameters and render the outline.
void TextDraw::TextDrawImpl::RenderSpans(FT_Outline* const outline, SpanArray* const spans) const
{
  FT_Raster_Params params;
  memset(&params, 0, sizeof(params));
  params.flags = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT;
  params.gray_spans = RasterCallback;
  params.user = spans;

  FT_Outline_Render(library, outline, &params);
}

TextDraw::TextDrawImpl::Spans TextDraw::TextDrawImpl::getSpans(const size_t textIndexOfChar) const
{
  const SpanArray stdSpans = getStdSpans();
  const int advance = toStdPixelCoord(face->glyph->advance.x) +
                      static_cast<int>(charSpacing * static_cast<float>(fontSize));
  FT_Glyph_Metrics metrics = face->glyph->metrics;
  if (stdSpans.empty())
  {
    return Spans{
        .stdSpans = stdSpans,
        .outlineSpans = SpanArray{},
        .textIndexOfChar = textIndexOfChar,
        .rect = RectImpl{},
        .advance = advance,
        .bearingX = toStdPixelCoord(metrics.horiBearingX),
        .bearingY = toStdPixelCoord(metrics.horiBearingY),
    };
  }

  const SpanArray outlineSpans = getOutlineSpans();
  return Spans{
      .stdSpans = stdSpans,
      .outlineSpans = outlineSpans,
      .textIndexOfChar = textIndexOfChar,
      .rect = getBoundingRect(stdSpans, outlineSpans),
      .advance = advance,
      .bearingX = toStdPixelCoord(metrics.horiBearingX),
      .bearingY = toStdPixelCoord(metrics.horiBearingY),
  };
}

TextDraw::TextDrawImpl::SpanArray TextDraw::TextDrawImpl::getStdSpans() const
{
  SpanArray spans{};

  RenderSpans(&face->glyph->outline, &spans);

  return spans;
}

TextDraw::TextDrawImpl::SpanArray TextDraw::TextDrawImpl::getOutlineSpans() const
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
  SpanArray outlineSpans{};
  RenderSpans(outline, &outlineSpans);
  if (outlineSpans.empty())
  {
    throw std::logic_error("Rendered outline spans are empty.");
  }

  // Clean up afterwards.
  FT_Stroker_Done(stroker);
  FT_Done_Glyph(glyph);

  return outlineSpans;
}

TextDraw::Rect TextDraw::TextDrawImpl::getPreparedTextBoundingRect() const
{
  return textBoundingRect;
}

int TextDraw::TextDrawImpl::getBearingX() const
{
  return textSpans.front().bearingX;
}

int TextDraw::TextDrawImpl::getBearingY() const
{
  return textSpans.front().bearingY;
}

TextDraw::TextDrawImpl::RectImpl TextDraw::TextDrawImpl::getBoundingRect(
    const SpanArray& stdSpans, const SpanArray& outlineSpans)
{
  RectImpl rect{stdSpans.front().x, stdSpans.front().y, stdSpans.front().x, stdSpans.front().y};

  for (const auto& s : stdSpans)
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

/* SIMPLE TEXT
void TextDraw::TextDrawImpl::writeGlyphSimple(
    const wchar_t ch, const int xPen, const int yPen, int& advance, PixelBuffer& buff)
{
  // Set the size to use.
  if (FT_Set_Char_Size(face, toFreeTypeCoord(fontSize), toFreeTypeCoord(fontSize),
                       horizontalResolution, verticalResolution) != 0)
  {
    throw std::runtime_error(std20::format("Could not set font size to {}.", fontSize));
  }

  const float angle = (25.0F / 360.0F) * 3.14159F * 2; // use 25 degrees

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
  drawBitmap(&slot->bitmap, slot->bitmap_left, screenHeight - slot->bitmap_top, ch, buff);

  advance = toStdPixelCoord(slot->advance.x);
}

void TextDraw::TextDrawImpl::drawBitmap(
    FT_Bitmap* bitmap, const FT_Int x, const FT_Int y, const wchar_t ch, PixelBuffer& buffer) const
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
        buffer(static_cast<size_t>(i), static_cast<size_t>(j)) =
            getFontColor(ch, p, q, bitmap->width, bitmap->rows);
      }
    }
  }
}
*/

} // namespace goom
