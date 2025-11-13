#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <locale.h>
#include <stdbool.h>
#include <windows.h> 


#define RU(text) win1251_to_utf8(text)

#define FONT_SIZE               36
#define BOARD_PADDING           200

#define MAX_PLAYERS             10
#define MAX_SAVES               3
#define MAX_SHIPS               11

#define MAIN_BUTTONS_COUNT      4
#define SETTINGS_BUTTONS_COUNT  9

#define CELL_SIZE_RATIO          0.03f  
#define MENU_BUTTON_WIDTH_RATIO 0.4f   
#define MENU_BUTTON_HEIGHT_RATIO 0.05f 


typedef struct HitCell {
    short int x;
    short int y;
    struct HitCell* next;
} HitCell;

void addHitCell(HitCell** head, short int x, short int y) {
    HitCell* newCell = (HitCell*)malloc(sizeof(HitCell));
    if (newCell == NULL) {
        return;
    }
    newCell->x = x;
    newCell->y = y;
    newCell->next = *head;
    *head = newCell;
}

void removeHitCell(HitCell** head, short int x, short int y) {
    HitCell* current = *head;
    HitCell* prev = NULL;

    while (current != NULL) {
        if (current->x == x && current->y == y) {
            if (prev == NULL) {
                *head = current->next;
            }
            else {
                prev->next = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

void clearHitList(HitCell** head) {
    HitCell* current = *head;
    while (current != NULL) {
        HitCell* next = current->next;
        free(current);
        current = next;
    }
    *head = NULL;
}


typedef struct {
    char chanceToHit;
} Computer;


typedef enum {
    BUTTON_NEW_GAME,
    BUTTON_LOAD_GAME,
    BUTTON_EXIT,
    BUTTON_SIZE_10X10,
    BUTTON_SIZE_15X15,
    BUTTON_EASY_DIFF,
    BUTTON_NORMAL_DIFF,
    BUTTON_HARD_DIFF,
    BUTTON_USER_COMP,
    BUTTON_COMP_COMP,
    BUTTON_START,
    BUTTON_RADAR,
    BUTTON_PLACE_RANDOM,
    BUTTON_SAVE_GAME,
    BUTTON_SAVE_SLOT,
    BUTTON_LEADERBOARD,
    BUTTON_ADD,
    BUTTON_EXIT_MENU,
} ButtonType;

typedef struct {
    const char* label;
    short int x_pos;
    short int y_pos;
    short int left;
    short int right;
    short int top;
    short int bottom;
    bool is_pressed;
    bool is_active;
    ButtonType type;
} Button;
Button main_menu_buttons[] = {
     {"Новая игра", 0, 0, 0, 0, 0, 0, false, true, BUTTON_NEW_GAME},
     {"Загрузить игру", 0, 0, 0, 0, 0, 0, false, true, BUTTON_LOAD_GAME},
     {"Таблица лидеров", 0, 0, 0, 0, 0, 0, false, true, BUTTON_LEADERBOARD},
     {"Выход", 0, 0, 0, 0, 0, 0, false, true, BUTTON_EXIT}
};

Button settings_menu_buttons[] = {
    {"10 x 10", 0, 0, 0, 0, 0, 0, true,  true, BUTTON_SIZE_10X10},
    {"15 x 15", 0, 0, 0, 0, 0, 0, false, true, BUTTON_SIZE_15X15},
    {"Легко", 0, 0, 0, 0, 0, 0, false, true, BUTTON_EASY_DIFF},
    {"Нормально", 0, 0, 0, 0, 0, 0, false, true, BUTTON_NORMAL_DIFF},
    {"Сложно", 0, 0, 0, 0, 0, 0, false, true, BUTTON_HARD_DIFF},
    {"Пользователь против компьютера", 0,0,0,0,0,0,true,true,BUTTON_USER_COMP},
    {"Компьютер против компьютера", 0,0,0,0,0,0,false,true,BUTTON_COMP_COMP},
    {"Начать", 0, 0, 0, 0, 0, 0, false, true, BUTTON_START},
    {"В меню", 0, 0, 0, 0, 0, 0, false, true, BUTTON_EXIT_MENU}
};

Button game_buttons[] = {
    {"Радар", 0, 0, 0, 0, 0, 0, false, false, BUTTON_RADAR},
    {"Расставить случайно", 0, 0, 0, 0, 0, 0, false, true,  BUTTON_PLACE_RANDOM},
    {"Сохранить игру", 0, 0, 0, 0, 0, 0, false, false, BUTTON_SAVE_GAME},
     {"В меню", 0, 0, 0, 0, 0, 0, false, true, BUTTON_EXIT_MENU}
};

Button leaderboard_buttons[] = {
    {"В меню", 0, 0, 0, 0, 0, 0, false, true, BUTTON_EXIT_MENU}
};

Button victory_buttons[] = {
    {"В меню", 0, 0, 0, 0, 0, 0, false, true, BUTTON_EXIT_MENU}
};
typedef struct {
    short int startX;
    short int startY;
    char length;
    char orientation;
    short int hits;
} Ship;

typedef struct {
    char name[64];
    short int score;
} Player;

const char* letters[15] = {
    "А", "Б", "В", "Г", "Д",
    "Е", "Ж", "З", "И", "К",
    "Л", "М", "Н", "О", "П"
};

typedef enum {
    WATER,
    SHIP,
    HIT,
    MISS,
    MINE
} CellState;

typedef enum {
    EASY,
    NORMAL,
    HARD
} Difficult;

typedef union {
    struct {
        char comp;
        char user;
    };
    short int both;
} Comp_or_User;

typedef enum {
    MENU,
    PLACING_SHIPS,
    PLAYING,
    PLACING_RADAR,
    PLACING_MINES
} GameMode;

typedef enum {
    MAIN_MENU,
    SETTINGS_MENU,
    LOAD_MENU,
    SAVE_MENU,
    LEADERBOARD_MENU,
    VICTORY_MENU
} MenuState;

typedef struct {
    char            date[64];
    char             radars_count;
    char             grid_size;
    Difficult       difficult;

    CellState** player_grid;
    CellState** enemy_grid;
} SaveData;

typedef struct {
    Button saves_buttons[MAX_SAVES];
    GameMode current_mode;
    MenuState menu_state;
    Comp_or_User current_user;
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_TextEngine* text_engine;
    TTF_Font* font;
    CellState** player_grid;
    CellState** enemy_grid;
    char ships_left_1;
    char player_turn;
    char turns_count;
    char is_hit;
    char fog_turns[2];
    char current_fog;
    char total_turns;
    char fog_active;
    char fog_turns_left;
    char fog_turns_in_progress;
    char storm_turns[2];
    char current_storm;
    char storm_active;
    float storm_time_elapsed;
    char computer_mines_left;
    char computer_radars_left;
    Player leaderboard[MAX_PLAYERS];
    char computer_win;
    char mines_left;
    char radars_left;
    char radar_running;
    float time_elapsed;
    char radar_x;
    char radar_y;
    Computer computer;
    Ship player_ships[MAX_SHIPS];
    Ship enemy_ships[MAX_SHIPS];
    char player_ships_count;
    char enemy_ships_count;
    int grid_size;
    Difficult current_difficult;
    SaveData saves[MAX_SAVES];
    int screen_width;
    int screen_height;
    int player_x_offset;
    int player_y_offset;
    int enemy_x_offset;
    int enemy_y_offset;
    char computer2_mines_left;
    int cell_size;
    int menu_button_width;
    int menu_button_height;

} GameState;

char isSpaceAvailable(GameState* game, CellState** grid, char grid_size, short int x, short int y, char length, char orientation);
char initSDL(GameState* game);
void closeSDL(GameState* game);

void loadSaves(GameState* game);
void loadGame(GameState* game, char saveIndex);
void saveGame(GameState* game, char saveIndex);
SaveData parseSaveFile(GameState* game, const char* save_file);

void initBoard(GameState* game, CellState*** grid, int size);
CellState** allocBoard(int size);
void freeBoard(CellState** grid, int size);

void initMenu(GameState* game);
void initGame(GameState* game);
void startGame(GameState* game);

void renderMenu(GameState* game);
void renderGame(GameState* game, float delta_time);

void checkButtonsClick(GameState* game, short int mouseX, short int mouseY, Button* buttons, int size);
void handleButtonClick(GameState* game, Button* button);
char* handleTextInput(GameState* game);

void placeRadar(GameState* game, SDL_Event event, short int x_offset, short int y_offset, CellState** grid, char grid_size, float delta_time);
void placeMine(GameState* game, SDL_Event event, short int x_offset, short int y_offset, CellState** grid, char grid_size);

void drawBackground(GameState* game);
void drawRadar(GameState* game, float delta_time);
void drawGrid(GameState* game, short int x_offset, short int y_offset);
void drawButton(GameState* game, Button button);
void drawCell(GameState* game, short int x, short int y, CellState state, short int x_offset, short int y_offset);
void drawBoard(GameState* game, CellState** grid, char grid_size, short int x_offset, short int y_offset);
void drawText(GameState* game, short int x_offset, short int y_offset, const char* text, int font_size);

char handleInput(GameState* game, SDL_Event event, short int x_offset, short int y_offset, CellState** grid, char grid_size);
char handleShipPlacement(GameState* game, SDL_Event event, short int x_offset, short int y_offset, CellState** grid, char grid_size, char ch, short int turn_ship);
char shootCell(GameState* game, CellState** shooter, CellState** target, short int x, short int y, char grid_size);

void addPlayerToLeaderboard(GameState* game, const char* playerName, short int score);
void initLeaderBoard(GameState* game);
void saveLeaderBoard(GameState* game);

void shuffle_for_xy(GameState* game, short int arr[][2], int size);
char computerTurn(GameState* game, CellState** grid, char grid_size, char user, HitCell** hitListHead, CellState** grid_for_comp);
void activateFog(GameState* game);
void checkFog(GameState* game);
void generateFogTurns(GameState* game);
char isShipSunk(GameState* game, CellState** grid, int grid_size, Ship* ship);

void generateStormTurns(GameState* game);
void activateStorm(GameState* game);
void checkStorm(GameState* game, float delta_time);
void drawStormEffect(GameState* game);
char shipsRemaining(GameState* game, CellState** grid, char grid_size);
char placeShipsRandomly(GameState* game, CellState** grid, char grid_size, Ship* ships);

char* win1251_to_utf8(const char* str) {
    int size = MultiByteToWideChar(1251, 0, str, -1, NULL, 0);
    wchar_t* wstr = (wchar_t*)malloc(size * sizeof(wchar_t));
    MultiByteToWideChar(1251, 0, str, -1, wstr, size);

    size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* utf8_str = (char*)malloc(size);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, utf8_str, size, NULL, NULL);

    free(wstr);
    return utf8_str;
}

int availableCells(GameState* game, CellState** grid, char grid_size) {
    int count = 0;
    for (int y = 0; y < grid_size; y++) {
        for (int x = 0; x < grid_size; x++) {
            if (grid[y][x] != HIT && grid[y][x] != MISS) {
                count++;
            }
        }
    }
    return count;
}

int main() {
    setlocale(LC_ALL, "Rus");
    GameState game = { 0 };
    game.current_mode = MENU;
    game.menu_state = MAIN_MENU;
    game.current_user.user = 1;
    game.ships_left_1 = 4;
    game.player_turn = 1;
    game.turns_count = 0;
    game.is_hit = -1;
    game.computer_mines_left = 2;
    game.computer2_mines_left = 2;
    game.computer_radars_left = 2;
    game.mines_left = 2;
    game.radars_left = 2;
    game.grid_size = 10;
    game.computer.chanceToHit = 1;
    game.current_difficult = EASY;

    if (initSDL(&game)) {
        return 1;
    }

    generateFogTurns(&game);
    generateStormTurns(&game);
    srand(time(NULL));
    loadSaves(&game);
    initLeaderBoard(&game);
    initMenu(&game);
    HitCell* computerHitListHead = NULL;
    Uint64 last_time = 0;
    char quit = 0;
    char ch = 0;
    short int turn_ship = 0;
    bool game_over = false;
    bool win = false;
    char end_random = 1;
    SDL_Event event;

    while (!quit) {
        Uint64 current_time = SDL_GetTicks();
        float delta_time = (current_time - last_time) / 1000.0f;
        last_time = current_time;
        checkStorm(&game, delta_time);

        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_EVENT_QUIT) {
                quit = 1;
            }
            else if (event.type == SDL_EVENT_WINDOW_RESIZED) {
                // Обновляем размеры экрана
                game.screen_width = event.window.data1;
                game.screen_height = event.window.data2;
                // Пересчитываем размеры и позиции
                initSDL(&game); // Повторно инициализируем размеры
                initMenu(&game); // Обновляем позиции кнопок меню
                initGame(&game); // Обновляем позиции кнопок игры
            }
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                short int mouseX = event.button.x;
                short int mouseY = event.button.y;

                if (game.current_mode == MENU) {
                    if (game.menu_state == MAIN_MENU) {
                        checkButtonsClick(&game, mouseX, mouseY, main_menu_buttons, MAIN_BUTTONS_COUNT);
                    }
                    else if (game.menu_state == SETTINGS_MENU) {
                        checkButtonsClick(&game, mouseX, mouseY, settings_menu_buttons, SETTINGS_BUTTONS_COUNT);
                    }
                    else if (game.menu_state == LOAD_MENU || game.menu_state == SAVE_MENU) {
                        checkButtonsClick(&game, mouseX, mouseY, game.saves_buttons, MAX_SAVES);
                        checkButtonsClick(&game, mouseX, mouseY, leaderboard_buttons, 1);
                    }
                    else if (game.menu_state == VICTORY_MENU) {
                        if (game.current_user.comp != 1) {
                            static bool name_entered = false;
                            char* player_name;

                            if (!name_entered) {
                                player_name = handleTextInput(&game);
                                while (player_name == NULL) {
                                    player_name = handleTextInput(&game);
                                }
                                addPlayerToLeaderboard(&game, player_name, game.turns_count);
                                free(player_name);
                                name_entered = true;
                            }
                        }
                        checkButtonsClick(&game, mouseX, mouseY, victory_buttons, 1);
                    }
                    else if (game.menu_state == LEADERBOARD_MENU) {
                        checkButtonsClick(&game, mouseX, mouseY, leaderboard_buttons, 1);
                    }
                }
                else if (game.current_mode == PLACING_SHIPS) {

                    switch (game.current_difficult) {
                    case EASY:
                        game.computer.chanceToHit = 1;
                        break;
                    case NORMAL:
                        game.computer.chanceToHit = 2;
                        break;
                    case HARD:
                        game.computer.chanceToHit = 3;
                        break;
                    }
                    if (game.current_user.comp != 1) {

                        if (end_random) {

                            checkButtonsClick(&game, mouseX, mouseY, game_buttons, 4);

                        }

                        if (mouseX >= game.player_x_offset && mouseX < game.player_x_offset + game.grid_size * game.cell_size &&
                            mouseY >= game.player_y_offset && mouseY < game.player_y_offset + game.grid_size * game.cell_size) {
                            end_random = 0;
                            if (handleShipPlacement(&game, event, game.player_x_offset, game.player_y_offset, game.player_grid, game.grid_size, !ch, ++turn_ship)) {
                            }
                        }
                    }
                    else {
                        game.player_ships_count = placeShipsRandomly(&game, game.player_grid, game.grid_size, game.player_ships);
                    }
                    if ((!end_random && game.player_ships_count >= 20 && game.grid_size == 10) || (end_random && game.player_ships_count >= 10 && game.grid_size == 10) || (game.player_ships_count >= 13 && !end_random && game.grid_size == 15) || (game.player_ships_count >= 10 && end_random && game.grid_size == 15)) {
                        game.enemy_ships_count = placeShipsRandomly(&game, game.enemy_grid, game.grid_size, game.enemy_ships);
                        if (game.current_user.comp == 1) {
                            startGame(&game);
                        }
                        if ((game.enemy_ships_count >= 10 && game.current_user.comp != 1 && game.grid_size == 10) || (game.enemy_ships_count >= 11 && game.current_user.comp != 1 && game.grid_size == 15)) {
                            game.current_mode = PLACING_MINES;
                        }
                    }
                }
                else if (game.current_mode == PLACING_MINES && game.current_user.comp != 1) {
                    placeMine(&game, event, game.player_x_offset, game.player_y_offset, game.player_grid, game.grid_size);
                    if (game.mines_left == 0) {
                        startGame(&game);
                    }
                }
                else if (game.current_mode == PLACING_RADAR && !game.radar_running && game.current_user.comp != 1) {
                    placeRadar(&game, event, game.enemy_x_offset, game.enemy_y_offset, game.enemy_grid, game.grid_size, delta_time);
                }
                else if (game.current_mode == PLAYING && game.player_turn && game.current_user.comp != 1) {
                    checkButtonsClick(&game, mouseX, mouseY, game_buttons, 4);
                    if (game.player_turn == 1 && !handleInput(&game, event, game.enemy_x_offset, game.enemy_y_offset, game.enemy_grid, game.grid_size)) {
                        game.player_turn = 0;
                        continue;
                    }
                }
                else if (game.current_user.comp == 1) {
                    checkButtonsClick(&game, mouseX, mouseY, game_buttons, 4);
                }
            }
        }
        char res;
        if (game.current_user.comp == 1) {
            if (game.current_mode == PLAYING && game.player_turn) {
                res = computerTurn(&game, game.enemy_grid, game.grid_size, 1, &computerHitListHead, game.player_grid);
                if (res != 1) {
                    game.player_turn = 0;
                }
            }
            SDL_Delay(160);
            if (game.current_mode == PLAYING && !game.player_turn) {
                res = computerTurn(&game, game.player_grid, game.grid_size, 0, &computerHitListHead, game.enemy_grid);
                if (res != 1) {
                    game.player_turn = 1;
                }
            }
        }
        else {
            if (game.current_mode == PLAYING && !game.player_turn) {
                res = computerTurn(&game, game.player_grid, game.grid_size, 1, &computerHitListHead, game.player_grid);
                if (res != 1) {
                    game.player_turn = 1;
                }
            }
        }
        if (game.current_mode == PLAYING) {
            if (!shipsRemaining(&game, game.enemy_grid, game.grid_size)) {
                game_over = true;
                win = true;
                game.menu_state = VICTORY_MENU;
                game.current_mode = MENU;
            }
            else if (!shipsRemaining(&game, game.player_grid, game.grid_size)) {
                game_over = true;
                win = true;
                game.computer_win = 1;
                game.menu_state = VICTORY_MENU;
                game.current_mode = MENU;
            }
        }
        checkFog(&game);
        SDL_SetRenderDrawColor(game.renderer, 255, 255, 255, 255);
        SDL_RenderClear(game.renderer);
        if (game.current_mode == MENU) {
            renderMenu(&game);
        }
        else if (
            game.current_mode == PLACING_SHIPS ||
            game.current_mode == PLAYING ||
            game.current_mode == PLACING_RADAR ||
            game.current_mode == PLACING_MINES) {

            renderGame(&game, delta_time);
        }
        SDL_RenderPresent(game.renderer);
        SDL_Delay(160);
    }
    if (game_over) {
        if (win) {
            game.menu_state = VICTORY_MENU;
        }
    }
    saveLeaderBoard(&game);
    closeSDL(&game);

    return 0;
}

