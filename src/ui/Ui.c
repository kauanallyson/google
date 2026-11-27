#include "Ui.h"
#include "Clic.h"
#include "File.h"
#include "Search.h"
#include "Site.h"
#include <errno.h>
#include <limits.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UI_PAGE_SIZE         5
#define UI_MAX_LINKS         5
#define UI_MAX_PANEL_CONTENT 52
#define UI_DESCRIPTION_SIZE 160

typedef struct {
  char name[SITE_MAX_NAME + 16];
  char weight[16];
  char description[UI_DESCRIPTION_SIZE];
  int  lines;
} UiResultRow;

enum {
  UiKeyEscape = 27,
  UiKeyUp     = 256,
  UiKeyDown,
  UiKeyRight,
  UiKeyLeft
};

static int Ui_readKeyByte(void) {
  unsigned char byte;
  return read(STDIN_FILENO, &byte, 1) == 1 ? byte : EOF;
}

static int Ui_readEscapeByte(void) {
  struct pollfd input = {.fd = STDIN_FILENO, .events = POLLIN};
  return poll(&input, 1, 50) > 0 ? Ui_readKeyByte() : EOF;
}

static int Ui_fromClicKey(int key) {
  if (key == Key_UP)
    return UiKeyUp;
  if (key == Key_DOWN)
    return UiKeyDown;
  if (key == Key_RIGHT)
    return UiKeyRight;
  if (key == Key_LEFT)
    return UiKeyLeft;
  return key;
}

static int Ui_captureKey(void) {
  struct termios original;
  if (tcgetattr(STDIN_FILENO, &original) == -1)
    return Ui_fromClicKey(Clic_keyCapture());

  struct termios mode = original;
  mode.c_lflag &= ~(ICANON | ECHO);
  mode.c_cc[VMIN]  = 1;
  mode.c_cc[VTIME] = 0;
  if (tcsetattr(STDIN_FILENO, TCSANOW, &mode) == -1)
    return Ui_fromClicKey(Clic_keyCapture());

  int key = Ui_readKeyByte();
  if (key == UiKeyEscape) {
    int prefix = Ui_readEscapeByte();
    if (prefix == '[' || prefix == 'O') {
      int code = Ui_readEscapeByte();
      if (code == 'A')
        key = UiKeyUp;
      else if (code == 'B')
        key = UiKeyDown;
      else if (code == 'C')
        key = UiKeyRight;
      else if (code == 'D')
        key = UiKeyLeft;
    }
  }

  tcsetattr(STDIN_FILENO, TCSANOW, &original);
  return key;
}

static void Ui_moveToCenter(int row, int width) {
  int column = (Clic_getScreenWidth() - width) / 2 + 1;
  Clic_move(row > 0 ? row : 1, column > 0 ? column : 1);
}

static void Ui_drawBorder(int row, int column, int width, char *left,
                          char *right) {
  Clic_move(row, column);
  Clic_printSymbol(left);
  Clic_printHLine(width - 2);
  Clic_printSymbol(right);
}

static int Ui_drawPanel(int rows, int contentWidth) {
  int height  = Clic_getScreenHeight();
  int columns = Clic_getScreenWidth();
  Clic_clearScreen();

  if (height < rows + 2 || columns < contentWidth + 4) {
    int top = (height - rows) / 2 + 1;
    return top > 0 ? top : 1;
  }

  int width  = contentWidth + 4;
  int column = (columns - width) / 2 + 1;
  int top    = (height - rows - 2) / 2 + 1;

  Clic_setFontColor(Color_CIAN);
  Ui_drawBorder(top, column, width, Symbol_CORNER_TL, Symbol_CORNER_TR);
  for (int row = top + 1; row <= top + rows; row++) {
    Clic_move(row, column);
    Clic_printSymbol(Symbol_VLINE);
    Clic_move(row, column + width - 1);
    Clic_printSymbol(Symbol_VLINE);
  }
  Ui_drawBorder(top + rows + 1, column, width, Symbol_CORNER_BL,
                Symbol_CORNER_BR);
  Clic_resetColor();
  return top + 1;
}

