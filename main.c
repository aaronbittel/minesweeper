#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <stddef.h>

#include "raylib.h"

#define GAME_MENU_HEIGHT        60
#define GAME_STATUS_HEIGHT 60

#define MENU_WIDTH 400
#define MENU_HEIGHT 300

#define FONT_SIZE          32
#define MENU_FONT_SIZE     24
#define SUB_MENU_FONT_SIZE 20

#define BEGINNER_GRID_SIZE   40
#define BEGINNER_FONT_SIZE FONT_SIZE
#define BEGINNER_ROWS         9
#define BEGINNER_COLUMNS      9
#define BEGINNER_MINE_COUNT  10

#define INTERMEDIATE_FONT_SIZE   32
#define INTERMEDIATE_GRID_SIZE   36
#define INTERMEDIATE_ROWS        16
#define INTERMEDIATE_COLUMNS     16
#define INTERMEDIATE_MINE_COUNT  40

#define EXPERT_FONT_SIZE   30
#define EXPERT_GRID_SIZE   30
#define EXPERT_ROWS        16
#define EXPERT_COLUMNS     30
#define EXPERT_MINE_COUNT  99

#define PADDING            10

#define GAME_START_Y       ((GAME_MENU_HEIGHT) + (PADDING))
#define GAME_START_X       (PADDING)


#define MINE               -1


#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))


typedef struct {
    int  value;
    bool flagged;
    bool revealed;
} ms_Cell;

typedef struct {
    ms_Cell game_data[EXPERT_ROWS * EXPERT_COLUMNS];
    int rows;
    int cols;
    int mines_left;
    bool first_click_done;
} ms_Game;

typedef struct {
    int width;
    int height;
    int grid_size;
    int font_size;
} ms_RenderConfig;

typedef struct {
    int x, y;
} ms_Pos;

typedef enum {
    ms_ScreenMenu = 0,
    ms_ScreenGame,
} ms_GameScreen;

typedef enum {
    ms_PLAYING,
    ms_GAME_OVER,
    ms_GAME_WON,
    ms_NEW_GAME,
} ms_GameState;

typedef struct {
    char* name;
    int text_size;
    int y;
    Color normal;
    Color highlight;
} ms_MenuItem;

typedef enum {
    ms_BEGINNER = 0,
    ms_INTERMEDIATE,
    ms_EXPERT,
} ms_Difficulty;

ms_GameState game_state = ms_PLAYING;
ms_GameScreen current_screen = ms_ScreenMenu;
bool show_debug = false;

ms_Game game = {0};
ms_RenderConfig config = {0};

long start_time;

void ms_InitGame(int rows, int columns, int mines);
void ms_InitBeginnerGame();
void ms_InitIntermediateGame();
void ms_InitExpertGame();

void ms_InitGameData(ms_Pos *first_click_pos);
void ms_DrawGrid();
void ms_DrawGameState();
void ms_DrawGameMenu(float game_time);
void ms_DrawItem(ms_MenuItem* item, bool selected, bool active);
void ms_InitMenuItems(ms_MenuItem items[3], int beginn_Y);
ms_Cell* ms_RevealCell(ms_Pos *pos);
int ms_FlagCell(ms_Pos *pos);
void ms_ExpandZeros(ms_Pos pos);

bool ms_GetMouseGridPos(ms_Pos* pos);
bool ms_CheckGameWon();
int ms_GetGameStatusStartY();
bool ms_PosEqual(ms_Pos *p1, ms_Pos *p2);
bool ms_PosIsNeighbour(ms_Pos *origin, ms_Pos *pos);
ms_Cell *ms_AtPos(ms_Pos *pos);
ms_Cell *ms_AtXY(int x, int y);

int ms_GetTotalGameWindowWidth();
int ms_GetTotalGameWindowHeight();

inline ms_Pos ms_PosXY(int x, int y);


