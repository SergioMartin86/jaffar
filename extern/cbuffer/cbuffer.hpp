#pragma once

/** \file
* @brief Implements a circular buffer with automatic overwrite on full
*        by Sergio Martin (2020), partially based on the implementation
*        by Jose Herrera https://gist.github.com/xstherrera1987/3196485
******************************************************************************/

#include <memory>
#include <vector>

/**
* \class cBuffer
* @brief This class defines a circular buffer with overwrite policy on add
*/
template <typename T>
class cBuffer
{
  private:
  /**
  * @brief Size of buffer container
  */
  size_t _maxSize;

  /**
  * @brief Number of elements already added
  */
  size_t _size;

  /**
  * @brief Container for data
  */
  std::unique_ptr<T[]> _data;

  /**
   * @brief Position of the start of the buffer
   */
  size_t _start;

  /**
   * @brief Position of the end of the buffer
   */
  size_t _end;

  public:
  /**
   * @brief Default constructor
   */
  cBuffer()
  {
    _maxSize = 0;
    _size = 0;
    _start = 0;
    _end = 0;
  };

  /**
   * @brief Constructor with a specific size
   * @param size The buffer size
   */
  cBuffer(size_t size)
  {
    resize(size);
  };

  /**
  * @brief Returns the current number of elements in the buffer
  * @return The number of elements
  */
  size_t size() { return _size; };

  /**
  * @brief Returns the current number of elements in the buffer
  * @param maxSize The buffer size
  */
  void resize(size_t maxSize)
  {
    _data = std::make_unique<T[]>(maxSize);

    _size = 0;
    _maxSize = maxSize;
    _start = 0;
    _end = 0;
  }

  /**
  * @brief Adds an element to the buffer
  * @param v The element to add
  */
  void add(const T &v)
  {
    // Storing value
    _data[_end] = v;

    // Increasing size until we reach the max size
    if (_size < _maxSize) _size++;

    // Increasing end pointer, and continuing from beginning if exceeding size
    _end++;
    if (_end == _maxSize) _end = 0;

    // If the buffer is full, the _start pointer follows the end pointer
    if (_size == _maxSize) _start = _end;
  }

  /**
  * @brief Returns the elements of the buffer in a vector format
  * @return The vector with the circular buffer elements
  */
  std::vector<T> getVector()
  {
    std::vector<T> vector(_size);
    for (size_t i = 0; i < _size; i++) vector[i] = (*this)[i];
    return vector;
  }

  /**
  * @brief Eliminates all contents of the buffer
  */
  void clear()
  {
    _size = 0;
    _start = 0;
    _end = 0;
  }

  /**
  * @brief Accesses an element at the required position
  * @param pos The access position
  * @return The element corresponding to the position
  */
  T &operator[](size_t pos)
  {
    return _data[(_start + pos) % _maxSize];
  }
};


