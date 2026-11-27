#include "File.h"
#include "Index.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool readLine(const char *prompt, char *buffer, size_t size) {
  printf("%s", prompt);
  fflush(stdout);

  if (!fgets(buffer, size, stdin))
    return false;

  size_t length = strlen(buffer);
  if (length > 0 && buffer[length - 1] == '\n') {
    buffer[--length] = '\0';
  } else {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF)
      ;
    fprintf(stderr, "Entrada longa demais (maximo de %zu caracteres).\n",
            size - 2);
    buffer[0] = '\0';
    return true;
  }

  if (length > 0 && buffer[length - 1] == '\r')
    buffer[--length] = '\0';

  return true;
}

static void searchWord(Index *index) {
  char word[SITE_MAX_WORD + 1];
  if (!readLine("Digite uma palavra: ", word, sizeof(word)))
    return;

  if (!*word || strlen(word) >= SITE_MAX_WORD) {
    fprintf(stderr, "Digite uma única palavra de até %d bytes.\n",
            SITE_MAX_WORD - 1);
    return;
  }

  const IndexEntry *entry = Index_get(index, word);
  printf("Sites com a palavra \"%s\":\n", word);
  if (!entry) {
    printf("Nenhum site encontrado.\n");
    return;
  }

  for (int i = 0; i < Index_count(entry); i++)
    printf("%s\n", Index_siteAt(entry, i)->name);
}

static void removeSite(Index *index) {
  char name[SITE_MAX_NAME + 1];
  if (!readLine("Nome do site a remover: ", name, sizeof(name)))
    return;

  if (!*name || strlen(name) >= SITE_MAX_NAME) {
    fprintf(stderr, "Nome de site invalido.\n");
    return;
  }

  if (Index_removeSiteByName(index, name))
    printf("Site '%s' removido com sucesso.\n", name);
  else
    printf("Site '%s' nao encontrado.\n", name);
}

static void removeLink(Index *index) {
  char origin[SITE_MAX_NAME + 1];
  char destination[SITE_MAX_NAME + 1];

  if (!readLine("Site de origem: ", origin, sizeof(origin)))
    return;
  if (!*origin || strlen(origin) >= SITE_MAX_NAME) {
    fprintf(stderr, "Nome de site de origem invalido.\n");
    return;
  }

  if (!readLine("Site de destino: ", destination, sizeof(destination)))
    return;
  if (!*destination || strlen(destination) >= SITE_MAX_NAME) {
    fprintf(stderr, "Nome de site de destino invalido.\n");
    return;
  }

  if (Index_removeLink(index, origin, destination))
    printf("Link '%s' -> '%s' removido com sucesso.\n", origin, destination);
  else
    printf("Link '%s' -> '%s' nao encontrado.\n", origin, destination);
}

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
      status = EXIT_SUCCESS;
      char option[16];
      bool running = true;

      while (running) {
        printf("\n1 - Pesquisar palavra\n"
               "2 - Remover site\n"
               "3 - Remover link\n"
               "4 - Listar sites e links\n"
               "0 - Sair\n");
        if (!readLine("Escolha uma opcao: ", option, sizeof(option)))
          break;

        if (strcmp(option, "1") == 0)
          searchWord(index);
        else if (strcmp(option, "2") == 0)
          removeSite(index);
        else if (strcmp(option, "3") == 0)
          removeLink(index);
        else if (strcmp(option, "4") == 0)
          Graph_print(graph, Site_print);
        else if (strcmp(option, "0") == 0)
          running = false;
        else
          printf("Opcao invalida.\n");
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