char initSDL(GameState* game) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        SDL_Log("Ошибка при инициализации SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_DisplayID display = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(display);
    if (!mode) {
        SDL_Log("Не удалось получить текущий режим дисплея: %s\n", SDL_GetError());
        return 1;
    }

    game->screen_width = mode->w - 100;
    game->screen_height = mode->h - 100;
    int grid_count = 2;
    int cell_padding = 2;

    float usable_width_ratio = 0.6f;
    float usable_height_ratio = 0.4f;

    int usable_width = game->screen_width * usable_width_ratio;
    int usable_height = game->screen_height * usable_height_ratio;

    int max_cell_width = usable_width / (game->grid_size * 2 + 1);
    int max_cell_height = usable_height / game->grid_size;

    int spacing = (int)(game->screen_width * 0.15f);

    game->menu_button_width = (int)(game->screen_width * MENU_BUTTON_WIDTH_RATIO);
    game->menu_button_height = (int)(game->screen_height * MENU_BUTTON_HEIGHT_RATIO);

    float scale_factor = 0.8f;

    game->cell_size = ((max_cell_width < max_cell_height) ? max_cell_width : max_cell_height) * scale_factor;

    int total_grids_width = game->grid_size * game->cell_size * 2 + spacing;
    game->player_x_offset = (int)((game->screen_width - total_grids_width) * 0.75f);

    game->enemy_x_offset = game->player_x_offset + game->grid_size * game->cell_size + spacing;
    game->player_y_offset = (game->screen_height - game->cell_size * game->grid_size) / 2;
    game->enemy_y_offset = game->player_y_offset;

    if (game->cell_size < 20) {
        game->cell_size = 20;
    }
    game->window = SDL_CreateWindow("Sea battle SDL3", game->screen_width, game->screen_height, 0);
    if (game->window == NULL) {
        SDL_Log("Ошибка при создании окна SDL: %s\n", SDL_GetError());
        return 1;
    }
    game->renderer = SDL_CreateRenderer(game->window, NULL);
    if (game->renderer == NULL) {
        SDL_Log("Ошибка при создании рендерера SDL: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() < 0) {
        SDL_Log("Ошибка при инициализации TTF: %s\n", SDL_GetError());
        return 1;
    }
    game->text_engine = TTF_CreateRendererTextEngine(game->renderer);
    if (game->text_engine == NULL) {
        SDL_Log("Ошибка при создании TTF_RendererTextEngine: %s\n", SDL_GetError());
        return 1;
    }
    game->font = TTF_OpenFont("font.ttf", FONT_SIZE);
    if (game->font == NULL) {
        SDL_Log("Не удалось загрузить шрифт: %s\n", SDL_GetError());
        return 1;
    }
    SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 255);
    return 0;

    int base_font_size = (int)(game->screen_height * 0.04f);
    game->font = TTF_OpenFont("font.ttf", base_font_size);
}

void closeSDL(GameState* game) {
    if (game->renderer) {
        SDL_DestroyRenderer(game->renderer);
        game->renderer = NULL;
    }
    if (game->window) {
        SDL_DestroyWindow(game->window);
        game->window = NULL;
    }

    SDL_Quit();
}

