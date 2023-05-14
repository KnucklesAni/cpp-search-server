#include "request_queue.h"

using namespace std;

RequestQueue ::RequestQueue(const SearchServer &search_server)
    : search_server_(search_server) {}

vector<Document> RequestQueue ::AddFindRequest(const string &raw_query,
                                               DocumentStatus status) {
  auto result = search_server_.FindTopDocuments(raw_query, status);
  AddResultToQuery(result);
  return result;
}
vector<Document> RequestQueue ::AddFindRequest(const string &raw_query) {
  auto result = search_server_.FindTopDocuments(raw_query);
  AddResultToQuery(result);
  return result;
}
int RequestQueue ::GetNoResultRequests() const { return empty_results_; }

void RequestQueue ::AddResultToQuery(const vector<Document> &result) {
  if (requests_.size() == min_in_day_) {
    if (requests_.front().empty) {
      empty_results_--;
    }
    requests_.pop_front();
  }
  if (result.empty()) {
    empty_results_++;
    requests_.push_back(QueryResult{/*empty=*/true});
  } else {
    requests_.push_back(QueryResult{/*empty=*/false});
  }
}
