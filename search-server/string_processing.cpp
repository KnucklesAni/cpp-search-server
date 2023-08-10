
#include "string_processing.h"

// Input: line of text, Output: vector of the words of that text
std::vector<std::string> SplitIntoWords(const std::string &text) {
  std::vector<std::string> words;
  std::string word;
  for (const char c : text) {
    if (c == ' ') {
      if (!word.empty()) {
        words.push_back(word);
        word.clear();
      }
    } else {
      word += c;
    }
  }
  if (!word.empty()) {
    words.push_back(word);
  }
  return words;
}

std::vector<std::string_view> SplitIntoWordsView(std::string_view str) {
  std::vector<std::string_view> result;
  while (true) {
    size_t space = str.find(' ');
    result.emplace_back(str.substr(0, space));
    if (space == str.npos) {
      break;
    } else {
      str.remove_prefix(space + 1);
    }
  }
  return result;
}
