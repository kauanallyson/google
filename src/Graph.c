#include "Graph.h"
#include <stdio.h>
#include <stdlib.h>

Graph *Graph_alloc(void) {
  Graph *g = malloc(sizeof(Graph));

  if (g) {
    g->n = 0;
    g->first = NULL;
    g->last = NULL;
  }

  return g;
}

void Graph_free(Graph *g) {
  Vertex *v = NULL, *nextv = NULL;
  Edge *e = NULL, *nexte = NULL;

  if (g) {
    v = g->first;

    while (v) {
      nextv = v->next;
      e = v->first;

      while (e) {
        nexte = e->next;
        free(e);
        e = nexte;
      }

      free(v);
      v = nextv;
    }

    free(g);
  }
}

void Graph_addVertex(Graph *g, int label, void *value) {
  Vertex *new = NULL;

  if (g) {
    new = malloc(sizeof(Vertex));
    new->first = NULL;
    new->last = NULL;
    new->next = NULL;
    new->n = 0;
    new->label = label;
    new->value = value;

    if (g->n == 0)
      g->first = new;
    else
      g->last->next = new;

    g->last = new;
    g->n++;
  }
}

Vertex *Graph_findVertexByLabel(Graph *g, int label) {
  Vertex *v = NULL;

  if (g && g->n) {

    v = g->first;

    while (v) {
      if (v->label == label)
        return v;

      v = v->next;
    }
  }

  return NULL;
}

Vertex *Graph_findVertexByValue(Graph *g, void *value,
                                int (*cmp)(void *, void *)) {
  Vertex *v = NULL;

  if (g && g->n && cmp) {

    v = g->first;

    while (v) {
      if (cmp(v->value, value) == 0)
        return v;

      v = v->next;
    }
  }

  return NULL;
}

void Graph_addEdge(Graph *g, int v0, int v1) {
  Edge *new = NULL;
  Vertex *tail = NULL;
  Vertex *head = NULL;

  if (g && g->n) {
    tail = Graph_findVertexByLabel(g, v0);
    head = Graph_findVertexByLabel(g, v1);

    if (tail && head) {
      new = malloc(sizeof(Edge));
      new->next = NULL;
      new->tail = tail;
      new->head = head;

      if (tail->n == 0)
        tail->first = new;
      else
        tail->last->next = new;

      tail->last = new;
      tail->n++;
    }
  }
}

void Graph_print(Graph *g, void (*print)(void *)) {
  Vertex *v = NULL;
  Edge *e = NULL;

  if (g) {
    v = g->first;

    while (v) {
      if (print) {
        print(v->value);
        printf(": ");
      } else
        printf("[%d]", v->label);

      e = v->first;
      while (e) {
        if (print)
          print(e->head->value);
        else
          printf("-->(%d)", e->head->label);
        e = e->next;
      }
      printf("\n");

      v = v->next;
    }
  }
}

Edge *Graph_findEdge(Graph *g, int v0, int v1) {
  Vertex *tail = NULL;
  Edge *e = NULL;

  if (g && g->n) {

    tail = Graph_findVertexByLabel(g, v0);

    if (tail) {
      e = tail->first;

      while (e) {
        if (e->head->label == v1)
          return e;

        e = e->next;
      }
    }
  }

  return NULL;
}

Edge *Graph_removeEdge(Graph *g, int v0, int v1) {
  Vertex *tail = NULL;
  Edge *e = NULL, *ant = NULL;

  if (g && g->n) {

    tail = Graph_findVertexByLabel(g, v0);

    if (tail) {
      e = tail->first;

      while (e) {
        if (e->head->label == v1) {
          if (ant == NULL)
            tail->first = e->next;
          else
            ant->next = e->next;

          tail->n--;
          return e;
        }

        ant = e;
        e = e->next;
      }
    }
  }

  return NULL;
}

void Edge_free(Vertex *v) {
  Edge *e = NULL, *next;

  if (v) {
    e = v->first;
    while (e) {
      next = e->next;
      free(e);
      e = next;
    }
    v->n = 0;
  }
}

Vertex *Graph_removeVertex(Graph *g, int label) {
  Vertex *v = NULL, *ant = NULL;

  if (g && g->n) {
    v = g->first;

    while (v) {
      Graph_removeEdge(g, v->label, label);
      v = v->next;
    }

    v = g->first;

    while (v) {
      if (v->label == label) {
        if (ant == NULL)
          g->first = v->next;
        else
          ant->next = v->next;

        g->n--;
        Edge_free(v);
        return v;
      }

      ant = v;
      v = v->next;
    }
  }

  return NULL;
}
