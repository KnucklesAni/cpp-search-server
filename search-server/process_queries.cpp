#include "process_queries.h"
#include <algorithm>
#include <execution>
#include <iostream>
#include <memory>
#include <optional>
using namespace std;

vector<vector<Document>> ProcessQueries(const SearchServer &search_server,
                                        const vector<string> &queries) {
  vector<vector<Document>> result(queries.size());
  transform(execution::par, queries.begin(), queries.end(), result.begin(),
            [&search_server](const string &query) {
              return search_server.FindTopDocuments(query);
            });

  return result;
}

Flatten<std::vector, Document>
ProcessQueriesJoined(const SearchServer &search_server,
                     const std::vector<std::string> &queries) {
  return Flatten(ProcessQueries(search_server, queries));
}

void RemoveDuplicates(SearchServer &search_server, bool silent = false) {
  std::set<int> ids_to_delete;
  std::set<std::set<std::string_view, std::less<>>> unique_documents;

  for (const int document_id : search_server) {
    const std::map<std::string_view, double> &word_frequencies =
        search_server.GetWordFrequencies(document_id);
    std::set<std::string_view, std::less<>> words_of_the_document;
    for (const auto &[word, _] : word_frequencies) {
      words_of_the_document.insert(word);
    }
    // Checking if insertion was not successful
    if (!unique_documents.insert(words_of_the_document).second) {
      ids_to_delete.insert(document_id);
    }
  }
  for (const int id : ids_to_delete) {
    search_server.RemoveDocument(id);
    if (!silent) {
      std::cout << "Found duplicate document id " << id << std::endl;
    }
  }
}