#ifndef INDEX_H
#define INDEX_H

#include "Site.h"

typedef struct Index Index;
typedef struct IndexEntry IndexEntry;

struct IndexEntry {
  char           key[SITE_MAX_WORD];
  const Vertex **value;
  int          count, capacity;
};

struct Index {
  IndexEntry *entries;
  int         count, capacity;
  Graph      *graph;
};

Index *Index_alloc(void);
Index *Index_fromGraph(Graph *g);
void Index_free(Index *index);

bool Index_add(Index *index, const char *word, const Vertex *vertex);
bool Index_addSite(Index *index, int label, Site *site);
bool Index_addWord(Index *index, int label, const char *word);
Site *Index_removeSite(Index *index, int label);
bool  Index_removeSiteByName(Index *index, const char *name);
bool  Index_removeLink(Index *index, const char *origin,
                       const char *destination);
bool Index_update(Index *index);
const IndexEntry *Index_get(Index *index, const char *word);
int Index_count(const IndexEntry *entry);
Site *Index_siteAt(const IndexEntry *entry, int index);

#endif /* INDEX_H */
