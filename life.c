#include <assert.h>
#include <ncurses/curses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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

typedef enum State {EXIT, PROCEED} State;

void view_init(View* view, int nlines, int ncols, int begin_y, int begin_x)
{
    if (nlines % 2 == 0) {
        nlines--;
    }

    if (ncols % 2 == 0) {
        ncols--;
    }

    *view = (View){.nlines = nlines, .ncols = ncols, 
                    .begin_y = begin_y, .begin_x = begin_x};
}

// transforms model coords to view coords
void model_to_view_coord(int *celly, int *cellx, View const *view)
{
    *celly = (*celly) * 2 + 1 + view->begin_y;
    *cellx = (*cellx) * 2 + 1 + view->begin_x;
}

// transforms view size to model size
int view_to_model_size(int n)
{
    return (n - 1) / 2;
}

void view_to_model_coord(int *y, int *x, View const *view)
{
    *y = (*y - (1 + view->begin_y)) / 2;
    *x = (*x - (1 + view->begin_x)) / 2;
}

void cell_move(int celly , int cellx, View const *view)
{
    model_to_view_coord(&celly, &cellx, view);
    move(celly, cellx);
}

Model *model_new(View *view)
{
    Model *model = malloc(sizeof *model);
    model->nrows = view_to_model_size(view->nlines);
    model->ncols = view_to_model_size(view->ncols);
    model->grid = calloc(model->nrows * model->ncols, sizeof *(model->grid));

    return model;
}

void model_destroy(Model *model)
{
    free(model->grid);
    free(model);
}

void clear_view(View *view)
{
    int nlines = view->nlines;
    int ncols = view->ncols;
    int begin_x = view->begin_x;
    int begin_y = view->begin_y;

    int end_x = ncols + begin_x;
    int end_y = nlines + begin_y;

    for (int i = begin_x; i < end_x; i++) {
        for (int j = begin_y; j < end_y; j++) {
            move(j, i);
            // corners where bounds meet
            if (i == begin_x && j == begin_y) {
                addch(ACS_ULCORNER);
            }
            else if (i == begin_x && j == end_y - 1) {
                addch(ACS_LLCORNER);
            }
            else if (i == end_x - 1 && j == begin_y) {
                addch(ACS_URCORNER);
            }
            else if (i == end_x - 1 && j == end_y - 1) {
                addch(ACS_LRCORNER);
            }
            // tees for even edge poses
            else if (i == begin_x && j % 2 == 0) {
                addch(ACS_LTEE);
            }
            else if (i % 2 == 0 && j == begin_y) {
                addch(ACS_TTEE);
            }
            else if (i == end_x - 1 && j % 2 == 0) {
                addch(ACS_RTEE);
            }
            else if (i % 2 == 0 && j == end_y - 1) {
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
            else
            {
                addch(' ');
            }
        }
    }
}

void clear_model(View const *view, Model *model)
{
    int nrows = model->nrows;
    int ncols = model->ncols;

    for (int i = 0; i < ncols; i++) {
        for (int j = 0; j < nrows; j++) {
            cell_move(j, i, view);
            model->grid[j * model->ncols + i] = 0;
            addch(' ');
        }
    }
}

// cell position corresponding to position of cursor in view
void getyx_cell(int *celly_ptr, int *cellx_ptr, View* const view) {
    int y, x;
    getyx(stdscr, y, x);

    view_to_model_coord(&y, &x, view);
    *celly_ptr = y;
    *cellx_ptr = x;
}

void toggle_fill(View *view, Model *model)
{
    int celly, cellx;

    getyx_cell(&celly, &cellx, view);

    if (model->grid[celly * model->ncols + cellx] == 0) 
    {
        model->grid[celly * model->ncols + cellx] = 1;
        addch(ACS_BLOCK);
    }
    else
    {
        model->grid[celly * model->ncols + cellx] = 0;
        addch(' ');
    }
}

State user_fill_grid(View *view, Model *model)
{
    move(0, 0);
    clrtoeol();
    printw("Arrow keys = move | r = run | f = fill | F1 = exit");
    cell_move(0, 0, view);
    curs_set(1);
    timeout(-1);

    int cellx = 0;
    int celly = 0;

    int ch;
    while ((ch = getch()))
    {
        switch (ch) 
        {
            case KEY_RIGHT:
                if (cellx + 1 < model->ncols)
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
                if (celly + 1 < model->nrows)
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
            case 'f':
                toggle_fill(view, model);
                break;
            case 'r':
                return PROCEED;
            case KEY_F(1):
                return EXIT;
        }
        cell_move(celly, cellx, view);
        refresh();
    }

    return EXIT;
}

void draw_view(View *view, Model const *model)
{
    int nrows = model->nrows;
    int ncols = model->ncols;

    for (int i = 0; i < ncols; i++) {
        for (int j = 0; j < nrows; j++) {
            cell_move(j, i, view);
            if (model->grid[j * model->ncols + i] == 1)
            {
                addch(ACS_BLOCK);
            }
            else 
            {
                addch(' ');
            }
            // o/w blank
        }
    }
}

bool out_of_bounds(int celly, int cellx, Model const *model) {

    return celly < 0 || celly >= model->nrows
            || cellx < 0 || cellx >= model->ncols;
}
int num_neighbors(int celly, int cellx, Model const *model) {

    int count = 0;
    for (int i = -1; i <= 1; i++)
    {
        for (int j = -1; j <= 1; j++) 
        {
            // ignore the node itself
            if (!((i == 0 && j == 0) 
                || out_of_bounds(celly + j, cellx + i, model))
                && model->grid[(celly + j) * model->ncols + (cellx + i)] == 1)
            {
                count++;
            }
        }
    }


    return count;
}

void single_turn(View *view, Model* model)
{
    int *temp = calloc(model->ncols * model->nrows, sizeof *temp);

    for (int i = 0; i < model->ncols; i++)
    {
        for (int j = 0; j < model->nrows; j++)
        {
            int n = num_neighbors(j, i, model);

            // populated
            if ((model->grid[j * model->ncols + i] == 1)
                && (n == 2 || n == 3))
            {
                temp[j * model->ncols + i] = 1;
            }
            // unpopulated
            else if ((model->grid[j * model->ncols + i] == 0)
                && (n == 3))
            {
                temp[j * model->ncols + i] = 1;
            }
            // o/w empty or dead
        }
    }

    // copy temp
    for (int i = 0; i < model->ncols; i++)
    {
        for (int j = 0; j < model->nrows; j++)
        {
            model->grid[j * model->ncols + i] = temp[j * model->ncols + i];
        }
    }
    free(temp);
    draw_view(view, model);
}

State simulate(View *view, Model *model)
{
    move(0, 0);
    clrtoeol();
    printw("e = end | F1 = exit");

    curs_set(0);
    timeout(0);

    int ch;
    while ((ch = getch()))
    {
        single_turn(view, model);

        refresh();
        napms(500);

        if (ch == 'e')
        {
            return PROCEED;
        }
        else if (ch == KEY_F(1))
        {
            return EXIT;
        }
    }

    return EXIT;
}

int main()
{
        initscr();
        cbreak();
        keypad(stdscr, TRUE);
        noecho();


        View *view;
        view_init(view, 25, 25, 2, 2);

        Model *model = model_new(view);
        while (true)
        {
            State state;
            clear_view(view);
            clear_model(view, model);
            state = user_fill_grid(view, model);
            if (state == EXIT) {
                break;
            }
            simulate(view, model);
            if (state == EXIT) {
                break;
            }
        }

        model_destroy(model);
        endwin();

	return EXIT_SUCCESS;
}