static void Ui_printCenteredWidth(int row, const char *text, Color color,
                                  int limit) {
  int width = (int)strlen(text);
  if (width > limit)
    width = limit;
  if (width < 0)
    width = 0;

  Ui_moveToCenter(row, width);
  Clic_setFontColor(color);
  printf("%-*.*s", width, width, text);
  Clic_resetColor();
}

static void Ui_printCentered(int row, const char *text, Color color) {
  int columns = Clic_getScreenWidth();
  Ui_printCenteredWidth(row, text, color, columns > 2 ? columns - 2 : 0);
}


static void Ui_printFit(const char *text, int width) {
  if (width < 1)
    return;
  printf("%-*.*s", width, width, text);
}

static void Ui_printSlice(const char *text, int offset, int width) {
  int length = (int)strlen(text);
  if (offset >= length || width < 1) {
    printf("%-*s", width > 0 ? width : 0, "");
    return;
  }

  int shown = length - offset;
  if (shown > width)
    shown = width;
  printf("%-*.*s", width, shown, text + offset);
}

static int Ui_panelContentWidth(void) {
  int columns = Clic_getScreenWidth();
  int width   = columns - 4;
  if (width > UI_MAX_PANEL_CONTENT)
    width = UI_MAX_PANEL_CONTENT;
  return width > 12 ? width : 12;
}

static int Ui_panelStartColumn(int contentWidth) {
  int columns     = Clic_getScreenWidth();
  int panelWidth  = contentWidth + 4;
  int panelColumn = (columns - panelWidth) / 2 + 1;
  return panelColumn > 0 ? panelColumn : 1;
}

static bool Ui_readEditedLine(char *buffer, size_t size, int row, int column,
                              int width) {
  struct termios original;
  bool hasTerminal = tcgetattr(STDIN_FILENO, &original) != -1;

  if (hasTerminal) {
    struct termios mode = original;
    mode.c_lflag &= ~(ICANON | ECHO);
    mode.c_cc[VMIN]  = 1;
    mode.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &mode) == -1)
      hasTerminal = false;
  }

  size_t length = 0;
  bool   ok     = true;

  while (true) {
    buffer[length] = '\0';
    size_t scroll = length > (size_t)width ? length - (size_t)width : 0;
    Clic_move(row, column);
    Ui_printSlice(buffer + scroll, 0, width);
    Clic_move(row, column + (int)(length - scroll));
    fflush(stdout);

    int ch = Ui_readKeyByte();
    if (ch == EOF || ch == UiKeyEscape) {
      ok = false;
      break;
    }
    if (ch == '\n' || ch == '\r')
      break;
    if (ch == 127 || ch == 8) {
      if (length > 0)
        length--;
    } else if (ch >= 32 && ch < 127 && length + 1 < size) {
      buffer[length++] = (char)ch;
    }
  }

  buffer[length] = '\0';
  if (hasTerminal)
    tcsetattr(STDIN_FILENO, TCSANOW, &original);
  return ok;
}

static bool Ui_readLine(const char *prompt, char *buffer, size_t size) {
  int contentWidth = Ui_panelContentWidth();
  int top          = Ui_drawPanel(3, contentWidth);
  int fieldWidth   = contentWidth - 2;
  if (fieldWidth < 1)
    fieldWidth = 1;

  Ui_printCenteredWidth(top, prompt, Color_CIAN, contentWidth);
  return Ui_readEditedLine(buffer, size, top + 2,
                           Ui_panelStartColumn(contentWidth) + 2, fieldWidth);
}

