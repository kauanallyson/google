#include "Site.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Site *Site_create(const char *name) {
  Site *s = NULL;

  if (name && name[0] != '\0' && strlen(name) < SITE_MAX_NAME) {
    s = malloc(sizeof(Site));

    if (s) {
      strcpy(s->name, name);
      s->nWords = 0;
      s->words  = NULL;
    }
  }

  return s;
}

void Site_free(void *site) {
  Site *s = site;
  int   i;

  if (s) {
    for (i = 0; i < s->nWords; i++)
      free(s->words[i]);

    free(s->words);
    free(s);
  }
}

bool Site_hasWord(Site *s, const char *word) {
  int i;

  if (s && word) {
    for (i = 0; i < s->nWords; i++)
      if (strcmp(s->words[i], word) == 0)
        return true;
  }

  return false;
}

bool Site_addWord(Site *s, const char *word) {
  char **words = NULL;
  char  *copy  = NULL;

  if (s && word && word[0] != '\0' && strlen(word) < SITE_MAX_WORD &&
      !Site_hasWord(s, word)) {

    copy = malloc(strlen(word) + 1);
    if (!copy)
      return false;
    strcpy(copy, word);

    words = realloc(s->words, (s->nWords + 1) * sizeof(char *));
    if (!words) {
      free(copy);
      return false;
    }

    s->words            = words;
    s->words[s->nWords] = copy;
    s->nWords++;
    return true;
  }

  return false;
}

int Site_cmpName(void *site, void *name) {
  return strcmp(((Site *)site)->name, (char *)name);
}

void Site_print(void *site) { printf("%s", ((Site *)site)->name); }

bool Site_removeWord(Site *s, const char *word) {
  int i;

  if (s && word) {
    for (i = 0; i < s->nWords; i++) {
      if (strcmp(s->words[i], word) == 0) {
        free(s->words[i]);
        memmove(&s->words[i], &s->words[i + 1],
                (s->nWords - i - 1) * sizeof(char *));
        s->nWords--;

        if (s->nWords == 0) {
          free(s->words);
          s->words = NULL;
        }

        return true;
      }
    }
  }

  return false;
}
