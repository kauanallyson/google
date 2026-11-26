#include "File.h"
#include "Ui.h"
#include <stdlib.h>

int main(int argc, char *argv[]) {
  if (argc > 3) {
    fprintf(stderr, "Uso: %s [arquivo_sites [arquivo_links]]\n", argv[0]);
    return EXIT_FAILURE;
  }

  const char *sitesPath = argc > 1 ? argv[1] : "data/sites.tsv";
  const char *linksPath = argc > 2 ? argv[2] : "data/links.tsv";
  Graph *graph = Graph_alloc();
  if (!graph) {
    perror("Erro ao criar o grafo");
    return EXIT_FAILURE;
  }

  bool ok = File_readSites(graph, sitesPath) &&
            File_readLinks(graph, linksPath) &&
            Ui_run(graph, sitesPath, linksPath);

  for (Vertex *vertex = graph->first; vertex; vertex = vertex->next)
    Site_free(vertex->value);
  Graph_free(graph);

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
