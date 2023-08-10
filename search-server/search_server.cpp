#include "read_input_functions.h"
#include "search_server.h"
#include "string_processing.h"

bool AlmostEqualRelative(double A, double B, double maxRelDiff) {
  // Calculate the difference.
  double diff = fabs(A - B);
  A = fabs(A);
  B = fabs(B);
  // Find the largest
  double largest = (B > A) ? B : A;

  if (diff <= largest * maxRelDiff) {
    return true;
  }
  return false;
}

SearchServer::SearchServer(const std::string &text) {
  for (const std::string &word : SplitIntoWords(text)) {
    if (!IsValidWord(word)) {
      throw std::invalid_argument("Invalid symbols in the list of stop words.");
    }
    if (!word.empty()) {
      stop_words_.insert(word);
    }
  }
}

SearchServer::SearchServer(const std::string_view text) {
  for (const std::string_view word : SplitIntoWords(text)) {
    if (!IsValidWord(word)) {
      throw std::invalid_argument("Invalid symbols in the list of stop words.");
    }
    if (!word.empty()) {
      stop_words_.insert(std::string(word));
    }
  }
}

// Input: document id, line of words we are planning to add to the document,
// document status(ACTUAL, IRRELEVANT, BANNED, REMODED), vector of ratings
void SearchServer::AddDocument(int document_id, const std::string_view document,
                               DocumentStatus status,
                               const std::vector<int> &ratings) {
  if (document_id < 0) {
    throw std::invalid_argument("Invalid document ID.");
  }
  if (documents_.count(document_id) > 0) {
    throw std::invalid_argument(
        "Document with the given ID is already existing.");
  }
  storage_.emplace_back(document);
  document_ids_.insert(document_id);
  const std::vector<std::string_view> words =
      SplitIntoWordsNoStop(std::string_view{storage_.back()});
  const double inv_word_count = 1.0 / words.size();
  for (const std::string_view word : words) {
    if (!IsStopWord(word)) {
      word_to_document_freqs_[word][document_id] += inv_word_count;
      id_to_word_freqs_[document_id][word] += inv_word_count;
    }
  }
  documents_.emplace(document_id,
                     DocumentData{ComputeAverageRating(ratings), status});
}

