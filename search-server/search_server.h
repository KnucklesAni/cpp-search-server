#pragma once

#include <algorithm> //std::sort
#include <cmath>     //natural log, DBL_EPSILON
#include <deque>
#include <execution>
#include <float.h>
#include <iostream>
#include <map>
#include <numeric> //std::accumulate
#include <set>
#include <string>
#include <vector>

#include "concurrent_map.h"
#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"
#ifndef _MAX_RESULT_DOCUMENT_COUNT_
#define _MAX_RESULT_DOCUMENT_COUNT_
const int MAX_RESULT_DOCUMENT_COUNT = 5; // Used in the FindTopDocuments
#endif                                   // !_MAX_RESULT_DOCUMENT_COUNT_

// static const double DBL_EPSILON = 2.2e-016; //In the latter standarts this
// variable is defined in the <cmath>

// This function is used to check if two doubles are equal within a margine of
// an _DBL_EPSILON = 2.2e-016
bool AlmostEqualRelative(double A, double B, double maxRelDiff = DBL_EPSILON);

class SearchServer {
public:
  explicit SearchServer(const std::string &text);
  explicit SearchServer(const std::string_view text);
  template <typename ContainerT>
  explicit SearchServer(const ContainerT &container);

  // Input: document id, line of words we are planning to add to the document,
  // document status(ACTUAL, IRRELEVANT, BANNED, REMODED), vector of ratings
  void AddDocument(int document_id, const std::string_view document,
                   DocumentStatus status, const std::vector<int> &ratings);
  void RemoveDocument(int document_id);
  void RemoveDocument(const std::execution::sequenced_policy &,
                      int document_id);
  void RemoveDocument(const std::execution::parallel_policy &, int document_id);

  // Input: query of words (line) we are searching for, predicate, results are
  // saved in result Predicate is used to filter documents in FindAllDocuments
  template <typename PredicateT>
  std::vector<Document> FindTopDocuments(const std::string_view raw_query,
                                         PredicateT predicate) const;
  std::vector<Document> FindTopDocuments(const std::string_view raw_query,
                                         DocumentStatus status) const;
  std::vector<Document>
  FindTopDocuments(const std::string_view raw_query) const;

  template <typename ExecutionPolicy, typename PredicateT>
  std::vector<Document> FindTopDocuments(ExecutionPolicy &&,
                                         const std::string_view raw_query,
                                         PredicateT predicate) const;
  template <typename ExecutionPolicy>
  std::vector<Document> FindTopDocuments(ExecutionPolicy &&,
                                         const std::string_view raw_query,
                                         DocumentStatus status) const;
  template <typename ExecutionPolicy>
  std::vector<Document>
  FindTopDocuments(ExecutionPolicy &&, const std::string_view raw_query) const;

  int GetDocumentCount() const;
  const std::map<std::string_view, double> &
  GetWordFrequencies(int document_id) const;
  std::set<int>::const_iterator begin();
  std::set<int>::const_iterator end();

  // Input: raw query (line of words), document id
  // Output: vector of matched words, document status
  std::tuple<std::vector<std::string_view>, DocumentStatus>
  MatchDocument(const std::string_view raw_query, int document_id) const;
  std::tuple<std::vector<std::string_view>, DocumentStatus>
  MatchDocument(const std::execution::sequenced_policy &,
                const std::string_view raw_query, int document_id) const;
  std::tuple<std::vector<std::string_view>, DocumentStatus>
  MatchDocument(const std::execution::parallel_policy &,
                const std::string_view raw_query, int document_id) const;

private:
  struct DocumentData {
    int rating;
    DocumentStatus status;
  };

  struct QueryWord {
    std::string_view data;
    bool is_minus;
  };

  struct Query {
    std::vector<std::string_view> plus_words;
    std::vector<std::string_view> minus_words;
  };

  std::set<std::string, std::less<>> stop_words_;
  std::map<std::string_view, std::map<int, double>>
      word_to_document_freqs_; // key - words, value - map of (document_id, TF)
  std::map<int, std::map<std::string_view, double>>
      id_to_word_freqs_; // key - document_id, value - map of (word, TF)
  std::map<int, DocumentData>
      documents_; // key - document_id, value - struct of (rating, status)
  std::set<int> document_ids_;
  std::deque<std::string> storage_; // Every document's word storage

  bool IsStopWord(const std::string &word) const;
  bool IsStopWord(const std::string_view word) const;

