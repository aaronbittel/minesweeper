#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <stddef.h>

#include "raylib.h"

#define GAME_MENU_HEIGHT        60

#define GRID_SIZE          40
#define ROWS               9
#define COLUMNS            9
#define MINE_COUNT         10
#define GAME_WIDTH         ((COLUMNS) * (GRID_SIZE))
#define GAME_HEIGHT        ((ROWS) * (GRID_SIZE))

#define PADDING            10

#define GAME_START_Y       ((GAME_MENU_HEIGHT) + (PADDING))
#define GAME_START_X       (PADDING)

#define GAME_STATUS_HEIGHT 60
#define GAME_STATUS_START_Y ((GAME_START_Y) + (GAME_HEIGHT))

#define WIDTH              ((GAME_WIDTH) + 2 * (PADDING))
#define HEIGHT             ((GAME_HEIGHT) + 2 * (PADDING) + (GAME_MENU_HEIGHT) + (GAME_STATUS_HEIGHT))

#define FONT_SIZE          32
#define MENU_FONT_SIZE     24

#define MINE               -1

#define MENU_WIDTH 400
#define MENU_HEIGHT 300

typedef struct {
    int rows;
    int cols;
} ms_Game;

typedef struct {
    int  value;
    bool flagged;
    bool revealed;
} ms_Cell;

typedef struct {
    int x, y;
} ms_Pos;

typedef enum {
    ms_MENU,
    ms_PLAYING,
    ms_GAME_OVER,
    ms_GAME_WON,
    ms_NEW_GAME,
    ms_FIRST_CLICK_MINE,
} ms_GameState;

typedef struct {
    char* name;
    int text_size;
    int y;
    Color normal;
    Color highlight;
} ms_MenuItem;

ms_GameState game_state = ms_MENU;
ms_Cell game_data[ROWS * COLUMNS] = {0};
bool show_debug = false;
int mines_left = MINE_COUNT;
long start_time;
long end_time;
bool update_time = true;
ms_Pos* first_cell = NULL;

void ms_InitGameData();
void ms_DrawGrid();
void ms_DrawGameState();
void ms_DrawMenu();
void ms_DrawItem(ms_MenuItem* item, bool selected, bool active);
void ms_InitMenuItems(ms_MenuItem items[3], int beginn_Y);
void ms_ExpandZeros(ms_Pos pos);
bool ms_MouseGridPos(ms_Pos* pos);
bool ms_CheckGameWon();

