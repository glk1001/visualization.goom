/*
 *      Copyright (C) 2019-2020 Team Kodi
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <algorithm>
#include <vector>

constexpr int SILENCE_THRESHOLD = 8;

template<typename T>
class CircularBuffer
{
public:
  explicit CircularBuffer(unsigned p_size) : m_size{p_size} { m_buffer.resize(p_size); }

  auto DataAvailable() -> unsigned { return m_used; }
  auto FreeSpace() -> unsigned { return m_size - m_used; }

  auto Write(const T* src, unsigned count) -> bool
  {
    if (count > FreeSpace())
    {
      return false;
    }
    while (count)
    {
      unsigned delta = m_size - m_writePtr;
      if (delta > count)
      {
        delta = count;
      }
      std::copy(src, src + delta, m_buffer.begin() + m_writePtr);
      m_used += delta;
      m_writePtr = (m_writePtr + delta) % m_size;
      src += delta;
      count -= delta;
    }
    return true;
  }

  auto Read(T* dst, unsigned count) -> unsigned
  {
    unsigned done = 0;
    for (;;)
    {
      unsigned delta = m_size - m_readptr;
      if (delta > m_used)
      {
        delta = m_used;
      }
      if (delta > count)
      {
        delta = count;
      }
      if (!delta)
      {
        break;
      }

      std::copy(m_buffer.begin() + m_readptr, m_buffer.begin() + m_readptr + delta, dst);
      dst += delta;
      done += delta;
      m_readptr = (m_readptr + delta) % m_size;
      count -= delta;
      m_used -= delta;
    }
    return done;
  }

  void Reset() { m_readptr = m_writePtr = m_used = 0; }

  void Resize(unsigned p_size)
  {
    m_size = p_size;
    m_buffer.resize(p_size);
    Reset();
  }

  [[nodiscard]] auto TestSilence() const -> bool
  {
    T* begin = (T*)&m_buffer[0];
    T first = *begin;
    *begin = SILENCE_THRESHOLD * 2;
    T* p = begin + m_size;
    while ((unsigned)(*--p + SILENCE_THRESHOLD) <= (unsigned)SILENCE_THRESHOLD * 2)
    {
    }
    *begin = first;
    return p == begin && ((unsigned)(first + SILENCE_THRESHOLD) <= (unsigned)SILENCE_THRESHOLD * 2);
  }

private:
  std::vector<T> m_buffer{};
  unsigned m_readptr = 0;
  unsigned m_writePtr = 0;
  unsigned m_used = 0;
  unsigned m_size;
};
