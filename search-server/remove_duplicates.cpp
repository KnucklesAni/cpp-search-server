#include <iostream>
#include <map>
#include <set>
#include <string>

#include "remove_duplicates.h"

using namespace std;
void RemoveDuplicates(SearchServer &search_server) {
  map<set<string>, set<int>> document_info;
  for (const int document_id : search_server) {
    set<string> words;
    for (const auto &[word, freq] :
         search_server.GetWordFrequencies(document_id)) {
      words.insert(word);
    }
    document_info[words].insert(document_id);
  }
  for (const auto &[words, ids] : document_info) {
    if (ids.size() == 1) {
      continue;
    }
    for (auto it = next(begin(ids)); it != end(ids); ++it) {
      cout << "Found duplicate document id " << *it << endl;
      RemoveDocument(search_server, *it);
    }
  }
}