void saveGame(GameState* game, char saveIndex) {
    char saveFile[64];
    snprintf(saveFile, sizeof(saveFile), "save_%d.txt", saveIndex);
    time_t rawtime;
    struct tm* info;
    char buffer[80];
    time(&rawtime);
    info = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%c - %I:%M%p - %H:%M", info);
    FILE* save_file = fopen(saveFile, "w");
    if (!save_file) {
        printf("Ошибка сохранения.\n");
        return;
    }

    fprintf(save_file,
        "# ДАТА\n%s\n"
        "# РАЗМЕР_СЕТКИ\n%c\n"
        "# ОСТАЛОСЬ_РАДАРОВ\n%c\n"
        "# СЛОЖНОСТЬ\n%c\n",
        buffer,
        game->grid_size,
        game->radars_left,
        game->current_difficult
    );

    fprintf(save_file, "# СЕТКА_ИГРОКА\n");
    for (int y = 0; y < game->grid_size; ++y) {
        for (int x = 0; x < game->grid_size; ++x) {
            fprintf(save_file, "%d ", game->player_grid[y][x]);
        }
        fprintf(save_file, "\n");
    }
    fprintf(save_file, "# СЕТКА_ПРОТИВНИКА\n");
    for (int y = 0; y < game->grid_size; ++y) {
        for (int x = 0; x < game->grid_size; ++x) {
            fprintf(save_file, "%d ", game->enemy_grid[y][x]);
        }
        fprintf(save_file, "\n");
    }

    fclose(save_file);
    printf("Игра успешно сохранена.\n");
}
void loadSaves(GameState* game) {
    char saveFile[64];

    for (int i = 0; i < MAX_SAVES; ++i) {
        snprintf(saveFile, sizeof(saveFile), "save_%d.txt", i);
        game->saves[i] = parseSaveFile(game, saveFile);
    }
}

void loadGame(GameState* game, char saveIndex) {
    char saveFile[64];
    snprintf(saveFile, sizeof(saveFile), "save_%hd.txt", saveIndex);
    SaveData save = parseSaveFile(game, saveFile);

    game->grid_size = save.grid_size;
    game->radars_left = save.radars_count;
    game->player_grid = save.player_grid;
    game->enemy_grid = save.enemy_grid;

    game->player_ships_count = 10;
    game->enemy_ships_count = 10;

    initGame(game);
    startGame(game);
}

SaveData parseSaveFile(GameState* game, const char* save_file) {
    SaveData save = { 0 };

    FILE* file = fopen(save_file, "r");
    if (!file) {
        printf("Не удалось открыть файл сохранения.\n");
        return save;
    }

    char line[256];
    int size = -1;
    char reading_player = 0;
    char reading_enemy = 0;
    short int row = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "# ДАТА")) {
            if (fgets(line, sizeof(line), file)) {
                sscanf(line, "%s", &save.date);
            }
            continue;
        }
        if (strstr(line, "# РАЗМЕР_СЕТКИ")) {
            if (fgets(line, sizeof(line), file)) {
                sscanf(line, "%c", &save.grid_size);
            }
            continue;
        }
        if (strstr(line, "# ОСТАЛОСЬ_РАДАРОВ")) {
            if (fgets(line, sizeof(line), file)) {
                sscanf(line, "%c", &save.radars_count);
            }
            continue;
        }
        if (strstr(line, "# СЛОЖНОСТЬ")) {
            if (fgets(line, sizeof(line), file)) {
                sscanf(line, "%c", &save.difficult);
            }
            continue;
        }
    }
    save.player_grid = allocBoard(save.grid_size);
    save.enemy_grid = allocBoard(save.grid_size);

    initBoard(game, &save.player_grid, save.grid_size);
    initBoard(game, &save.enemy_grid, save.grid_size);
    rewind(file);

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "# СЕТКА_ИГРОКА")) {
            reading_player = 1;
            reading_enemy = 0;
            row = 0;
            continue;
        }

        if (strstr(line, "# СЕТКА_ПРОТИВНИКА")) {
            reading_player = 0;
            reading_enemy = 1;
            row = 0;
            continue;
        }

        if (reading_player && row < save.grid_size) {
            sscanf(line,
                "%hd %hd %hd %hd %hd %hd %hd %hd %hd %hd",
                &save.player_grid[row][0], &save.player_grid[row][1], &save.player_grid[row][2], &save.player_grid[row][3], &save.player_grid[row][4],
                &save.player_grid[row][5], &save.player_grid[row][6], &save.player_grid[row][7], &save.player_grid[row][8], &save.player_grid[row][9]
            );
            row++;
        }

        if (reading_enemy && row < save.grid_size) {
            sscanf(line,
                "%hd %hd %hd %hd %hd %hd %hd %hd %hd %hd",
                &save.enemy_grid[row][0], &save.enemy_grid[row][1], &save.enemy_grid[row][2], &save.enemy_grid[row][3], &save.enemy_grid[row][4],
                &save.enemy_grid[row][5], &save.enemy_grid[row][6], &save.enemy_grid[row][7], &save.enemy_grid[row][8], &save.enemy_grid[row][9]
            );
            row++;
        }
    }

    fclose(file);
    return save;
}

void initMenu(GameState* game) {
    // Процентные отступы и размеры
    float button_width_ratio = MENU_BUTTON_WIDTH_RATIO; // 40% ширины экрана
    float button_height_ratio = MENU_BUTTON_HEIGHT_RATIO; // 5% высоты экрана
    float start_y_ratio = 0.2f; // Начальная позиция по Y (20% от верха)
    float spacing_ratio = 0.07f; // Отступ между кнопками (7% высоты экрана)

    game->menu_button_width = (int)(game->screen_width * button_width_ratio);
    game->menu_button_height = (int)(game->screen_height * button_height_ratio);

    // Главное меню
    for (int i = 0; i < MAIN_BUTTONS_COUNT; ++i) {
        main_menu_buttons[i].x_pos = game->screen_width / 2; // Центр по X
        main_menu_buttons[i].y_pos = (int)(game->screen_height * (start_y_ratio + i * spacing_ratio));

        main_menu_buttons[i].left = main_menu_buttons[i].x_pos - game->menu_button_width / 2;
        main_menu_buttons[i].right = main_menu_buttons[i].x_pos + game->menu_button_width / 2;
        main_menu_buttons[i].top = main_menu_buttons[i].y_pos - game->menu_button_height / 2;
        main_menu_buttons[i].bottom = main_menu_buttons[i].y_pos + game->menu_button_height / 2;
    }

    // Меню настроек
    for (int i = 0; i < SETTINGS_BUTTONS_COUNT; ++i) {
        settings_menu_buttons[i].x_pos = game->screen_width / 2;
        settings_menu_buttons[i].y_pos = (int)(game->screen_height * (start_y_ratio + i * spacing_ratio));

        settings_menu_buttons[i].left = settings_menu_buttons[i].x_pos - game->menu_button_width / 2;
        settings_menu_buttons[i].right = settings_menu_buttons[i].x_pos + game->menu_button_width / 2;
        settings_menu_buttons[i].top = settings_menu_buttons[i].y_pos - game->menu_button_height / 2;
        settings_menu_buttons[i].bottom = settings_menu_buttons[i].y_pos + game->menu_button_height / 2;
    }
    settings_menu_buttons[2].is_pressed = true;

    // Меню сохранений
    for (int i = 0; i < MAX_SAVES; ++i) {
        game->saves_buttons[i].x_pos = game->screen_width / 2;
        game->saves_buttons[i].y_pos = (int)(game->screen_height * (start_y_ratio + i * spacing_ratio));
        game->saves_buttons[i].left = game->saves_buttons[i].x_pos - game->menu_button_width / 2;
        game->saves_buttons[i].right = game->saves_buttons[i].x_pos + game->menu_button_width / 2;
        game->saves_buttons[i].top = game->saves_buttons[i].y_pos - game->menu_button_height / 2;
        game->saves_buttons[i].bottom = game->saves_buttons[i].y_pos + game->menu_button_height / 2;
        game->saves_buttons[i].type = BUTTON_SAVE_SLOT;
        game->saves_buttons[i].is_pressed = false;
        game->saves_buttons[i].is_active = true;
        game->saves_buttons[i].label = (game->saves[i].date && game->saves[i].date[0] != '\0') ? game->saves[i].date : "Пусто";
    }

    // Кнопка "В меню" для таблицы лидеров
    leaderboard_buttons[0].x_pos = game->screen_width / 2;
    leaderboard_buttons[0].y_pos = (int)(game->screen_height * 0.9f); // 90% от верха
    leaderboard_buttons[0].left = leaderboard_buttons[0].x_pos - game->menu_button_width / 2;
    leaderboard_buttons[0].right = leaderboard_buttons[0].x_pos + game->menu_button_width / 2;
    leaderboard_buttons[0].top = leaderboard_buttons[0].y_pos - game->menu_button_height / 2;
    leaderboard_buttons[0].bottom = leaderboard_buttons[0].y_pos + game->menu_button_height / 2;
    leaderboard_buttons[0].is_pressed = false;
    leaderboard_buttons[0].is_active = true;

    // Кнопка "В меню" для экрана победы
    victory_buttons[0].x_pos = game->screen_width / 2;
    victory_buttons[0].y_pos = (int)(game->screen_height * 0.7f); // 70% от верха
    victory_buttons[0].left = victory_buttons[0].x_pos - game->menu_button_width / 2;
    victory_buttons[0].right = victory_buttons[0].x_pos + game->menu_button_width / 2;
    victory_buttons[0].top = victory_buttons[0].y_pos - game->menu_button_height / 2;
    victory_buttons[0].bottom = victory_buttons[0].y_pos + game->menu_button_height / 2;
    victory_buttons[0].is_pressed = false;
    victory_buttons[0].is_active = true;
}

void initBoard(GameState* game, CellState*** grid, int size) {
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            (*grid)[i][j] = WATER;
        }
    }
}

char isSpaceAvailable(GameState* game, CellState** grid, char grid_size, short int x, short int y, char length, char orientation) {
    if (orientation == 0) {
        if (x + length > grid_size) return 0;
        for (int i = 0; i < length; i++) {
            if (grid[y][x + i] != WATER) return 0;
        }
        for (int i = 0; i < length; i++)
        {
            if (y > 0 && grid[y - 1][x + i] != WATER)
                return 0;
            if (y < grid_size - 1 && grid[y + 1][x + i] != WATER)
                return 0;
            if (x + i > 0 && grid[y][x + i - 1] != WATER)
                return 0;
            if (x + i < grid_size - 1 && grid[y][x + i + 1] != WATER)
                return 0;
        }
        if (y > 0 && x > 0 && grid[y - 1][x - 1] != WATER)
            return 0;
        if (y > 0 && x + length < grid_size && grid[y - 1][x + length] != WATER)
            return 0;
        if (y < grid_size - 1 && x > 0 && grid[y + 1][x - 1] != WATER)
            return 0;
        if (y < grid_size - 1 && x + length < grid_size && grid[y + 1][x + length] != WATER)
            return 0;
    }
    else {
        if (y + length > grid_size) return 0;
        for (int i = 0; i < length; i++) {
            if (grid[y + i][x] != WATER) return 0;
        }

        for (int i = 0; i < length; i++)
        {
            if (x > 0 && grid[y + i][x - 1] != WATER)
                return 0;
            if (x < grid_size - 1 && grid[y + i][x + 1] != WATER)
                return 0;
            if (y + i > 0 && grid[y + i - 1][x] != WATER)
                return 0;
            if (y + i < grid_size - 1 && grid[y + i + 1][x] != WATER)
                return 0;
        }
        if (x > 0 && y > 0 && grid[y - 1][x - 1] != WATER)
            return 0;
        if (x < grid_size - 1 && y > 0 && grid[y - 1][x + 1] != WATER)
            return 0;
        if (x > 0 && y + length < grid_size && grid[y + length][x - 1] != WATER)
            return 0;
        if (x < grid_size - 1 && y + length < grid_size && grid[y + length][x + 1] != WATER)
            return 0;
    }
    return 1;
}

