#include <assert.h>
#include <ncurses/curses.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct View{
    int nlines;
    int ncols;
    int begin_y;
    int begin_x;
} View;

typedef struct Model{
    int nrows;
    int ncols;
    int *grid;
} Model;

View const init_view(int nlines, int ncols, int begin_y, int begin_x);

Model init_model(View view);

void draw_view(View const view, Model model);

void user_fill_grid(View const grid, Model model);

int main()
{
        initscr();
        cbreak();
        keypad(stdscr, TRUE);
        noecho();


        View const view = init_view(7, 7, 0, 0);
        Model model = init_model(view);
        draw_view(view, model);

        printw("%d", LINES);
        printw("%d", COLS);

        user_fill_grid(view, model);


        refresh();
        free(model.grid);
        endwin();

	return 0;
}


View const init_view(int nlines, int ncols, int begin_y, int begin_x)
{
    if (nlines % 2 == 0) {
        nlines--;
    }

    if (ncols % 2 == 0) {
        ncols--;
    }

    return (View) {.nlines = nlines, .ncols = ncols, 
                    .begin_y = begin_y, .begin_x = begin_x};
}

// transforms model coords to view coords
int model_to_view_coord(int n) {
    return n * 2 + 1;
}

void cell_move(int celly , int cellx) {
    move(model_to_view_coord(celly), model_to_view_coord(cellx));
}

// transforms view coords to model coords
int view_to_model_coord(int n) {
    return (n - 1) / 2;
}

Model init_model(View view) {
    int nrows = view_to_model_coord(view.nlines);
    int ncols = view_to_model_coord(view.ncols);
    int *grid = calloc(nrows * ncols, sizeof grid);
    return (Model) {.nrows = nrows, .ncols = ncols, .grid = grid};
}

void draw_view(View const view, Model model)
{
    int nlines = view.nlines;
    int ncols = view.ncols;

    for (int i = 0; i < ncols; i++) {
        for (int j = 0; j < nlines; j++) {
            move(j, i);
            // corners where bounds meet
            if (i == 0 && j == 0) {
                addch(ACS_ULCORNER);
            }
            else if (i == 0 && j == nlines - 1) {
                addch(ACS_LLCORNER);
            }
            else if (i == ncols - 1 && j == 0) {
                addch(ACS_URCORNER);
            }
            else if (i == ncols - 1 && j == nlines - 1) {
                addch(ACS_LRCORNER);
            }
            // tees for even edge poses
            else if (i == 0 && j % 2 == 0) {
                addch(ACS_LTEE);
            }
            else if (i % 2 == 0 && j == 0) {
                addch(ACS_TTEE);
            }
            else if (i == ncols - 1 && j % 2 == 0) {
                addch(ACS_RTEE);
            }
            else if (i % 2 == 0 && j == nlines - 1) {
                addch(ACS_BTEE);
            }
            // lines
            else if (i % 2 != 0 && j % 2 == 0) {
                addch(ACS_HLINE);
            }
            else if (i % 2 == 0 && j % 2 != 0) {
                addch(ACS_VLINE);
            }
            // plus for both even poses
            else if (i % 2 == 0 && j % 2 == 0) {
                addch(ACS_PLUS);
            }
            // if space is filled in model
            else if (model.grid[view_to_model_coord(j) 
                    * model.ncols + view_to_model_coord(i)] == 1) {
                addch(ACS_BLOCK);
            }
            // o/w blank
        }
    }
    // return to origin
    cell_move(0, 0);
}



void toggle_fill(int celly , int cellx, Model model) {
    if (model.grid[celly * model.ncols + cellx] == 0) 
    {
        model.grid[celly * model.ncols + cellx] = 1;
        addch(ACS_BLOCK);
    }
    else
    {
        model.grid[celly * model.ncols + cellx] = 0;
        addch(' ');
    }
}

void user_fill_grid(View const grid, Model model) {

    int cellx = 0;
    int celly = 0;

    int ch;
    while ((ch = getch()) != KEY_F(1))
    {
        switch (ch) 
        {
            case KEY_RIGHT:
                if (cellx + 1 < model.ncols)
                {
                    cellx += 1;
                }
                break;
            case KEY_UP:
                if (celly - 1 >= 0)
                {
                    celly -= 1;
                }
                break;
            case KEY_DOWN:
                if (celly + 1 < model.nrows)
                {
                    celly += 1;
                }
                break;
            case KEY_LEFT:
                if (cellx - 1 >= 0)
                {
                    cellx -= 1;
                }
                break;
            case '\n':
                toggle_fill(celly, cellx, model);
        }
        cell_move(celly, cellx);
        refresh();
    }
}
