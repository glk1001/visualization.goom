#ifndef LIBS_GOOM_INCLUDE_GOOM_TENTACLES_H_
#define LIBS_GOOM_INCLUDE_GOOM_TENTACLES_H_

#include "goom_graphic.h"
#include "goomutils/colormap.h"
#include "goomutils/mathutils.h"

#include <functional>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <vector>
#include <vivid/vivid.h>

namespace goom
{

class TentacleColorizer
{
public:
  virtual ~TentacleColorizer() {}
  virtual utils::ColorMapGroup getColorMapGroup() const = 0;
  virtual void setColorMapGroup(const utils::ColorMapGroup) = 0;
  virtual void changeColorMap() = 0;
  virtual Pixel getColor(const size_t nodeNum) const = 0;
};

class TentacleTweaker
{
public:
public:
  explicit TentacleTweaker(std::unique_ptr<utils::DampingFunction> dampingFunc) noexcept;
  TentacleTweaker(const TentacleTweaker&) = delete;
  TentacleTweaker& operator=(const TentacleTweaker&) = delete;

  double getDamping(const double x);
  void setDampingFunc(utils::DampingFunction& dampingFunc);

private:
  std::unique_ptr<utils::DampingFunction> dampingFunc;
};

class Tentacle2D
{
private:
  using XandYVectors = std::tuple<std::vector<double>&, std::vector<double>&>;

public:
  static constexpr size_t minNumNodes = 10;

  explicit Tentacle2D(const size_t ID, std::unique_ptr<TentacleTweaker> tweaker) noexcept;
  Tentacle2D(const Tentacle2D&) = delete;
  Tentacle2D& operator=(const Tentacle2D&) = delete;

  size_t getID() const;

  void startIterating();
  void finishIterating();

  size_t getIterNum() const;
  void iterate();
  void iterateNTimes(const size_t n);

  const XandYVectors& getXandYVectors() const;
  const XandYVectors& getDampedXandYVectors() const;

  double getLength() const;

  double getXMin() const;
  double getXMax() const;
  void setXDimensions(const double x0, const double y0);

  double getYMin() const;
  double getYMax() const;
  void setYDimensions(const double y0, const double y1);

  size_t getNumNodes() const;
  void setNumNodes(const size_t val);

  double getPrevYWeight() const;
  void setPrevYWeight(const double val);

  double getCurrentYWeight() const;
  void setCurrentYWeight(const double val);

  double getIterZeroYVal() const;
  void setIterZeroYVal(const double val);

  double getIterZeroLerpFactor() const;
  void setIterZeroLerpFactor(const double val);

  bool getDoDamping() const;
  void setDoDamping(const bool val);

private:
  size_t ID{};
  std::unique_ptr<TentacleTweaker> tweaker{};
  size_t iterNum = 0;
  size_t numNodes = 0;
  bool startedIterating = false;
  std::vector<double> xvec{};
  std::vector<double> yvec{};
  XandYVectors vecs{std::make_tuple(std::ref(xvec), std::ref(yvec))};
  std::vector<double> dampedYVec{};
  std::vector<double> dampingCache{};
  XandYVectors dampedVecs{std::make_tuple(std::ref(xvec), std::ref(dampedYVec))};
  double xmin = 0;
  double xmax = 0;
  double ymin = 0;
  double ymax = 0;
  double basePrevYWeight = 0.770;
  double baseCurrentYWeight = 0.230;
  double iterZeroYVal = 0.9;
  double iterZeroLerpFactor = 0.8;
  bool doDamping = true;

  float getFirstY();
  float getNextY(const size_t nodeNum);
  float getDampedVal(const size_t nodeNum, const float val) const;
  void updateDampedVals(const std::vector<double>& yvals);
  double damp(const size_t nodeNum) const;
  void validateSettings() const;
  void validateXDimensions() const;
  void validateYDimensions() const;
  void validateNumNodes() const;
  void validatePrevYWeight() const;
  void validateCurrentYWeight() const;
};

struct V3d
{
  float x = 0;
  float y = 0;
  float z = 0;
  bool ignore = false;
};

class Tentacle3D
{
public:
  explicit Tentacle3D(std::unique_ptr<Tentacle2D>,
                      const Pixel& headColor,
                      const Pixel& headColorLow,
                      const V3d& head) noexcept;
  explicit Tentacle3D(std::unique_ptr<Tentacle2D>,
                      const TentacleColorizer& colorizer,
                      const Pixel& headColor,
                      const Pixel& headColorLow,
                      const V3d& head) noexcept;
  Tentacle3D(Tentacle3D&&) noexcept;
  Tentacle3D(const Tentacle3D&) = delete;
  Tentacle3D& operator=(const Tentacle3D&) = delete;

  Tentacle2D& get2DTentacle() { return *tentacle; }
  const Tentacle2D& get2DTentacle() const { return *tentacle; }

