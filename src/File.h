#ifndef FILE_H
#define FILE_H

#include "Graph.h"
#include "Site.h"
#include <stdbool.h>
#include <stdio.h>

#define FILE_MAX_LINE 512

bool File_readSites(Graph *g, const char *filename);
bool File_readLinks(Graph *g, const char *filename);
bool File_writeSiteToFile(const char *filename);

#endif /* FILE_H */
