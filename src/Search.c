#include "Search.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool Search_isValidWord(const char *word) {
  if (!word || !*word || strlen(word) >= SITE_MAX_WORD)
    return false;

  return strpbrk(word, " \t\r\n\v\f") == NULL;
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
