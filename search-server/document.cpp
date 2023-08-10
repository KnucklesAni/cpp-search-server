#include <iostream>
#include <string>

#include "document.h"

void PrintDocument(const Document &document) {
  std::cout << document << std::endl;
}

std::ostream &operator<<(std::ostream &os, const Document &document) {
  using namespace std::literals;
  os << "{ "s
     << "document_id = "s << document.id << ", "s
     << "relevance = "s << document.relevance << ", "s
     << "rating = "s << document.rating << " }"s;
  return os;
}