static void Ui_showMessage(const char *message, bool error) {
  int contentWidth = Ui_panelContentWidth();
  int top          = Ui_drawPanel(3, contentWidth);

  Ui_printCenteredWidth(top, message, error ? Color_RED : Color_GREEN,
                        contentWidth);
  Ui_printCenteredWidth(top + 2, "ENTER para continuar", Color_GRAY,
                        contentWidth);
  fflush(stdout);
  Ui_captureKey();
}

static char *Ui_trim(char *text) {
  const char *whitespace = " \t\r\n\v\f";

  while (*text && strchr(whitespace, *text))
    text++;

  size_t length = strlen(text);
  while (length > 0 && strchr(whitespace, text[length - 1]))
    text[--length] = '\0';

  return text;
}

static bool Ui_isValidToken(const char *text, size_t maxLength) {
  if (!text || !*text || strlen(text) >= maxLength)
    return false;

  return strpbrk(text, " \t\r\n\v\f") == NULL;
}

static Vertex *Ui_findSite(Index *index, const char *name) {
  if (!index || !index->graph || !Ui_isValidToken(name, SITE_MAX_NAME))
    return NULL;

  return Graph_findVertexByValue(index->graph, (void *)name, Site_cmpName);
}

static int Ui_nextSiteLabel(const Graph *graph) {
  int label = 0;
  for (const Vertex *vertex = graph->first; vertex; vertex = vertex->next)
    if (vertex->label >= label)
      label = vertex->label + 1;

  return label;
}

static void Ui_buildDescription(char *buffer, size_t size, const Site *site) {
  if (size == 0)
    return;

  buffer[0] = '\0';
  if (!site || site->nWords == 0) {
    snprintf(buffer, size, "Sem palavras-chave cadastradas.");
    return;
  }

  size_t used = 0;
  for (int i = 0; i < site->nWords; i++) {
    const char *separator = i ? " | " : "";
    int written = snprintf(buffer + used, size - used, "%s%s", separator,
                           site->words[i]);
    if (written < 0 || (size_t)written >= size - used)
      break;
    used += (size_t)written;
  }
}

static int Ui_compareLinks(const void *a, const void *b) {
  const Edge  *left     = *(const Edge *const *)a;
  const Edge  *right    = *(const Edge *const *)b;
  const Site  *leftSite = left->head->value;
  const Site  *rightSite = right->head->value;

  return strcmp(leftSite->name, rightSite->name);
}

static void Ui_printLinks(int row, const Vertex *vertex) {
  int count = 0;
  for (const Edge *edge = vertex ? vertex->first : NULL; edge;
       edge = edge->next)
    count++;

  if (count == 0) {
    Ui_printCentered(row, "nenhum.", Color_GRAY);
    return;
  }

  const Edge **edges = malloc((size_t)count * sizeof(*edges));
  if (!edges) {
    Ui_printCentered(row, "nao foi possivel listar os links.", Color_RED);
    return;
  }

  int index = 0;
  for (const Edge *edge = vertex->first; edge; edge = edge->next)
    edges[index++] = edge;

  qsort(edges, (size_t)count, sizeof(*edges), Ui_compareLinks);

  int printed = 0;
  for (int i = 0; i < count && printed < UI_MAX_LINKS; i++) {
    const Site *destination = edges[i]->head->value;
    char        line[SITE_MAX_NAME + 32];
    snprintf(line, sizeof(line), "-> %s (peso %d)", destination->name,
             edges[i]->weight);
    Ui_printCentered(row + printed, line, Color_CIAN);
    printed++;
  }

  if (printed < count) {
    char more[32];
    snprintf(more, sizeof(more), "... +%d link(s)", count - printed);
    Ui_printCentered(row + printed, more, Color_GRAY);
  }

  free(edges);
}

