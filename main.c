#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <stddef.h>

#include "raylib.h"

#define MENU_HEIGHT        60

#define GRID_SIZE          40
#define ROWS               9
#define BOMB_COUNT         10
#define GAME_WIDTH         ((ROWS) * (GRID_SIZE))
#define GAME_HEIGHT        ((ROWS) * (GRID_SIZE))

#define PADDING            10

#define GAME_START_Y       ((MENU_HEIGHT) + (PADDING))
#define GAME_START_X       (PADDING)

#define GAME_STATUS_HEIGHT 60
#define GAME_STATUS_START_Y ((GAME_START_Y) + (GAME_HEIGHT))

#define WIDTH              ((GAME_WIDTH) + 2 * (PADDING))
#define HEIGHT             ((GAME_HEIGHT) + 2 * (PADDING) + (MENU_HEIGHT) + (GAME_STATUS_HEIGHT))

#define FONT_SIZE          32
#define MENU_FONT_SIZE     24

#define BOMB               -1

typedef struct {
    int  value;
    bool marked;
    bool selected;
} ms_Field;

typedef struct {
    int x, y;
} ms_Pos;

typedef enum {
    ms_PLAYING,
    ms_GAME_OVER,
    ms_GAME_WON,
    ms_NEW_GAME,
} ms_GameState;

ms_GameState game_state = ms_PLAYING;
ms_Field game_data[ROWS * ROWS] = {0};
bool show_debug = false;
int booms_left = BOMB_COUNT;
long start_time;
long end_time;
bool update_time = true;

void ms_InitGameData();
void ms_DrawGrid();
void ms_DrawGameState();
void ms_DrawMenu();
void ms_ExpandZeros(ms_Pos pos);
bool ms_MouseGridPos(ms_Pos* pos);
bool ms_CheckGameWon();

int main() {
    srand(time(NULL));
    ms_InitGameData();

    InitWindow(WIDTH, HEIGHT, "Minesweeper");
    SetTargetFPS(60);

    start_time = time(NULL);

    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_R)) {
            game_state = ms_NEW_GAME;
        }

        BeginDrawing();

        ClearBackground(RAYWHITE);
        ms_DrawMenu();
        ms_DrawGrid();
        ms_DrawGameState();

        switch (game_state) {
            case ms_PLAYING:
                {
                    ms_Pos grid_pos = {0};
                    if (ms_MouseGridPos(&grid_pos)) {
                        ms_Field* field = &game_data[grid_pos.y * ROWS + grid_pos.x];
                        if (!field->selected && !field->marked) {
                            DrawRectangle(GAME_START_X+grid_pos.x*GRID_SIZE, GAME_START_Y+grid_pos.y*GRID_SIZE, GRID_SIZE, GRID_SIZE, LIGHTGRAY);
                        }

                        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !field->selected && !field->marked) {
                            field->selected = true;
                            // TODO: handle game over
                            if (field->value == BOMB) {
                                game_state = ms_GAME_OVER;
                                for (int y = 0; y < ROWS; y++) {
                                    for (int x = 0; x < ROWS; x++) {
                                        ms_Field* field = &game_data[y * ROWS + x];
                                        if (field->value == BOMB) {
                                            field->selected = true;
                                        }
                                    }
                                }
                            } else if (field->value == 0) {
                                ms_ExpandZeros(grid_pos);
                            }
                        }

                        if (IsKeyPressed(KEY_M) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
                            if (!field->selected) {
                                field->marked = !field->marked;
                                booms_left += field->marked ? -1 : 1;
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
                    booms_left = BOMB_COUNT;
                }
                break;
        }

        EndDrawing();

        if (ms_CheckGameWon()) {
            game_state = ms_GAME_WON;
        }
    }
    CloseWindow();
}

/*Returns true if mouse is inside the grid and sets the ms_Pos struct accordingly,
 * otherwise returns false.*/
bool ms_MouseGridPos(ms_Pos* grid_pos) {
    Vector2 mouse_pos = GetMousePosition();
    mouse_pos.y -= GAME_START_Y;
    mouse_pos.x -= GAME_START_X;

    if (mouse_pos.y < 0 || mouse_pos.y >= ROWS * GRID_SIZE || mouse_pos.x < 0 || mouse_pos.x >= ROWS * GRID_SIZE) {
        return false;
    }

    grid_pos->y = mouse_pos.y / GRID_SIZE;
    grid_pos->x = mouse_pos.x / GRID_SIZE;
    return true;
}

