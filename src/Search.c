#include "Search.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const SEARCH_WHITESPACE = " \t\r\n\v\f";

static bool Search_isValidWord(const char *word) {
  if (!word || !*word || strlen(word) >= SITE_MAX_WORD)
    return false;

  return strpbrk(word, SEARCH_WHITESPACE) == NULL;
}

static int Search_cmpVertices(const void *a, const void *b) {
  const Vertex *left  = *(const Vertex *const *)a;
  const Vertex *right = *(const Vertex *const *)b;

  if (left->weight != right->weight)
    return left->weight > right->weight ? -1 : 1;

  const Site *leftSite  = left->value;
  const Site *rightSite = right->value;
  return strcmp(leftSite->name, rightSite->name);
}

static bool Search_containsVertex(const IndexEntry *entry,
                                  const Vertex     *vertex) {
  int count = Index_count(entry);
  for (int i = 0; i < count; i++)
    if (Index_vertexAt(entry, i) == vertex)
      return true;

  return false;
}

static bool Search_nextWord(const char **cursor, char *word) {
  *cursor += strspn(*cursor, SEARCH_WHITESPACE);
  if (!**cursor)
    return false;

  size_t len = strcspn(*cursor, SEARCH_WHITESPACE);
  if (len >= SITE_MAX_WORD)
    return false;

  memcpy(word, *cursor, len);
  word[len] = '\0';
  *cursor += len;
  return true;
}

static int Search_countValidWords(const char *query) {
  const char *cursor = query;
  int         count  = 0;

  while (*cursor) {
    cursor += strspn(cursor, SEARCH_WHITESPACE);
    if (!*cursor)
      break;

    size_t len = strcspn(cursor, SEARCH_WHITESPACE);
    if (len >= SITE_MAX_WORD)
      return -1;

    count++;
    cursor += len;
  }

  return count;
}

SearchResult *Search_word(Index *index, const char *word) {
  if (!index || !Search_isValidWord(word))
    return NULL;

  SearchResult *result = malloc(sizeof(*result));
  if (!result)
    return NULL;

  const IndexEntry *entry = Index_get(index, word);
  int               count = Index_count(entry);

  result->sites = NULL;
  result->count = 0;

  if (count == 0)
    return result;

  result->sites = malloc((size_t)count * sizeof(*result->sites));
  if (!result->sites) {
    free(result);
    return NULL;
  }

  for (int i = 0; i < count; i++)
    result->sites[i] = Index_vertexAt(entry, i);

  result->count = count;
  return result;
}

SearchResult *Search_words(Index *index, const char *query) {
  if (!index || !query)
    return NULL;

  int wordCount = Search_countValidWords(query);
  if (wordCount <= 0)
    return NULL;

  const char *cursor = query;
  char        word[SITE_MAX_WORD];
  Search_nextWord(&cursor, word);

  SearchResult *result = Search_word(index, word);
  if (!result || wordCount == 1 || result->count == 0)
    return result;

  while (Search_nextWord(&cursor, word)) {
    const IndexEntry *entry = Index_get(index, word);
    if (!entry) {
      result->count = 0;
      break;
    }

    int kept = 0;
    for (int i = 0; i < result->count; i++)
      if (Search_containsVertex(entry, result->sites[i]))
        result->sites[kept++] = result->sites[i];

    result->count = kept;
    if (result->count == 0)
      break;
  }

  if (result->count == 0) {
    free(result->sites);
    result->sites = NULL;
  } else if (result->count > 1) {
    qsort(result->sites, (size_t)result->count, sizeof(*result->sites),
          Search_cmpVertices);
  }

  return result;
}

void Search_free(SearchResult *result) {
  if (!result)
    return;

  free(result->sites);
  free(result);
}

int Search_count(const SearchResult *result) {
  return result ? result->count : 0;
}

Site *Search_siteAt(const SearchResult *result, int index) {
  if (!result || index < 0 || index >= result->count)
    return NULL;

  return result->sites[index]->value;
}

int Search_importanceAt(const SearchResult *result, int index) {
  if (!result || index < 0 || index >= result->count)
    return 0;

  return result->sites[index]->weight;
}

void Search_print(const SearchResult *result) {
  if (!result || result->count == 0) {
    printf("Nenhum site encontrado.\n");
    return;
  }

  for (int i = 0; i < result->count; i++) {
    const Site *site = Search_siteAt(result, i);
    printf("%d. %s (importancia: %d)\n", i + 1, site->name,
           Search_importanceAt(result, i));
  }
}