static void Ui_showVertices(const Vertex **vertices, int count,
                            const char *title, const char *subtitle) {
  if (count <= 0) {
    Ui_showMessage("Nenhum site encontrado.", false);
    return;
  }

  int pageCount = (count + UI_PAGE_SIZE - 1) / UI_PAGE_SIZE;
  int page      = 0;
  int selected  = 0;
  const char *help = "Setas: navegar | ENTER/Q/ESC: voltar";

  while (true) {
    int start    = page * UI_PAGE_SIZE;
    int end      = start + UI_PAGE_SIZE;
    if (end > count)
      end = count;
    int pageRows = end - start;

    int titleWidth  = 17;
    int descWidth   = 24;
    int weightWidth = 11;
    int columns     = Clic_getScreenWidth();
    if (columns > 70) {
      descWidth = columns - 6 - titleWidth - weightWidth - 2;
      if (descWidth > 80)
        descWidth = 80;
    }
    if (descWidth < 16)
      descWidth = 16;

    UiResultRow rows[UI_PAGE_SIZE];
    int         dataLines = 0;

    for (int i = start; i < end; i++) {
      int slot = i - start;
      const Site *site = vertices[i] ? vertices[i]->value : NULL;
      if (!site) {
        rows[slot].lines = 0;
        continue;
      }

      snprintf(rows[slot].name, sizeof(rows[slot].name), "%d. %s", i + 1,
               site->name);
      snprintf(rows[slot].weight, sizeof(rows[slot].weight), "%d",
               vertices[i]->weight);
      Ui_buildDescription(rows[slot].description,
                          sizeof(rows[slot].description), site);

      int length = (int)strlen(rows[slot].description);
      int lines  = (length + descWidth - 1) / descWidth;
      if (lines < 1)
        lines = 1;
      if (lines > 3)
        lines = 3;
      rows[slot].lines = lines;
      dataLines += lines;
    }

    int tableWidth   = titleWidth + descWidth + weightWidth + 2;
    int contentWidth = tableWidth;
    if ((int)strlen(subtitle) > contentWidth)
      contentWidth = (int)strlen(subtitle);
    if ((int)strlen(help) > contentWidth)
      contentWidth = (int)strlen(help);

    int rowsCount = dataLines + UI_MAX_LINKS + 8;
    int top       = Ui_drawPanel(rowsCount, contentWidth);

    int panelWidth  = contentWidth + 4;
    int panelColumn = (columns - panelWidth) / 2 + 1;
    if (panelColumn < 1)
      panelColumn = 1;
    int tableColumn = panelColumn + 2;

    Ui_printCentered(top, title, Color_CIAN);
    Ui_printCentered(top + 1, subtitle, Color_GRAY);

    char pageInfo[96];
    snprintf(pageInfo, sizeof(pageInfo), "Pagina %d de %d | site %d de %d",
             page + 1, pageCount, selected + 1, count);
    Ui_printCentered(top + 2, pageInfo, Color_GRAY);

    Clic_move(top + 4, tableColumn);
    Clic_setFontColor(Color_YELLOW);
    Ui_printFit("Titulo", titleWidth);
    printf(" ");
    Ui_printFit("Descricao", descWidth);
    printf(" ");
    Ui_printFit("Importancia", weightWidth);
    Clic_resetColor();

    int lineRow = top + 5;
    for (int slot = 0; slot < pageRows; slot++) {
      if (rows[slot].lines == 0)
        continue;

      bool highlighted = (start + slot == selected);
      for (int line = 0; line < rows[slot].lines; line++) {
        Clic_move(lineRow++, tableColumn);
        if (highlighted) {
          Clic_setBackgroundColor(Color_YELLOW);
          Clic_setFontColor(Color_BLACK);
        }

        if (line == 0)
          Ui_printFit(rows[slot].name, titleWidth);
        else
          printf("%-*s", titleWidth, "");
        printf(" ");

        Ui_printSlice(rows[slot].description, line * descWidth, descWidth);
        printf(" ");
        if (line == 0)
          Ui_printFit(rows[slot].weight, weightWidth);
        else
          printf("%-*s", weightWidth, "");
        Clic_resetColor();
      }
    }

    int linksRow = top + 6 + dataLines;
    Ui_printCentered(linksRow, "Sites conectados (links de saida):",
                     Color_YELLOW);
    Ui_printLinks(linksRow + 1, vertices[selected]);
    Ui_printCentered(linksRow + UI_MAX_LINKS + 1, help, Color_GRAY);
    fflush(stdout);

    int key = Ui_captureKey();
    if (key == UiKeyUp) {
      if (selected > 0) {
        selected--;
        if (selected < start)
          page--;
      }
    } else if (key == UiKeyDown) {
      if (selected < count - 1) {
        selected++;
        if (selected >= end) {
          page++;
          start = page * UI_PAGE_SIZE;
        }
      }
    } else if (key == UiKeyRight) {
      if (end < count) {
        page++;
        selected = page * UI_PAGE_SIZE;
      }
    } else if (key == UiKeyLeft) {
      if (page > 0) {
        page--;
        selected = page * UI_PAGE_SIZE + UI_PAGE_SIZE - 1;
        if (selected >= count)
          selected = count - 1;
      }
    }

    if (key == Key_ENTER || key == '\r' || key == Key_Q || key == UiKeyEscape ||
        key == EOF)
      break;
  }
}