void placeShip(GameState* game, CellState** grid, short int startX, short int startY, char length, char orientation) {
    if (orientation == 0) {
        for (int i = 0; i < length; i++) {
            grid[startY][startX + i] = SHIP;
        }
    }
    else {
        for (int i = 0; i < length; i++) {
            grid[startY + i][startX] = SHIP;
        }
    }
}

char tryShipOnEdge(GameState* game, CellState** grid, char grid_size, char length, char orientation, short int* x, short int* y) {
    short int attempts = 0;
    while (attempts < 1000) {
        attempts++;
        char side = 0;
        if (game->computer.chanceToHit == 3) {
            if (orientation == 0) {
                side = rand() % 2;
            }
            else {
                side = 2 + (rand() % 2);
            }
        }
        else if (game->computer.chanceToHit == 2) {
            side = rand() % 4;
        }
        if (game->computer.chanceToHit != 1) {
            switch (side) {
            case 0:
                *x = rand() % grid_size;
                *y = 0;
                break;
            case 1:
                *x = rand() % grid_size;
                *y = grid_size - 1;

                break;
            case 2:
                *x = grid_size - 1;
                *y = rand() % grid_size;
                break;
            case 3:
                *x = 0;
                *y = rand() % grid_size;
                break;
            }
        }
        if (game->computer.chanceToHit == 1) {
            *x = rand() % grid_size;
            *y = rand() % grid_size;
        }

        if (isSpaceAvailable(game, grid, grid_size, *x, *y, length, orientation)) {
            return 1;
        }
    }
    return 0;
}

void getRandomCoordinatesInRegion(GameState* game, char grid_size, short int region_x_start, short int region_y_start, short int region_x_end, short int region_y_end, short int* x, short int* y) {
    *x = region_x_start + (rand() % (region_x_end - region_x_start));
    *y = region_y_start + (rand() % (region_y_end - region_y_start));
}

char placeShipsRandomly(GameState* game, CellState** grid, char grid_size, Ship* ships) {
    char placed_ships = 0;
    char ships_count = 0;
    char ships_left[] = { game->ships_left_1 - 3, game->ships_left_1 - 3,game->ships_left_1 - 2, game->ships_left_1 - 1 };
    char i = 0;
    char lengths[] = { 5, 4, 3, 2 };
    char num_ship_types = sizeof(lengths) / sizeof(lengths[0]);
    if (grid_size == 15) {
        i = 0;
    }
    else {
        i = 1;
    }
    for (; i < num_ship_types; i++) {
        char count_all = 0;
        switch (i) {
        case 0:
            count_all = game->ships_left_1 - 3;
            break;
        case 1:
            count_all = game->ships_left_1 - 3;
            break;
        case 2:
            count_all = game->ships_left_1 - 2;
            break;
        case 3:
            count_all = game->ships_left_1 - 1;
            break;
        }

        while (count_all > 0) {
            char orientation = rand() % 2;

            short int x, y;
            if (tryShipOnEdge(game, grid, grid_size, lengths[i], orientation, &x, &y)) {
                placeShip(game, grid, x, y, lengths[i], orientation);
                ships[ships_count].startX = x;
                ships[ships_count].startY = y;
                ships[ships_count].length = lengths[i];
                ships[ships_count].orientation = orientation;
                ships_count++;
                placed_ships++;
                count_all--;

            }

        }
    }
    placed_ships = 0;
    while (placed_ships < game->ships_left_1) {
        short int x, y;

        if (game->computer.chanceToHit != 1) {
            x = 1 + (rand() % (grid_size - 2));
            y = 1 + (rand() % (grid_size - 2));
        }

        if (game->computer.chanceToHit == 1) {
            x = rand() % grid_size;
            y = rand() % grid_size;
        }
        if (grid[y][x] == WATER && isSpaceAvailable(game, grid, grid_size, x, y, 1, 0)) {
            grid[y][x] = SHIP;
            ships[ships_count].startX = x;
            ships[ships_count].startY = y;
            ships[ships_count].length = 1;
            ships[ships_count].orientation = 0;
            ships_count++;
            placed_ships++;
        }
    }
    return ships_count;
}

void shuffle_for_xy(GameState* game, short int arr[][2], int size) {
    if (size <= 1) return;
    for (int i = size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        short int temp_x = arr[i][0];
        short int temp_y = arr[i][1];
        arr[i][0] = arr[j][0];
        arr[i][1] = arr[j][1];
        arr[j][0] = temp_x;
        arr[j][1] = temp_y;
    }
}

char computerTurn(GameState* game, CellState** grid, char grid_size, char user, HitCell** hitListHead, CellState** grid_for_comp) {
    CellState** temp = grid;
    if (game->computer_radars_left > 0 && (rand() % 100) < 10) {
        if (game->current_user.comp == 1) {
            grid = grid_for_comp;
        }
        short int radar_x = rand() % grid_size;
        short int radar_y = rand() % grid_size;
        for (int blink = 0; blink < 3; ++blink) {
            for (int y = radar_y - 1; y <= radar_y + 1; ++y) {
                for (int x = radar_x - 1; x <= radar_x + 1; ++x) {
                    if (x >= 0 && x < grid_size && y >= 0 && y < grid_size) {
                        SDL_FRect cellRect = {
                            (user ? game->player_x_offset : game->enemy_x_offset) + x * game->cell_size + 1,
                            (user ? game->player_y_offset : game->enemy_y_offset) + y * game->cell_size + 1,
                            game->cell_size - 2,
                            game->cell_size - 2
                        };

                        if (blink % 2 == 0) {
                            if (grid[y][x] == SHIP || grid[y][x] == HIT) {
                                grid[y][x] = HIT;
                            }
                            else if (grid[y][x] == WATER) {
                                SDL_SetRenderDrawColor(game->renderer, 5, 74, 41, 255);
                            }
                            else if (grid[y][x] == MISS) {
                                SDL_SetRenderDrawColor(game->renderer, 238, 198, 67, 255);
                            }
                            else if (grid[y][x] == MINE) {
                                SDL_SetRenderDrawColor(game->renderer, 175, 77, 152, 255);
                            }

                            SDL_RenderFillRect(game->renderer, &cellRect);

                            SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
                            SDL_FRect borderRect = {
                                (user ? game->player_x_offset : game->enemy_x_offset) + x * game->cell_size,
                                (user ? game->player_y_offset : game->enemy_y_offset) + y * game->cell_size,
                                game->cell_size,
                                game->cell_size
                            };
                            SDL_RenderRect(game->renderer, &borderRect);
                        }
                        else {
                            SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
                            SDL_RenderFillRect(game->renderer, &cellRect);
                        }
                    }
                }
            }
            SDL_RenderPresent(game->renderer);
            SDL_Delay(200);
        }

        game->computer_radars_left--;
        grid = temp;
        return 1;
    }
    game->total_turns++;
    if (game->fog_active) {
        game->fog_turns_in_progress++;

        if (game->fog_turns_in_progress == 2) {
            game->fog_active = 0;
            game->fog_turns_in_progress = 0;
        }
    }
    short int available = 0;
    static int i = 0;
    for (int y = 0; y < grid_size; y++) {
        for (int x = 0; x < grid_size; x++) {
            if (grid[y][x] != HIT && grid[y][x] != MISS) {
                available++;
            }
        }
    }
    if (available == 0) {
        return 0;
    }
    short int x, y;
    char hit_found = 0;
    if (*hitListHead != NULL && game->computer.chanceToHit != 1) {
        HitCell* currentHit = *hitListHead;
        x = currentHit->x;
        y = currentHit->y;
        short int offsets[4][2] = {
            {1, 0},
            {-1, 0},
            {0, 1},
             {0, -1}
        };

        for (int i = 0; i < 4; i++) {
            short int next_x = x + offsets[i][0];
            short int next_y = y + offsets[i][1];

            if (next_x >= 0 && next_x < grid_size && next_y >= 0 && next_y < grid_size) {
                if (grid[next_y][next_x] != HIT && grid[next_y][next_x] != MISS) {
                    x = next_x;
                    y = next_y;
                    goto try_shot;
                }
            }
        }
        clearHitList(hitListHead);
    }

    short int attempts = 0;
    const short int max_attempts = 1000;

    do {
        short int for_xy_10_grid_4[8][2] =
        {
            {0,3},
            {3,0},
            {0,7},
            {7,0},
            {2,9},
            {9,2},
            {6,9},
            {9,6}
        };
        short int for_xy_10_4[16][2] =
        {
            {1,2},
            {2,1},
            {1,6},
            {2,5},
            {3,4},
            {4,3},
            {5,2},
            {6,1},
            {3,8},
            {4,7},
            {5,6},
            {6,5},
            {7,4},
            {8,3},
            {7,8},
            {8,7}
        };

        short int for_xy_10_grid_2_3[10][2] = {
            {0,1},
            {1,0},
            {0,5},
            {5,0},
            {0,9},
            {9,0},
            {4,9},
            {9,4},
            {8,9},
            {9,8}
        };
        short int for_xy_10_2_3[16][2] = {
            {1,4},
            {2,3},
            {3,2},
            {4,1},
            {1,8},
            {2,7},
            {3,6},
            {4,5},
            {5,4},
            {6, 3},
            {7,2},
            {8,1},
            {5,8},
            {6,7},
            {7,6},
            {8,5},

        };

        short int for_xy_15_grid_4[10][2] = {
            {0, 4},
            {4,0},
            {0,9},
            {9,0},
            {0,14},
            {14,0},
            {5,14},
            {14,5},
            {10,14},
            {14,10},
        };
        short int for_xy_15_4[35][2] =
        {
            {1,3},
            {2,2},
            {3,1},
            {1,8},
            {2,7},
            {3,6},
            {4,5},
            {5,4},
            {6,3},
            {7,2},
            {8,1},
            {1,13},
            {2,12},
            {3,11},
            {4,10},
            {5,9},
            {6,8},
            {7,7},
            {8,6},
            {9,5},
            {10,4},
            {11,3},
            {12,2},
            {13,1},
            {6, 13},
            {7,12},
            {8,11},
            {9,10},
            {10,9},
            {11,8},
            {12,7},
            {13,6},
            {11,13},
            {12,12},
            {13,11},
        };
        short int for_xy_15_grid_3[14][2] = {
            {0,2},
            {2,0},
            {0,6},
            {6,0},
            {0,11},
            {11,0},
            {2,14},
            {14,2},
            {7,14},
            {14,7},
            {11,14},
            {14,11},
            {13,14},
            {14,13}
        };
        short int for_xy_15_3[35][2] = {
            {1,1},
            {1,5},
            {2,4},
            {3,3},
            {4,2},
            {5,1},
            {1,10},
            {2,9},
            {3,8},
            {4,7},
            {5,6},
            {6,5},
            {7,4},
            {8,3},
            {9,2},
            {10,1},
            {3,13},
            {4,12},
            {5,11},
            {6,10},
            {7,9},
            {8,8},
            {9,7},
            {10,6},
            {11,5},
            {12,4},
            {13,3},
            {8,13},
            {9,12},
            {10,11},
            {11,10},
            {12,9},
            {13,8},
            {12,13},
            {13,12},
        };
        short int for_xy_15_grid_2[9][2] = {
            {0,0},
            {0,7},
            {7,0},
            {0,12},
            {12,0},
            {3,14},
            {14,3},
            {8,14},
            {14,8}

        };
        short int for_xy_15_2[32][2] = {
            {1,6},
            {2,5},
            {3,4},
            {4,3},
            {5,2},
            {6,1},
             {1,11},
            {2,10},
            {3,9},
            {4,8},
            {5,7},
            {6,6},
            {7,5},
            {8,4},
            {9,3},
            {10,2},
            {11,1},
            {4,13},
            {5,12},
            {6,11},
            {7,10},
            {8,9},
            {9,8},
            {10,7},
            {11,6},
            {12,5},
            {13,4},
             {9,13},
            {10,12},
            {11,11},
            {12,10},
            {13,9}
        };
        if (game->current_user.comp != 1 || user == 1) {
            if (grid_size == 15) {
                shuffle_for_xy(game, for_xy_15_grid_2, 9);
                shuffle_for_xy(game, for_xy_15_2, 32);
                shuffle_for_xy(game, for_xy_15_grid_3, 14);
                shuffle_for_xy(game, for_xy_15_3, 35);
                shuffle_for_xy(game, for_xy_15_grid_4, 10);
                shuffle_for_xy(game, for_xy_15_4, 35);
            }
            else {
                shuffle_for_xy(game, for_xy_10_grid_4, 8);
                shuffle_for_xy(game, for_xy_10_4, 16);
                shuffle_for_xy(game, for_xy_10_grid_2_3, 10);
                shuffle_for_xy(game, for_xy_10_2_3, 16);
            }
        }
        else {
            if (grid_size == 15) {
                shuffle_for_xy(game, for_xy_15_grid_2, 9);
                shuffle_for_xy(game, for_xy_15_2, 32);
                shuffle_for_xy(game, for_xy_15_grid_3, 14);
                shuffle_for_xy(game, for_xy_15_3, 35);
                shuffle_for_xy(game, for_xy_15_grid_4, 10);
                shuffle_for_xy(game, for_xy_15_4, 35);
            }
            else {
                shuffle_for_xy(game, for_xy_10_grid_4, 8);
                shuffle_for_xy(game, for_xy_10_4, 16);
                shuffle_for_xy(game, for_xy_10_grid_2_3, 10);
                shuffle_for_xy(game, for_xy_10_2_3, 16);
            }
        }

        if (game->computer.chanceToHit == 3) {

            if (grid_size == 10) {
                if (i < 8) {
                    x = for_xy_10_grid_4[i][0];
                    y = for_xy_10_grid_4[i][1];
                }
                else if (i < 24) {
                    x = for_xy_10_4[i - 8][0];
                    y = for_xy_10_4[i - 8][1];
                }
                else if (i < 34) {
                    x = for_xy_10_grid_2_3[i - 24][0];
                    y = for_xy_10_grid_2_3[i - 24][1];
                }
                else if (i < 50) {
                    x = for_xy_10_2_3[i - 34][0];
                    y = for_xy_10_2_3[i - 34][1];
                }
                else {
                    x = rand() % grid_size;
                    y = rand() % grid_size;
                }
            }
            else {
                if (i < 10) {
                    x = for_xy_15_grid_4[i][0];
                    y = for_xy_15_grid_4[i][1];
                }
                else if (i < 45) {
                    x = for_xy_15_4[i - 10][0];
                    y = for_xy_15_4[i - 10][1];
                }
                else if (i < 59) {
                    x = for_xy_15_grid_3[i - 45][0];
                    y = for_xy_15_grid_3[i - 45][1];
                }
                else if (i < 94) {
                    x = for_xy_15_3[i - 59][0];
                    y = for_xy_15_3[i - 59][1];
                }
                else if (i < 103) {
                    x = for_xy_15_grid_2[i - 94][0];
                    y = for_xy_15_grid_2[i - 94][1];
                }
                else if (i < 135) {
                    x = for_xy_15_2[i - 103][0];
                    y = for_xy_15_2[i - 103][1];
                }
                else {
                    x = rand() % grid_size;
                    y = rand() % grid_size;
                }
            }

            i++;
        }
        else {
            x = rand() % grid_size;
            y = rand() % grid_size;
        }
        attempts++;
        if (attempts > max_attempts) {
            for (int i = 0; i < grid_size; i++) {
                for (int j = 0; j < grid_size; j++) {
                    if (grid[i][j] != HIT && grid[i][j] != MISS) {
                        x = j;
                        y = i;
                        goto try_shot;
                    }
                }
            }
        }
    } while (grid[y][x] == HIT || grid[y][x] == MISS);

try_shot:

    if (grid[y][x] == SHIP) {
        grid[y][x] = HIT;
        if (game->computer.chanceToHit != 1 && (user == 0 || game->current_user.comp != 1)) {
            addHitCell(hitListHead, x, y);
            for (int i = 0; i < MAX_SHIPS; i++) {
                if ((x >= game->player_ships[i].startX && x < game->player_ships[i].startX + game->player_ships[i].length &&
                    game->player_ships[i].orientation == 0 && y == game->player_ships[i].startY) ||
                    (y >= game->player_ships[i].startY && y < game->player_ships[i].startY + game->player_ships[i].length &&
                        game->player_ships[i].orientation == 1 && x == game->player_ships[i].startX)) {

                    game->player_ships[i].hits++;
                    if (isShipSunk(game, grid, grid_size, &game->player_ships[i])) {
                        clearHitList(hitListHead);
                    }
                    break;
                }
            }
        }
        else if (game->computer.chanceToHit != 1 && user == 1) {
            addHitCell(hitListHead, x, y);
            for (int i = 0; i < MAX_SHIPS; i++) {
                if ((x >= game->enemy_ships[i].startX && x < game->enemy_ships[i].startX + game->enemy_ships[i].length &&
                    game->enemy_ships[i].orientation == 0 && y == game->enemy_ships[i].startY) ||
                    (y >= game->enemy_ships[i].startY && y < game->enemy_ships[i].startY + game->enemy_ships[i].length &&
                        game->enemy_ships[i].orientation == 1 && x == game->enemy_ships[i].startX)) {

                    game->enemy_ships[i].hits++;
                    if (isShipSunk(game, grid, grid_size, &game->enemy_ships[i])) {
                        clearHitList(hitListHead);
                    }
                    break;
                }
            }
        }
        return 1;
        if (game->computer.chanceToHit != 1) {
            hit_found = 1;
        }
    }
    else if (grid[y][x] == MINE) {
        if (user == 1) {
            shootCell(game, game->player_grid, game->enemy_grid, x, y, grid_size);
        }
        else {
            shootCell(game, game->enemy_grid, game->player_grid, x, y, grid_size);
        }
    }
    else {
        grid[y][x] = MISS;
        return 0;
    }
    game->player_turn = 1;
    game->total_turns++;
    checkFog(game);
}

