#include <ncurses.h>
#include <string.h>
#include <unistd.h> 
#include <math.h>
#include <time.h>
#include <stdlib.h>

// Typedef for circle struct
typedef struct {
    int x,y;
}CIRCLE;

// Circle variable definition
CIRCLE circle;
// Window for buttons
WINDOW *print_btn, *exit_btn, *normal_btn, *client_btn, *server_btn;
// Mouse event variable
MEVENT event;

int BTN_SIZE_Y = 3;
int BTN_SIZE_X = 7;

// Method to instantiate button window
void make_print_button() {
    normal_btn = newwin(BTN_SIZE_Y, BTN_SIZE_X, LINES / 6, (COLS - BTN_SIZE_X));
    client_btn = newwin(BTN_SIZE_Y, BTN_SIZE_X, LINES / 3, (COLS - BTN_SIZE_X));
    server_btn = newwin(BTN_SIZE_Y, BTN_SIZE_X, LINES / 2, (COLS - BTN_SIZE_X));
    print_btn = newwin(BTN_SIZE_Y, BTN_SIZE_X, 2 * LINES / 3, (COLS - BTN_SIZE_X));
    exit_btn = newwin(BTN_SIZE_Y, BTN_SIZE_X, 5 * LINES / 6, (COLS - BTN_SIZE_X));
}

// Draw button with colored background
void draw_btn(WINDOW *btn, char label, int color) {

    wbkgd(btn, COLOR_PAIR(color));
    wmove(btn, floor(BTN_SIZE_Y / 2), floor(BTN_SIZE_X / 2));

    attron(A_BOLD);
    waddch(btn, label);
    attroff(A_BOLD);

    wrefresh(btn);
}

// Utility method to check if button has been pressed
int check_button_pressed(WINDOW *btn, MEVENT *event) {

    if(event->y >= btn->_begy && event->y < btn->_begy + BTN_SIZE_Y) {
        if(event->x >= btn->_begx && event->x < btn->_begx + BTN_SIZE_X) {
            return TRUE;
        }
    }
    return FALSE;

}

// Method to draw lateral elements of the UI (button)
void draw_side_ui() {
    mvvline(0, COLS - BTN_SIZE_X - 1, ACS_VLINE, LINES);
    draw_btn(normal_btn, 'N', 4);
    draw_btn(client_btn, 'C', 5);
    draw_btn(server_btn, 'S', 6);
    draw_btn(print_btn, 'P', 2);
    draw_btn(exit_btn, 'X', 3);
    refresh();
}

// Set circle's initial position in the center of the window
void set_circle() {
    circle.y = LINES / 2;
    circle.x = COLS / 2;
}

// Get circle's current position (center):
int get_circle_x() {
    return circle.x;
}

int get_circle_y() {
    return circle.y;
}

// Draw filled circle according to its equation
void draw_circle() {
    attron(COLOR_PAIR(1));
    mvaddch(circle.y, circle.x, '@');
    mvaddch(circle.y - 1, circle.x, '@');
    mvaddch(circle.y + 1, circle.x, '@');
    mvaddch(circle.y, circle.x - 1, '@');
    mvaddch(circle.y, circle.x + 1, '@');
    attroff(COLOR_PAIR(1));
    refresh();
}

// Move circle window according to user's input
void move_circle(int cmd) {
    // First, clear previous circle positions
    mvaddch(circle.y, circle.x, ' ');
    mvaddch(circle.y - 1, circle.x, ' ');
    mvaddch(circle.y + 1, circle.x, ' ');
    mvaddch(circle.y, circle.x - 1, ' ');
    mvaddch(circle.y, circle.x + 1, ' ');

    // Move circle by one character based on cmd
    switch (cmd)
    {
        case KEY_LEFT:
            if(circle.x - 1 > 0) {
                circle.x--;
            }
            break;
        case KEY_RIGHT:
            if(circle.x + 1 < COLS - BTN_SIZE_X - 2) {
                circle.x++;
            }
            break;
        case KEY_UP:
            if(circle.y - 1 > 0) {
                circle.y--;
            }
            break;
        case KEY_DOWN:
            if(circle.y + 2 < LINES) {
                circle.y++;
            }
            break;
        default:
            break;
    }
    refresh();
}

void init_console_ui() {

    // Initialize curses mode
    initscr();		
	start_color();

    // Disable line buffering...
	cbreak();

    // Disable char echoing and blinking cursos
    noecho();
    nodelay(stdscr, TRUE);
    curs_set(0);

    init_pair(1, COLOR_BLACK, COLOR_GREEN);
    init_pair(2, COLOR_WHITE, COLOR_BLUE);
    init_pair(3, COLOR_BLACK, COLOR_CYAN);
    init_pair(4, COLOR_WHITE, COLOR_MAGENTA);
    init_pair(5, COLOR_WHITE, COLOR_GREEN);
    init_pair(6, COLOR_WHITE, COLOR_YELLOW);

    // Initialize UI elements
    set_circle();
    make_print_button();

    // Draw UI elements
    draw_circle();
    draw_side_ui();

    // Activate input listening (keybord + mouse events ...)
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS, NULL);

    refresh();
}


void reset_console_ui() {

    // Free resources
    delwin(print_btn);
    delwin(exit_btn);
    delwin(normal_btn);
    delwin(client_btn);
    delwin(server_btn);

    // Clear screen
    erase();
    refresh();

    // Initialize UI elements
    set_circle();
    make_print_button();
    
    // Draw UI elements
    draw_circle();
    draw_side_ui();
}


