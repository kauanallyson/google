#include "Ui.h"
#include "Clic.h"
#include "Site.h"
#include <poll.h>

enum MenuOption { Search, List, Exit, MenuCount };

enum UiKey { UiEscape = 27, UiUp = 256, UiDown };

static int arrowKey(int code) {
  switch (code) {
  case 'A': return UiUp;
  case 'B': return UiDown;
  default: return 0;
  }
}

static int readKeyByte(void) {
  unsigned char byte;
  return read(STDIN_FILENO, &byte, 1) == 1 ? byte : EOF;
}

static int readEscapeByte(void) {
  struct pollfd input = {.fd = STDIN_FILENO, .events = POLLIN};
  return poll(&input, 1, 50) > 0 ? readKeyByte() : EOF;
}

static int captureKey(void) {
  struct termios original;
  if (tcgetattr(STDIN_FILENO, &original) == -1)
    return EOF;

  struct termios mode = original;
  mode.c_lflag &= ~(ICANON | ECHO);
  mode.c_cc[VMIN] = 1;
  mode.c_cc[VTIME] = 0;
  if (tcsetattr(STDIN_FILENO, TCSANOW, &mode) == -1)
    return EOF;

  int key = readKeyByte();
  if (key == UiEscape) {
    int prefix = readEscapeByte();
    if (prefix == '[' || prefix == 'O')
      key = arrowKey(readEscapeByte());
  }

  tcsetattr(STDIN_FILENO, TCSANOW, &original);
  return key;
}

static void moveToCenter(int row, int width) {
  int column = (Clic_getScreenWidth() - width) / 2 + 1;
  Clic_move(row, column > 0 ? column : 1);
}

static void drawBorder(int row, int column, int width, char *left,
                       char *right) {
  Clic_move(row, column);
  Clic_printSymbol(left);
  Clic_printHLine(width - 2);
  Clic_printSymbol(right);
}

static int drawPanel(int rows, int contentWidth) {
  int height = Clic_getScreenHeight();
  int columns = Clic_getScreenWidth();
  Clic_clearScreen();

  if (height < rows + 2 || columns < contentWidth + 4) {
    int top = (height - rows) / 2 + 1;
    return top > 0 ? top : 1;
  }

  int width = contentWidth + 4;
  int column = (columns - width) / 2 + 1;
  int top = (height - rows - 2) / 2 + 1;
  Clic_setFontColor(Color_CIAN);
  drawBorder(top, column, width, Symbol_CORNER_TL, Symbol_CORNER_TR);
  for (int row = top + 1; row <= top + rows; row++) {
    Clic_move(row, column);
    Clic_printSymbol(Symbol_VLINE);
    Clic_move(row, column + width - 1);
    Clic_printSymbol(Symbol_VLINE);
  }
  drawBorder(top + rows + 1, column, width, Symbol_CORNER_BL, Symbol_CORNER_BR);
  Clic_resetColor();
  return top + 1;
}

static bool confirmExit(void) {
  const char *warning = "Mudancas nao salvas podem ser perdidas.";
  const char *question = "Deseja sair?";
  const char *options[] = {"Cancelar", "Sair"};
  int selected = 0;

  for (;;) {
    int top = drawPanel(5, strlen(warning));
    moveToCenter(top, strlen(warning));
    Clic_setFontColor(Color_RED);
    fputs(warning, stdout);
    Clic_resetColor();
    moveToCenter(top + 1, strlen(question));
    fputs(question, stdout);

    for (int i = 0; i < 2; i++) {
      moveToCenter(top + 3 + i, strlen(options[i]) + 2);
      if (i == selected) {
        Clic_setBackgroundColor(Color_YELLOW);
        Clic_setFontColor(Color_BLACK);
        Clic_printSymbol(Symbol_ARROW);
      } else {
        putchar(' ');
      }
      printf(" %s", options[i]);
      Clic_resetColor();
    }
    fflush(stdout);

    switch (captureKey()) {
    case UiUp:
    case UiDown:
      selected = 1 - selected;
      break;
    case Key_ENTER:
    case '\r':
      return selected == 1;
    case UiEscape:
      return false;
    case EOF:
      return true;
    }
  }
}