void drawGrid(GameState* game, short int x_offset, short int y_offset) {
    SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);

    for (int i = 0; i <= game->grid_size; i++) {
        SDL_RenderLine(game->renderer, x_offset, y_offset + i * game->cell_size, x_offset + game->grid_size * game->cell_size, y_offset + i * game->cell_size);
        SDL_RenderLine(game->renderer, x_offset + i * game->cell_size, y_offset, x_offset + i * game->cell_size, y_offset + game->grid_size * game->cell_size);
    }
}

void drawButton(GameState* game, Button button) {
    short int x = button.x_pos - game->menu_button_width / 2;
    short int y = button.y_pos - game->menu_button_height / 2;

    SDL_FRect buttonRect = { x, button.y_pos, game->menu_button_width, game->menu_button_height };

    if (!button.is_active) {
        SDL_SetRenderDrawColor(game->renderer, 204, 204, 204, 255);
    }
    else {
        if (button.is_pressed) {
            SDL_SetRenderDrawColor(game->renderer, 184, 12, 9, 255);
        }
        else {
            SDL_SetRenderDrawColor(game->renderer, 140, 173, 167, 255);
        }
    }

    SDL_RenderFillRect(game->renderer, &buttonRect);
    drawText(game, button.x_pos, button.y_pos, button.label, game->screen_height * 0.015f);

}

void drawCell(GameState* game, short int x, short int y, CellState state, short int x_offset, short int y_offset) {
    SDL_FRect cellRect = { x_offset + x * game->cell_size + 1, y_offset + y * game->cell_size + 1, game->cell_size - 2, game->cell_size - 2 };
    if (game->fog_active) {
        SDL_SetRenderDrawColor(game->renderer, 169, 169, 169, 255);
    }
    else {
        switch (state) {
        case WATER:
            SDL_SetRenderDrawColor(game->renderer, 37, 78, 112, 255);
            break;
        case SHIP:
            if (game->current_user.comp != 1) {
                if (x_offset == game->player_x_offset) {
                    SDL_SetRenderDrawColor(game->renderer, 140, 173, 167, 255);
                }
                else {
                    SDL_SetRenderDrawColor(game->renderer, 37, 78, 112, 255);
                }
            }
            else {
                SDL_SetRenderDrawColor(game->renderer, 140, 173, 167, 255);
            }
            break;
        case HIT:
            SDL_SetRenderDrawColor(game->renderer, 184, 12, 9, 255);
            break;
        case MISS:
            SDL_SetRenderDrawColor(game->renderer, 238, 198, 67, 255);
            break;
        case MINE:
            if (x_offset == game->enemy_x_offset && game->current_user.comp != 1) {
                SDL_SetRenderDrawColor(game->renderer, 37, 78, 112, 255);
            }
            else {
                SDL_SetRenderDrawColor(game->renderer, 175, 77, 152, 255);
            }
            break;
        }
    }
    SDL_RenderFillRect(game->renderer, &cellRect);
}

void drawBoard(GameState* game, CellState** grid, char grid_size, short int x_offset, short int y_offset) {
    drawGrid(game, x_offset, y_offset);
    for (int y = 0; y < game->grid_size; y++) {
        char text[10];
        snprintf(text, sizeof(text), "%d", y + 1);

        drawText(game, x_offset - game->cell_size / 2, y * game->cell_size + y_offset, text, FONT_SIZE);

        for (int x = 0; x < grid_size; x++) {
            drawText(game, x * game->cell_size + game->cell_size / 2 + x_offset, y_offset - game->cell_size, letters[x], FONT_SIZE);
            drawCell(game, x, y, grid[y][x], x_offset, y_offset);
        }
    }
}

void drawText(GameState* game, short int x_offset, short int y_offset, const char* text, int font_size) {
    SDL_Surface* surface = NULL;
    SDL_Texture* texture = NULL;
    int scaled_font_size = (int)(game->screen_height * (font_size / 1080.0f));
    TTF_SetFontSize(game->font, scaled_font_size);

    SDL_Color color = { 0, 0, 0 };
    char* utf8_text = win1251_to_utf8(text);
    surface = TTF_RenderText_Solid(game->font, utf8_text, 0, color);
    texture = SDL_CreateTextureFromSurface(game->renderer, surface);
    short    int x = x_offset - texture->w / 2;
    SDL_FRect dest_rect = { x, y_offset, 0.0f, 0.0f };
    dest_rect.w = texture->w;
    dest_rect.h = texture->h;
    SDL_RenderTexture(game->renderer, texture, NULL, &dest_rect);
    SDL_DestroySurface(surface);
    SDL_DestroyTexture(texture);
    free(utf8_text);
}


void drawBackground(GameState* game) {
    SDL_FRect rect = { 0, 0, game->screen_width, game->screen_height };
    SDL_SetRenderDrawColor(game->renderer, 242, 204, 195, 255);
    SDL_RenderFillRect(game->renderer, &rect);
}

