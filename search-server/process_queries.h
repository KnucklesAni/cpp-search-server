#pragma once
#include "search_server.h"
#include <algorithm>
#include <functional>
#include <iterator>
#include <optional>

std::vector<std::vector<Document>>
ProcessQueries(const SearchServer &search_server,
               const std::vector<std::string> &queries);

template <template <typename...> typename Container, typename T>
class FlatIterator {
  using OuterIterator = typename Container<Container<T>>::iterator;
  using InnerIterator = typename Container<T>::iterator;
  static OuterIterator ValidOuter(OuterIterator outer, OuterIterator end) {
    while (outer != end && outer->size() == 0) {
      ++outer;
    }
    return outer;
  }

public:
  FlatIterator(OuterIterator outer, OuterIterator end)
      : outer_(ValidOuter(outer, end)),
        inner_(outer_ == end
                   ? std::optional<InnerIterator>{}
                   : std::optional<InnerIterator>{std::begin(*outer_)}),
        end_(end) {}
  bool operator==(FlatIterator other) const {
    return outer_ == other.outer_ && (outer_ == end_ || inner_ == other.inner_);
  }
  bool operator!=(FlatIterator other) const { return !(*this == other); }
  auto operator++() {
    ++*inner_;
    if (inner_ == std::end(*outer_)) {
      if (outer_ = ValidOuter(++outer_, end_); outer_ == end_) {
        *inner_ = {};
      } else {
        *inner_ = std::begin(*outer_);
      }
    }
    return *this;
  }
  auto operator++(int) {
    auto result = *this;
    operator++();
    return result;
  }
  decltype(auto) operator*() { return **inner_; }
  decltype(auto) operator-> () { return **inner_; }

private:
  OuterIterator outer_;
  std::optional<InnerIterator> inner_;
  OuterIterator end_;
};

template <template <typename...> typename Container, typename T> class Flatten {
  using OuterIterator = typename Container<Container<T>>::iterator;
  using InnerIterator = typename Container<T>::iterator;

public:
  Flatten(Container<Container<T>> &&container) : container_(container) {}
  FlatIterator<Container, T> begin() {
    return FlatIterator<Container, T>(std::begin(container_),
                                      std::end(container_));
  }
  FlatIterator<Container, T> end() {
    return FlatIterator<Container, T>(std::end(container_),
                                      std::end(container_));
  }

public:
  Container<Container<T>> container_;
};

namespace std {
template <template <typename...> typename Container, typename T>
struct iterator_traits<FlatIterator<Container, T>> {
  using difference_type = typename Container<Container<T>>::difference_type;
  using value_type = typename Container<Container<T>>::value_type;
  using pointer = typename Container<Container<T>>::pointer;
  using reference = typename Container<Container<T>>::reference;
  using iterator_category = std::forward_iterator_tag;
};
} // namespace std

Flatten<std::vector, Document>
ProcessQueriesJoined(const SearchServer &search_server,
                     const std::vector<std::string> &queries);

class SearchServer;
void RemoveDuplicates(SearchServer &search_server, bool silent);