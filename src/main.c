#include "File.h"
#include "Index.h"
#include "Ui.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  Graph *graph = Graph_alloc();
  if (!graph) {
    fprintf(stderr, "Não foi possível criar o grafo.\n");
    return EXIT_FAILURE;
  }

  const char *sitesPath = NULL;
  const char *linksPath = NULL;
  File_resolvePaths(argc, argv, &sitesPath, &linksPath);

  Index *index  = NULL;
  int    status = EXIT_FAILURE;

  if (File_readFromArgs(graph, argc, argv)) {
    index = Index_fromGraph(graph);
    if (index) {
      status = EXIT_SUCCESS;
      Ui_run(index, sitesPath, linksPath);
    } else {
      fprintf(stderr, "Não foi possível criar o índice.\n");
    }
  }

  if (index && !File_writeGraph(graph, sitesPath, linksPath))
    status = EXIT_FAILURE;

  Index_free(index);
  Vertex *vertex = graph->first;
  while (vertex) {
    Site_free(vertex->value);
    vertex = vertex->next;
  }
  Graph_free(graph);

  return status;
}