int main() {
    InitWindow(MENU_WIDTH, MENU_HEIGHT, "Minesweeper");
    SetExitKey(KEY_Q);
    SetTargetFPS(60);

    // game menu

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

    ms_Difficulty selected_difficulty = ms_BEGINNER;

    float highlight_timer = 0.0f;
    const float highligh_duration = 0.12f;

    // gameplay

    srand(time(NULL));

    ms_Pos grid_pos = {0};
    bool mouse_inside_grid = false;
    float game_time = 0.f;

    while (!WindowShouldClose()) {
        switch (current_screen) {
            case ms_ScreenMenu:
                {
                    int mouseY = GetMouseY();
                    for (size_t i = 0; i < ARRAY_LEN(items); i++) {
                        if (mouseY >= items[i].y-MENU_FONT_SIZE/2 &&
                            mouseY <= items[i].y + MENU_FONT_SIZE*2)
                        {
                            selected_difficulty = i;
                        }
                    }
                    if (IsKeyPressed(KEY_ENTER) ||
                        IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    {
                        highlight_timer = highligh_duration;
                        locked_in = true;
                    }
                    if (locked_in) highlight_timer -= GetFrameTime();
                    if (highlight_timer < 0) {
                        switch (selected_difficulty) {
                            case ms_BEGINNER: ms_InitBeginnerGame(); break;
                            case ms_INTERMEDIATE: ms_InitIntermediateGame(); break;
                            case ms_EXPERT: ms_InitExpertGame(); break;
                            // TODO: add support for custom values
                            default: assert(0 && "unreachable");
                        }
                        highlight_timer = 0;
                        locked_in = false;

                        SetWindowSize(
                            2 * PADDING + config.width,
                            2 * PADDING + GAME_MENU_HEIGHT + config.height + GAME_STATUS_HEIGHT
                        );
                        int monitor = GetCurrentMonitor();
                        int monitor_width =  GetMonitorWidth(monitor);
                        int monitor_height = GetMonitorHeight(monitor);
                        SetWindowPosition(
                            (monitor_width - ms_GetTotalGameWindowWidth()) / 2,
                            (monitor_height - ms_GetTotalGameWindowHeight()) / 2
                        );
                        current_screen = ms_ScreenGame;
                        game_state = ms_PLAYING;
                    }
                } break;
            case ms_ScreenGame:
                {
                    if (IsKeyPressed(KEY_ESCAPE)) {
                        int monitor = GetCurrentMonitor();
                        int monitor_width =  GetMonitorWidth(monitor);
                        int monitor_height = GetMonitorHeight(monitor);
                        SetWindowPosition(
                            (monitor_width - MENU_WIDTH) / 2,
                            (monitor_height - MENU_HEIGHT) / 2
                        );
                        SetWindowSize(MENU_WIDTH, MENU_HEIGHT);
                        current_screen = ms_ScreenMenu;
                        break;
                    }
                    if (IsKeyPressed(KEY_R)) {
                        game_state = ms_NEW_GAME;
                    }
                    if (IsKeyPressed(KEY_G)) {
                        show_debug = !show_debug;
                    }
                    switch (game_state) {
                        case ms_PLAYING:
                            {
                                game_time += GetFrameTime();
                                mouse_inside_grid = ms_GetMouseGridPos(&grid_pos);
                                if (mouse_inside_grid) {
                                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                        if (!game.first_click_done) {
                                            ms_InitGameData(&grid_pos);
                                            game.first_click_done = true;
                                        }
                                        ms_Cell *cell = ms_RevealCell(&grid_pos);
                                        if (cell) {
                                            if (cell->value == 0) {
                                                ms_ExpandZeros(grid_pos);
                                            } else if (cell->value == MINE) {
                                                game_state = ms_GAME_OVER;
                                            }
                                        }
                                    }
                                    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) ||
                                        IsKeyPressed(KEY_M))
                                    {
                                        game.mines_left += ms_FlagCell(&grid_pos);
                                    }
                                    if (ms_CheckGameWon()) {
                                        game_state = ms_GAME_WON;
                                    }
                                }
                            } break;
                        case ms_GAME_OVER:
                        case ms_GAME_WON:
                            {
                                if (IsKeyPressed(KEY_R)) {
                                    game_state = ms_NEW_GAME;
                                }
                            } break;
                        case ms_NEW_GAME:
                            {
                                switch (selected_difficulty) {
                                    case ms_BEGINNER: ms_InitBeginnerGame(); break;
                                    case ms_INTERMEDIATE: ms_InitIntermediateGame(); break;
                                    case ms_EXPERT: ms_InitExpertGame(); break;
                                    default: assert(0 && "unreachable");
                                }
                            } break;
                    }
                } break;
            default: assert(0 && "unreachable");
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        switch (current_screen) {
            case ms_ScreenMenu:
                {
                    DrawText(
                        menu_title,
                        (MENU_WIDTH-menu_title_size)/2,
                        menu_title_Y,
                        FONT_SIZE, RED);
                    DrawText(sub_title,
                             (MENU_WIDTH-sub_title_size)/2,
                             submenu_title_Y,
                             MENU_FONT_SIZE, DARKGRAY);
                    for (size_t i = 0; i < 3; i++) {
                        ms_DrawItem(&items[i], selected_difficulty == i, highlight_timer > 0);
                    }
                } break;
            case ms_ScreenGame:
                {
                    ms_DrawGameMenu(game_time);
                    ms_DrawGrid();
                    ms_DrawGameState();
                    switch(game_state) {
                        case ms_PLAYING:
                            {
                                if (mouse_inside_grid) {
                                    ms_Cell* cell = ms_AtPos(&grid_pos);
                                    if (!cell->revealed && !cell->flagged) {
                                        DrawRectangle(GAME_START_X+grid_pos.x*config.grid_size, GAME_START_Y+grid_pos.y*config.grid_size, config.grid_size, config.grid_size, LIGHTGRAY);
                                    }
                                }
                            } break;
                        case ms_GAME_OVER:
                            {
                                char* msg = "Game Over!";
                                int text_size = MeasureText(msg, FONT_SIZE);
                                DrawText(
                                    msg,
                                    (config.width - text_size) / 2,
                                    ms_GetGameStatusStartY(),
                                    FONT_SIZE, RED);
                            } break;
                        case ms_GAME_WON:
                            {
                                char* msg = "Congrats, Game Won!";
                                int text_size = MeasureText(msg, FONT_SIZE);
                                DrawText(
                                    msg,
                                    (config.width - text_size) / 2,
                                    ms_GetGameStatusStartY(),
                                    FONT_SIZE, GREEN);
                            } break;
                        case ms_NEW_GAME:
                            {
                                game_time = 0;
                                game_state = ms_PLAYING;
                            } break;
                    }
                } break;
            default: assert(0 && "unreachable");
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

/*Returns true if mouse is inside the grid and sets the ms_Pos struct accordingly,
 * otherwise returns false.*/
bool ms_GetMouseGridPos(ms_Pos* grid_pos) {
    Vector2 mouse_pos = GetMousePosition();
    mouse_pos.y -= GAME_START_Y;
    mouse_pos.x -= GAME_START_X;

    if (mouse_pos.y < 0 || mouse_pos.y >= game.rows * config.grid_size || mouse_pos.x < 0 || mouse_pos.x >= game.cols * config.grid_size) {
        return false;
    }

    grid_pos->y = mouse_pos.y / config.grid_size;
    grid_pos->x = mouse_pos.x / config.grid_size;
    return true;
}

void ms_DrawGameState() {
    for (int y = 0; y < game.rows; y++) {
        for (int x = 0; x < game.cols; x++) {
            ms_Cell *cell = ms_AtXY(x, y);
            int posX = GAME_START_X + x * config.grid_size;
            int posY = GAME_START_Y + y * config.grid_size;
            if (show_debug || cell->revealed) {
                if (cell->value == MINE) {
                    DrawCircle( posX + config.grid_size/2, posY + config.grid_size/2, (float)config.grid_size/3, RED);
                } else {
                    char val[2] = {0};
                    sprintf(val, "%d", cell->value);
                    int text_size = MeasureText(val, config.font_size);
                    DrawText(
                        val,
                        posX + (config.grid_size - text_size) / 2,
                        posY + (config.grid_size - config.font_size) / 2,
                        config.font_size, DARKGRAY);
                }
            }
            if (!show_debug && cell->flagged) {
                char* flag = "M";
                int text_size = MeasureText(flag, config.font_size);
                DrawText(
                    flag,
                    posX + (config.grid_size - text_size) / 2,
                    posY + (config.grid_size - config.font_size) / 2,
                    config.font_size, RED);
            }
        }
    }
}

void ms_DrawGrid() {
    for (int i = 0; i < game.cols + 1; i++) {
        Vector2 startPosV = { .x = GAME_START_X + i * config.grid_size, .y = GAME_START_Y };
        Vector2 endPosV = { .x = GAME_START_X + i * config.grid_size, .y = GAME_START_Y + game.rows * config.grid_size };
        DrawLineEx(startPosV, endPosV, 3, BLACK);
    }
    for (int i = 0; i < game.rows + 1; i++) {
        Vector2 startPosH = { .x = GAME_START_X, .y = GAME_START_Y + i * config.grid_size };
        Vector2 endPosH = { .x = GAME_START_X + game.cols * config.grid_size, .y = GAME_START_Y + i * config.grid_size };
        DrawLineEx(startPosH, endPosH, 3, BLACK);
    }
}

void ms_ExpandZeros(ms_Pos pos) {
    ms_Pos queue[EXPERT_ROWS * EXPERT_COLUMNS] = { pos };
    int index = 0;

    while (index >= 0) {
        ms_Pos cur = queue[index--];
        ms_Cell* cell = ms_AtPos(&cur);
        cell->revealed = true;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dy == 0 && dx == 0) {
                    continue;
                }
                ms_Pos new_pos = { .x = cur.x + dx, .y = cur.y + dy };
                if (new_pos.y < 0 || new_pos.y >= game.rows || new_pos.x < 0 || new_pos.x >= game.cols) {
                    continue;
                }
                ms_Cell* cell = ms_AtPos(&new_pos);
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

void ms_DrawGameMenu(float game_time) {
    const int BUF_LEN = 64;
    char msg[BUF_LEN];

    snprintf(msg, BUF_LEN, "Mines Left: %d", game.mines_left);
    DrawText(msg, PADDING, PADDING, MENU_FONT_SIZE, BLUE);

    snprintf(msg, BUF_LEN, "Time: %03ld", (long)game_time);
    int text_size = MeasureText(msg, MENU_FONT_SIZE);
    DrawText(msg, config.width - text_size - PADDING, PADDING, MENU_FONT_SIZE, BLUE);

    strncpy(msg, "R - New Game | ECS - Menu | Q - Quit", BUF_LEN);
    text_size = MeasureText(msg, SUB_MENU_FONT_SIZE);
    DrawText(
        msg,
        (config.width - text_size) / 2,
        GAME_START_Y - SUB_MENU_FONT_SIZE - PADDING,
        SUB_MENU_FONT_SIZE, DARKGRAY);
}

void ms_InitGameData(ms_Pos *first_click_pos) {
    ms_Pos mines[EXPERT_MINE_COUNT] = {0};
    for (size_t i = 0; i < (size_t)game.mines_left; i++) {
        ms_Pos pos;
        do {
            pos.x = rand() % game.cols;
            pos.y = rand() % game.rows;
        } while(ms_PosIsNeighbour(&pos, first_click_pos) || ms_AtPos(&pos)->value == MINE);
        mines[i] = pos;
        ms_AtPos(&pos)->value = MINE;
    }
    for (size_t i = 0; i < (size_t)game.mines_left; i++) {
        ms_Pos mine = mines[i];
        // mark neighbours
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dy == 0 && dx == 0) {
                    continue;
                }
                ms_Pos new_pos = { .x = mine.x + dx, .y = mine.y + dy };
                // check out-of-bounds
                if (new_pos.y < 0 || new_pos.y >= game.rows || new_pos.x < 0 || new_pos.x >= game.cols) {
                    continue;
                }
                ms_Cell *cell = ms_AtPos(&new_pos);
                if (cell->value == MINE) {
                    continue;
                }
                cell->value++;
            }
        }
    }
}