void ms_DrawGameState() {
    for (int y = 0; y < ROWS; y++) {
        for (int x = 0; x < ROWS; x++) {
            ms_Field field = game_data[y * ROWS + x];
            int posX = GAME_START_X + x * GRID_SIZE;
            int posY = GAME_START_Y + y * GRID_SIZE;
            if (show_debug || field.selected) {
                if (field.value == BOMB) {
                    DrawCircle( posX + GRID_SIZE/2, posY + GRID_SIZE/2, (float)GRID_SIZE/3, RED);
                } else {
                    char tmp[2] = {0};
                    sprintf(tmp, "%d", field.value);
                    int text_size = MeasureText(tmp, FONT_SIZE);
                    DrawText(tmp, posX + (GRID_SIZE - text_size) / 2, posY + (GRID_SIZE - FONT_SIZE) / 2, FONT_SIZE, DARKGRAY);
                }
            }
            if (!show_debug && field.marked) {
                char* txt = "M";
                int text_size = MeasureText(txt, FONT_SIZE);
                DrawText(txt, posX + (GRID_SIZE - text_size) / 2, posY + (GRID_SIZE - FONT_SIZE) / 2, FONT_SIZE, RED);
            }
        }
    }
}


void ms_DrawGrid() {
    for (int i = 0; i < ROWS + 1; i++) {
        Vector2 startPosV = { .x = GAME_START_X + i * GRID_SIZE, .y = GAME_START_Y };
        Vector2 endPosV = { .x = GAME_START_X + i * GRID_SIZE, .y = GAME_START_Y + GAME_HEIGHT };
        DrawLineEx(startPosV, endPosV, 3, BLACK);
        Vector2 startPosH = { .x = GAME_START_X, .y = GAME_START_Y + i * GRID_SIZE };
        Vector2 endPosH = { .x = GAME_START_X + GAME_WIDTH, .y = GAME_START_Y + i * GRID_SIZE };
        DrawLineEx(startPosH, endPosH, 3, BLACK);
    }
}

void ms_ExpandZeros(ms_Pos pos) {
    ms_Pos queue[ROWS * ROWS] = { pos };
    int index = 0;

    while (index >= 0) {
        ms_Pos cur = queue[index--];
        ms_Field* field = &game_data[cur.y * ROWS + cur.x];
        field->selected = true;
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dy == 0 && dx == 0) {
                    continue;
                }
                ms_Pos new_pos = { .x = cur.x + dx, .y = cur.y + dy };
                if (new_pos.y < 0 || new_pos.y >= ROWS || new_pos.x < 0 || new_pos.x >= ROWS) {
                    continue;
                }
                ms_Field* field = &game_data[new_pos.y * ROWS + new_pos.x];
                if (!field->selected && !field->marked && field->value == 0) {
                    queue[++index] = new_pos;
                }
                if (!field->marked) {
                    field->selected = true;
                }
            }
        }
    }
}

void ms_DrawMenu() {
    char msg[32];

    sprintf(msg, "Bombs Left: %d", booms_left);
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
    ms_Pos bombs[BOMB_COUNT] = {0};
    size_t cur = 0;
    while (cur < BOMB_COUNT) {
        int x = rand() % ROWS;
        int y = rand() % ROWS;
        bool foundNew = true;
        for (size_t i = 0; i < cur; i++) {
            ms_Pos bomb = bombs[i];
            if (bomb.x == x && bomb.y == y) {
                foundNew = false;
                break;
            }
        }
        if (foundNew) {
            bombs[cur++] = (ms_Pos){ .x = x, .y = y };
        }
    }

    for (size_t i = 0; i < BOMB_COUNT; i++) {
        ms_Pos bomb = bombs[i];
        game_data[bomb.y * ROWS + bomb.x].value = BOMB;
        // mark neighbours
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dy == 0 && dx == 0) {
                    continue;
                }
                ms_Pos new_pos = { .x = bomb.x + dx, .y = bomb.y + dy };
                // out-of-bounds
                if (new_pos.y < 0 || new_pos.y >= ROWS || new_pos.x < 0 || new_pos.x >= ROWS) {
                    continue;
                }
                if (game_data[new_pos.y * ROWS + new_pos.x].value == BOMB) {
                    continue;
                }
                game_data[new_pos.y * ROWS + new_pos.x].value++;
            }
        }
    }
}

bool ms_CheckGameWon() {
    for (int y = 0; y < ROWS; y++) {
        for (int x = 0; x < ROWS; x++) {
            ms_Field field = game_data[y * ROWS + x];
            if (!field.selected && !field.marked) {
                return false;
            }
        }
    }
    return true;
}
