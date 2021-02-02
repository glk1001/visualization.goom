#include "image_fx.h"

#include "goom_draw.h"
#include "goom_graphic.h"
#include "goom_plugin_info.h"
#include "goom_visual_fx.h"
#include "goomutils/colormaps.h"
#include "goomutils/colorutils.h"
#include "goomutils/image_bitmaps.h"
#include "goomutils/logging_control.h"
#undef NO_LOGGING
#include "goomutils/goomrand.h"
#include "goomutils/logging.h"
#include "v2d.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace GOOM
{

using namespace GOOM::UTILS;

class ImageFx::ImageFxImpl
{
public:
  ImageFxImpl() noexcept;
  explicit ImageFxImpl(const std::shared_ptr<const PluginInfo>& info) noexcept;
  ~ImageFxImpl() noexcept = default;
  ImageFxImpl(const ImageFxImpl&) noexcept = delete;
  ImageFxImpl(ImageFxImpl&&) noexcept = delete;
  auto operator=(const ImageFxImpl&) -> ImageFxImpl& = delete;
  auto operator=(ImageFxImpl&&) -> ImageFxImpl& = delete;

  [[nodiscard]] auto GetResourcesDirectory() const -> const std::string&;
  void SetResourcesDirectory(const std::string& dirName);

  void SetBuffSettings(const FXBuffSettings& settings);

  void Start();

  void Apply(PixelBuffer& currentBuff);

  auto operator==(const ImageFxImpl& i) const -> bool;

private:
  std::shared_ptr<const PluginInfo> m_goomInfo{};
  std::string m_resourcesDirectory{};
  FXBuffSettings m_buffSettings{};
  GoomDraw m_draw{};
  ImageBitmap m_image{};
  GammaCorrection m_gammaCorrect{2.0, 0.01};
  auto GetNextImageCentre() -> V2dInt;
};

ImageFx::ImageFx() noexcept : m_fxImpl{new ImageFxImpl{}}
{
}

ImageFx::ImageFx(const std::shared_ptr<const PluginInfo>& info) noexcept
  : m_fxImpl{new ImageFxImpl{info}}
{
}

ImageFx::~ImageFx() noexcept = default;

auto ImageFx::operator==(const ImageFx& i) const -> bool
{
  return m_fxImpl->operator==(*i.m_fxImpl);
}

auto ImageFx::GetResourcesDirectory() const -> const std::string&
{
  return m_fxImpl->GetResourcesDirectory();
}

void ImageFx::SetResourcesDirectory(const std::string& dirName)
{
  m_fxImpl->SetResourcesDirectory(dirName);
}

void ImageFx::SetBuffSettings(const FXBuffSettings& settings)
{
  m_fxImpl->SetBuffSettings(settings);
}

void ImageFx::Start()
{
  m_fxImpl->Start();
}

void ImageFx::Log([[maybe_unused]] const StatsLogValueFunc& l) const
{
}

void ImageFx::Finish()
{
}

auto ImageFx::GetFxName() const -> std::string
{
  return "image";
}

void ImageFx::Apply(PixelBuffer& currentBuff)
{
  if (!m_enabled)
  {
    return;
  }
  m_fxImpl->Apply(currentBuff);
}

ImageFx::ImageFxImpl::ImageFxImpl() noexcept = default;

ImageFx::ImageFxImpl::ImageFxImpl(const std::shared_ptr<const PluginInfo>& info) noexcept
  : m_goomInfo(info), m_draw{m_goomInfo->GetScreenInfo().width, m_goomInfo->GetScreenInfo().height}
{
}

auto ImageFx::ImageFxImpl::operator==(const ImageFx::ImageFxImpl& i) const -> bool
{
  return true;
}

void ImageFx::ImageFxImpl::Start()
{
  m_image.SetFilename(m_resourcesDirectory + "/" + "mountain_sunset.png");
  m_image.Load();
}

auto ImageFx::ImageFxImpl::GetResourcesDirectory() const -> const std::string&
{
  return m_resourcesDirectory;
}

void ImageFx::ImageFxImpl::SetResourcesDirectory(const std::string& dirName)
{
  m_resourcesDirectory = dirName;
}

void ImageFx::ImageFxImpl::SetBuffSettings(const FXBuffSettings& settings)
{
  m_buffSettings = settings;
  m_draw.SetBuffIntensity(m_buffSettings.buffIntensity);
  m_draw.SetAllowOverexposed(m_buffSettings.allowOverexposed);
}

void ImageFx::ImageFxImpl::Apply(PixelBuffer& currentBuff)
{
  constexpr float BRIGHTNESS = 0.2F;
  const auto getColor = [&]([[maybe_unused]] const int x, [[maybe_unused]] const int y,
                            const Pixel& b) -> Pixel {
    return GetBrighterColor(BRIGHTNESS, b, m_buffSettings.allowOverexposed);
  };

  const V2dInt imageLocation = GetNextImageCentre();
  m_draw.Bitmap(currentBuff, imageLocation.x, imageLocation.y, m_image, getColor);
}

auto ImageFx::ImageFxImpl::GetNextImageCentre() -> V2dInt
{
  const auto buffCentreX = static_cast<int>(m_goomInfo->GetScreenInfo().width / 2);
  const auto buffCentreY = static_cast<int>(m_goomInfo->GetScreenInfo().height / 2);
  return V2dInt{buffCentreX, buffCentreY};
  const int x = GetRandInRange(buffCentreX - 10, buffCentreX + 10);
  const int y = GetRandInRange(buffCentreY - 10, buffCentreY + 10);
  return V2dInt{x, y};
}

} // namespace GOOM