static void Ui_search(Index *index) {
  char buffer[FILE_MAX_LINE];

  if (!Ui_readLine("Consulta (ex: computador AND preco): ", buffer,
                   sizeof(buffer)))
    return;

  char *query = Ui_trim(buffer);
  if (!*query) {
    Ui_showMessage("Consulta vazia.", true);
    return;
  }

  SearchResult *result = Search_query(index, query);
  if (!result) {
    char message[128];
    snprintf(message, sizeof(message),
             "Consulta invalida: use palavras de ate %d bytes e AND/OR.",
             SITE_MAX_WORD - 1);
    Ui_showMessage(message, true);
    return;
  }

  char subtitle[FILE_MAX_LINE + 64];
  snprintf(subtitle, sizeof(subtitle), "Consulta: \"%s\" | %d resultado(s)",
           query, Search_count(result));

  Ui_showVertices(result->sites, result->count, "RESULTADOS DA PESQUISA",
                  subtitle);
  Search_free(result);
}

static void Ui_listSites(Index *index) {
  int count = 0;
  for (const Vertex *vertex = index->graph->first; vertex;
       vertex = vertex->next)
    count++;

  if (count == 0) {
    Ui_showMessage("Nenhum site cadastrado.", false);
    return;
  }

  const Vertex **vertices = malloc((size_t)count * sizeof(*vertices));
  if (!vertices) {
    Ui_showMessage("Nao foi possivel listar os sites.", true);
    return;
  }

  int position = 0;
  for (const Vertex *vertex = index->graph->first; vertex;
       vertex = vertex->next)
    vertices[position++] = vertex;

  Search_sortVertices(vertices, count);

  Ui_showVertices(vertices, count, "SITES CADASTRADOS",
                  "Todos os sites da miniatura da internet");
  free(vertices);
}

static bool Ui_removeSite(Index *index) {
  char name[SITE_MAX_NAME + 1];
  char message[SITE_MAX_NAME + 64];

  if (!Ui_readLine("Nome do site a remover: ", name, sizeof(name)))
    return false;

  if (!*name || strlen(name) >= SITE_MAX_NAME) {
    Ui_showMessage("Nome de site invalido.", true);
    return false;
  }

  if (Index_removeSiteByName(index, name)) {
    snprintf(message, sizeof(message), "Site '%s' removido com sucesso.", name);
    Ui_showMessage(message, false);
    return true;
  }

  snprintf(message, sizeof(message), "Site '%s' nao encontrado.", name);
  Ui_showMessage(message, true);
  return false;
}

