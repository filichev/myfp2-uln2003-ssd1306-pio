// Minimal FIFO Queue compatible with myFocuserPro2 usage
// Provides: Queue<T>(capacity), count(), push(const T&), pop()
// Drops oldest element when full
#ifndef MY_QUEUE_H
#define MY_QUEUE_H

#include <Arduino.h>

template <typename T>
class Queue {
 public:
  explicit Queue(size_t capacity)
      : _capacity(capacity), _size(0), _head(0), _tail(0) {
    if (_capacity < 1) _capacity = 1;
    _data = new T[_capacity];
  }

  ~Queue() { delete[] _data; }

  size_t count() const { return _size; }
  bool isEmpty() const { return _size == 0; }

  void push(const T& item) {
    // If full, drop oldest to make room
    if (_size == _capacity) {
      _head = (_head + 1) % _capacity;
      _size--;
    }
    _data[_tail] = item;
    _tail = (_tail + 1) % _capacity;
    _size++;
  }

  T pop() {
    if (_size == 0) {
      return T();
    }
    T item = _data[_head];
    _head = (_head + 1) % _capacity;
    _size--;
    return item;
  }

 private:
  T* _data;
  size_t _capacity;
  size_t _size;
  size_t _head;
  size_t _tail;
};

#endif  // MY_QUEUE_H
