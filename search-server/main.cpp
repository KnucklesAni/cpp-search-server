#if 0
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

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
    words.push_back(word);
  }
  return words;
}

struct Document {
  int id;
  double relevance;
  int rating;
};

enum class DocumentStatus {
  ACTUAL,
  IRRELEVANT,
  BANNED,
  REMOVED,
};

class SearchServer {
public:
  void SetStopWords(const string &text) {
    for (const string &word : SplitIntoWords(text)) {
      stop_words_.insert(word);
    }
  }
  void AddDocument(int document_id, const string &document,
                   DocumentStatus status, const vector<int> &ratings) {
    const vector<string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const string &word : words) {
      word_to_document_freqs_[word][document_id] += inv_word_count;
    }
    documents_.emplace(document_id,
                       DocumentData{ComputeAverageRating(ratings), status});
  }
  inline static constexpr double TOLERANCE = 1e-6;
  template <typename Predicate>
  vector<Document> FindTopDocuments(const string &raw_query,
                                    Predicate predicate) const {
    const Query query = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query, predicate);
    sort(matched_documents.begin(), matched_documents.end(),
         [](const Document &lhs, const Document &rhs) {
           if (abs(lhs.relevance - rhs.relevance) < TOLERANCE) {
             return lhs.rating > rhs.rating;
           } else {
             return lhs.relevance > rhs.relevance;
           }
         });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
      matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
    return matched_documents;
  }
  vector<Document> FindTopDocuments(const string &raw_query,
                                    DocumentStatus status) const {
    return FindTopDocuments(raw_query,
                            [status](auto, auto filter_status, auto) {
                              return filter_status == status;
                            });
  }
  vector<Document> FindTopDocuments(const string &raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
  }
  int GetDocumentCount() const { return documents_.size(); }
  tuple<vector<string>, DocumentStatus> MatchDocument(const string &raw_query,
                                                      int document_id) const {
    const Query query = ParseQuery(raw_query);
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

private:
  struct DocumentData {
    int rating;
    DocumentStatus status;
  };
  set<string> stop_words_;
  map<string, map<int, double>> word_to_document_freqs_;
  map<int, DocumentData> documents_;
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
    return accumulate(ratings.begin(), ratings.end(), 0) / int(ratings.size());
  }
  struct QueryWord {
    string data;
    bool is_minus;
    bool is_stop;
  };
  struct Query {
    set<string> plus_words;
    set<string> minus_words;
  };
  QueryWord ParseQueryWord(string text) const {
    bool is_minus = false;
    if (text[0] == '-') {
      is_minus = true;
      text = text.substr(1);
    }
    return {text, is_minus, IsStopWord(text)};
  }
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
  double ComputeWordInverseDocumentFreq(const string &word) const {
    return log(GetDocumentCount() * 1.0 /
               word_to_document_freqs_.at(word).size());
  }
  template <typename Predicate>
  vector<Document> FindAllDocuments(const Query &query,
                                    Predicate predicate) const {
    map<int, double> document_to_relevance;
    for (const string &word : query.plus_words) {
      if (word_to_document_freqs_.count(word) == 0) {
        continue;
      }
      const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
      for (const auto [document_id, term_freq] :
           word_to_document_freqs_.at(word)) {
        document_to_relevance[document_id] += term_freq * inverse_document_freq;
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
      const auto &document = documents_.at(document_id);
      if (predicate(document_id, document.status, document.rating)) {
        matched_documents.push_back(
            {document_id, relevance, documents_.at(document_id).rating});
      }
    }
    return matched_documents;
  }
};

template <typename T>
void AssertImpl(const T &t, const string &t_str, const string &file,
                const string &func, unsigned line, const string &hint) {
  if (!t) {
    cout << boolalpha;
    cout << file << "("s << line << "): "s << func << ": "s;
    cout << "ASSERT("s << t_str << ") failed."s;
    if (!hint.empty()) {
      cout << " Hint: "s << hint;
    }
    cout << endl;
    abort();
  }
}