static bool Ui_removeLink(Index *index) {
  char origin[SITE_MAX_NAME + 1];
  char destination[SITE_MAX_NAME + 1];
  char message[SITE_MAX_NAME * 2 + 64];

  if (!Ui_readLine("Site de origem: ", origin, sizeof(origin)))
    return false;
  if (!*origin || strlen(origin) >= SITE_MAX_NAME) {
    Ui_showMessage("Nome de site de origem invalido.", true);
    return false;
  }

  if (!Ui_readLine("Site de destino: ", destination, sizeof(destination)))
    return false;
  if (!*destination || strlen(destination) >= SITE_MAX_NAME) {
    Ui_showMessage("Nome de site de destino invalido.", true);
    return false;
  }

  if (Index_removeLink(index, origin, destination)) {
    snprintf(message, sizeof(message), "Link '%s' -> '%s' removido com sucesso.",
             origin, destination);
    Ui_showMessage(message, false);
    return true;
  }

  snprintf(message, sizeof(message), "Link '%s' -> '%s' nao encontrado.",
           origin, destination);
  Ui_showMessage(message, true);
  return false;
}

static bool Ui_save(Index *index, const char *sitesPath,
                    const char *linksPath) {
  if (!index || !index->graph || !sitesPath || !linksPath)
    return false;

  bool saved = File_writeGraph(index->graph, sitesPath, linksPath);
  if (saved) {
    char message[FILE_MAX_LINE];
    snprintf(message, sizeof(message), "Alteracoes salvas em '%s' e '%s'.",
             sitesPath, linksPath);
    Ui_showMessage(message, false);
  } else {
    Ui_showMessage("Nao foi possivel salvar as alteracoes.", true);
  }

  return saved;
}

static bool Ui_registerSite(Index *index) {
  char name[SITE_MAX_NAME + 1];
  char message[SITE_MAX_NAME + 96];

  if (!index || !index->graph)
    return false;

  if (!Ui_readLine("Nome do novo site: ", name, sizeof(name)))
    return false;

  char *siteName = Ui_trim(name);
  if (!Ui_isValidToken(siteName, SITE_MAX_NAME)) {
    Ui_showMessage("Nome de site invalido.", true);
    return false;
  }

  if (Ui_findSite(index, siteName)) {
    snprintf(message, sizeof(message), "Site '%s' ja cadastrado.", siteName);
    Ui_showMessage(message, true);
    return false;
  }

  Site *site = Site_alloc(siteName);
  if (!site) {
    Ui_showMessage("Nao foi possivel alocar o novo site.", true);
    return false;
  }

  if (!Index_addSite(index, Ui_nextSiteLabel(index->graph), site)) {
    Site_free(site);
    Ui_showMessage("Nao foi possivel cadastrar o site.", true);
    return false;
  }

  snprintf(message, sizeof(message),
           "Site '%s' cadastrado. Adicione palavras-chave para pesquisar.",
           siteName);
  Ui_showMessage(message, false);
  return true;
}

static bool Ui_registerWord(Index *index) {
  char name[SITE_MAX_NAME + 1];
  char word[SITE_MAX_WORD + 1];
  char message[FILE_MAX_LINE];

  if (!index || !index->graph)
    return false;

  if (!Ui_readLine("Nome do site: ", name, sizeof(name)))
    return false;

  char *siteName = Ui_trim(name);
  Vertex *vertex = Ui_findSite(index, siteName);
  if (!vertex) {
    snprintf(message, sizeof(message), "Site '%s' nao encontrado.", siteName);
    Ui_showMessage(message, true);
    return false;
  }

  if (!Ui_readLine("Nova palavra-chave: ", word, sizeof(word)))
    return false;

  char *keyword = Ui_trim(word);
  if (!Ui_isValidToken(keyword, SITE_MAX_WORD)) {
    Ui_showMessage("Palavra-chave invalida.", true);
    return false;
  }

  Site *site = vertex->value;
  if (Site_hasWord(site, keyword)) {
    snprintf(message, sizeof(message),
             "Palavra-chave '%s' ja cadastrada em '%s'.", keyword, siteName);
    Ui_showMessage(message, true);
    return false;
  }

  if (!Index_addWord(index, vertex->label, keyword)) {
    Ui_showMessage("Nao foi possivel cadastrar a palavra-chave.", true);
    return false;
  }

  snprintf(message, sizeof(message), "Palavra-chave '%s' adicionada a '%s'.",
           keyword, siteName);
  Ui_showMessage(message, false);
  return true;
}

