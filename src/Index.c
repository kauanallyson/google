#include "Index.h"
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int Index_findPosition(const Index *index, const char *word) {
  int first = 0;
  int last  = index->count;

  while (first < last) {
    int middle = first + (last - first) / 2;
    if (strcmp(index->entries[middle].key, word) < 0)
      first = middle + 1;
    else
      last = middle;
  }

  return first;
}

Index *Index_alloc(void) { return calloc(1, sizeof(Index)); }

static bool Index_addSiteWords(Index *index, const Vertex *vertex) {
  const Site *site = vertex->value;
  for (int i = 0; i < site->nWords; i++)
    if (!Index_add(index, site->words[i], vertex))
      return false;

  return true;
}

Index *Index_fromGraph(Graph *g) {
  if (!g)
    return NULL;

  Index *index = Index_alloc();
  if (!index)
    return NULL;

  const Vertex *vertex = g->first;
  while (vertex) {
    if (!Index_addSiteWords(index, vertex)) {
      Index_free(index);
      return NULL;
    }
    vertex = vertex->next;
  }

  index->graph = g;
  return index;
}

bool Index_update(Index *index) {
  if (!index || !index->graph)
    return false;

  Index *updated = Index_fromGraph(index->graph);
  if (!updated)
    return false;

  Index previous = *index;
  *index         = *updated;
  *updated       = previous;
  Index_free(updated);
  return true;
}

void Index_free(Index *index) {
  if (!index)
    return;

  for (int i = 0; i < index->count; i++)
    free(index->entries[i].value);

  free(index->entries);
  free(index);
}

static bool Index_insertWord(Index *index, int position, const char *word,
                             const Vertex *vertex) {
  IndexEntry entry = {.count = 1, .capacity = 1};
  strcpy(entry.key, word);
  entry.value = malloc(sizeof(*entry.value));
  if (!entry.value)
    return false;

  if (index->count == index->capacity) {
    if (index->capacity > INT_MAX / 2 ||
        (uintmax_t)index->capacity > SIZE_MAX / sizeof(*index->entries) / 2) {
      free(entry.value);
      return false;
    }

    int         capacity = index->capacity ? index->capacity * 2 : 1;
    IndexEntry *entries  = realloc(index->entries, capacity * sizeof(*entries));
    if (!entries) {
      free(entry.value);
      return false;
    }

    index->entries  = entries;
    index->capacity = capacity;
  }

  entry.value[0] = (IndexedSite){vertex->value, vertex};
  if (position < index->count)
    memmove(&index->entries[position + 1], &index->entries[position],
            (index->count - position) * sizeof(*index->entries));

  index->entries[position] = entry;
  index->count++;
  return true;
}

bool Index_add(Index *index, const char *word, const Vertex *vertex) {
  if (!index || !vertex || !vertex->value || !word || !*word ||
      strlen(word) >= SITE_MAX_WORD)
    return false;

  int position = Index_findPosition(index, word);
  if (position == index->count ||
      strcmp(index->entries[position].key, word) != 0)
    return Index_insertWord(index, position, word, vertex);

  IndexEntry *entry = &index->entries[position];
  for (int i = 0; i < entry->count; i++)
    if (entry->value[i].value == vertex->value)
      return true;

  if (entry->count == entry->capacity) {
    if (entry->capacity > INT_MAX / 2 ||
        (uintmax_t)entry->capacity > SIZE_MAX / sizeof(*entry->value) / 2)
      return false;

    int          capacity = entry->capacity * 2;
    IndexedSite *value    = realloc(entry->value, capacity * sizeof(*value));
    if (!value)
      return false;

    entry->value    = value;
    entry->capacity = capacity;
  }

  entry->value[entry->count++] = (IndexedSite){vertex->value, vertex};
  return true;
}

static void Index_removeSiteWords(Index *index, const Vertex *vertex) {
  int remaining = 0;
  for (int i = 0; i < index->count; i++) {
    IndexEntry *entry = &index->entries[i];
    for (int j = 0; j < entry->count; j++) {
      if (entry->value[j].vertex == vertex) {
        entry->count--;
        if (j < entry->count)
          entry->value[j] = entry->value[entry->count];
        break;
      }
    }

    if (entry->count)
      index->entries[remaining++] = *entry;
    else
      free(entry->value);
  }
  index->count = remaining;
}

bool Index_addSite(Index *index, int label, Site *site) {
  if (!index || !index->graph || !site)
    return false;

  Graph *g = index->graph;
  if (Graph_findVertexByLabel(g, label) ||
      Graph_findVertexByValue(g, site->name, Site_cmpName))
    return false;

  Vertex *previousLast = g->last;
  Graph_addVertex(g, label, site);
  if (g->last == previousLast)
    return false;

  Vertex *vertex = g->last;
  if (!Index_addSiteWords(index, vertex)) {
    Index_removeSiteWords(index, vertex);
    free(Graph_removeVertex(g, label));
    return false;
  }

  return true;
}

Site *Index_removeSite(Index *index, int label) {
  if (!index || !index->graph || !Graph_findVertexByLabel(index->graph, label))
    return NULL;

  Vertex *vertex = Graph_removeVertex(index->graph, label);
  Site   *site   = vertex->value;
  Index_removeSiteWords(index, vertex);
  free(vertex);
  return site;
}

static int Index_cmpSites(const void *a, const void *b) {
  const IndexedSite *left        = a;
  const IndexedSite *right       = b;
  int                leftWeight  = left->vertex->weight;
  int                rightWeight = right->vertex->weight;

  if (leftWeight != rightWeight)
    return leftWeight > rightWeight ? -1 : 1;

  return strcmp(left->value->name, right->value->name);
}

const IndexEntry *Index_get(Index *index, const char *word) {
  if (!index || !word || strlen(word) >= SITE_MAX_WORD)
    return NULL;

  int position = Index_findPosition(index, word);
  if (position == index->count ||
      strcmp(index->entries[position].key, word) != 0)
    return NULL;

  IndexEntry *entry = &index->entries[position];
  if (entry->count > 1)
    qsort(entry->value, entry->count, sizeof(*entry->value), Index_cmpSites);

  return entry;
}

int Index_count(const IndexEntry *entry) { return entry ? entry->count : 0; }

Site *Index_siteAt(const IndexEntry *entry, int index) {
  if (!entry || index < 0 || index >= entry->count)
    return NULL;

  return entry->value[index].value;
}
