#pragma once

#include <algorithm>
#include <set>
#include <string>
#include <vector>

std::string ReadLine();

int ReadLineWithNumber();

template <typename StringType>
std::vector<std::decay_t<StringType>> SplitIntoWords(StringType &&text) {
  std::vector<std::decay_t<StringType>> words;
  typename std::decay_t<StringType>::const_pointer start = &text.front();
  for (const char &c : text) {
    if (c == ' ') {
      if (start != &c) {
        words.push_back(StringType{start, size_t(&c - start)});
      }
      start = &c + 1;
    }
  }
  if (start != &*text.end()) {
    words.push_back(StringType{start, size_t(&*text.end() - start)});
  }

  return words;
}

template <typename StringContainer>
std::set<std::string, std::less<>>
MakeUniqueNonEmptyStrings(const StringContainer &strings) {
  std::set<std::string, std::less<>> non_empty_strings;
  for (const auto &str : strings) {
    if (!str.empty()) {
      non_empty_strings.insert(std::string(str));
    }
  }
  return non_empty_strings;
}