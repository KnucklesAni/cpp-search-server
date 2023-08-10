#pragma once
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <ostream>
#include <set>
#include <string>
#include <utility>

#include "search_server.h"

template <typename T>
void AssertImpl(const T &t, const std::string &t_str, const std::string &file,
                const std::string &func, unsigned line,
                const std::string &hint) {
  if (!t) {
    std::cerr << std::boolalpha;
    std::cerr << file << "(" << line << "): " << func << ": ";
    std::cerr << "ASSERT(" << t_str << ") failed.";
    if (!hint.empty()) {
      std::cerr << " Hint: " << hint;
    }
    std::cerr << std::endl;
    abort();
  }
}

#define ASSERT(a) AssertImpl((a), #a, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(a, hint)                                                   \
  AssertImpl((a), #a, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename Container>
std::ostream &Print(std::ostream &out, const Container &container) {
  for (const auto &element : container) {
    if (&element != &*begin(container)) {
      out << ", ";
    }
    out << element;
  }
  return out;
}

template <typename Element>
std::ostream &operator<<(std::ostream &out,
                         const std::set<Element> &container) {
  out << "{";
  return Print(out, container) << "}";
}

template <typename Element>
std::ostream &operator<<(std::ostream &out,
                         const std::vector<Element> &container) {
  out << "[";
  return Print(out, container) << "]";
}

template <typename Key, typename Element>
std::ostream &operator<<(std::ostream &out,
                         const std::map<Key, Element> &container) {
  out << "{";
  return Print(out, container) << "}";
}

template <typename Key, typename Element>
std::ostream &operator<<(std::ostream &out,
                         const std::pair<Key, Element> &container) {
  out << container.first << ": " << container.second;
  return out;
}

template <typename T, typename U>
void AssertEqualImpl(const T &t, const U &u, const std::string &t_str,
                     const std::string &u_str, const std::string &file,
                     const std::string &func, unsigned line,
                     const std::string &hint) {
  if (t != u) {
    std::cerr << std::boolalpha;
    std::cerr << file << "(" << line << "): " << func << ": ";
    std::cerr << "ASSERT_EQUAL(" << t_str << ", " << u_str << ") failed: ";
    std::cerr << t << " != " << u << ".";
    if (!hint.empty()) {
      std::cerr << " Hint: " << hint;
    }
    std::cerr << std::endl;
    abort();
  }
}

#define ASSERT_EQUAL(a, b)                                                     \
  AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, "")

#define ASSERT_EQUAL_HINT(a, b, hint)                                          \
  AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename Func>
void RunTestImpl(const Func &func, const std::string &func_str) {
  func();
  std::cerr << func_str << " OK" << std::endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func)

std::ostream &operator<<(std::ostream &os,
                         const DocumentStatus &document_status) {
  switch (document_status) {
  case DocumentStatus::ACTUAL:
    os << "ACTUAL";
    break;
  case DocumentStatus::IRRELEVANT:
    os << "IRRELEVANT";
    break;
  case DocumentStatus::BANNED:
    os << "BANNED";
    break;
  case DocumentStatus::REMOVED:
    os << "REMOVED";
    break;
  }
  return os;
}