static bool Ui_registerLink(Index *index) {
  char originName[SITE_MAX_NAME + 1];
  char destinationName[SITE_MAX_NAME + 1];
  char weightText[32];
  char message[FILE_MAX_LINE];

  if (!index || !index->graph)
    return false;

  if (!Ui_readLine("Site de origem: ", originName, sizeof(originName)))
    return false;

  char *origin = Ui_trim(originName);
  Vertex *tail = Ui_findSite(index, origin);
  if (!tail) {
    snprintf(message, sizeof(message), "Site de origem '%s' nao encontrado.",
             origin);
    Ui_showMessage(message, true);
    return false;
  }

  if (!Ui_readLine("Site de destino: ", destinationName,
                   sizeof(destinationName)))
    return false;

  char *destination = Ui_trim(destinationName);
  Vertex *head = Ui_findSite(index, destination);
  if (!head) {
    snprintf(message, sizeof(message), "Site de destino '%s' nao encontrado.",
             destination);
    Ui_showMessage(message, true);
    return false;
  }

  if (strcmp(origin, destination) == 0) {
    Ui_showMessage("Origem e destino devem ser diferentes.", true);
    return false;
  }

  if (Site_hasLink(index->graph, tail->value, head->value)) {
    snprintf(message, sizeof(message), "Link '%s' -> '%s' ja existe.", origin,
             destination);
    Ui_showMessage(message, true);
    return false;
  }

  if (!Ui_readLine("Peso do link (ENTER para 1): ", weightText,
                   sizeof(weightText)))
    return false;

  char *weightValue = Ui_trim(weightText);
  int weight = 1;
  if (*weightValue) {
    char *end = NULL;
    errno = 0;
    long parsed = strtol(weightValue, &end, 10);
    if (errno || *end != '\0' || parsed < 1 || parsed > INT_MAX) {
      Ui_showMessage("Peso invalido. Use um inteiro positivo.", true);
      return false;
    }
    weight = (int)parsed;
  }

  Site_addLink(index->graph, tail->value, head->value, weight);
  if (!Site_hasLink(index->graph, tail->value, head->value)) {
    Ui_showMessage("Nao foi possivel cadastrar o link.", true);
    return false;
  }

  snprintf(message, sizeof(message), "Link '%s' -> '%s' cadastrado (peso %d).",
           origin, destination, weight);
  Ui_showMessage(message, false);
  return true;
}

static bool Ui_loadFiles(Index *index, const char *sitesPath,
                         const char *linksPath) {
  char newSitesPath[FILE_MAX_LINE];
  char newLinksPath[FILE_MAX_LINE];
  char message[FILE_MAX_LINE];

  if (!index || !index->graph || !sitesPath || !linksPath)
    return false;

  if (!Ui_readLine("Arquivo de sites: ", newSitesPath, sizeof(newSitesPath)))
    return false;

  char *sites = Ui_trim(newSitesPath);
  if (!*sites) {
    Ui_showMessage("Caminho do arquivo de sites invalido.", true);
    return false;
  }

  if (!Ui_readLine("Arquivo de links: ", newLinksPath, sizeof(newLinksPath)))
    return false;

  char *links = Ui_trim(newLinksPath);
  if (!*links || strcmp(sites, links) == 0) {
    Ui_showMessage("Informe arquivos de sites e links diferentes.", true);
    return false;
  }

  if (!File_loadIntoIndex(index, sites, links)) {
    Ui_showMessage("Nao foi possivel carregar os arquivos informados.", true);
    return false;
  }

  if (!File_writeGraph(index->graph, sitesPath, linksPath)) {
    Ui_showMessage("Nao foi possivel sobrescrever os arquivos atuais.", true);
    return false;
  }

  snprintf(message, sizeof(message),
           "Dados carregados de '%s' e '%s'. Arquivos atuais sobrescritos.",
           sites, links);
  Ui_showMessage(message, false);
  return true;
}

