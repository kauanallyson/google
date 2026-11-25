#ifndef SITE_H
#define SITE_H

#include "Graph.h"
#include <stdbool.h>
#include <stdint.h>

#define SITE_MAX_NAME 64
#define SITE_MAX_WORD 32

typedef struct {
  char   name[SITE_MAX_NAME];
  int    nWords;
  char **words;
} Site;

Site *Site_alloc(const char *name);
void  Site_free(void *site);

bool Site_hasWord(Site *s, const char *word);
void Site_addWord(Site *s, const char *word);

bool Site_hasLink(Graph *g, Site *s1, Site *s2);
void Site_addLink(Graph *g, Site *s1, Site *s2, int weight);

int  Site_cmpName(void *site, void *name);
void Site_print(void *site);

#endif /* SITE_H */
