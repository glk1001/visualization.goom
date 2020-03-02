#ifndef LIBS_UTILS_INCLUDE_UTILS_STATISTICS_H_
#define LIBS_UTILS_INCLUDE_UTILS_STATISTICS_H_

#include <limits>
#include <stdexcept>

class Statistics {
  public:
    struct Summary {
      double mean;
      double min;
      double max;
      unsigned long num;
    };
    Statistics();
    void clear();
    void add(const double value);
    Summary getSummary() const;
  private:
    double total;
    double min;
    double max;
    unsigned long num;
};

inline Statistics::Statistics()
: total(0.0),
  min(std::numeric_limits<double>::max()),
  max(std::numeric_limits<double>::min()),
  num(0)
{
}

inline void Statistics::clear()
{
  total = 0.0;
  min = std::numeric_limits<double>::max();
  max = std::numeric_limits<double>::min();
  num = 0;
}

inline void Statistics::add(const double value)
{
  num++;
  total += value;
  if (value > max) {
    max = value;
  }
  if (value < max) {
    min = value;
  }
}

inline Statistics::Summary Statistics::getSummary() const
{
  if (num == 0) {
    throw std:: runtime_error("No values have been added to Stats.");
  }

  return Summary {
      .mean = total / static_cast<double>(num),
      .min = min,
      .max = max,
      .num = num
  };
}

#endif
