#ifndef LIBS_GOOM_INCLUDE_GOOM_TENTACLES_H_
#define LIBS_GOOM_INCLUDE_GOOM_TENTACLES_H_

#include "goom_graphic.h"
#include "goomutils/colormap.h"
#include "goomutils/mathutils.h"

#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/types/vector.hpp>
#include <functional>
#include <memory>
#include <tuple>
#include <vector>
#include <vivid/vivid.h>

namespace goom
{

class TentacleColorizer
{
public:
  virtual ~TentacleColorizer() noexcept = default;
  virtual utils::ColorMapGroup getColorMapGroup() const = 0;
  virtual void setColorMapGroup(const utils::ColorMapGroup) = 0;
  virtual void changeColorMap() = 0;
  virtual Pixel getColor(const size_t nodeNum) const = 0;
};

class Tentacle2D
{
private:
  using XandYVectors = std::tuple<std::vector<double>&, std::vector<double>&>;

public:
  static constexpr size_t minNumNodes = 10;

  Tentacle2D() noexcept;
  explicit Tentacle2D(const size_t ID,
                      const size_t numNodes,
                      const double xmin,
                      const double xmax,
                      const double ymin,
                      const double ymax,
                      const double basePrevYWeight,
                      const double baseCurrentYWeight) noexcept;
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

  size_t getNumNodes() const;

  double getPrevYWeight() const;
  double getCurrentYWeight() const;

  double getIterZeroYVal() const;
  void setIterZeroYVal(const double val);

  double getIterZeroLerpFactor() const;
  void setIterZeroLerpFactor(const double val);

  bool getDoDamping() const;
  void setDoDamping(const bool val);

  bool operator==(const Tentacle2D&) const;

  template<class Archive>
  void serialize(Archive&);

private:
  size_t ID = 0;
  size_t numNodes = 0;
  double xmin = 0;
  double xmax = 0;
  double ymin = 0;
  double ymax = 0;
  double basePrevYWeight = 0.770;
  double baseCurrentYWeight = 0.230;
  double iterZeroYVal = 0.9;
  double iterZeroLerpFactor = 0.8;
  size_t iterNum = 0;
  bool startedIterating = false;
  std::vector<double> xvec{};
  std::vector<double> yvec{};
  XandYVectors vecs{std::make_tuple(std::ref(xvec), std::ref(yvec))};
  std::vector<double> dampedYVec{};
  std::vector<double> dampingCache{};
  XandYVectors dampedVecs{std::make_tuple(std::ref(xvec), std::ref(dampedYVec))};
  std::unique_ptr<utils::DampingFunction> dampingFunc{};
  bool doDamping = true;

  float getFirstY();
  float getNextY(const size_t nodeNum);
  double getDamping(const double x);
  float getDampedVal(const size_t nodeNum, const float val) const;
  void updateDampedVals(const std::vector<double>& yvals);
  double damp(const size_t nodeNum) const;
  void validateSettings() const;
  void validateXDimensions() const;
  void validateYDimensions() const;
  void validateNumNodes() const;
  void validatePrevYWeight() const;
  void validateCurrentYWeight() const;

  using DampingFuncPtr = std::unique_ptr<utils::DampingFunction>;
  static DampingFuncPtr createDampingFunc(const double prevYWeight,
                                          const double xmin,
                                          const double xmax);
  static DampingFuncPtr createExpDampingFunc(const double xmin, const double xmax);
  static DampingFuncPtr createLinearDampingFunc(const double xmin, const double xmax);
};

struct V3d
{
  float x = 0;
  float y = 0;
  float z = 0;
  bool ignore = false;

  bool operator==(const V3d&) const = default;

  template<class Archive>
  void serialize(Archive& ar)
  {
    ar(CEREAL_NVP(x), CEREAL_NVP(y), CEREAL_NVP(z), CEREAL_NVP(ignore));
  }
};

class Tentacle3D
{
public:
  Tentacle3D() noexcept = default;
  explicit Tentacle3D(std::unique_ptr<Tentacle2D>,
                      const Pixel& headColor,
                      const Pixel& headColorLow,
                      const V3d& head) noexcept;
  explicit Tentacle3D(std::unique_ptr<Tentacle2D>,
                      const std::shared_ptr<const TentacleColorizer>&,
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

  bool operator==(const Tentacle3D&) const;

  template<class Archive>
  void serialize(Archive&);

private:
  std::unique_ptr<Tentacle2D> tentacle{};
  std::shared_ptr<const TentacleColorizer> colorizer{};
  Pixel headColor{};
  Pixel headColorLow{};
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
  Tentacles3D() noexcept = default;

  void addTentacle(Tentacle3D&&);

  Iter begin() { return Iter(this, 0); }
  Iter end() { return Iter(this, tentacles.size()); }

  const Tentacle3D& operator[](const size_t i) const { return tentacles.at(i); }
  Tentacle3D& operator[](const size_t i) { return tentacles.at(i); }

  void setAllowOverexposed(const bool val);

  bool operator==(const Tentacles3D&) const;

  template<class Archive>
  void serialize(Archive&);

private:
  std::vector<Tentacle3D> tentacles{};
};

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

inline double Tentacle2D::getPrevYWeight() const
{
  return basePrevYWeight;
}

inline double Tentacle2D::getCurrentYWeight() const
{
  return baseCurrentYWeight;
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

inline double Tentacle2D::getYMin() const
{
  return ymin;
}
inline double Tentacle2D::getYMax() const
{
  return ymax;
}

inline bool Tentacle2D::getDoDamping() const
{
  return doDamping;
};
inline void Tentacle2D::setDoDamping(const bool val)
{
  doDamping = val;
};

inline double Tentacle2D::getDamping(const double x)
{
  return (*dampingFunc)(x);
}

inline void Tentacle2D::finishIterating()
{
  startedIterating = false;
}

template<class Archive>
void Tentacle2D::serialize(Archive& ar)
{
  ar(CEREAL_NVP(ID), CEREAL_NVP(numNodes), CEREAL_NVP(xmin), CEREAL_NVP(xmax), CEREAL_NVP(ymin),
     CEREAL_NVP(ymax), CEREAL_NVP(basePrevYWeight), CEREAL_NVP(baseCurrentYWeight),
     CEREAL_NVP(iterZeroYVal), CEREAL_NVP(iterZeroLerpFactor), CEREAL_NVP(iterNum),
     CEREAL_NVP(startedIterating), CEREAL_NVP(xvec), CEREAL_NVP(yvec), CEREAL_NVP(dampedYVec),
     CEREAL_NVP(dampingCache), CEREAL_NVP(dampingFunc), CEREAL_NVP(doDamping));
}

inline Tentacle3D& Tentacles3D::Iter::operator*() const
{
  return (*tentacles)[pos];
}

template<class Archive>
void Tentacle3D::serialize(Archive& ar)
{
  ar(CEREAL_NVP(tentacle), CEREAL_NVP(colorizer), CEREAL_NVP(headColor), CEREAL_NVP(headColorLow),
     CEREAL_NVP(head), CEREAL_NVP(reverseColorMix), CEREAL_NVP(allowOverexposed));
}

template<class Archive>
void Tentacles3D::serialize(Archive& ar)
{
  ar(CEREAL_NVP(tentacles));
}

} // namespace goom
#endif