int main() {
    srand(time(NULL));
    ms_InitGameData();

    InitWindow(MENU_WIDTH, MENU_HEIGHT, "Minesweeper");
    SetTargetFPS(60);

    char* menu_title = "Menu";
    int menu_title_size = MeasureText(menu_title, FONT_SIZE);

    char* sub_title = "Choose the difficulty";
    int sub_title_size = MeasureText(sub_title, MENU_FONT_SIZE);

    int current_Y = 30;
    int menu_title_Y = current_Y;
    current_Y += FONT_SIZE + 20;
    int submenu_title_Y = current_Y;
    current_Y += MENU_FONT_SIZE + 40;

    ms_MenuItem items[3] = {0};
    ms_InitMenuItems(items, current_Y);
    bool locked_in = false;

    int selected_item = 0;

    float highlight_timer = 0.0f;
    const float highligh_duration = 0.12f;

    start_time = time(NULL);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_R)) {
            game_state = ms_NEW_GAME;
        }

        BeginDrawing();

        ClearBackground(RAYWHITE);

        if (game_state != ms_MENU) {
            ms_DrawMenu();
            ms_DrawGrid();
            ms_DrawGameState();
        }

        switch (game_state) {
            case ms_MENU:
                {
                    ClearBackground(RAYWHITE);
                    DrawText(menu_title, (MENU_WIDTH-menu_title_size)/2, menu_title_Y, FONT_SIZE, RED);
                    DrawText(sub_title, (MENU_WIDTH-sub_title_size)/2, submenu_title_Y, MENU_FONT_SIZE, DARKGRAY);


                    if (IsKeyPressed(KEY_ENTER)) {
                        highlight_timer = highligh_duration;
                        locked_in = true;
                    }

                    if (locked_in) highlight_timer -= GetFrameTime();
                    if (highlight_timer < 0) {
                        highlight_timer = 0;
                        locked_in = false;
                        SetWindowSize(WIDTH, HEIGHT);
                        game_state = ms_PLAYING;
                    }

                    int mouseY = GetMouseY();
                    for (size_t i = 0; i < 3; i++) {
                        if (mouseY >= items[i].y-MENU_FONT_SIZE/2 && mouseY <= items[i].y + MENU_FONT_SIZE*2) {
                            selected_item = i;
                        }
                    }

                    for (size_t i = 0; i < 3; i++) {
                        ms_DrawItem(&items[i], selected_item == i, highlight_timer > 0);
                    }
                } break;
            case ms_PLAYING:
                {
                    ms_Pos grid_pos = {0};
                    if (ms_MouseGridPos(&grid_pos)) {
                        ms_Cell* cell = &game_data[grid_pos.y * COLUMNS + grid_pos.x];
                        if (!cell->revealed && !cell->flagged) {
                            DrawRectangle(GAME_START_X+grid_pos.x*GRID_SIZE, GAME_START_Y+grid_pos.y*GRID_SIZE, GRID_SIZE, GRID_SIZE, LIGHTGRAY);
                        }

                        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !cell->revealed && !cell->flagged) {
                            if (first_cell == NULL && cell->value == MINE) {
                                first_cell = &grid_pos;
                                game_state = ms_FIRST_CLICK_MINE;
                                break;
                            }
                            first_cell = &grid_pos;
                            cell->revealed = true;
                            // TODO: handle game over
                            if (cell->value == MINE) {
                                game_state = ms_GAME_OVER;
                                for (int y = 0; y < ROWS; y++) {
                                    for (int x = 0; x < COLUMNS; x++) {
                                        ms_Cell* cell = &game_data[y * COLUMNS + x];
                                        if (cell->value == MINE) {
                                            cell->revealed = true;
                                        }
                                    }
                                }
                            } else if (cell->value == 0) {
                                ms_ExpandZeros(grid_pos);
                            }
                        }

                        if (IsKeyPressed(KEY_M) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                            if (!cell->revealed) {
                                cell->flagged = !cell->flagged;
                                mines_left += cell->flagged ? -1 : 1;
                            }
                        }
                    }

                    if (IsKeyPressed(KEY_G)) {
                        show_debug = !show_debug;
                    }
                }
                break;
            case ms_GAME_OVER:
                {
                    if (update_time) {
                        end_time = time(NULL);
                        update_time = false;
                    }
                    char* msg = "Game Over!";
                    int text_size = MeasureText(msg, FONT_SIZE);
                    DrawText(msg, (WIDTH - text_size) / 2, GAME_STATUS_START_Y + (HEIGHT - GAME_STATUS_START_Y - FONT_SIZE) / 2, FONT_SIZE, RED);
                }
                break;
            case ms_GAME_WON:
                {
                    if (update_time) {
                        end_time = time(NULL);
                        update_time = false;
                    }
                    char* msg = "Congrats, Game Won!";
                    int text_size = MeasureText(msg, FONT_SIZE);
                    DrawText(msg, (WIDTH - text_size) / 2 , GAME_STATUS_START_Y + (HEIGHT - GAME_STATUS_START_Y - FONT_SIZE) / 2, FONT_SIZE, GREEN);
                }
                break;
            case ms_NEW_GAME:
                {
                    memset(game_data, 0, sizeof(game_data));
                    ms_InitGameData();
                    start_time = time(NULL);
                    game_state = ms_PLAYING;
                    end_time = 0;
                    update_time = true;
                    mines_left = MINE_COUNT;
                    first_cell = NULL;
                }
                break;
            case ms_FIRST_CLICK_MINE:
                {
                    do {
                        memset(game_data, 0, sizeof(game_data));
                        ms_InitGameData();
                        start_time = time(NULL);
                        game_state = ms_PLAYING;
                        end_time = 0;
                        update_time = true;
                        mines_left = MINE_COUNT;
                    } while (game_data[first_cell->y * COLUMNS + first_cell->x].value == MINE);
                    ms_Cell* cell = &game_data[first_cell->y * COLUMNS + first_cell->x];
                    cell->revealed = true;
                    if (cell->value == 0) {
                        ms_ExpandZeros(*first_cell);
                    }
                    first_cell = NULL;
                }
                break;
        }

        EndDrawing();

        if (ms_CheckGameWon()) {
            game_state = ms_GAME_WON;
        }
    }
    CloseWindow();
    return 0;
}

