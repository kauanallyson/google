#include "File.h"
#include "Index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
  Graph *graph = Graph_alloc();
  if (!graph) {
    fprintf(stderr, "Não foi possível criar o grafo.\n");
    return EXIT_FAILURE;
  }

  Index *index  = NULL;
  int    status = EXIT_FAILURE;

  if (File_readFromArgs(graph, argc, argv)) {
    index = Index_fromGraph(graph);
    if (index) {
      char word[SITE_MAX_WORD + 1];
      printf("Digite uma palavra: ");
      fflush(stdout);

      if (fgets(word, sizeof(word), stdin)) {
        word[strcspn(word, "\n")] = '\0';
        if (!*word || strlen(word) >= SITE_MAX_WORD) {
          fprintf(stderr, "Digite uma única palavra de até %d bytes.\n",
                  SITE_MAX_WORD - 1);
        } else {
          const IndexEntry *entry = Index_get(index, word);
          printf("Sites com a palavra \"%s\":\n", word);
          if (!entry)
            printf("Nenhum site encontrado.\n");

          for (int i = 0; i < Index_count(entry); i++)
            printf("%s\n", Index_siteAt(entry, i)->name);

          status = EXIT_SUCCESS;
        }
      }
    } else {
      fprintf(stderr, "Não foi possível criar o índice.\n");
    }
  }

  if (index && !File_writeGraph(graph, "sites-out.tsv", "links-out.tsv"))
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