#define ASSERT(a) AssertImpl((a), #a, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(a, hint)                                                   \
  AssertImpl((a), #a, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename Container>
ostream &Print(ostream &out, const Container &container) {
  for (const auto &element : container) {
    if (&element != &*begin(container)) {
      out << ", "s;
    }
    out << element;
  }
  return out;
}
template <typename Element>
ostream &operator<<(ostream &out, const set<Element> &container) {
  out << "{";
  return Print(out, container) << "}";
}
template <typename Element>
ostream &operator<<(ostream &out, const vector<Element> &container) {
  out << "[";
  return Print(out, container) << "]";
}
template <typename Key, typename Element>
ostream &operator<<(ostream &out, const map<Key, Element> &container) {
  out << "{";
  return Print(out, container) << "}";
}
template <typename Key, typename Element>
ostream &operator<<(ostream &out, const pair<Key, Element> &container) {
  out << container.first << ": " << container.second;
  return out;
}
template <typename T, typename U>
void AssertEqualImpl(const T &t, const U &u, const string &t_str,
                     const string &u_str, const string &file,
                     const string &func, unsigned line, const string &hint) {
  if (t != u) {
    cout << boolalpha;
    cout << file << "("s << line << "): "s << func << ": "s;
    cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
    cout << t << " != "s << u << "."s;
    if (!hint.empty()) {
      cout << " Hint: "s << hint;
    }
    cout << endl;
    abort();
  }
}

#define ASSERT_EQUAL(a, b)                                                     \
  AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint)                                          \
  AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))
