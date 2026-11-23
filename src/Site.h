#ifndef SITE_H
#define SITE_H

#include <stdbool.h>

#define SITE_MAX_NAME 64
#define SITE_MAX_WORD 32

typedef struct {
  char   name[SITE_MAX_NAME];
  int    nWords;
  char **words;
} Site;

Site *Site_create(const char *name);
void  Site_free(void *site);

bool Site_addWord(Site *s, const char *word);
bool Site_hasWord(Site *s, const char *word);
bool Site_removeWord(Site *s, const char *word);

int  Site_cmpName(void *site, void *name);
void Site_print(void *site);

#endif /* SITE_H */