  size_t getID() const { return tentacle->getID(); }

  Pixel getColor(const size_t nodeNum) const;
  std::tuple<Pixel, Pixel> getMixedColors(const size_t nodeNum,
                                          const Pixel& color,
                                          const Pixel& colorLow) const;
  std::tuple<Pixel, Pixel> getMixedColors(const size_t nodeNum,
                                          const Pixel& color,
                                          const Pixel& colorLow,
                                          const float brightness) const;

  bool getReverseColorMix() const { return reverseColorMix; }
  void setReverseColorMix(const bool val) { reverseColorMix = val; }

  bool getAllowOverexposed() const { return allowOverexposed; }
  void setAllowOverexposed(const bool val) { allowOverexposed = val; }

  const V3d& getHead() const { return head; }
  void setHead(const V3d& val) { head = val; }

  std::vector<V3d> getVertices() const;

private:
  std::unique_ptr<Tentacle2D> tentacle{};
  const TentacleColorizer* const colorizer = nullptr;
  const Pixel headColor{};
  const Pixel headColorLow{};
  V3d head{};
  bool reverseColorMix = false;
  bool allowOverexposed = true;
};

class Tentacles3D
{
private:
  class Iter
  {
  public:
    Iter(Tentacles3D* const tents, const size_t p) : pos(p), tentacles(tents) {}
    bool operator!=(const Iter& other) const { return pos != other.pos; }
    Tentacle3D& operator*() const;
    const Iter& operator++()
    {
      ++pos;
      return *this;
    }

  private:
    size_t pos;
    Tentacles3D* tentacles;
  };

public:
  Tentacles3D() noexcept;

  void addTentacle(Tentacle3D&&);

  Iter begin() { return Iter(this, 0); }
  Iter end() { return Iter(this, tentacles.size()); }

  const Tentacle3D& operator[](const size_t i) const { return tentacles.at(i); }
  Tentacle3D& operator[](const size_t i) { return tentacles.at(i); }

  void setAllowOverexposed(const bool val);

private:
  std::vector<Tentacle3D> tentacles{};
};

inline double TentacleTweaker::getDamping(const double x)
{
  return (*dampingFunc)(x);
}

inline size_t Tentacle2D::getID() const
{
  return ID;
}

inline double Tentacle2D::getIterZeroYVal() const
{
  return iterZeroYVal;
}
inline double Tentacle2D::getIterZeroLerpFactor() const
{
  return iterZeroLerpFactor;
}

inline void Tentacle2D::setIterZeroYVal(const double val)
{
  iterZeroYVal = val;
}

inline void Tentacle2D::setIterZeroLerpFactor(const double val)
{
  iterZeroLerpFactor = val;
}

inline size_t Tentacle2D::getNumNodes() const
{
  return numNodes;
}

inline void Tentacle2D::setNumNodes(const size_t val)
{
  if (startedIterating)
  {
    throw std::runtime_error("Can't set numNodes dimensions after iteration start.");
  }

  numNodes = val;
  validateNumNodes();
}

inline double Tentacle2D::getPrevYWeight() const
{
  return basePrevYWeight;
}

inline void Tentacle2D::setPrevYWeight(const double val)
{
  basePrevYWeight = val;
  validatePrevYWeight();
}

inline double Tentacle2D::getCurrentYWeight() const
{
  return baseCurrentYWeight;
}

inline void Tentacle2D::setCurrentYWeight(const double val)
{
  baseCurrentYWeight = val;
  validateCurrentYWeight();
}

inline size_t Tentacle2D::getIterNum() const
{
  return iterNum;
}

inline double Tentacle2D::getLength() const
{
  return xmax - xmin;
}

inline double Tentacle2D::getXMin() const
{
  return xmin;
}
inline double Tentacle2D::getXMax() const
{
  return xmax;
}

inline void Tentacle2D::setXDimensions(const double x0, const double x1)
{
  if (startedIterating)
  {
    throw std::runtime_error("Can't set x dimensions after iteration start.");
  }

  xmin = x0;
  xmax = x1;
  validateXDimensions();
}

inline double Tentacle2D::getYMin() const
{
  return ymin;
}
inline double Tentacle2D::getYMax() const
{
  return ymax;
}

inline void Tentacle2D::setYDimensions(const double y0, const double y1)
{
  if (startedIterating)
  {
    throw std::runtime_error("Can't set y dimensions after iteration start.");
  }

  ymin = y0;
  ymax = y1;
  validateYDimensions();
}

inline bool Tentacle2D::getDoDamping() const
{
  return doDamping;
};
inline void Tentacle2D::setDoDamping(const bool val)
{
  doDamping = val;
};

inline void Tentacle2D::finishIterating()
{
  startedIterating = false;
}

inline Tentacle3D& Tentacles3D::Iter::operator*() const
{
  return (*tentacles)[pos];
}

} // namespace goom
#endif