static int selectOption(const char *sitesPath, const char *linksPath) {
  const char *labels[] = {"Buscar dominio", "Listar sites", "Sair"};
  const char *title = "Google em C";
  const char *instructions = "Setas: navegar  Enter: abrir  Esc: sair";
  const char *sitesLabel = "Arquivo de sites lido: ";
  const char *linksLabel = "Arquivo de links lido: ";
  const int menuHeight = MenuCount + 6;
  const int optionWidth = 16;
  int width = strlen(instructions);
  int sitesWidth = strlen(sitesLabel) + strlen(sitesPath);
  int linksWidth = strlen(linksLabel) + strlen(linksPath);
  if (sitesWidth > width)
    width = sitesWidth;
  if (linksWidth > width)
    width = linksWidth;
  int selected = Search;

  for (;;) {
    int top = drawPanel(menuHeight, width);
    moveToCenter(top, strlen(title));
    Clic_setFontColor(Color_CIAN);
    fputs(title, stdout);
    Clic_resetColor();
    moveToCenter(top + 1, strlen(instructions));
    fputs(instructions, stdout);

    for (int i = 0; i < MenuCount; i++) {
      moveToCenter(top + 3 + i, optionWidth + 2);
      if (i == selected) {
        Clic_setBackgroundColor(Color_YELLOW);
        Clic_setFontColor(Color_BLACK);
        Clic_printSymbol(Symbol_ARROW);
      } else {
        putchar(' ');
      }
      printf(" %-*s", optionWidth, labels[i]);
      Clic_resetColor();
    }
    moveToCenter(top + MenuCount + 4, strlen(sitesLabel) + strlen(sitesPath));
    printf("%s%s", sitesLabel, sitesPath);
    moveToCenter(top + MenuCount + 5, strlen(linksLabel) + strlen(linksPath));
    printf("%s%s", linksLabel, linksPath);
    fflush(stdout);

    switch (captureKey()) {
    case UiUp:
      selected = (selected + MenuCount - 1) % MenuCount;
      break;
    case UiDown:
      selected = (selected + 1) % MenuCount;
      break;
    case Key_ENTER:
    case '\r':
      if (selected == Exit && !confirmExit())
        break;
      Clic_clearScreen();
      return selected;
    case UiEscape:
      if (!confirmExit())
        break;
      Clic_clearScreen();
      return Exit;
    case EOF:
      Clic_clearScreen();
      return Exit;
    }
  }
}

static bool showNotice(const char *label, const char *detail) {
  const char *help = "Esc ou Enter para voltar";
  int width = strlen(label) + strlen(detail);
  int top = drawPanel(3, width > (int)strlen(help) ? width : (int)strlen(help));
  moveToCenter(top, width);
  Clic_setFontColor(Color_YELLOW);
  printf("%s%s", label, detail);
  Clic_resetColor();
  moveToCenter(top + 2, strlen(help));
  fputs(help, stdout);
  fflush(stdout);

  int key;
  do {
    key = captureKey();
  } while (key != Key_ENTER && key != '\r' && key != UiEscape && key != EOF);
  return key != EOF;
}

static bool showKeywords(const Site *site) {
  const char *title = "Palavras-chave de ";
  const char *help = "Setas: rolar  Esc ou Enter: voltar";
  int first = 0;
  int titleWidth = strlen(title) + strlen(site->name);
  int width = strlen(help);
  if (titleWidth > width)
    width = titleWidth;

  for (;;) {
    int visible = Clic_getScreenHeight() - 6;
    if (visible < 1)
      visible = 1;
    int rows = site->nWords < visible ? site->nWords : visible;
    if (first > site->nWords - rows)
      first = site->nWords - rows;
    int top = drawPanel(rows + 4, width);
    moveToCenter(top, titleWidth);
    Clic_setFontColor(Color_CIAN);
    printf("%s%s", title, site->name);
    Clic_resetColor();
    for (int i = 0; i < rows; i++) {
      const char *word = site->words[first + i];
      moveToCenter(top + 2 + i, strlen(word));
      fputs(word, stdout);
    }
    moveToCenter(top + 3 + rows, strlen(help));
    fputs(help, stdout);
    fflush(stdout);

    switch (captureKey()) {
    case UiUp:
      if (first > 0)
        first--;
      break;
    case UiDown:
      if (first + rows < site->nWords)
        first++;
      break;
    case UiEscape:
    case Key_ENTER:
    case '\r':
      return true;
    case EOF:
      return false;
    }
  }
}

