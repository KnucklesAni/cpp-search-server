#pragma once

#include <deque>
#include <string>
#include <vector>

#include "search_server.h"

class RequestQueue {
public:
  explicit RequestQueue(const SearchServer &search_server);
  template <typename DocumentPredicate>
  std::vector<Document> AddFindRequest(const std::string &raw_query,
                                       DocumentPredicate document_predicate);

  std::vector<Document> AddFindRequest(const std::string &raw_query,
                                       DocumentStatus status);
  std::vector<Document> AddFindRequest(const std::string &raw_query);
  int GetNoResultRequests() const;

private:
  void AddResultToQuery(const std::vector<Document> &result);
  struct QueryResult {
    bool empty;
  };
  std::deque<QueryResult> requests_;
  int empty_results_ = 0;
  const static int min_in_day_ = 1440;
  const SearchServer &search_server_;
};
template <typename DocumentPredicate>
std::vector<Document>
RequestQueue ::AddFindRequest(const std::string &raw_query,
                              DocumentPredicate document_predicate) {
  auto result = search_server_.FindTopDocuments(raw_query, document_predicate);
  AddResultToQuery(result);
  return result;
}