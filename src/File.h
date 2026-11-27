#ifndef FILE_H
#define FILE_H

#include "Graph.h"
#include "Index.h"
#include "Site.h"
#include <stdbool.h>
#include <stdio.h>

#define FILE_MAX_LINE        512
#define FILE_SITES_DEFAULT   "data/sites.tsv"
#define FILE_LINKS_DEFAULT   "data/links.tsv"

bool File_readSites(Graph *g, const char *filename);
bool File_readLinks(Graph *g, const char *filename);
bool File_readFromArgs(Graph *g, int argc, char *argv[]);
bool File_loadIntoIndex(Index *index, const char *sitesPath,
                        const char *linksPath);
void File_resolvePaths(int argc, char *argv[], const char **sitesPath,
                       const char **linksPath);
bool File_writeGraph(const Graph *g, const char *sitesPath,
                     const char *linksPath);

#endif /* FILE_H */
