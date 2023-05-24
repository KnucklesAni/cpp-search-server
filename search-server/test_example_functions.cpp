// в качестве заготовки кода используйте последнюю версию своей поисковой
// системы

#include "test_example_functions.h"

void TestAddedDocumentContent() {
  const int doc_id = 42;
  const std::string content = "cat in the city";
  const std::vector<int> ratings = {1, 2, 3};

  const int doc2_id = 43;
  const std::string contentx = "dog in the city";
  const std::vector<int> ratingsx = {5, 6, 7};

  SearchServer server{std::string{""}};
  ASSERT_EQUAL(server.GetDocumentCount(), 0);
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratingsx);
  ASSERT_EQUAL(server.GetDocumentCount(), 1);
  server.AddDocument(doc2_id, contentx, DocumentStatus::ACTUAL, ratings);
  ASSERT_EQUAL(server.GetDocumentCount(), 2);

  const auto found_docs = server.FindTopDocuments("in");
  ASSERT_EQUAL(found_docs.size(), 2);
  const Document &doc0 = found_docs[0];
  ASSERT_EQUAL(doc0.id, doc_id);
  const Document &doc1 = found_docs[1];
  ASSERT_EQUAL(doc1.id, doc2_id);
}

void TestSearchWithMinusWords() {
  const int doc_id = 42;
  const std::string content = "cat in the city";
  const std::vector<int> ratings = {1, 2, 3};

  const int doc2_id = 43;
  const std::string contentx = "dog in the city";
  const std::vector<int> ratingsx = {5, 6, 7};

  SearchServer server{std::string{""}};
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
  server.AddDocument(doc2_id, contentx, DocumentStatus::ACTUAL, ratingsx);

  const auto found_docs = server.FindTopDocuments("in -cat");
  ASSERT_EQUAL(found_docs.size(), 1);
  const Document &doc0 = found_docs[0];
  ASSERT_EQUAL(doc0.id, doc2_id);
}

void TestMatchDocument() {
  const int doc_id = 42;
  const std::string content = "cat in the city";
  const std::vector<int> ratings = {1, 2, 3};

  SearchServer server{std::string{""}};
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

  const auto [words, status] = server.MatchDocument("in", doc_id);
  ASSERT_EQUAL(words, std::vector<std::string>{"in"});
  ASSERT_EQUAL(status, DocumentStatus::ACTUAL);

  const auto [words2, status2] = server.MatchDocument("in cat", doc_id);
  ASSERT_EQUAL(words2, (std::vector<std::string>{"cat", "in"}));
  ASSERT_EQUAL(status2, DocumentStatus::ACTUAL);
}

void TestMatchDocumentMinus() {
  const int doc_id = 42;
  const std::string content = "cat in the city";
  const std::vector<int> ratings = {1, 2, 3};

  SearchServer server{std::string{""}};
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
  const auto [words, status] = server.MatchDocument("in -cat", doc_id);

  ASSERT_HINT(words.empty(), "Minus words must suppress results");
  ASSERT_EQUAL(status, DocumentStatus::ACTUAL);
}
void TestSortByRelevance() {
  const int doc_id = 42;
  const std::string content = "in";
  const std::vector<int> ratings = {1, 2, 3};

  const int doc2_id = 43;
  const std::string content2 = "dog in the city";
  const std::vector<int> ratings2 = {5, 6, 7};

  const int doc3_id = 44;
  const std::string content3 = "cat in the car in the house";
  const std::vector<int> ratings3 = {8, 9, 3};

  const int doc4_id = 45;
  const std::string content4 = "alien in the alien space ship";
  const std::vector<int> ratings4 = {1, 6, 7};

  SearchServer server{std::string{""}};
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
  server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
  server.AddDocument(doc3_id, content3, DocumentStatus::ACTUAL, ratings3);
  server.AddDocument(doc4_id, content4, DocumentStatus::ACTUAL, ratings4);

  const auto found_docs = server.FindTopDocuments("in");
  ASSERT_EQUAL(found_docs.size(), 4);
  const Document &doc0 = found_docs[0];
  const Document &doc1 = found_docs[1];
  const Document &doc2 = found_docs[2];
  const Document &doc3 = found_docs[3];
  ASSERT_EQUAL(doc0.id, doc2_id);
  ASSERT_EQUAL(doc1.id, doc3_id);
  ASSERT_EQUAL(doc2.id, doc4_id);
  ASSERT_EQUAL(doc3.id, doc_id);
}