static int Ui_menu(bool pending) {
  const char *labels[] = {"Cadastrar site", "Cadastrar palavra-chave",
                          "Cadastrar link", "Carregar arquivos",
                          "Pesquisar sites", "Listar sites", "Remover site",
                          "Remover link", "Salvar alteracoes", "Sair"};
  const int count = (int)(sizeof(labels) / sizeof(*labels));
  const char *title = "GOOGLE EM C";
  const char *help  = "Setas: navegar | ENTER: abrir | Q/ESC: sair";

  char subtitle[80];
  snprintf(subtitle, sizeof(subtitle), "Miniatura da internet | %s",
           pending ? "alteracoes nao salvas" : "dados salvos");

  int width = (int)strlen(help);
  for (int i = 0; i < count; i++) {
    int optionWidth = (int)strlen(labels[i]) + 2;
    if (optionWidth > width)
      width = optionWidth;
  }
  if ((int)strlen(subtitle) > width)
    width = (int)strlen(subtitle);

  int selected = 0;
  for (;;) {
    int top = Ui_drawPanel(count + 4, width);

    Ui_printCentered(top, title, Color_CIAN);
    Ui_printCentered(top + 1, subtitle, Color_GRAY);

    for (int i = 0; i < count; i++) {
      Ui_moveToCenter(top + 3 + i, (int)strlen(labels[i]) + 2);
      if (i == selected) {
        Clic_setBackgroundColor(Color_YELLOW);
        Clic_setFontColor(Color_BLACK);
        Clic_printSymbol(Symbol_ARROW);
      } else {
        putchar(' ');
      }
      printf(" %s", labels[i]);
      Clic_resetColor();
    }

    Ui_printCentered(top + 3 + count, help, Color_GRAY);
    fflush(stdout);

    int key = Ui_captureKey();
    if (key == UiKeyUp)
      selected = (selected + count - 1) % count;
    else if (key == UiKeyDown)
      selected = (selected + 1) % count;
    else if (key == Key_ENTER || key == '\r')
      return selected;
    else if (key == Key_Q || key == UiKeyEscape || key == EOF)
      return count - 1;
  }
}

void Ui_run(Index *index, const char *sitesPath, const char *linksPath) {
  if (!index)
    return;

  setvbuf(stdin, NULL, _IONBF, 0);

  bool running = true;
  bool pending = false;

  while (running) {
    int option = Ui_menu(pending);
    switch (option) {
    case 0:
      if (Ui_registerSite(index))
        pending = true;
      break;
    case 1:
      if (Ui_registerWord(index))
        pending = true;
      break;
    case 2:
      if (Ui_registerLink(index))
        pending = true;
      break;
    case 3:
      if (Ui_loadFiles(index, sitesPath, linksPath))
        pending = false;
      break;
    case 4:
      Ui_search(index);
      break;
    case 5:
      Ui_listSites(index);
      break;
    case 6:
      if (Ui_removeSite(index))
        pending = true;
      break;
    case 7:
      if (Ui_removeLink(index))
        pending = true;
      break;
    case 8:
      pending = !Ui_save(index, sitesPath, linksPath);
      break;
    default:
      running = false;
      break;
    }
  }

  int top = Ui_drawPanel(3, (int)strlen("ATE BREVE!"));
  Ui_printCentered(top, "ATE BREVE!", Color_CIAN);
  Ui_printCentered(top + 2, "Alteracoes salvas automaticamente ao sair.",
                   Color_GRAY);
  fflush(stdout);
}
