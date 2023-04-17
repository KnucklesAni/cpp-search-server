/* Решение из урока №3 (на основе optional) */
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
  string s;
  getline(cin, s);
  return s;
}

int ReadLineWithNumber() {
  int result;
  cin >> result;
  ReadLine();
  return result;
}

inline bool AreAllSymbolsLegal(const string &s) {
  return count_if(s.begin(), s.end(),
                  [](unsigned char c) { return c < ' '; }) == 0;
}

vector<string> SplitIntoWords(const string &text) {
  vector<string> words;
  string word;
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
    if (!AreAllSymbolsLegal(word)) {
      throw invalid_argument{"Invalid string"};
      ;
    }
    words.push_back(word);
  }

  return words;
}

struct Document {
  Document() = default;

  Document(int id, double relevance, int rating)
      : id(id), relevance(relevance), rating(rating) {}

  int id = 0;
  double relevance = 0.0;
  int rating = 0;
};

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer &strings) {
  set<string> non_empty_strings;
  for (const string &str : strings) {
    if (!str.empty()) {
      if (!AreAllSymbolsLegal(str)) {
        throw invalid_argument{"Invalid string"};
        ;
      }
      non_empty_strings.insert(str);
    }
  }
  return non_empty_strings;
}

enum class DocumentStatus {
  ACTUAL,
  IRRELEVANT,
  BANNED,
  REMOVED,
};

class SearchServer {
public:
  template <typename StringContainer>
  explicit SearchServer(const StringContainer &stop_words)
      : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {}

  explicit SearchServer(const string &stop_words_text)
      : SearchServer(
            SplitIntoWords(stop_words_text)) // Invoke delegating constructor
                                             // from string container
  {}

  void AddDocument(int document_id, const string &document,
                   DocumentStatus status, const vector<int> &ratings) {
    if (document_id < 0) {
      throw invalid_argument{"Invalid Document ID"};
    }
    if (!AreAllSymbolsLegal(document)) {
      throw invalid_argument{"Invalid Document Content"};
    }
    if (documents_.count(document_id) > 0) {
      throw invalid_argument{"Duplicated Document ID"};
    }
    const vector<string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const string &word : words) {
      word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(document_id,
                       DocumentData{ComputeAverageRating(ratings), status});
    documents_ids_.push_back(document_id);
  }

  template <typename DocumentPredicate>
  vector<Document>
  FindTopDocuments(const string &raw_query,
                   DocumentPredicate document_predicate) const {
    Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(),
         [](const Document &lhs, const Document &rhs) {
           if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
             return lhs.rating > rhs.rating;
           } else {
             return lhs.relevance > rhs.relevance;
           }
         });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
      matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return {matched_documents};
  }

  vector<Document> FindTopDocuments(const string &raw_query,
                                    DocumentStatus status) const {
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status,
                            int rating) { return document_status == status; });
  }

  vector<Document> FindTopDocuments(const string &raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
  }

  int GetDocumentCount() const { return documents_.size(); }

  tuple<vector<string>, DocumentStatus> MatchDocument(const string &raw_query,
                                                      int document_id) const {
    if (!AreAllSymbolsLegal(raw_query)) {
      throw invalid_argument{"Invalid query content"};
    }
    Query query = ParseQuery(raw_query);
    vector<string> matched_words;
    for (const string &word : query.plus_words) {
      if (word_to_document_freqs_.count(word) == 0) {
        continue;
      }
      if (word_to_document_freqs_.at(word).count(document_id)) {
        matched_words.push_back(word);
      }
    }
    for (const string &word : query.minus_words) {
      if (word_to_document_freqs_.count(word) == 0) {
        continue;
      }
      if (word_to_document_freqs_.at(word).count(document_id)) {
        matched_words.clear();
        break;
      }
    }
    return {matched_words, documents_.at(document_id).status};
  }
  int GetDocumentId(size_t index) const {
    if (index >= documents_ids_.size()) {
      throw out_of_range{"Index out of range"};
    }
    return documents_ids_[index];
  }

private:
  struct DocumentData {
    int rating;
    DocumentStatus status;
  };
  const set<string> stop_words_;
  map<string, map<int, double>> word_to_document_freqs_;
  map<int, DocumentData> documents_;
  vector<int> documents_ids_;

  bool IsStopWord(const string &word) const {
    return stop_words_.count(word) > 0;
  }

  vector<string> SplitIntoWordsNoStop(const string &text) const {
    vector<string> words;
    for (const string &word : SplitIntoWords(text)) {
      if (!IsStopWord(word)) {
        words.push_back(word);
      }
    }
    return words;
  }

  static int ComputeAverageRating(const vector<int> &ratings) {
    if (ratings.empty()) {
      return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
      rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
  }

  struct QueryWord {
    string data;
    bool is_minus;
    bool is_stop;
  };

  QueryWord ParseQueryWord(string text) const {
    bool is_minus = false;
    if (text.empty()) {
      throw invalid_argument{"Empty query word"};
    }
    if (text[0] == '-') {
      is_minus = true;
      if (text == "-") {
        throw invalid_argument{"Unaccceptable --word"};
      }
      if (text[1] == '-') {
        throw invalid_argument{"Singular minus in request"};
      }
      text = text.substr(1);
    }
    if (!AreAllSymbolsLegal(text)) {
      throw invalid_argument{"Invalid query content"};
    }
    return {text, is_minus, IsStopWord(text)};
  }

  struct Query {
    set<string> plus_words;
    set<string> minus_words;
  };

  Query ParseQuery(const string &text) const {
    Query query;
    for (const string &word : SplitIntoWords(text)) {
      const QueryWord query_word = ParseQueryWord(word);
      if (!query_word.is_stop) {
        if (query_word.is_minus) {
          query.minus_words.insert(query_word.data);
        } else {
          query.plus_words.insert(query_word.data);
        }
      }
    }
    return query;
  }

  // Existence required
  double ComputeWordInverseDocumentFreq(const string &word) const {
    return log(GetDocumentCount() * 1.0 /
               word_to_document_freqs_.at(word).size());
  }

  template <typename DocumentPredicate>
  vector<Document>
  FindAllDocuments(const Query &query,
                   DocumentPredicate document_predicate) const {
    map<int, double> document_to_relevance;
    for (const string &word : query.plus_words) {
      if (word_to_document_freqs_.count(word) == 0) {
        continue;
      }
      const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
      for (const auto [document_id, term_freq] :
           word_to_document_freqs_.at(word)) {
        const auto &document_data = documents_.at(document_id);
        if (document_predicate(document_id, document_data.status,
                               document_data.rating)) {
          document_to_relevance[document_id] +=
              term_freq * inverse_document_freq;
        }
      }
    }

    for (const string &word : query.minus_words) {
      if (word_to_document_freqs_.count(word) == 0) {
        continue;
      }
      for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
        document_to_relevance.erase(document_id);
      }
    }

    vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
      matched_documents.push_back(
          {document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
  }
};