void TestCalculatedRating() {
  const int doc_id = 42;
  const std::string content = "cat in the city";
  const std::vector<int> ratings = {1, 2, 3};

  SearchServer server{std::string{""}};
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

  const auto found_docs = server.FindTopDocuments("in");
  ASSERT_EQUAL(found_docs.size(), 1);
  const Document &doc0 = found_docs[0];
  ASSERT_EQUAL(doc0.id, doc_id);
  ASSERT_EQUAL(doc0.rating, std::accumulate(begin(ratings), end(ratings), 0) /
                                ratings.size());
}

void TestSearchWithPredicate() {
  const int doc_id = 42;
  const std::string content = "cat in the city";
  const std::vector<int> ratings = {1, 2, 3};

  const int doc2_id = 43;
  const std::string content2 = "dog in the city";
  const std::vector<int> ratings2 = {5, 6, 7};

  const int doc3_id = 44;
  const std::string content3 = "cat in the car in the house";
  const std::vector<int> ratings3 = {8, 9, 3};

  const int doc4_id = 45;
  const std::string content4 = "alien in the alien space ship";
  const std::vector<int> ratings4 = {1, 6, 7};

  SearchServer server{std::string{""}};
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
  server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, ratings2);
  server.AddDocument(doc3_id, content3, DocumentStatus::ACTUAL, ratings3);
  server.AddDocument(doc4_id, content4, DocumentStatus::ACTUAL, ratings4);

  const auto found_docs = server.FindTopDocuments("in");
  ASSERT_EQUAL(found_docs.size(), 4);

  const auto found_docs2 =
      server.FindTopDocuments("in", [](auto...) { return false; });
  ASSERT_HINT(found_docs2.empty(),
              "Predicate rejects everything, there should be no results");

  const auto found_docs3 = server.FindTopDocuments(
      "in", [](auto, auto, auto rating) { return rating == 6; });
  ASSERT_EQUAL(found_docs.size(), 4);
  const Document &doc0 = found_docs[0];
  const Document &doc1 = found_docs[1];
  ASSERT_EQUAL(doc0.id, doc2_id);
  ASSERT_EQUAL(doc1.id, doc3_id);
}

void TestSearchDocumentsByStatus() {
  const int doc_id = 42;

  const int doc2_id = 43;
  const std::string content = "cat in the city";
  const std::vector<int> ratings = {1, 2, 3};

  SearchServer server{std::string{""}};
  server.AddDocument(doc_id, content, DocumentStatus::REMOVED, ratings);
  server.AddDocument(doc2_id, content, DocumentStatus::ACTUAL, ratings);

  const auto found_docs =
      server.FindTopDocuments("in", DocumentStatus::REMOVED);
  ASSERT_EQUAL(found_docs.size(), 1);
  const Document &doc0 = found_docs[0];
  ASSERT_EQUAL(doc0.id, doc_id);
}

void TestCalculatedRelevance() {
  const int doc_id = 42;
  const std::string content = "Cat on the table";
  const std::vector<int> ratings = {1, 2, 3};

  const int doc2_id = 43;
  const std::string contentx = "dog in the city";
  const std::vector<int> ratingsx = {5, 6, 7};

  SearchServer server{std::string{""}};
  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
  server.AddDocument(doc2_id, contentx, DocumentStatus::ACTUAL, ratingsx);

  const auto found_docs = server.FindTopDocuments("in");
  ASSERT_EQUAL(found_docs.size(), 1);
  const Document &doc0 = found_docs[0];
  ASSERT_EQUAL(doc0.id, doc2_id);
  ASSERT_HINT(abs(doc0.relevance -
                  log(server.GetDocumentCount() * 1.0 / found_docs.size()) *
                      (1.0 / 4)) < 1e-6,
              "Calculated tolerance should approximately 0.173287");
}

void TestExcludeStopWordsFromAddedDocumentContent() {
  const int doc_id = 42;
  const std::string content = "cat in the city";
  const std::vector<int> ratings = {1, 2, 3};

  SearchServer server{std::string{"in the"}};

  server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

  ASSERT_HINT(server.FindTopDocuments("in").empty(),
              "Stop words must be excluded from documents");
}

const class TestSearchServer {
public:
  TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestAddedDocumentContent);
    RUN_TEST(TestSearchWithMinusWords);
    RUN_TEST(TestMatchDocument);
    RUN_TEST(TestMatchDocumentMinus);
    RUN_TEST(TestSortByRelevance);
    RUN_TEST(TestCalculatedRating);
    RUN_TEST(TestSearchWithPredicate);
    RUN_TEST(TestSearchDocumentsByStatus);
    RUN_TEST(TestCalculatedRelevance);
  }
} TEST_SEARCHSERVER;