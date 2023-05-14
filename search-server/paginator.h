#pragma once
#include <cstddef>
#include <sstream>
template <typename BeginIteratorType, typename EndIteratorType>
class Paginator {
public:
  Paginator(BeginIteratorType begin, EndIteratorType end, size_t page_size)
      : begin_(begin), end_(end), page_size_(page_size) {}
  auto begin() const { return Iterator{begin_, end_, page_size_}; }
  auto end() const { return end_; }

private:
  struct Iterator {
    BeginIteratorType begin;
    EndIteratorType end;
    size_t page_size;
    bool operator==(EndIteratorType other) const { return begin == other; }
    bool operator!=(EndIteratorType other) const { return begin != other; }
    auto operator++() {
      for (size_t count = 0; begin != end && count < page_size;
           count++, begin++) {
      }
      return *this;
    }
    auto operator++(int) {
      auto result = *this;
      for (size_t count = 0; begin != end && count < page_size;
           count++, begin++) {
      }
      return result;
    }
    auto operator*() {
      std::stringstream result;
      auto current = begin;
      for (size_t count = 0; current != end && count < page_size;
           count++, current++) {
        result << "{ document_id = " << current->id
               << ", relevance = " << current->relevance
               << ", rating = " << current->rating << " }";
      }
      return result.str();
    }
  };
  BeginIteratorType begin_;
  EndIteratorType end_;
  size_t page_size_;
};
template <typename Container>
auto Paginate(const Container &c, size_t page_size) {
  return Paginator(begin(c), end(c), page_size);
}