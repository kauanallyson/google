#ifndef GRAPH_H
#define GRAPH_H

#include <stdbool.h>

typedef struct Graph  Graph;
typedef struct Vertex Vertex;
typedef struct Edge   Edge;

struct Graph {
  int     n;
  Vertex *first, *last;
};

struct Vertex {
  void *value;

  int     label;
  int     weight;
  int     n;
  Vertex *next;
  Edge   *first, *last;
};

struct Edge {
  Vertex *tail, *head;
  int     weight;
  Edge   *next;
};

Graph *Graph_alloc(void);
void   Graph_free(Graph *g);

Vertex *Graph_findVertexByLabel(Graph *g, int label);
Vertex *Graph_findVertexByValue(Graph *g, void *value,
                                int (*cmp)(void *, void *));

Edge *Graph_findEdge(Graph *g, int v0, int v1);

void Graph_addVertex(Graph *g, int label, void *value);
void Graph_addEdge(Graph *g, int v0, int v1, int weight);
bool Graph_setEdgeWeight(Graph *g, int v0, int v1, int weight);

Vertex *Graph_removeVertex(Graph *g, int label);
Edge   *Graph_removeEdge(Graph *g, int v0, int v1);

void Graph_print(Graph *g, void (*print)(void *));

#endif /* GRAPH_H */