/*Returns true if mouse is inside the grid and sets the ms_Pos struct accordingly,
 * otherwise returns false.*/
bool ms_MouseGridPos(ms_Pos* grid_pos) {
    Vector2 mouse_pos = GetMousePosition();
    mouse_pos.y -= GAME_START_Y;
    mouse_pos.x -= GAME_START_X;

    if (mouse_pos.y < 0 || mouse_pos.y >= ROWS * GRID_SIZE || mouse_pos.x < 0 || mouse_pos.x >= COLUMNS * GRID_SIZE) {
        return false;
    }

    grid_pos->y = mouse_pos.y / GRID_SIZE;
    grid_pos->x = mouse_pos.x / GRID_SIZE;
    return true;
}

void ms_DrawGameState() {
    for (int y = 0; y < ROWS; y++) {
        for (int x = 0; x < COLUMNS; x++) {
            ms_Cell cell = game_data[y * COLUMNS + x];
            int posX = GAME_START_X + x * GRID_SIZE;
            int posY = GAME_START_Y + y * GRID_SIZE;
            if (show_debug || cell.revealed) {
                if (cell.value == MINE) {
                    DrawCircle( posX + GRID_SIZE/2, posY + GRID_SIZE/2, (float)GRID_SIZE/3, RED);
                } else {
                    char tmp[2] = {0};
                    sprintf(tmp, "%d", cell.value);
                    int text_size = MeasureText(tmp, FONT_SIZE);
                    DrawText(tmp, posX + (GRID_SIZE - text_size) / 2, posY + (GRID_SIZE - FONT_SIZE) / 2, FONT_SIZE, DARKGRAY);
                }
            }
            if (!show_debug && cell.flagged) {
                char* txt = "M";
                int text_size = MeasureText(txt, FONT_SIZE);
                DrawText(txt, posX + (GRID_SIZE - text_size) / 2, posY + (GRID_SIZE - FONT_SIZE) / 2, FONT_SIZE, RED);
            }
        }
    }
}


void ms_DrawGrid() {
    for (int i = 0; i < COLUMNS + 1; i++) {
        Vector2 startPosV = { .x = GAME_START_X + i * GRID_SIZE, .y = GAME_START_Y };
        Vector2 endPosV = { .x = GAME_START_X + i * GRID_SIZE, .y = GAME_START_Y + GAME_HEIGHT };
        DrawLineEx(startPosV, endPosV, 3, BLACK);
    }
    for (int i = 0; i < ROWS + 1; i++) {
        Vector2 startPosH = { .x = GAME_START_X, .y = GAME_START_Y + i * GRID_SIZE };
        Vector2 endPosH = { .x = GAME_START_X + GAME_WIDTH, .y = GAME_START_Y + i * GRID_SIZE };
        DrawLineEx(startPosH, endPosH, 3, BLACK);
    }
}

void ms_ExpandZeros(ms_Pos pos) {
    ms_Pos queue[ROWS * COLUMNS] = { pos };
    int index = 0;

    while (index >= 0) {
        ms_Pos cur = queue[index--];
        ms_Cell* cell = &game_data[cur.y * COLUMNS + cur.x];
        cell->revealed = true;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dy == 0 && dx == 0) {
                    continue;
                }
                ms_Pos new_pos = { .x = cur.x + dx, .y = cur.y + dy };
                if (new_pos.y < 0 || new_pos.y >= ROWS || new_pos.x < 0 || new_pos.x >= COLUMNS) {
                    continue;
                }
                ms_Cell* cell = &game_data[new_pos.y * COLUMNS + new_pos.x];
                if (!cell->revealed && !cell->flagged && cell->value == 0) {
                    queue[++index] = new_pos;
                }
                if (!cell->flagged) {
                    cell->revealed = true;
                }
            }
        }
    }
}

