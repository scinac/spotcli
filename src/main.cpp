#include "../include/authSpotify.hpp"
#include "../include/playback.hpp"
#include "../include/playlist.hpp"
#include "../include/spotcli.hpp"
#include "../include/token.hpp"
#include "../include/utils.hpp"
#include <iostream>
#include <ncurses.h>
#include <string>
#include <vector>

void showPlaylistsFunction();

static Tokens tokens;

int main() {
  initscr();
  noecho();
  cbreak();
  keypad(stdscr, true);

  std::cout << "spotcli main" << '\n';
  for (;;) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    uint32_t choice{askForChoice()};

    processChoice(choice);
  }

  endwin();
  killAllInstances();
  return 0;
}

uint32_t askForChoice() {
  std::vector<std::string> options = {
      "auth", "play playlist", "play liked songs", "play artist", "play song"};

  int highlight = 0;
  int choice = -1;
  int ch;

  while (true) {
    clear();
    mvprintw(
        0, 0,
        "(!You have to auth at least once) navigate and Enter to select:\n");
    for (size_t i = 0; i < options.size(); i++) {
      if ((int)i == highlight)
        attron(A_REVERSE);
      mvprintw(i + 1, 2, "%s", options[i].c_str());
      if ((int)i == highlight)
        attroff(A_REVERSE);
    }

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    move(rows - 1, 0);
    clrtoeol();
    attron(A_REVERSE);
    mvprintw(rows - 1, 0, "(Space) Pause | (N) Next | (M) Previous");
    attroff(A_REVERSE);

    refresh();
    ch = getch();
    switch (ch) {
    case KEY_UP:
      highlight--;
      if (highlight < 0)
        highlight = options.size() - 1;
      break;

    case KEY_DOWN:
      highlight++;
      if (highlight >= (int)options.size())
        highlight = 0;
      break;

    case ' ':
      togglePause();
      break;

    case 10:
      choice = highlight;
      goto endLoop;
    }
  }

endLoop:
  endwin();
  return choice;
}
void processChoice(uint32_t choice) {
  switch (choice) {
  case 0:
    if (authSpotify()) {
      choice = askForChoice();
      processChoice(choice);
    } else {
      std::cerr << "auth to spotify didnt work" << '\n';
    }
    break;
  case 1:
    showPlaylistsFunction();
    break;
  }
}
void showPlaylistsFunction() {
  showPlaylists(getClientId(), getClientSecret(), getRedirect_uri());
}
