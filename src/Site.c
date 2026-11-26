#include "Site.h"
#include "Graph.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Site *Site_alloc(const char *name) {
  Site *s = NULL;

  if (!name || name[0] == '\0') {
    fprintf(stderr, "Site_alloc: nome nulo ou vazio\n");
    return NULL;
  }

  if (strlen(name) >= SITE_MAX_NAME) {
    fprintf(stderr, "Site_alloc: nome '%s' maior que %d caracteres\n", name,
            SITE_MAX_NAME - 1);
    return NULL;
  }

  s = malloc(sizeof(Site));

  if (!s) {
    fprintf(stderr, "Site_alloc: sem memoria para o site '%s'\n", name);
    return NULL;
  }

  strcpy(s->name, name);
  s->nWords = 0;
  s->words  = NULL;

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

void Site_addWord(Site *s, const char *word) {
  char **words = NULL;
  char  *copy  = NULL;

  if (!s) {
    fprintf(stderr, "Site_addWord: site nulo\n");
    return;
  }

  if (!word || word[0] == '\0') {
    fprintf(stderr, "Site_addWord: palavra nula ou vazia em '%s'\n", s->name);
    return;
  }

  if (strlen(word) >= SITE_MAX_WORD) {
    fprintf(stderr, "Site_addWord: palavra '%s' maior que %d caracteres\n",
            word, SITE_MAX_WORD - 1);
    return;
  }

  if (Site_hasWord(s, word))
    return;

  copy = malloc(strlen(word) + 1);
  if (!copy) {
    fprintf(stderr, "Site_addWord: sem memoria para a palavra '%s'\n", word);
    return;
  }
  strcpy(copy, word);

  words = realloc(s->words, (s->nWords + 1) * sizeof(char *));
  if (!words) {
    fprintf(stderr, "Site_addWord: sem memoria para a lista de '%s'\n",
            s->name);
    free(copy);
    return;
  }

  s->words            = words;
  s->words[s->nWords] = copy;
  s->nWords++;
}

bool Site_hasLink(Graph *g, Site *s1, Site *s2) {
  if (!g || !s1 || !s2) {
    fprintf(stderr, "Site_hasLink: grafo ou site nulo\n");
    return false;
  }

  return Graph_findEdgeByValue(g, s1->name, s2->name, Site_cmpName) != NULL;
}

void Site_addLink(Graph *g, Site *s1, Site *s2, int weight) {
  if (!g || !s1 || !s2) {
    fprintf(stderr, "Site_addLink: grafo ou site nulo\n");
    return;
  }

  if (Graph_findEdgeByValue(g, s1->name, s2->name, Site_cmpName)) {
    fprintf(stderr, "Site_addLink: link '%s' -> '%s' ja existe\n", s1->name,
            s2->name);
    return;
  }

  Graph_addEdgeByValue(g, s1->name, s2->name, weight, Site_cmpName);

  if (!Graph_findEdgeByValue(g, s1->name, s2->name, Site_cmpName))
    fprintf(stderr,
            "Site_addLink: nao foi possivel criar o link '%s' -> '%s'\n",
            s1->name, s2->name);
}

int Site_cmpName(void *site, void *name) {
  return strcmp(((Site *)site)->name, (char *)name);
}

void Site_print(void *site) {
  if (!site) {
    fprintf(stderr, "Site_print: site nulo\n");
    return;
  }

  printf("%s", ((Site *)site)->name);
}