void ms_DrawMenu() {
    char msg[32];

    sprintf(msg, "Mines Left: %d", mines_left);
    DrawText(msg, 5, 5, MENU_FONT_SIZE, BLUE);

    long duration = (end_time ? end_time : time(NULL)) - start_time;
    sprintf(msg, "Time: %03ld", duration);
    int text_size = MeasureText(msg, MENU_FONT_SIZE);
    DrawText(msg, WIDTH - text_size - 5, 5, MENU_FONT_SIZE, BLUE);

    strcpy(msg, "Press 'R' to start a new game");
    text_size = MeasureText(msg, MENU_FONT_SIZE);
    DrawText(msg, (WIDTH - text_size) / 2, GAME_START_Y - MENU_FONT_SIZE - 5, MENU_FONT_SIZE, DARKGRAY);
}

void ms_InitGameData() {
    ms_Pos mines[MINE_COUNT] = {0};
    size_t cur = 0;
    while (cur < MINE_COUNT) {
        int y = rand() % ROWS;
        int x = rand() % COLUMNS;
        bool foundNew = true;
        for (size_t i = 0; i < cur; i++) {
            ms_Pos mine = mines[i];
            if (mine.x == x && mine.y == y) {
                foundNew = false;
                break;
            }
        }
        if (foundNew) {
            mines[cur++] = (ms_Pos){ .x = x, .y = y };
        }
    }

    for (size_t i = 0; i < MINE_COUNT; i++) {
        ms_Pos mine = mines[i];
        game_data[mine.y * COLUMNS + mine.x].value = MINE;
        // mark neighbours
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dy == 0 && dx == 0) {
                    continue;
                }
                ms_Pos new_pos = { .x = mine.x + dx, .y = mine.y + dy };
                // out-of-bounds
                if (new_pos.y < 0 || new_pos.y >= ROWS || new_pos.x < 0 || new_pos.x >= COLUMNS) {
                    continue;
                }
                if (game_data[new_pos.y * COLUMNS + new_pos.x].value == MINE) {
                    continue;
                }
                game_data[new_pos.y * COLUMNS + new_pos.x].value++;
            }
        }
    }
}

bool ms_CheckGameWon() {
    for (int y = 0; y < ROWS; y++) {
        for (int x = 0; x < COLUMNS; x++) {
            ms_Cell cell = game_data[y * COLUMNS + x];
            if (cell.value != MINE && !cell.revealed) {
                return false;
            }
        }
    }
    return true;
}

void ms_DrawItem(ms_MenuItem* item, bool selected, bool active) {
    const int OUTER_ELLIPSE_H = 100;
    const int OUTER_ELLIPSE_V = 20;
    const int INNER_ELLIPSE_H = 95;
    const int INNER_ELLIPSE_V = 17;

    if (selected) {
        Color border_color = active ? item->normal : RAYWHITE;
        DrawEllipse(MENU_WIDTH/2, item->y+MENU_FONT_SIZE/2, OUTER_ELLIPSE_H, OUTER_ELLIPSE_V, BLACK);
        DrawEllipse(MENU_WIDTH/2, item->y+MENU_FONT_SIZE/2, INNER_ELLIPSE_H, INNER_ELLIPSE_V, border_color);
    }

    Color font_color = (selected && active) ? item->highlight : item->normal;
    DrawText(item->name, (MENU_WIDTH-item->text_size)/2, item->y, MENU_FONT_SIZE, font_color);
}

void ms_InitMenuItems(ms_MenuItem items[3], int beginn_Y) {
    char* names[] = { "Beginner", "Intermediate", "Expert"};
    Color colors[] = { GREEN, BLUE, RED };
    for (size_t i = 0; i < 3; i++) {
        items[i].name = names[i];
        items[i].text_size = MeasureText(names[i], MENU_FONT_SIZE);
        items[i].normal = colors[i];
        items[i].highlight = BLACK;
        items[i].y = beginn_Y;
        beginn_Y += MENU_FONT_SIZE + 25;
    }
}