void renderGame(GameState* game, float delta_time) {
    drawBackground(game);
    int font_size = (int)(game->screen_height * 0.03f); // Размер шрифта 3% высоты экрана

    // Фон для текста и кнопок
    SDL_SetRenderDrawColor(game->renderer, 242, 204, 195, 255);
    SDL_FRect rect = { 0, 0, game->screen_width, (int)(game->screen_height * 0.1f) }; // Верхние 10% экрана
    SDL_RenderFillRect(game->renderer, &rect);
    rect.h = game->screen_height;
    rect.w = game->menu_button_width + (int)(game->screen_width * 0.05f); // 5% ширины для отступа
    SDL_RenderFillRect(game->renderer, &rect);

    // Отрисовка игровых полей (без изменений)
    drawBoard(game, game->player_grid, game->grid_size, game->player_x_offset, game->player_y_offset);
    drawBoard(game, game->enemy_grid, game->grid_size, game->enemy_x_offset, game->enemy_y_offset);

    // Отрисовка кнопок
    for (int i = 0; i < 4; ++i) {
        drawButton(game, game_buttons[i]);
    }

    // Текст в зависимости от режима игры
    if (game->current_mode == PLACING_SHIPS) {
        if (game->current_user.comp != 1) {
            drawText(game, game->screen_width / 2, (int)(game->screen_height * 0.05f), "Выберите расположение своих кораблей.", font_size);
        }
        else {
            drawText(game, game->screen_width / 2, (int)(game->screen_height * 0.05f), "Нажмите на любую клетку поля, чтобы начать игру Компьютер против Компьютера.", font_size);
        }
    }
    else if (game->current_mode == PLAYING) {
        if (game->turns_count == 0) {
            if (game->current_user.comp != 1) {
                drawText(game, game->screen_width / 2, (int)(game->screen_height * 0.05f), "Размещение завершено, начинайте игру!", font_size);
            }
            else {
                drawText(game, game->screen_width / 2, (int)(game->screen_height * 0.05f), "Игра Компьютер против Компьютера началась!", font_size);
            }
        }
        else {
            if (game->storm_active) {
                drawText(game, game->screen_width / 2, (int)(game->screen_height * 0.05f), "Шторм! Случайная часть кораблей разрушена!", font_size);
            }
            else {
                if (game->player_turn) {
                    drawText(game, game->screen_width / 2, (int)(game->screen_height * 0.05f), "Ваш ход.", font_size);
                }
                else {
                    drawText(game, game->screen_width / 2, (int)(game->screen_height * 0.05f), "Ход компьютера.", font_size);
                }

                if (game->fog_active) {
                    drawText(game, game->screen_width / 2, (int)(game->screen_height * 0.08f), "Туман! Вы не можете увидеть, попали ли вы в корабль.", font_size);
                }
                else if (game->is_hit == -2) {
                    drawText(game, game->screen_width / 2, (int)(game->screen_height * 0.08f), "о нет! мина", font_size);
                }
                else if (game->is_hit >= 0) {
                    drawText(game, game->screen_width / 2, (int)(game->screen_height * 0.08f), game->is_hit ? "Попали!" : "Промах", font_size);
                }
            }
        }
    }
    else if (game->current_mode == PLACING_RADAR) {
        if (!game->radar_running) {
            drawText(game, game->screen_width / 2, (int)(game->screen_height * 0.05f), "Выберите расположение радара:", font_size);
        }
        else {
            drawText(game, game->screen_width / 2, (int)(game->screen_height * 0.05f), "Проверка радара...", font_size);
            drawRadar(game, delta_time);
        }
    }
    else if (game->current_mode == PLACING_MINES) {
        drawText(game, game->screen_width / 2, (int)(game->screen_height * 0.05f), "Выберите расположение мин:", font_size);
    }

    if (game->current_user.comp != 1) {
        drawText(game, game->player_x_offset + (game->cell_size * game->grid_size) / 2,
            game->player_y_offset - (int)(game->screen_height * 0.1f), "Ваше поле", font_size);
    }

    drawStormEffect(game);
    SDL_RenderPresent(game->renderer);
}

void renderMenu(GameState* game) {
    drawBackground(game);
    int font_size = (int)(game->screen_height * 0.015f); // Размер шрифта 3% высоты экрана
    int title_font_size = (int)(game->screen_height * 0.04f); // Заголовок 4% высоты экрана

    if (game->menu_state == MAIN_MENU) {
        drawText(game, game->screen_width / 2, (int)(game->screen_height * 0.1f), "Морской бой с бонусами", title_font_size);
        for (int i = 0; i < MAIN_BUTTONS_COUNT; ++i) {
            drawButton(game, main_menu_buttons[i]);
        }
        float y_pos = main_menu_buttons[MAIN_BUTTONS_COUNT - 1].y_pos + game->menu_button_height + (int)(game->screen_height * 0.05f);
        drawText(game, game->screen_width / 2, (int)y_pos, "о программе:", font_size);
        y_pos += game->screen_height * 0.03f;
        drawText(game, game->screen_width / 2, (int)y_pos, "Геймдевы: Дмитриева А.В. и Виноградова А.Д.", font_size - 4);
        y_pos += game->screen_height * 0.03f;
        drawText(game, game->screen_width / 2, (int)y_pos, "При наставничестве Панкова И. Д.", font_size - 4);
        y_pos += game->screen_height * 0.03f;
        drawText(game, game->screen_width / 2, (int)y_pos, "Санкт-Петербургский Политехнический Университет Петра Великого", font_size - 4);
        y_pos += game->screen_height * 0.03f;
        drawText(game, game->screen_width / 2, (int)y_pos, "Институт компьютерных наук и кибербезопасности", font_size - 4);
        y_pos += game->screen_height * 0.03f;
        drawText(game, game->screen_width / 2, (int)y_pos, "Высшая школа кибербезопасности", font_size - 4);
        y_pos += game->screen_height * 0.03f;
        drawText(game, game->screen_width / 2, (int)y_pos, "2025", font_size - 4);
    }
    else if (game->menu_state == SETTINGS_MENU) {
        for (int i = 0; i < SETTINGS_BUTTONS_COUNT; ++i) {
            drawButton(game, settings_menu_buttons[i]);
        }
    }
    else if (game->menu_state == LOAD_MENU || game->menu_state == SAVE_MENU) {
        for (int i = 0; i < MAX_SAVES; ++i) {
            drawButton(game, game->saves_buttons[i]);
        }
        drawButton(game, leaderboard_buttons[0]);
    }
    else if (game->menu_state == LEADERBOARD_MENU) {
        float name_col_x = game->screen_width * 0.3f; // 30% от левого края
        float score_col_x = game->screen_width * 0.7f; // 70% от левого края
        float y_pos = game->screen_height * 0.15f; // Начало таблицы на 15% высоты
        float row_height = game->screen_height * 0.05f; // Высота строки 5%

        drawText(game, (int)name_col_x, (int)(y_pos - row_height), "Имя", font_size);
        drawText(game, (int)score_col_x, (int)(y_pos - row_height), "Очки", font_size);

        for (int i = 0; i < MAX_PLAYERS; ++i) {
            if (game->leaderboard[i].name[0] == '\0' || game->leaderboard[i].score == -1) {
                continue;
            }
            char scoreText[16];
            snprintf(scoreText, sizeof(scoreText), "%hd", game->leaderboard[i].score);
            drawText(game, (int)name_col_x, (int)(y_pos + i * row_height), game->leaderboard[i].name, font_size);
            drawText(game, (int)score_col_x, (int)(y_pos + i * row_height), scoreText, font_size);
        }
        drawButton(game, leaderboard_buttons[0]);
    }
    else if (game->menu_state == VICTORY_MENU) {
        if (game->computer_win != 1) {
            if (game->current_user.comp != 1) {
                drawText(game, game->screen_width / 2, (int)(game->screen_height * 0.25f), "Вы победили! Введите ваше имя на английском, иначе вы не попадете в таблицу лидеров:", font_size);
                SDL_FRect inputRect = {
                    game->screen_width * 0.5f - (game->screen_width * 0.3f) / 2, // 30% ширины экрана
                    game->screen_height * 0.5f - (game->screen_height * 0.05f) / 2, // 5% высоты
                    game->screen_width * 0.3f,
                    game->screen_height * 0.05f
                };
                SDL_SetRenderDrawColor(game->renderer, 140, 173, 167, 255);
                SDL_RenderFillRect(game->renderer, &inputRect);
            }
            else {
                drawText(game, game->screen_width / 2, (int)(game->screen_height * 0.25f), "Компьютер 1 победил!", font_size);
            }
        }
        else {
            if (game->current_user.comp != 1) {
                drawText(game, game->screen_width / 2, (int)(game->screen_height * 0.25f), "Компьютер победил!", font_size);
            }
            else {
                drawText(game, game->screen_width / 2, (int)(game->screen_height * 0.25f), "Компьютер 2 победил!", font_size);
            }
        }
        victory_buttons[0].is_active = true;
        drawButton(game, victory_buttons[0]);
    }
}

CellState** allocBoard(int size) {
    CellState** board = malloc(sizeof(int*) * size);
    for (int i = 0; i < size; i++) {
        board[i] = malloc(sizeof(int) * size);
    }
    return board;
}

void checkButtonsClick(GameState* game, short int mouseX, short int mouseY, Button* buttons, int size) {
    for (int i = 0; i < size; ++i) {
        if (mouseX >= buttons[i].left && mouseX <= buttons[i].right &&
            mouseY >= buttons[i].top && mouseY <= buttons[i].bottom + game->menu_button_height) {
            handleButtonClick(game, &buttons[i]);
        }
    }
}

void handleButtonClick(GameState* game, Button* button) {
    if (!button->is_active) {
        return;
    }

    switch (button->type) {
    case BUTTON_NEW_GAME:
        game->menu_state = SETTINGS_MENU;
        break;

    case BUTTON_LOAD_GAME:
        game->menu_state = LOAD_MENU;
        break;

    case BUTTON_EXIT: {
        SDL_Event e;
        e.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&e);
        break;
    }

    case BUTTON_SIZE_10X10:
        button->is_pressed = !button->is_pressed;
        settings_menu_buttons[1].is_pressed = false;
        game->grid_size = 10;
        break;

    case BUTTON_SIZE_15X15:
        button->is_pressed = !button->is_pressed;
        settings_menu_buttons[0].is_pressed = false;
        game->grid_size = 15;
        break;

    case BUTTON_EASY_DIFF:
        button->is_pressed = !button->is_pressed;
        settings_menu_buttons[3].is_pressed = false;
        settings_menu_buttons[4].is_pressed = false;
        game->current_difficult = EASY;
        break;

    case BUTTON_NORMAL_DIFF:
        button->is_pressed = !button->is_pressed;
        settings_menu_buttons[2].is_pressed = false;
        settings_menu_buttons[4].is_pressed = false;
        game->current_difficult = NORMAL;
        break;

    case BUTTON_HARD_DIFF:
        button->is_pressed = !button->is_pressed;
        settings_menu_buttons[2].is_pressed = false;
        settings_menu_buttons[3].is_pressed = false;
        game->current_difficult = HARD;
        break;

    case BUTTON_USER_COMP:
        button->is_pressed = !button->is_pressed;
        settings_menu_buttons[6].is_pressed = false;
        game->current_user.both = 1;
        game->current_user.both = 0;
        break;

    case BUTTON_COMP_COMP:
        button->is_pressed = !button->is_pressed;
        settings_menu_buttons[5].is_pressed = false;
        game->current_user.comp = 1;
        game->current_user.user = 0;
        break;

    case BUTTON_START:
        game->player_grid = allocBoard(game->grid_size);
        game->enemy_grid = allocBoard(game->grid_size);
        initBoard(game, &game->player_grid, game->grid_size);
        initBoard(game, &game->enemy_grid, game->grid_size);
        initGame(game);
        game->current_mode = PLACING_SHIPS;
        break;

    case BUTTON_PLACE_RANDOM:
        if (game->current_user.comp != 1) {
            if (button->is_active) {
                button->is_active = false;
                game->player_ships_count = placeShipsRandomly(game, game->player_grid, game->grid_size, game->player_ships);
            }
        }

        break;

    case BUTTON_RADAR:
        if (game->current_user.comp != 1) {
            button->is_pressed = !button->is_pressed;
            game->current_mode = PLACING_RADAR;
        }

        break;

    case BUTTON_SAVE_GAME:
        game->menu_state = SAVE_MENU;
        game->current_mode = MENU;

    case BUTTON_SAVE_SLOT:
        button->is_pressed = true;
        for (int i = 0; i < MAX_SAVES; ++i) {
            if (game->saves_buttons[i].is_pressed) {
                if (game->menu_state == LOAD_MENU) {
                    loadGame(game, i);
                }
                else if (game->menu_state == SAVE_MENU) {
                    saveGame(game, i);
                    game->current_mode = PLAYING;
                }
            }
        }
        button->is_pressed = false;
        break;

    case BUTTON_LEADERBOARD:
        game->menu_state = LEADERBOARD_MENU;
        break;

    case BUTTON_EXIT_MENU:
        saveLeaderBoard(game);
        printf("Нажата кнопка выхода\n");
        game->current_mode = MENU;
        game->menu_state = MAIN_MENU;
        game->computer_win = 0;
        game->turns_count = 0;
        game->player_turn = 1;
        game->fog_active = 0;
        game->storm_active = 0;
        game->current_fog = 0;
        game->current_storm = 0;
        game->total_turns = 0;
        game->radars_left = 2;
        game->mines_left = 2;

        if (game->player_grid != NULL) {
            freeBoard(game->player_grid, game->grid_size);
            game->player_grid = NULL;
        }
        if (game->enemy_grid != NULL) {
            freeBoard(game->enemy_grid, game->grid_size);
            game->enemy_grid = NULL;
        }

        for (int i = 0; i < 3; i++) {
            game_buttons[i].is_pressed = false;
            game_buttons[i].is_active = true;
        }
        game_buttons[0].is_active = false;
        game_buttons[2].is_active = false;

        break;
    }
}