/*Returns true if game is won, false if not.*/
bool ms_CheckGameWon() {
    for (int y = 0; y < game.rows; y++) {
        for (int x = 0; x < game.cols; x++) {
            ms_Cell *cell = ms_AtXY(x, y);
            if (cell->revealed) {
                continue;
            }
            if (cell->value == MINE && cell->flagged) {
                continue;
            }
            return false;
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

ms_Cell* ms_RevealCell(ms_Pos *pos) {
    ms_Cell *cell = ms_AtPos(pos);
    if (cell->revealed || cell->flagged) {
        return NULL;
    }
    cell->revealed = true;
    return cell;
}

/*Flag a cell if possible.
 * Returns how the mine_count need to change.
 * Update the mine_count with `mine_count += ms_FlagCell()`.
 *  - 0  invalid cell, dont change mine_count
 *  - +1 flagged a cell, decrease mine_count
 *  - -1 unflagged a cell, increase mine_count*/
int ms_FlagCell(ms_Pos *pos) {
    ms_Cell *cell = ms_AtPos(pos);
    if (cell->revealed) {
        return 0;
    }
    cell->flagged = !cell->flagged;
    return cell->flagged ? -1 : 1;
}

void ms_InitGame(int rows, int columns, int mines) {
    memset(game.game_data, 0, sizeof(game.game_data));
    game.rows = rows;
    game.cols = columns;
    game.mines_left = mines;
    game.first_click_done = false;
}

void ms_InitBeginnerGame() {
    config.width = 2 * PADDING + BEGINNER_COLUMNS * BEGINNER_GRID_SIZE;
    config.height = 2 * PADDING + BEGINNER_ROWS * BEGINNER_GRID_SIZE;
    config.grid_size = BEGINNER_GRID_SIZE;
    config.font_size = BEGINNER_FONT_SIZE;
    ms_InitGame(BEGINNER_ROWS, BEGINNER_COLUMNS, BEGINNER_MINE_COUNT);
}

void ms_InitIntermediateGame() {
    config.width = 2 * PADDING + INTERMEDIATE_COLUMNS * INTERMEDIATE_GRID_SIZE;
    config.height = 2 * PADDING + INTERMEDIATE_ROWS * INTERMEDIATE_GRID_SIZE;
    config.grid_size = INTERMEDIATE_GRID_SIZE;
    config.font_size = INTERMEDIATE_FONT_SIZE;
    ms_InitGame(INTERMEDIATE_ROWS, INTERMEDIATE_COLUMNS, INTERMEDIATE_MINE_COUNT);
}

void ms_InitExpertGame() {
    config.width = 2 * PADDING + EXPERT_COLUMNS * EXPERT_GRID_SIZE;
    config.height = 2 * PADDING + EXPERT_ROWS * EXPERT_GRID_SIZE;
    config.grid_size = EXPERT_GRID_SIZE;
    config.font_size = EXPERT_FONT_SIZE;
    ms_InitGame(EXPERT_ROWS, EXPERT_COLUMNS, EXPERT_MINE_COUNT);
}

int ms_GetGameStatusStartY() {
    return GAME_START_Y + config.height + PADDING;
}

bool ms_PosEqual(ms_Pos *p1, ms_Pos *p2) {
    return p1->x == p2->x && p1->y == p2->y;
}

// Returns true if `pos` is the origin or one of the 8 surrounding cells
bool ms_PosIsNeighbour(ms_Pos *origin, ms_Pos *pos) {
    int dx = abs(origin->x-pos->x);
    int dy = abs(origin->y-pos->y);
    return (dx <= 1 && dy <= 1);
}

ms_Cell *ms_AtPos(ms_Pos *pos) {
    return &game.game_data[pos->y * game.cols + pos->x];
}

inline ms_Cell *ms_AtXY(int x, int y) {
    return &game.game_data[y * game.cols + x];
}

inline ms_Pos ms_PosXY(int x, int y) {
    return (ms_Pos) { .x = x, .y = y };
}

int ms_GetTotalGameWindowWidth() {
    return config.width + 2 * PADDING;
}

int ms_GetTotalGameWindowHeight() {
    return GAME_MENU_HEIGHT + config.height + 2 * PADDING + GAME_STATUS_HEIGHT;
}