template <typename Func>
void RunTestImpl(const Func &func, const string &func_str) {
  func();
  cerr << func_str << " OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl((func), #func)

#endif

ostream &operator<<(ostream &os, const DocumentStatus &document_status) {
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

void TestAddedDocumentContent() {
  const int doc_id = 42;
  const string content = "cat in the city"s;
  const vector<int> ratings = {1, 2, 3};
  SearchServer server;
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
  const auto found_docs = server.FindTopDocuments("in"s);
  ASSERT_EQUAL(found_docs.size(), 1);
  const Document &doc0 = found_docs[0];
  ASSERT_EQUAL(doc0.id, doc_id);
}
void TestMinusWords() {
  const int doc_id = 42;
  const string content = "cat in the city"s;
  const vector<int> ratings = {1, 2, 3};
  const int doc2_id = 43;
  const string contentx = "dog in the city"s;
  const vector<int> ratingsx = {5, 6, 7};

  SearchServer server;
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
  server.AddDocument(doc2_id, contentx, DocumentStatus::ACTUAL, ratingsx);
  const auto found_docs = server.FindTopDocuments("in -cat"s);
  ASSERT_EQUAL(found_docs.size(), 1);
  const Document &doc0 = found_docs[0];
  ASSERT_EQUAL(doc0.id, doc2_id);
}
void TestMatchDocument() {
  const int doc_id = 42;
  const string content = "cat in the city"s;
  const vector<int> ratings = {1, 2, 3};

  SearchServer server;
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
  const auto [words, status] = server.MatchDocument("in"s, doc_id);

  ASSERT_EQUAL(words.size(), 1);
  ASSERT_EQUAL(status, DocumentStatus::ACTUAL);
}
void TestMatchDocumentMinus() {
  const int doc_id = 42;
  const string content = "cat in the city"s;
  const vector<int> ratings = {1, 2, 3};

  SearchServer server;
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
  const auto [words, status] = server.MatchDocument("in -cat"s, doc_id);

  ASSERT_EQUAL(words.size(), 0);
  ASSERT_EQUAL(status, DocumentStatus::ACTUAL);
}
void TestRelevance() {
  const int doc_id = 42;
  const string content = "in"s;
  const vector<int> ratings = {1, 2, 3};
  const int doc2_id = 43;
  const string contentx = "dog in the city"s;
  const vector<int> ratingsx = {5, 6, 7};

  SearchServer server;
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
  server.AddDocument(doc2_id, contentx, DocumentStatus::ACTUAL, ratingsx);
  const auto found_docs = server.FindTopDocuments("in"s);
  ASSERT_EQUAL(found_docs.size(), 2);
  const Document &doc0 = found_docs[0];
  const Document &doc1 = found_docs[0];
  ASSERT_EQUAL(doc0.id, doc2_id);
  ASSERT_EQUAL(doc1.id, doc2_id);
}
void TestRating() {
  const int doc_id = 42;
  const string content = "cat in the city"s;
  const vector<int> ratings = {1, 2, 3};

  SearchServer server;
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
  const auto found_docs = server.FindTopDocuments("in"s);
  ASSERT_EQUAL(found_docs.size(), 1);
  const Document &doc0 = found_docs[0];
  ASSERT_EQUAL(doc0.id, doc_id);
  ASSERT_EQUAL(doc0.rating, 2);
}
void TestPredicate() {
  const int doc_id = 42;
  const string content = "cat in the city"s;
  const vector<int> ratings = {1, 2, 3};

  SearchServer server;
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
  const auto found_docs = server.FindTopDocuments("in"s);
  ASSERT_EQUAL(found_docs.size(), 1);
  const Document &doc0 = found_docs[0];
  ASSERT_EQUAL(doc0.id, doc_id);
  const auto found_docs2 =
      server.FindTopDocuments("in"s, [](auto...) { return false; });
  ASSERT_HINT(found_docs2.empty(),
              "Predicate rejects everything, there should be no results");
}

void TestStatus() {
  const int doc_id = 42;
  const string content = "cat in the city"s;
  const vector<int> ratings = {1, 2, 3};

  SearchServer server;
  server.AddDocument(doc_id, content, DocumentStatus::REMOVED, ratings);
  const auto found_docs =
      server.FindTopDocuments("in"s, DocumentStatus::REMOVED);
  ASSERT_EQUAL(found_docs.size(), 1);
  const Document &doc0 = found_docs[0];
  ASSERT_EQUAL(doc0.id, doc_id);
}
void TestCalculatedRelevance() {
  const int doc_id = 42;
  const string content = "Cat on the table"s;
  const vector<int> ratings = {1, 2, 3};
  const int doc2_id = 43;
  const string contentx = "dog in the city"s;
  const vector<int> ratingsx = {5, 6, 7};

  SearchServer server;
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
  server.AddDocument(doc2_id, contentx, DocumentStatus::ACTUAL, ratingsx);
  const auto found_docs = server.FindTopDocuments("in"s);
  ASSERT_EQUAL(found_docs.size(), 1);
  const Document &doc0 = found_docs[0];
  ASSERT_EQUAL(doc0.id, doc2_id);
  ASSERT_HINT(abs(doc0.relevance - 0.173287) < 1e-6,
              "Calculated tolerance should approximately 0.173287");
}
void TestExcludeStopWordsFromAddedDocumentContent() {
  const int doc_id = 42;
  const string content = "cat in the city"s;
  const vector<int> ratings = {1, 2, 3};
  SearchServer server;
  server.SetStopWords("in the"s);
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
  ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
              "Stop words must be excluded from documents"s);
}
void TestSearchServer() {
  RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
  RUN_TEST(TestAddedDocumentContent);
  RUN_TEST(TestMinusWords);
  RUN_TEST(TestMatchDocument);
  RUN_TEST(TestMatchDocumentMinus);
  RUN_TEST(TestRelevance);
  RUN_TEST(TestRating);
  RUN_TEST(TestPredicate);
  RUN_TEST(TestStatus);
  RUN_TEST(TestCalculatedRelevance);
}

int main() {
  TestSearchServer();
  cout << "Search server testing finished"s << endl;
}
