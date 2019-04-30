#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <ctype.h>

#define BUF_LEN 256

char buf[BUF_LEN];
int pos = 0;

// Nettoye le buffer
void clear_buff() {
  memset(buf,0, BUF_LEN);
  pos = 0;
}

// Récupère l'entrée du buffer
int get_input()  {
  char c;
  if((c = getch()) != ERR) {
    if(pos < BUF_LEN-1 && isprint(c)) {
      buf[pos] = c;
      pos++;
    } else if(c == '\n') {
      buf[pos] == '\0';
      return 1;
    } else if(c == 7) {
      pos = (pos-1 >= 0) ? pos-1 : 0;
      buf[pos] = '\0';
    } 
  }
  return 0;
}

// Affiche les messages
void print_msg(WINDOW *win, char *content) {
 int size = strlen(content);
 if(size > 1 && content[size] == '\0')
    wprintw(win, "%s\n", content);
}

// Affiche le buffer courant
void print_buff(WINDOW *win) {
  werase(win);
  box(win, ACS_VLINE, ACS_HLINE);
  mvwprintw(win,1,1, buf);
}

int handle_cmd () {
  if(pos > 1 && buf[0] == '/') {
    if(buf[1] == 'q')
      return 1;
  }
  return 0;
}

int main() {
 
  // Initialise l'écran
  initscr();
  noecho();
  cbreak();

  WINDOW * top, *bottom, *panel;
  top  = subwin(stdscr,LINES * 3 / 4, COLS, 0,0);
  panel = subwin(top, LINES*3/4-2, COLS-2,1,1);
  bottom = subwin(stdscr, LINES/4, COLS, LINES*3/4, 0);
  box(top, ACS_VLINE, ACS_HLINE);
  box(bottom, ACS_VLINE, ACS_HLINE);
  
  nodelay(stdscr, TRUE);
  scrollok(panel, TRUE);
  curs_set(0);
  keypad(stdscr, TRUE);


  while(1) {
    wrefresh(top);
    wrefresh(bottom);
    wrefresh(panel);

    //print_msg(panel, buf);
    print_buff(bottom);
    if(get_input()) {
      if(handle_cmd())
        break;
      print_msg(panel, buf);
      clear_buff();
    }
      
  }
 
  endwin();
  
  return 0;
}