void SearchServer::RemoveDocument(int document_id) {
  RemoveDocument(std::execution::seq, document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy &,
                                  int document_id) {
  // Trying to delete unexisting document.
  if (!id_to_word_freqs_.count(document_id)) {
    return;
  }
  // Clearing the set of document IDs
  document_ids_.erase(document_id);
  // Clearing the word to doc_ID_freqs index
  for (const auto &[word, _] : id_to_word_freqs_.at(document_id)) {
    std::map<int, double> &document_freqs = word_to_document_freqs_.at(word);
    document_freqs.erase(document_id);
    // If the word is empty, erasing the word itself
    if (document_freqs.empty()) {
      word_to_document_freqs_.erase(word);
    }
  }
  // Clearing the ID to word_freqs index
  id_to_word_freqs_.erase(document_id);
  // Clearing documents map
  documents_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy &,
                                  int document_id) {
  // Trying to delete unexisting document.
  if (!id_to_word_freqs_.count(document_id)) {
    return;
  }
  // Clearing the set of document IDs
  document_ids_.erase(document_id);
  // Clearing the word to doc_ID_freqs index
  std::map<std::string_view, double> &words_ref =
      id_to_word_freqs_.at(document_id);
  std::for_each(std::execution::par, words_ref.begin(), words_ref.end(),
                [&](const std::pair<std::string_view, double> &word_TF) {
                  word_to_document_freqs_.at(word_TF.first).erase(document_id);
                });
  // Clearing the ID to word_freqs index
  id_to_word_freqs_.erase(document_id);
  // Clearing documents map
  documents_.erase(document_id);
}

std::vector<Document>
SearchServer::FindTopDocuments(const std::string_view raw_query,
                               DocumentStatus status) const {
  const auto lambda = [status](int, DocumentStatus status1, int) {
    return status1 == status;
  };
  return FindTopDocuments(raw_query, lambda);
}

std::vector<Document>
SearchServer::FindTopDocuments(const std::string_view raw_query) const {
  const auto lambda = [](int, DocumentStatus status, int) {
    return status == DocumentStatus::ACTUAL;
  };
  return FindTopDocuments(raw_query, lambda);
}

int SearchServer::GetDocumentCount() const {
  return static_cast<int>(documents_.size());
}

// Returns reference to a map of words and their TFs of the current document
const std::map<std::string_view, double> &
SearchServer::GetWordFrequencies(int document_id) const {
  static std::map<std::string_view, double> empty_map;
  if (id_to_word_freqs_.count(document_id) == 0) {
    return empty_map;
  }
  return id_to_word_freqs_.at(document_id);
}

std::set<int>::const_iterator SearchServer::begin() {
  return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() {
  return document_ids_.end();
}

// Input: raw query (line of words), document id
// Output: vector of matched words, document status
std::tuple<std::vector<std::string_view>, DocumentStatus>
SearchServer::MatchDocument(const std::string_view raw_query,
                            int document_id) const {
  const DocumentStatus status = documents_.at(document_id).status;
  std::vector<std::string_view> matched_words;
  if (raw_query.empty()) {
    return tie(matched_words, status);
  }

  const Query query = ParseQuery(std::execution::seq, raw_query);
  // Refs for easier readability
  const std::vector<std::string_view> &plus_words = query.plus_words;
  const std::vector<std::string_view> &minus_words = query.minus_words;

  if (std::any_of(minus_words.begin(), minus_words.end(),
                  [&](const std::string_view &word) {
                    return word_to_document_freqs_.count(word) > 0 &&
                           word_to_document_freqs_.at(word).count(
                               document_id) != 0;
                  })) {
    return tie(matched_words, status);
  }
  std::for_each(
      plus_words.begin(), plus_words.end(), [&](const std::string_view word) {
        if (word_to_document_freqs_.count(word) != 0 &&
            word_to_document_freqs_.at(word).count(document_id) != 0) {
          matched_words.emplace_back(word);
        }
      });
  return tie(matched_words, status);
}

std::tuple<std::vector<std::string_view>, DocumentStatus>
SearchServer::MatchDocument(const std::execution::sequenced_policy &,
                            const std::string_view raw_query,
                            int document_id) const {
  return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus>
SearchServer::MatchDocument(const std::execution::parallel_policy &,
                            const std::string_view raw_query,
                            int document_id) const {
  Query query = ParseQuery(std::execution::par, raw_query);

  // Refs for easier readability
  const DocumentStatus status = documents_.at(document_id).status;
  const std::vector<std::string_view> &plus_words = query.plus_words;
  const std::vector<std::string_view> &minus_words = query.minus_words;

  std::vector<std::string_view> matched_words;
  // If there are any minus_words in the document, return empty result
  // It seems like there are too few minus_words, using ::par here only slows
  // algorythm
  if (std::any_of(/*std::execution::par,*/
                  minus_words.begin(), minus_words.end(),
                  [&](const std::string_view &word) {
                    return word_to_document_freqs_.count(word) > 0 &&
                           word_to_document_freqs_.at(word).count(document_id) >
                               0;
                  })) {
    return tie(matched_words, status);
  }
  matched_words.resize(plus_words.size());
  const std::vector<std::string_view>::iterator end_copy_it = std::copy_if(
      std::execution::par,
      // I tested both copy and move, and copy is faster in general
      /*std::make_move_iterator(plus_words.begin()),
      std::make_move_iterator(plus_words.end()),*/
      plus_words.begin(), plus_words.end(), matched_words.begin(),
      [&](const std::string_view word) {
        return word_to_document_freqs_.count(word) > 0 &&
               word_to_document_freqs_.at(word).count(document_id) > 0;
      });
  // Removing duplicates
  matched_words.resize(std::distance(matched_words.begin(), end_copy_it));
  std::sort(matched_words.begin(), matched_words.end());
  const std::vector<std::string_view>::iterator end_unique_it =
      std::unique(matched_words.begin(), matched_words.end());
  matched_words.erase(end_unique_it, matched_words.end());

  return tie(matched_words, status);
}

bool SearchServer::IsStopWord(const std::string &word) const {
  return stop_words_.count(word) > 0;
}

bool SearchServer::IsStopWord(const std::string_view word) const {
  return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string &word) {
  // A valid word must not contain special characters
  return std::none_of(word.begin(), word.end(),
                      [](char c) { return c >= '\0' && c < ' '; });
}

bool SearchServer::IsValidWord(const std::string_view word) {
  // A valid word must not contain special characters
  return std::none_of(word.begin(), word.end(),
                      [](char c) { return c >= '\0' && c < ' '; });
}

// Input: line of words, splitting them to vector, ignoring stop_words
std::vector<std::string>
SearchServer::SplitIntoWordsNoStop(const std::string &text) const {
  std::vector<std::string> result;
  for (const std::string &word : SplitIntoWords(text)) {
    if (!IsValidWord(word)) {
      throw std::invalid_argument(
          "Invalid query. Special symbols in the query.");
    }
    if (!IsStopWord(word)) {
      result.push_back(word);
    }
  }
  return result;
}

std::vector<std::string_view>
SearchServer::SplitIntoWordsNoStop(std::string_view str) const {
  std::vector<std::string_view> result;
  while (true) {
    size_t space = str.find(' ');
    const std::string_view &word = str.substr(0, space);
    if (!IsValidWord(word)) {
      throw std::invalid_argument(
          "Invalid query. Special symbols in the query.");
    }
    if (!IsStopWord(word)) {
      result.push_back(word);
    }
    if (space == str.npos) {
      break;
    } else {
      str.remove_prefix(space + 1);
    }
  }
  return result;
}

// Input: vector of ratings, output: average rating
int SearchServer::ComputeAverageRating(const std::vector<int> &ratings) {
  if (ratings.empty()) {
    return 0;
  }
  int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);
  return rating_sum / static_cast<int>(ratings.size());
}

// Input: word, if first character is '-', remove it, add is_minus flag to the
// word
SearchServer::QueryWord
SearchServer::ParseQueryWord(std::string_view word) const {
  bool is_minus = false;
  if (word.empty()) {
    return {word, is_minus};
  }
  // Word shouldn't be empty
  if (word[0] == '-') {
    is_minus = true;
    word = word.substr(1); // Removing '-' from the string
  }
  if (word.empty()) {
    throw std::invalid_argument("Invalid query. Empty minus word.");
  }
  if (word[0] == '-') {
    throw std::invalid_argument(
        "Invalid query. Double minus in the minus word.");
  }
  if (!IsValidWord(word)) {
    throw std::invalid_argument("Invalid query. Special symbols in the query.");
  }
  return {word, is_minus};
}

// Calculating IDF as log(number of documents / number of documents with word
// encountered in them Input: word we are calculating IDF for
double SearchServer::ComputeWordInverseDocumentFreq(
    const std::string_view word) const {
  return log(GetDocumentCount() * 1.0 /
             word_to_document_freqs_.at(word).size());
}