int check_eng(GameState* game, char* str) {
    int len = strlen(str);

    for (int i = 0; i < len; i++) {
        if (str[i] > 127 || str[i] < 0) return 0;
    }

    return 1;
}

char* handleTextInput(GameState* game) {
    short int max_len = 256;
    char* output_text = malloc(256);
    SDL_Event event;
    char input_text[256] = { 0 };
    size_t input_len = 0;

    SDL_StartTextInput(game->window);

    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                done = true;
                break;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_RETURN) {
                    done = true;
                    break;
                }
                else if (event.key.key == SDLK_BACKSPACE && input_len > 0) {
                    input_text[--input_len] = '\0';
                }
            }
            else if (event.type == SDL_EVENT_TEXT_INPUT) {
                if (input_len + strlen(event.text.text) < sizeof(input_text) - 1) {
                    strcat(input_text, event.text.text);
                    input_len += strlen(event.text.text);
                }
            }
        }

        renderMenu(game);

        if (input_len > 0) {
            drawText(game, game->screen_width / 2 - 350 / 2, game->screen_height / 2 - 40 / 2, input_text, FONT_SIZE);
        }

        SDL_RenderPresent(game->renderer);
        SDL_Delay(16);
    }

    SDL_StopTextInput(game->window);
    strncpy(output_text, input_text, max_len - 1);
    output_text[max_len - 1] = '\0';

    if (check_eng(game, output_text) == 0) {
        free(output_text);
        output_text = NULL;
    }
    return output_text;
}

void initGame(GameState* game) {
    float button_width_ratio = MENU_BUTTON_WIDTH_RATIO;
    float button_height_ratio = MENU_BUTTON_HEIGHT_RATIO;
    float start_x_ratio = 0.1f; // 10% от левого края
    float start_y_ratio = 0.2f; // 20% от верха
    float spacing_ratio = 0.07f; // Отступ 7% высоты экрана

    game->menu_button_width = (int)(game->screen_width * button_width_ratio);
    game->menu_button_height = (int)(game->screen_height * button_height_ratio);

    // Кнопки игры
    for (int i = 0; i < 4; i++) {
        game_buttons[i].x_pos = (int)(game->screen_width * start_x_ratio);
        game_buttons[i].y_pos = (int)(game->screen_height * (start_y_ratio + i * spacing_ratio));
        game_buttons[i].left = game_buttons[i].x_pos - game->menu_button_width / 2;
        game_buttons[i].right = game_buttons[i].x_pos + game->menu_button_width / 2;
        game_buttons[i].top = game_buttons[i].y_pos - game->menu_button_height / 2;
        game_buttons[i].bottom = game_buttons[i].y_pos + game->menu_button_height / 2;
    }

    game->computer_mines_left = 2;
    game->computer_radars_left = 2;
}

void startGame(GameState* game) {
    if (game->radars_left > 0) {
        game_buttons[0].is_active = true;
    }
    game_buttons[2].is_active = true;
    game_buttons[1].is_active = false;
    while (game->computer_mines_left > 0) {
        short int x = rand() % game->grid_size;
        short int y = rand() % game->grid_size;

        if (game->enemy_grid[y][x] == WATER) {
            game->enemy_grid[y][x] = MINE;
            game->computer_mines_left--;
        }
        x = rand() % game->grid_size;
        y = rand() % game->grid_size;
        if (game->current_user.comp == 1 && game->player_grid[y][x] == WATER) {
            game->player_grid[y][x] = MINE;
            game->computer2_mines_left--;
        }

    }
    game->current_mode = PLAYING;
}
void placeRadar(GameState* game, SDL_Event event, short int x_offset, short int y_offset, CellState** grid, char grid_size, float delta_time) {
    short int mouse_x = event.button.x;
    short int mouse_y = event.button.y;

    if (game->radars_left > 0 &&
        mouse_x >= game->enemy_x_offset && mouse_x < game->enemy_x_offset + grid_size * game->cell_size &&
        mouse_y >= game->enemy_y_offset && mouse_y < game->enemy_y_offset + grid_size * game->cell_size) {

        game->radar_x = (mouse_x - x_offset) / game->cell_size;
        game->radar_y = (mouse_y - y_offset) / game->cell_size;


        game->radar_running = 1;
        game->radars_left--;
    }
    else {
        game->current_mode = PLAYING;
    }

    if (game->radars_left == 0) {
        game_buttons[0].is_active = false;
    }
    game_buttons[0].is_pressed = false;
}

void placeMine(GameState* game, SDL_Event event, short int x_offset, short int y_offset, CellState** grid, char grid_size) {
    short int mouse_x = event.button.x;
    short int mouse_y = event.button.y;

    if (game->mines_left > 0 &&
        mouse_x >= game->player_x_offset && mouse_x < game->player_x_offset + grid_size * game->cell_size &&
        mouse_y >= game->player_y_offset && mouse_y < game->player_y_offset + grid_size * game->cell_size) {

        short  int x = (mouse_x - x_offset) / game->cell_size;
        short int y = (mouse_y - y_offset) / game->cell_size;


        if (game->player_grid[y][x] != WATER) {
            return;
        }
        else {
            game->player_grid[y][x] = MINE;
            --game->mines_left;
        }
    }
}

char handleInput(GameState* game, SDL_Event event, short int x_offset, short int y_offset, CellState** grid, char grid_size) {
    short int mouseX = event.button.x;
    short int mouseY = event.button.y;

    if (mouseX >= x_offset && mouseX < x_offset + grid_size * game->cell_size &&
        mouseY >= y_offset && mouseY < y_offset + grid_size * game->cell_size) {

        short  int gridX = (mouseX - x_offset) / game->cell_size;
        short int gridY = (mouseY - y_offset) / game->cell_size;

        game->total_turns++;

        if (game->fog_active) {
            game->fog_turns_in_progress++;

            if (game->fog_turns_in_progress == 2) {
                game->fog_active = 0;
                game->fog_turns_in_progress = 0;
            }

            grid[gridY][gridX] = MISS;
        }
        else {
            if (grid[gridY][gridX] == SHIP) {
                grid[gridY][gridX] = HIT;

                for (int i = 0; i < MAX_SHIPS; i++) {
                    if ((gridX >= game->enemy_ships[i].startX && gridX < game->enemy_ships[i].startX + game->enemy_ships[i].length &&
                        game->enemy_ships[i].orientation == 0 && gridY == game->enemy_ships[i].startY) ||
                        (gridY >= game->enemy_ships[i].startY && gridY < game->enemy_ships[i].startY + game->enemy_ships[i].length &&
                            game->enemy_ships[i].orientation == 1 && gridX == game->enemy_ships[i].startX)) {

                        game->enemy_ships[i].hits++;
                        if (isShipSunk(game, game->enemy_grid, grid_size, &game->enemy_ships[i])) {
                        }
                        break;
                    }
                }
                game->player_turn = 1;
                game->turns_count++;
                game->is_hit = 1;
            }
            else if (grid[gridY][gridX] == WATER) {
                grid[gridY][gridX] = MISS;
                game->player_turn = 0;
                game->turns_count++;
                game->is_hit = 0;
            }
            else if (grid[gridY][gridX] == MINE) {
                shootCell(game, game->player_grid, game->enemy_grid, gridX, gridY, grid_size);
            }
            else {
                game->player_turn = 1;
            }
        }

        checkFog(game);

        return 1;
    }
    return 0;
}
char handleShipPlacement(GameState* game, SDL_Event event, short int x_offset, short int y_offset, CellState** grid, char grid_size, char ch, short int turn_ship) {

    short int mouseX = event.button.x;
    short int mouseY = event.button.y;

    if (mouseX > x_offset && mouseX < x_offset + grid_size * game->cell_size &&
        mouseY > y_offset && mouseY < y_offset + grid_size * game->cell_size) {

        short int gridX = (mouseX - x_offset) / game->cell_size;
        short int gridY = (mouseY - y_offset) / game->cell_size;
        char count_c = 0;


        if (grid[gridY][gridX] == WATER) {
            if (grid[gridY][gridX + 1] == SHIP && grid[gridY][gridX + 2] == SHIP && grid[gridY][gridX + 3] == SHIP && grid[gridY][gridX + 4] == SHIP) {
                if (grid[gridY][gridX + 5] == SHIP && grid_size == 15) {
                    count_c += 2;
                }
                if (grid_size == 10) {
                    count_c += 2;
                }
            }
            if (grid[gridY][gridX - 1] == SHIP && grid[gridY][gridX - 2] == SHIP && grid[gridY][gridX - 3] == SHIP && grid[gridY][gridX - 4] == SHIP) {
                if (grid[gridY][gridX - 5] == SHIP && grid_size == 15) {
                    count_c += 2;
                }
                if (grid_size == 10) {
                    count_c += 2;
                }
            }
            if (gridY != 9 && gridX + 1 != grid_size && grid[gridY + 1][gridX + 1] == SHIP) {
                count_c += 2;
            }
            if (gridY < 6 && grid[gridY + 1][gridX] == SHIP && grid[gridY + 2][gridX] == SHIP && grid[gridY + 3][gridX] == SHIP && grid[gridY + 4][gridX] == SHIP) {
                if (grid_size == 15 && grid[gridY + 5][gridX] == SHIP) {
                    count_c += 2;
                }
                if (grid_size == 10) {
                    count_c += 2;
                }
            }
            if (gridY > 3 && grid[gridY - 1][gridX] == SHIP && grid[gridY - 2][gridX] == SHIP && grid[gridY - 3][gridX] == SHIP && grid[gridY - 4][gridX] == SHIP) {
                if (grid_size == 15 && grid[gridY - 5][gridX] == SHIP) {
                    count_c += 2;
                }
                if (grid_size == 10) {
                    count_c += 2;
                }
            }
            if (gridY != 0 && gridX - 1 != -1 && grid[gridY - 1][gridX - 1] == SHIP) {
                count_c += 2;
            }
            if (gridY != 9 && gridX - 1 != -1 && grid[gridY + 1][gridX - 1] == SHIP) {
                count_c += 2;
            }
            if (gridY != 0 && gridX + 1 != grid_size && grid[gridY - 1][gridX + 1] == SHIP) {
                count_c += 2;
            }
            if (count_c < 2) {
                grid[gridY][gridX] = SHIP;

                game->player_ships[game->player_ships_count].startX = gridX;
                game->player_ships[game->player_ships_count].startY = gridY;
                game->player_ships[game->player_ships_count].orientation = 0;


                game->player_ships_count++;


                return 1;
            }
        }
    }

    return 0;
}

