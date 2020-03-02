#ifndef LIBS_UTILS_INCLUDE_UTILS_BUFFER_SAVERS_H_
  #define LIBS_UTILS_INCLUDE_UTILS_BUFFER_SAVERS_H_

  #include "fmt/format.h"
  #include <string>
  #include <memory>
  #include <stdexcept>
  #include <ostream>
  #include <fstream>


  template <class T> void WriteBinaryArray(
    const std::string& filename, long tag, const T* buffer, size_t bufferLen)
  {
    std::ofstream f(filename, std::ios::out | std::ios::binary);
    // MORE HEADER STUFF
    f.write(reinterpret_cast<const char *>(&tag), sizeof(tag));
    f.write(reinterpret_cast<const char *>(&bufferLen), sizeof(bufferLen));
    f.write(reinterpret_cast<const char *>(buffer), static_cast<std::streamsize>(bufferLen*sizeof(T)));
  }


  template <class T> bool ReadBinaryArray(const std::string& filename, long& tag, T* buffer, size_t& bufferLen)
  {
    std::ifstream f(filename, std::ios::in | std::ios::binary);
    if (!f.good()) {
      return false;
    }
    long tag1;
    f.read(reinterpret_cast<char *>(&tag1), sizeof(tag1));
    tag = tag1;
    size_t bufferLen1;
    f.read(reinterpret_cast<char *>(&bufferLen1), sizeof(bufferLen1));
    bufferLen = bufferLen1;
    f.read(reinterpret_cast<char *>(buffer), static_cast<std::streamsize>(bufferLen*sizeof(T)));
    return true;
  }


  template <class T> void WriteFormattedArray(
    const std::string& filename, long tag, const T* buffer, size_t bufferLen)
  {
    std::ofstream f(filename, std::ios::out);

    // MORE HEADER STUFF
    f << "tag: " << tag << std::endl;
    for (size_t i = 0; i < bufferLen; i++) {
      f << buffer[i];
      if (i < bufferLen-1) {
        f << ", ";
      }
      f << std::endl;
    }
    f << std::endl;
  }


  template <class T> bool ReadFormattedArray(const std::string& filename, 
    [[maybe_unused]] long& tag, [[maybe_unused]] T* buffer, [[maybe_unused]] size_t& bufferLen)
  {
    std::ifstream f(filename, std::ios::in);
    return false;
  }


  template <class T> class BufferSaver {
  public:
    BufferSaver(const std::string& filenamePrefix, 
      bool forWriting, size_t bufferLen, long startBuffNum=0, long endBuffNum=1000000);
    void ResetBufferLen(int newBuffLen);

    std::string getCurrentFilename() const;
    long getCurrentBufferNum() const;
    long getStartBufferNum() const;
    long getEndBufferNum() const;
    const std::shared_ptr<T> getNewBuffer() const;

    void Write(const T* buffer, bool binaryFormat);
    bool Read(T* buffer, bool binaryFormat);

  private:
    const std::string m_filenamePrefix;
    const bool m_forWriting;
    size_t m_bufferLen;  
    size_t m_bufferSize;
    const long m_startBuffNum;
    const long m_endBuffNum;
    long m_currentBuffNum;
  };

  template<class T> inline BufferSaver<T>::BufferSaver(
    const std::string& filenamePrefix, bool forWriting, 
    size_t bufferLen, long startBuffNum, long endBuffNum)
  : m_filenamePrefix { filenamePrefix },
    m_forWriting { forWriting },
    m_bufferLen { bufferLen },
    m_bufferSize { bufferLen*sizeof(T) },
    m_startBuffNum { startBuffNum },
    m_endBuffNum { endBuffNum },
    m_currentBuffNum { startBuffNum }
  {
  }

  template<class T> void BufferSaver<T>::ResetBufferLen(int newBuffLen) 
  {
    m_bufferLen = newBuffLen;
    m_bufferSize = m_bufferLen*sizeof(T);
  }

template<class T> inline const std::shared_ptr<T> BufferSaver<T>::getNewBuffer() const 
  {
    return std::shared_ptr<T>(new T[m_bufferLen], std::default_delete<T[]>());
  }

  template<class T> inline std::string BufferSaver<T>::getCurrentFilename() const 
  {
    return fmt::format(m_filenamePrefix + "_{:05}", m_currentBuffNum);
  }

  template<class T> inline long BufferSaver<T>::getCurrentBufferNum() const 
  {
    return m_currentBuffNum;
  }

  template<class T> inline long BufferSaver<T>::getStartBufferNum() const
  {
    return m_startBuffNum;
  }

  template<class T> inline long BufferSaver<T>::getEndBufferNum() const
  {
    return m_endBuffNum;
  }

  template<class T> void BufferSaver<T>::Write(const T* buffer, bool binaryFormat)
  {
    if (!m_forWriting) {
      throw std::runtime_error("Cannot write to a 'reading' buffer saver.");
    }
    if ((m_currentBuffNum < m_startBuffNum) || (m_currentBuffNum > m_endBuffNum)) {
      return;
    }

    const std::string filename = getCurrentFilename();
    if (binaryFormat) {
      WriteBinaryArray(filename, m_currentBuffNum, buffer, m_bufferLen);
    } else {
      WriteFormattedArray(filename, m_currentBuffNum, buffer, m_bufferLen);
    }
    m_currentBuffNum++;
  }

  template<class T> bool BufferSaver<T>::Read(T* buffer, bool binaryFormat)
  {
    if (m_forWriting) {
      throw std::runtime_error("Cannot read from a 'writing' buffer saver.");
    }
    if ((m_currentBuffNum < m_startBuffNum) || (m_currentBuffNum > m_endBuffNum)) {
      return false;
    }
  
    const std::string filename = getCurrentFilename();
	bool done;
    long tag;
    size_t actualBuffLen;
    if (binaryFormat) {
      done = ReadBinaryArray(filename, tag, buffer, actualBuffLen);
    } else {
      done = ReadFormattedArray(filename, tag, buffer, actualBuffLen);
    }

    if (!done) {
      return false;
    }
    if (m_currentBuffNum != tag) {
      throw std::runtime_error(fmt::format("Read Buffer from file '{}': "
        "expected buffer number {}, but buffer number read = {}", filename, m_currentBuffNum, tag));
    }
    if (m_bufferLen != actualBuffLen) {
      throw std::runtime_error(fmt::format("Read Buffer from file '{}': "
        "expected buffer length {}, but actual buffer length read = {}", filename, m_bufferLen, actualBuffLen));
    }

    m_currentBuffNum++;

    return true;
  }

#endif