  // A valid word must not contain special characters(in the halfinterval of
  // ['\0', ' '))
  static bool IsValidWord(const std::string &word);
  static bool IsValidWord(const std::string_view word);

  // Input: line of words, splitting them to vector, ignoring stop_words
  std::vector<std::string> SplitIntoWordsNoStop(const std::string &text) const;
  std::vector<std::string_view>
  SplitIntoWordsNoStop(std::string_view str) const;

  // Input: vector of ratings, output: average rating
  static int ComputeAverageRating(const std::vector<int> &ratings);

  // Input: word, if first character is '-', remove it, add is_minus flag to the
  // word
  QueryWord ParseQueryWord(std::string_view word) const;

  template <typename ExecutionPolicy>
  Query ParseQuery(ExecutionPolicy &&, const std::string_view text) const;
  /*Query ParseQuerySeq(const std::string_view text) const;
  Query ParseQueryPar(const std::string_view text) const;*/

  // Calculating IDF as log(number of documents / number of documents with word
  // encountered in them Input: word we are calculating IDF for
  double ComputeWordInverseDocumentFreq(const std::string_view word) const;

  // Finding all of the documents of the given status
  // Input: query (line of words), predicate function object, output: vector of
  // documents Predicate is receiving (document_id, status, rating) returning
  // bool, used to filter documents
  template <typename PredicateT>
  std::vector<Document> FindAllDocuments(const Query &query,
                                         PredicateT predicate) const;
  template <typename PredicateT>
  std::vector<Document>
  FindAllDocuments(const std::execution::sequenced_policy &, const Query &query,
                   PredicateT predicate) const;
  template <typename PredicateT>
  std::vector<Document>
  FindAllDocuments(const std::execution::parallel_policy &, const Query &query,
                   PredicateT predicate) const;
};

template <typename ContainerT>
SearchServer::SearchServer(const ContainerT &container) {
  for (const std::string &word : container) {
    if (!IsValidWord(word)) {
      throw std::invalid_argument("Invalid symbols in the list of stop words.");
    }
    if (!word.empty()) {
      stop_words_.insert(word);
    }
  }
}

template <typename PredicateT>
std::vector<Document>
SearchServer::FindTopDocuments(const std::string_view raw_query,
                               PredicateT predicate) const {
  return FindTopDocuments(std::execution::seq, raw_query, predicate);
}

template <typename ExecutionPolicy, typename PredicateT>
std::vector<Document>
SearchServer::FindTopDocuments(ExecutionPolicy &&pol,
                               const std::string_view raw_query,
                               PredicateT predicate) const {
  std::vector<Document> result;
  if (raw_query.empty()) {
    return result;
  }
  Query query = ParseQuery(std::execution::seq, raw_query);
  result = FindAllDocuments(pol, query, predicate);

  std::sort(std::execution::par, std::begin(result), std::end(result),
            [](const Document &lhs, const Document &rhs) {
              if (AlmostEqualRelative(lhs.relevance, rhs.relevance)) {
                return lhs.rating > rhs.rating;
              } else {
                return lhs.relevance > rhs.relevance;
              }
            });
  if (result.size() > MAX_RESULT_DOCUMENT_COUNT) {
    result.resize(MAX_RESULT_DOCUMENT_COUNT);
  }
  return result;
}

template <typename ExecutionPolicy>
std::vector<Document>
SearchServer::FindTopDocuments(ExecutionPolicy &&pol,
                               const std::string_view raw_query) const {
  const auto lambda = [](int, DocumentStatus status, int) {
    return status == DocumentStatus::ACTUAL;
  };
  return FindTopDocuments(pol, raw_query, lambda);
}

template <typename ExecutionPolicy>
std::vector<Document>
SearchServer::FindTopDocuments(ExecutionPolicy &&pol,
                               const std::string_view raw_query,
                               DocumentStatus status) const {

  const auto lambda = [status](int, DocumentStatus status1, int) {
    return status1 == status;
  };

  return FindTopDocuments(pol, raw_query, lambda);
}