char shootCell(GameState* game, CellState** shooter, CellState** target, short int x, short int y, char grid_size) {
    switch (target[y][x]) {
    case WATER:
        target[y][x] = MISS;
        return 0;
        break;
    case SHIP:
        target[y][x] = HIT;
        for (int i = 0; i < game->player_ships_count; i++) {
            if ((x >= game->player_ships[i].startX && x < game->player_ships[i].startX + game->player_ships[i].length &&
                game->player_ships[i].orientation == 0 && y == game->player_ships[i].startY) ||
                (y >= game->player_ships[i].startY && y < game->player_ships[i].startY + game->player_ships[i].length &&
                    game->player_ships[i].orientation == 1 && x == game->player_ships[i].startX)) {
                game->player_ships[i].hits++;
                if (isShipSunk(game, target, grid_size, &game->player_ships[i])) {
                }
                break;
            }
        }
        return 1;
        break;
    case HIT:
        return 0;
        break;
    case MINE:
        if (shooter[y][x] == SHIP) {
            target[y][x] = MISS;
            shooter[y][x] = HIT;
        }
        else {
            target[y][x] = MISS;
            shooter[y][x] = MISS;
        }

        game->is_hit = -2;

        return 0;
        break;
    }
}

void addPlayerToLeaderboard(GameState* game, const char* playerName, short int score) {
    int insert_pos = MAX_PLAYERS;
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        if (game->leaderboard[i].score == -1 || score < game->leaderboard[i].score) {
            insert_pos = i;
            break;
        }
    }

    if (insert_pos == MAX_PLAYERS) return;

    for (int j = MAX_PLAYERS - 1; j > insert_pos; --j) {
        game->leaderboard[j] = game->leaderboard[j - 1];
    }

    strncpy(game->leaderboard[insert_pos].name, playerName, sizeof(game->leaderboard[insert_pos].name) - 1);
    game->leaderboard[insert_pos].name[sizeof(game->leaderboard[insert_pos].name) - 1] = '\0';
    game->leaderboard[insert_pos].score = score;

    saveLeaderBoard(game);
}

void initLeaderBoard(GameState* game) {
    for (int i = 0; i < MAX_PLAYERS; ++i) {
        game->leaderboard[i].name[0] = '\0';
        game->leaderboard[i].score = -1;
    }

    FILE* file = fopen("leaderboard.txt", "r");
    if (!file) {
        return;
    }

    char line[256];
    int index = 0;

    while (fgets(line, sizeof(line), file) && index < MAX_PLAYERS) {
        line[strcspn(line, "\n")] = '\0';

        if (line[0] == '#') continue;

        strncpy(game->leaderboard[index].name, line, sizeof(game->leaderboard[index].name) - 1);
        game->leaderboard[index].name[sizeof(game->leaderboard[index].name) - 1] = '\0';

        if (!fgets(line, sizeof(line), file)) break;
        line[strcspn(line, "\n")] = '\0';
        sscanf(line, "%hd", &game->leaderboard[index].score);

        index++;
    }

    fclose(file);
}

void saveLeaderBoard(GameState* game) {
    FILE* file = fopen("leaderboard.txt", "w");
    if (!file) {
        printf("Не удалось сохранить таблицу лидеров\n");
        return;
    }

    for (int i = 0; i < MAX_PLAYERS; ++i) {
        if (game->leaderboard[i].score == -1) continue;

        fprintf(file, "%s\n", game->leaderboard[i].name);
        fprintf(file, "%hd\n", game->leaderboard[i].score);
    }

    fclose(file);
}

void drawRadar(GameState* game, float delta_time) {
    static char count = 0;

    for (int y = game->radar_y - 1; y <= game->radar_y + 1; ++y) {
        for (int x = game->radar_x - 1; x <= game->radar_x + 1; ++x) {
            if (x < 0 || x >= game->grid_size || y < 0 || y >= game->grid_size) {
                continue;
            }

            SDL_FRect cellRect = {
                game->enemy_x_offset + x * game->cell_size + 1,
                game->enemy_y_offset + y * game->cell_size + 1,
                game->cell_size - 2,
                game->cell_size - 2
            };

            SDL_FRect borderRect = {
                game->enemy_x_offset + x * game->cell_size,
                game->enemy_y_offset + y * game->cell_size,
                game->cell_size,
                game->cell_size
            };
            SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
            SDL_RenderRect(game->renderer, &borderRect);

            if (count % 2 == 0) {
                switch (game->enemy_grid[y][x]) {
                case SHIP:
                    SDL_SetRenderDrawColor(game->renderer, 184, 12, 9, 255);

                    break;
                default:
                    SDL_SetRenderDrawColor(game->renderer, 5, 74, 41, 255);
                    break;
                }
                SDL_RenderFillRect(game->renderer, &cellRect);
            }
            else {
                SDL_SetRenderDrawColor(game->renderer, 37, 78, 112, 255);
                SDL_RenderFillRect(game->renderer, &cellRect);
            }
        }
    }

    game->time_elapsed += delta_time;

    if (game->time_elapsed > 0.2f) {
        ++count;
        game->time_elapsed = 0.0f;
    }

    if (count >= 6) {
        count = 0;
        game->radar_running = 0;
        game->current_mode = PLAYING;
    }
}

void activateFog(GameState* game) {
    if (game->current_fog < 2 && game->total_turns == game->fog_turns[game->current_fog]) {
        game->fog_active = 1;
        game->fog_turns_in_progress = 0;
        game->current_fog++;
    }
}

void checkFog(GameState* game) {
    if (!game->fog_active) {
        activateFog(game);
    }
}

void generateFogTurns(GameState* game) {
    for (int i = 0; i < 2; i++) {
        game->fog_turns[i] = (rand() % 26) + 5;
    }
}

void markAroundSunkShip(GameState* game, CellState** grid, int grid_size, Ship ship) {
    int startX = ship.startX;
    int startY = ship.startY;
    int endX = ship.startX;
    int endY = ship.startY;

    if (ship.orientation == 0) {
        endX = startX + ship.length - 1;
    }
    else {
        endY = startY + ship.length - 1;
    }

    for (int y = startY - 1; y <= endY + 1; y++) {
        for (int x = startX - 1; x <= endX + 1; x++) {
            if (x >= 0 && x < grid_size && y >= 0 && y < grid_size) {
                if (!(y >= startY && y <= endY && x >= startX && x <= endX) &&
                    (grid[y][x] == WATER || grid[y][x] == MINE)) {
                    grid[y][x] = MISS;
                }
            }
        }
    }
}

char isShipSunk(GameState* game, CellState** grid, int grid_size, Ship* ship) {
    if (ship->length == ship->hits) {
        markAroundSunkShip(game, grid, grid_size, *ship);
        return 1;
    }
    return 0;
}


char shipsRemaining(GameState* game, CellState** grid, char grid_size) {
    for (int y = 0; y < grid_size; y++) {
        for (int x = 0; x < grid_size; x++) {
            if (grid[y][x] == SHIP) {
                return 1;
            }
        }
    }
    return 0;
}

void freeBoard(CellState** grid, int size) {
    for (int i = 0; i < size; ++i) {
        free(grid[i]);
    }
    free(grid);
}



void generateStormTurns(GameState* game) {
    for (int i = 0; i < 2; i++) {
        game->storm_turns[i] = (rand() % 21) + 10;
    }
}

void activateStorm(GameState* game) {
    if (game->current_storm < 2 && game->total_turns == game->storm_turns[game->current_storm]) {
        game->storm_active = 1;
        game->storm_time_elapsed = 0.0f;
        game->current_storm++;

        short int damage_count = rand() % 5 + 1;
        for (int i = 0; i < damage_count; i++) {
            short int x, y;
            do {
                x = rand() % game->grid_size;
                y = rand() % game->grid_size;
            } while (game->player_grid[y][x] != SHIP);

            game->player_grid[y][x] = HIT;
            for (int j = 0; j < game->player_ships_count; j++) {
                if ((x >= game->player_ships[j].startX && x < game->player_ships[j].startX + game->player_ships[j].length &&
                    game->player_ships[j].orientation == 0 && y == game->player_ships[j].startY) ||
                    (y >= game->player_ships[j].startY && y < game->player_ships[j].startY + game->player_ships[j].length &&
                        game->player_ships[j].orientation == 1 && x == game->player_ships[j].startX)) {
                    game->player_ships[j].hits++;
                    break;
                }
            }
        }

        for (int i = 0; i < damage_count; i++) {
            short  int x, y;
            do {
                x = rand() % game->grid_size;
                y = rand() % game->grid_size;
            } while (game->enemy_grid[y][x] != SHIP);

            game->enemy_grid[y][x] = HIT;
            for (int j = 0; j < game->enemy_ships_count; j++) {
                if ((x >= game->enemy_ships[j].startX && x < game->enemy_ships[j].startX + game->enemy_ships[j].length &&
                    game->enemy_ships[j].orientation == 0 && y == game->enemy_ships[j].startY) ||
                    (y >= game->enemy_ships[j].startY && y < game->enemy_ships[j].startY + game->enemy_ships[j].length &&
                        game->enemy_ships[j].orientation == 1 && x == game->enemy_ships[j].startX)) {
                    game->enemy_ships[j].hits++;
                    break;
                }
            }
        }
    }
}

void checkStorm(GameState* game, float delta_time) {
    if (game->storm_active) {
        game->storm_time_elapsed += delta_time;
        if (game->storm_time_elapsed >= 2.0f) {
            game->storm_active = 0;
            game->is_hit = -1;
        }
    }
    else {
        activateStorm(game);
    }
}

void drawStormEffect(GameState* game) {
    if (game->storm_active) {
        for (int y = 0; y < game->grid_size; y++) {
            for (int x = 0; x < game->grid_size; x++) {
                SDL_FRect playerCellRect = {
                    game->player_x_offset + x * game->cell_size + 1,
                    game->player_y_offset + y * game->cell_size + 1,
                    game->cell_size - 2,
                    game->cell_size - 2
                };
                SDL_SetRenderDrawColor(game->renderer, 173, 216, 230, 150);
                SDL_RenderFillRect(game->renderer, &playerCellRect);
                SDL_FRect enemyCellRect = {
                    game->enemy_x_offset + x * game->cell_size + 1,
                    game->enemy_y_offset + y * game->cell_size + 1,
                    game->cell_size - 2,
                    game->cell_size - 2
                };
                SDL_SetRenderDrawColor(game->renderer, 173, 216, 230, 150);
                SDL_RenderFillRect(game->renderer, &enemyCellRect);
            }
        }
    }
}


//helllo ibks