static bool searchDomain(Graph *graph) {
  const char *title = "Google em C";
  const char *prompt = "Digite um dominio (ex.: hardware.com)";
  const char *help = "Esc para voltar";
  const int searchHeight = 5;
  char line[SITE_MAX_NAME];
  size_t entered = 0;
  const char *whitespace = " \t\r\n\v\f";

  int columns = Clic_getScreenWidth();
  int inputWidth = SITE_MAX_NAME + 1;
  if (inputWidth > columns - 4)
    inputWidth = columns > 6 ? columns - 4 : 2;
  int width = strlen(prompt);
  if (inputWidth > width)
    width = inputWidth;
  int top = drawPanel(searchHeight, width);

  moveToCenter(top, strlen(title));
  Clic_setFontColor(Color_CIAN);
  fputs(title, stdout);
  Clic_resetColor();
  moveToCenter(top + 2, strlen(prompt));
  fputs(prompt, stdout);
  moveToCenter(top + 4, strlen(help));
  fputs(help, stdout);
  for (;;) {
    moveToCenter(top + 3, inputWidth);
    Clic_setFontColor(Color_CIAN);
    fputs("> ", stdout);
    Clic_resetColor();
    int shown = entered;
    if (shown > inputWidth - 2)
      shown = inputWidth - 2;
    fwrite(line + entered - shown, 1, shown, stdout);
    if (shown < inputWidth - 2) {
      putchar(' ');
      Clic_moveLeft(1);
    }
    fflush(stdout);
    int key = captureKey();
    if (key == EOF)
      return false;
    if (key == UiEscape)
      return true;
    if (key == Key_ENTER || key == '\r')
      break;
    if (key == Key_DELETE || key == '\b') {
      if (entered > 0)
        entered--;
    } else if (key >= ' ' && key <= '~' && entered < sizeof(line) - 1) {
      line[entered++] = key;
    }
  }
  line[entered] = '\0';

  char *domain = line + strspn(line, whitespace);
  size_t length = strlen(domain);
  while (length > 0 && strchr(whitespace, domain[length - 1]))
    domain[--length] = '\0';

  for (size_t i = 0; i < length; i++)
    domain[i] = tolower((unsigned char)domain[i]);

  if (length == 0)
    return true;

  Vertex *vertex = Graph_findVertexByValue(graph, domain, Site_cmpName);
  if (vertex)
    return showKeywords(vertex->value);

  return showNotice("Dominio nao encontrado: ", domain);
}

static bool listSites(Graph *graph) {
  const char *title = "Sites cadastrados";
  const char *help = "Setas: rolar  Esc ou Enter: voltar";
  Vertex *first = graph->first;

  int width = strlen(help);
  for (Vertex *site = first; site; site = site->next) {
    const Site *entry = site->value;
    if (strlen(entry->name) > (size_t)width)
      width = strlen(entry->name);
  }

  for (;;) {
    int visible = Clic_getScreenHeight() - 6;
    if (visible < 1)
      visible = 1;
    int rows = graph->n < visible ? graph->n : visible;
    int top = drawPanel(rows + 4, width);
    moveToCenter(top, strlen(title));
    Clic_setFontColor(Color_CIAN);
    fputs(title, stdout);
    Clic_resetColor();

    Vertex *site = first;
    for (int i = 0; site && i < visible; i++, site = site->next) {
      const Site *entry = site->value;
      moveToCenter(top + 2 + i, strlen(entry->name));
      Site_print(site->value);
    }
    moveToCenter(top + 3 + rows, strlen(help));
    fputs(help, stdout);
    fflush(stdout);

    switch (captureKey()) {
    case UiUp:
      if (first && first != graph->first) {
        Vertex *previous = graph->first;
        while (previous->next != first)
          previous = previous->next;
        first = previous;
      }
      break;
    case UiDown:
      if (site)
        first = first->next;
      break;
    case UiEscape:
    case Key_ENTER:
    case '\r':
      return true;
    case EOF:
      return false;
    }
  }
}

bool Ui_run(Graph *graph, const char *sitesPath, const char *linksPath) {
  if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
    fputs("Execute o programa em um terminal para usar o menu.\n", stderr);
    return false;
  }

  int option;
  while ((option = selectOption(sitesPath, linksPath)) != Exit) {
    if (!(option == List ? listSites(graph) : searchDomain(graph)))
      break;
  }
  return !ferror(stdin);
}
