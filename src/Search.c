#include "Search.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const SEARCH_WHITESPACE = " \t\r\n\v\f";

typedef enum {
  SEARCH_TOKEN_WORD,
  SEARCH_TOKEN_AND,
  SEARCH_TOKEN_OR
} SearchTokenType;

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

static bool Search_resultContains(const SearchResult *result,
                                  const Vertex       *vertex) {
  for (int i = 0; i < result->count; i++)
    if (result->sites[i] == vertex)
      return true;

  return false;
}

static void Search_intersectEntry(SearchResult     *result,
                                  const IndexEntry *entry) {
  if (!entry) {
    result->count = 0;
    return;
  }

  int kept = 0;
  for (int i = 0; i < result->count; i++)
    if (Search_containsVertex(entry, result->sites[i]))
      result->sites[kept++] = result->sites[i];

  result->count = kept;
}

static bool Search_mergeUnion(SearchResult *target, int *capacity,
                              const SearchResult *source) {
  for (int i = 0; i < source->count; i++) {
    const Vertex *vertex = source->sites[i];
    if (Search_resultContains(target, vertex))
      continue;

    if (target->count == *capacity) {
      int newCapacity = *capacity ? *capacity * 2 : 4;
      const Vertex **sites =
          realloc(target->sites, (size_t)newCapacity * sizeof(*sites));
      if (!sites)
        return false;

      target->sites = sites;
      *capacity     = newCapacity;
    }

    target->sites[target->count++] = vertex;
  }

  return true;
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

static bool Search_nextToken(const char **cursor, SearchTokenType *type,
                             char *word) {
  *cursor += strspn(*cursor, SEARCH_WHITESPACE);
  if (!**cursor)
    return false;

  size_t len = strcspn(*cursor, SEARCH_WHITESPACE);
  if (len == 3 && strncmp(*cursor, "AND", 3) == 0) {
    *type = SEARCH_TOKEN_AND;
    *cursor += len;
    return true;
  }
  if (len == 2 && strncmp(*cursor, "OR", 2) == 0) {
    *type = SEARCH_TOKEN_OR;
    *cursor += len;
    return true;
  }
  if (len >= SITE_MAX_WORD)
    return false;

  *type = SEARCH_TOKEN_WORD;
  memcpy(word, *cursor, len);
  word[len] = '\0';
  *cursor += len;
  return true;
}

static bool Search_validateQuery(const char *query) {
  const char     *cursor     = query;
  bool            expectWord = true;
  SearchTokenType type;
  char            word[SITE_MAX_WORD];

  while (true) {
    cursor += strspn(cursor, SEARCH_WHITESPACE);
    if (!*cursor)
      break;

    if (!Search_nextToken(&cursor, &type, word))
      return false;

    if (type == SEARCH_TOKEN_WORD) {
      expectWord = false;
    } else {
      if (expectWord)
        return false;
      expectWord = true;
    }
  }

  return !expectWord;
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
    Search_intersectEntry(result, Index_get(index, word));
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

SearchResult *Search_query(Index *index, const char *query) {
  if (!index || !query || !Search_validateQuery(query))
    return NULL;

  SearchResult *total = calloc(1, sizeof(*total));
  if (!total)
    return NULL;

  int             totalCapacity = 0;
  SearchResult   *clause        = NULL;
  const char     *cursor        = query;
  SearchTokenType type;
  char            word[SITE_MAX_WORD];

  while (Search_nextToken(&cursor, &type, word)) {
    if (type == SEARCH_TOKEN_OR) {
      if (!Search_mergeUnion(total, &totalCapacity, clause)) {
        Search_free(clause);
        Search_free(total);
        return NULL;
      }
      Search_free(clause);
      clause = NULL;
    } else if (type == SEARCH_TOKEN_WORD) {
      if (!clause) {
        clause = Search_word(index, word);
        if (!clause) {
          Search_free(total);
          return NULL;
        }
      } else if (clause->count > 0) {
        Search_intersectEntry(clause, Index_get(index, word));
      }
    }
  }

  if (clause) {
    bool merged = Search_mergeUnion(total, &totalCapacity, clause);
    Search_free(clause);
    if (!merged) {
      Search_free(total);
      return NULL;
    }
  }

  if (total->count > 1)
    qsort(total->sites, (size_t)total->count, sizeof(*total->sites),
          Search_cmpVertices);

  return total;
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
