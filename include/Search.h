#ifndef SEARCH_H
#define SEARCH_H

#include "Index.h"

typedef struct {
  const Vertex **sites;
  int            count;
} SearchResult;

SearchResult *Search_word(Index *index, const char *word);
SearchResult *Search_words(Index *index, const char *query);
SearchResult *Search_query(Index *index, const char *query);
void          Search_free(SearchResult *result);
void          Search_sortVertices(const Vertex **vertices, int count);

int   Search_count(const SearchResult *result);
Site *Search_siteAt(const SearchResult *result, int index);
int   Search_importanceAt(const SearchResult *result, int index);
void  Search_print(const SearchResult *result);

#endif /* SEARCH_H */
