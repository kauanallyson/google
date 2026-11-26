#include "File.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool File_readFromArgs(Graph *g, int argc, char *argv[]) {
  if (argc == 1)
    return File_readSites(g, "data/sites.tsv") &&
           File_readLinks(g, "data/links.tsv");

  if (argc == 3)
    return File_readSites(g, argv[1]) && File_readLinks(g, argv[2]);

  fprintf(stderr, "Uso: %s [arquivo-de-sites arquivo-de-links]\n", argv[0]);
  return false;
}

bool File_readSites(Graph *g, const char *filename) {
  FILE *f;
  char  row[FILE_MAX_LINE];
  int   line = 0;

  if (!g || !filename) {
    fprintf(stderr, "File_readSites: grafo ou nome de arquivo nulo\n");
    return false;
  }

  if (!(f = fopen(filename, "r"))) {
    fprintf(stderr, "File_readSites: nao foi possivel abrir '%s': %s\n",
            filename, strerror(errno));
    return false;
  }

  while (fgets(row, sizeof row, f)) {
    line++;
    size_t length = strlen(row);
    if (length == sizeof row - 1 && row[length - 1] != '\n') {
      int ch;
      while ((ch = fgetc(f)) != '\n' && ch != EOF)
        ;
      fprintf(stderr, "%s:%d: linha longa demais, ignorada\n", filename, line);
      continue;
    }

    if (length > 0 && row[length - 1] == '\n')
      row[--length] = '\0';
    if (length > 0 && row[length - 1] == '\r')
      row[--length] = '\0';

    char *word = strchr(row, '\t');
    if (word)
      *word++ = '\0';

    const char *whitespace = " \t\r\n\v\f";
    if (!word || !*row || strlen(row) >= SITE_MAX_NAME ||
        strlen(word) >= SITE_MAX_WORD || strpbrk(row, whitespace) ||
        strpbrk(word, whitespace)) {
      fprintf(stderr, "%s:%d: linha mal formada (esperado: site e palavra)\n",
              filename, line);
      continue;
    }

    Vertex *vertex = Graph_findVertexByValue(g, row, Site_cmpName);
    Site *site;
    if (vertex) {
      site = vertex->value;
    } else {
      site = Site_alloc(row);
      if (!site)
        continue;

      int label = g->last ? g->last->label + 1 : 0;
      Graph_addVertex(g, label, site);
      if (!Graph_findVertexByLabel(g, label)) {
        fprintf(stderr, "%s:%d: nao foi possivel adicionar '%s' ao grafo\n",
                filename, line, row);
        Site_free(site);
        continue;
      }
    }

    if (*word)
      Site_addWord(site, word);
  }

  bool ok = !ferror(f);
  return fclose(f) == 0 && ok;
}

bool File_writeGraph(const Graph *g, const char *sitesPath,
                     const char *linksPath) {
  if (!g || !sitesPath || !linksPath || strcmp(sitesPath, linksPath) == 0)
    return false;

  FILE *sites = fopen(sitesPath, "w");
  if (!sites) {
    perror(sitesPath);
    return false;
  }

  FILE *links = fopen(linksPath, "w");
  if (!links) {
    perror(linksPath);
    fclose(sites);
    return false;
  }

  bool ok = true;
  const Vertex *vertex = g->first;
  while (ok && vertex) {
    const Site *site = vertex->value;
    if (site->nWords == 0)
      ok = fprintf(sites, "%s\t\n", site->name) >= 0;
    for (int i = 0; ok && i < site->nWords; i++)
      ok = fprintf(sites, "%s\t%s\n", site->name, site->words[i]) >= 0;

    const Edge *edge = vertex->first;
    while (ok && edge) {
      const Site *destination = edge->head->value;
      ok = fprintf(links, "%s\t%s\t%d\n", site->name, destination->name,
                   edge->weight) >= 0;
      edge = edge->next;
    }
    vertex = vertex->next;
  }

  if (fclose(sites) == EOF)
    ok = false;
  if (fclose(links) == EOF)
    ok = false;
  if (!ok)
    fprintf(stderr, "Erro ao salvar o grafo em '%s' e '%s'.\n", sitesPath,
            linksPath);
  return ok;
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
