#include "File.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool File_readSites(Graph *g, const char *filename) {
  FILE *f = NULL;
  Site *s = NULL;
  char  name[SITE_MAX_NAME];
  char  word[SITE_MAX_WORD];
  char  sep;
  int   label, n, nWords, line = 0;

  if (!g || !filename) {
    fprintf(stderr, "File_readSites: grafo ou nome de arquivo nulo\n");
    return false;
  }

  if (!(f = fopen(filename, "r"))) {
    fprintf(stderr, "File_readSites: nao foi possivel abrir '%s': %s\n",
            filename, strerror(errno));
    return false;
  }

  while ((n = fscanf(f, "%63s%c", name, &sep)) >= 1) {
    line++;
    s = NULL;

    if (n == 2 && sep != '\t' && sep != '\n') {
      fprintf(stderr,
              "%s:%d: nome do site maior que %d caracteres ou separado por "
              "espaco, linha ignorada\n",
              filename, line, SITE_MAX_NAME - 1);
      fscanf(f, "%*[^\n]");
      continue;
    }

    if (Graph_findVertexByValue(g, name, Site_cmpName))
      fprintf(stderr, "%s:%d: site '%s' repetido, linha ignorada\n", filename,
              line, name);
    else if (!(s = Site_alloc(name)))
      fprintf(stderr, "%s:%d: nao foi possivel criar o site '%s'\n", filename,
              line, name);

    while (n == 2 && sep == '\t') {
      if (fscanf(f, "%c", &sep) != 1 || sep == '\n') {
        fprintf(stderr, "%s:%d: tab sobrando no fim da linha\n", filename,
                line);
        break;
      }

      if (sep == '\t') {
        fprintf(stderr, "%s:%d: dois tabs seguidos (palavra vazia)\n",
                filename, line);
        continue;
      }

      ungetc(sep, f);
      n = fscanf(f, "%31s%c", word, &sep);

      if (n < 1) {
        fprintf(stderr, "%s:%d: fim do arquivo inesperado\n", filename, line);
        break;
      }

      if (n == 2 && sep != '\t' && sep != '\n') {
        fprintf(stderr,
                "%s:%d: palavra maior que %d caracteres ou separada por "
                "espaco, resto da linha ignorado\n",
                filename, line, SITE_MAX_WORD - 1);
        fscanf(f, "%*[^\n]");
        break;
      }

      if (s) {
        nWords = s->nWords;
        Site_addWord(s, word);

        if (s->nWords == nWords)
          fprintf(stderr, "%s:%d: palavra '%s' repetida em '%s', ignorada\n",
                  filename, line, word, name);
      }
    }

    if (s) {
      label = g->last ? g->last->label + 1 : 0;
      Graph_addVertex(g, label, s);

      if (!Graph_findVertexByLabel(g, label)) {
        fprintf(stderr, "%s:%d: nao foi possivel adicionar '%s' ao grafo\n",
                filename, line, name);
        Site_free(s);
      }
    }
  }

  fclose(f);
  return true;
}

bool File_readLinks(Graph *g, const char *filename) {
  FILE *f = NULL;
  char  tail[SITE_MAX_NAME];
  char  head[SITE_MAX_NAME];
  int   weight, n, line = 0;

  if (!g || !filename) {
    fprintf(stderr, "File_readLinks: grafo ou nome de arquivo nulo\n");
    return false;
  }

  if (!(f = fopen(filename, "r"))) {
    fprintf(stderr, "File_readLinks: nao foi possivel abrir '%s': %s\n",
            filename, strerror(errno));
    return false;
  }

  while ((n = fscanf(f, "%63s %63s %d", tail, head, &weight)) != EOF) {
    line++;

    if (n != 3) {
      fprintf(stderr,
              "%s:%d: linha mal formada (esperado: origem destino peso), "
              "ignorada\n",
              filename, line);
      fscanf(f, "%*[^\n]");
      continue;
    }

    if (!Graph_findVertexByValue(g, tail, Site_cmpName))
      fprintf(stderr, "%s:%d: site de origem '%s' nao existe\n", filename,
              line, tail);
    else if (!Graph_findVertexByValue(g, head, Site_cmpName))
      fprintf(stderr, "%s:%d: site de destino '%s' nao existe\n", filename,
              line, head);
    else if (Graph_findEdgeByValue(g, tail, head, Site_cmpName))
      fprintf(stderr, "%s:%d: link '%s' -> '%s' repetido, ignorado\n",
              filename, line, tail, head);
    else {
      Graph_addEdgeByValue(g, tail, head, weight, Site_cmpName);

      if (!Graph_findEdgeByValue(g, tail, head, Site_cmpName))
        fprintf(stderr,
                "%s:%d: nao foi possivel adicionar o link '%s' -> '%s'\n",
                filename, line, tail, head);
    }
  }

  fclose(f);
  return true;
}