template <typename ExecutionPolicy>
SearchServer::Query
SearchServer::ParseQuery(ExecutionPolicy &&,
                         const std::string_view text) const {
  Query result;
  for (const std::string_view word : SplitIntoWordsNoStop(text)) {
    const QueryWord query_word = ParseQueryWord(word);
    if (query_word.is_minus) {
      result.minus_words.emplace_back(query_word.data);
    } else {
      result.plus_words.emplace_back(query_word.data);
    }
  }
  // Erasing duplicates from plus and minus words
  // Only for a sequenced policy
  if (std::is_same_v<std::decay_t<ExecutionPolicy>,
                     std::execution::sequenced_policy>) {
    std::sort(result.minus_words.begin(), result.minus_words.end());
    std::sort(result.plus_words.begin(), result.plus_words.end());
    const std::vector<std::string_view>::iterator last_minus =
        std::unique(result.minus_words.begin(), result.minus_words.end());
    const std::vector<std::string_view>::iterator last_plus =
        std::unique(result.plus_words.begin(), result.plus_words.end());
    result.minus_words.erase(last_minus, result.minus_words.end());
    result.plus_words.erase(last_plus, result.plus_words.end());
  }
  return result;
}

template <typename PredicateT>
std::vector<Document>
SearchServer::FindAllDocuments(const Query &query, PredicateT predicate) const {
  return FindAllDocuments(std::execution::seq, query, predicate);
}

template <typename PredicateT>
std::vector<Document>
SearchServer::FindAllDocuments(const std::execution::sequenced_policy &,
                               const Query &query, PredicateT predicate) const {
  std::map<int, double>
      document_to_relevance; // Key - document ID, value - relevance
  std::for_each(
      std::execution::seq, query.plus_words.begin(), query.plus_words.end(),
      [&, predicate](const std::string_view &word) {
        if (word_to_document_freqs_.count(word)) {
          const double inverse_document_freq =
              ComputeWordInverseDocumentFreq(word);
          for (const auto [document_id, term_freq] :
               word_to_document_freqs_.at(word)) {
            const DocumentData &current_document =
                documents_.at(document_id); // Adding reference to call .at()
                                            // method once, instead of 2 times
            if (predicate(document_id, current_document.status,
                          current_document.rating)) {
              document_to_relevance[document_id] +=
                  term_freq * inverse_document_freq;
            }
          }
        }
      });

  std::for_each(std::execution::seq, query.minus_words.begin(),
                query.minus_words.end(), [&](const std::string_view &word) {
                  if (word_to_document_freqs_.count(word)) {
                    for (const auto [document_id, _] :
                         word_to_document_freqs_.at(word)) {
                      document_to_relevance.erase(document_id);
                    }
                  }
                });
  std::vector<Document> matched_documents;
  for (const auto [document_id, relevance] : document_to_relevance) {
    matched_documents.push_back(
        {document_id, relevance,
         documents_.at(document_id)
             .rating}); // Moving everything from index to the vector<Document>
  }
  return matched_documents;
}

template <typename PredicateT>
std::vector<Document>
SearchServer::FindAllDocuments(const std::execution::parallel_policy &,
                               const Query &query, PredicateT predicate) const {
  static int BUCKET_COUNT = 8;
  ConcurrentMap<int, double> document_to_relevance(
      BUCKET_COUNT); // Key - document ID, value - relevance

  std::for_each(
      std::execution::par, query.plus_words.begin(), query.plus_words.end(),
      [&, predicate](const std::string_view &word) {
        if (word_to_document_freqs_.count(word)) {
          const double inverse_document_freq =
              ComputeWordInverseDocumentFreq(word);
          for (const auto [document_id, term_freq] :
               word_to_document_freqs_.at(word)) {
            const DocumentData &current_document =
                documents_.at(document_id); // Adding reference to call .at()
                                            // method once, instead of 2 times
            if (predicate(document_id, current_document.status,
                          current_document.rating)) {
              document_to_relevance[document_id].ref_to_value +=
                  term_freq * inverse_document_freq;
            }
          }
        }
      });

  std::for_each(std::execution::par, query.minus_words.begin(),
                query.minus_words.end(), [&](const std::string_view &word) {
                  if (word_to_document_freqs_.count(word)) {
                    for (const auto [document_id, _] :
                         word_to_document_freqs_.at(word)) {
                      document_to_relevance.Erase(document_id);
                    }
                  }
                });

  std::vector<Document> matched_documents;
  for (const auto [document_id, relevance] :
       document_to_relevance.BuildOrdinaryMap()) {
    matched_documents.push_back(
        {document_id, relevance,
         documents_.at(document_id)
             .rating}); // Moving everything from index to the vector<Document>
  }
  return matched_documents;
}