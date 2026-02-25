#include <Arduino.h>
#include <SPI.h>
#include "core/display.h"
#include "core/mykeyboard.h"
#include "globals.h"

// --- CONSTANTS ---
#define TETRIS_BG_COLOR TFT_BLACK
#define GRID_W 10
#define GRID_H 20

// --- STATIC GLOBALS ---
static int screen[GRID_W][GRID_H] = {0};
struct Point { int x, y; };
struct Block { Point square[4][4]; int num_rotations; int color_index; };

// Dynamic Layout Variables
static int BLOCK_SIZE = 10;
static int OFFSET_X = 0;
static int OFFSET_Y = 0;
static int UI_X = 0;
static int UI_Y = 0;

// Player State
static Point current_pos;
static Block current_block;
static int current_rotation = 0;
static bool game_running = false;
static bool game_over = false;
static int tetris_score = 0;
static int tetris_level = 1;
static int speed_delay = 500; 

// Block Definitions
static Block blocks[7] = {
    {{{{-1,0},{0,0},{1,0},{2,0}}, {{0,-1},{0,0},{0,1},{0,2}}, {{-1,0},{0,0},{1,0},{2,0}}, {{0,-1},{0,0},{0,1},{0,2}}}, 2, 1}, // I
    {{{{0,-1},{1,-1},{0,0},{1,0}}, {{0,-1},{1,-1},{0,0},{1,0}}, {{0,-1},{1,-1},{0,0},{1,0}}, {{0,-1},{1,-1},{0,0},{1,0}}}, 1, 2}, // O
    {{{{-1,-1},{-1,0},{0,0},{1,0}}, {{-1,1},{0,1},{0,0},{0,-1}}, {{-1,0},{0,0},{1,0},{1,1}}, {{1,-1},{0,-1},{0,0},{0,1}}}, 4, 3}, // J
    {{{{-1,0},{0,0},{0,1},{1,1}}, {{0,-1},{0,0},{-1,0},{-1,1}}, {{-1,0},{0,0},{0,1},{1,1}}, {{0,-1},{0,0},{-1,0},{-1,1}}}, 2, 4}, // S
    {{{{-1,0},{0,0},{1,0},{1,-1}}, {{-1,-1},{0,-1},{0,0},{0,1}}, {{-1,1},{-1,0},{0,0},{1,0}}, {{0,-1},{0,0},{0,1},{1,1}}}, 4, 5}, // L
    {{{{-1,1},{0,1},{0,0},{1,0}}, {{0,-1},{0,0},{1,0},{1,1}}, {{-1,1},{0,1},{0,0},{1,0}}, {{0,-1},{0,0},{1,0},{1,1}}}, 2, 6}, // Z
    {{{{-1,0},{0,0},{1,0},{0,-1}}, {{0,-1},{0,0},{0,1},{-1,0}}, {{-1,0},{0,0},{1,0},{0,1}}, {{0,-1},{0,0},{0,1},{1,0}}}, 4, 7}  // T
};

static uint16_t block_colors[] = {
    TETRIS_BG_COLOR, TFT_CYAN, TFT_YELLOW, TFT_BLUE, TFT_GREEN, TFT_ORANGE, TFT_RED, TFT_MAGENTA
};

// --- HELPER FUNCTIONS ---

void calculate_layout() {
    int screen_w = tft.width();
    int screen_h = tft.height();

    int max_w = (screen_w - 4) / GRID_W; 
    int max_h = (screen_h - 4) / GRID_H; 
    
    BLOCK_SIZE = (max_w < max_h) ? max_w : max_h;
    
    if (BLOCK_SIZE < 6) BLOCK_SIZE = 6;

    int grid_pixel_w = GRID_W * BLOCK_SIZE;
    int grid_pixel_h = GRID_H * BLOCK_SIZE;

    OFFSET_X = (screen_w - grid_pixel_w) / 2;
    OFFSET_Y = (screen_h - grid_pixel_h) / 2;

    if (screen_w - (OFFSET_X + grid_pixel_w) > 40) {
        UI_X = OFFSET_X + grid_pixel_w + 5;
        UI_Y = OFFSET_Y + 10;
    } 
    else {
        UI_X = 2;
        UI_Y = 2;
    }
}

void draw_cell(int x, int y, int color_idx) {
    if (x < 0 || x >= GRID_W || y < 0 || y >= GRID_H) return;
    int px = OFFSET_X + x * BLOCK_SIZE;
    int py = OFFSET_Y + y * BLOCK_SIZE;
    
    tft.fillRect(px, py, BLOCK_SIZE, BLOCK_SIZE, block_colors[color_idx]);
    if (BLOCK_SIZE > 8) {
        uint16_t border = (color_idx == 0) ? 0x18E3 : TFT_BLACK;
        tft.drawRect(px, py, BLOCK_SIZE, BLOCK_SIZE, border);
    }
}

void draw_grid() {
    tft.drawRect(OFFSET_X - 1, OFFSET_Y - 1, (GRID_W * BLOCK_SIZE) + 2, (GRID_H * BLOCK_SIZE) + 2, TFT_WHITE);
    
    for (int x = 0; x < GRID_W; x++) {
        for (int y = 0; y < GRID_H; y++) {
            draw_cell(x, y, screen[x][y]);
        }
    }
}

void draw_ui() {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    
    if (UI_X < OFFSET_X + (GRID_W * BLOCK_SIZE)) {
        tft.setCursor(UI_X, UI_Y);
        tft.print("SC:"); tft.print(tetris_score);
    } else {
        tft.setCursor(UI_X, UI_Y);
        tft.print("SCORE");
        tft.setCursor(UI_X, UI_Y + 10);
        tft.print(tetris_score);
        
        tft.setCursor(UI_X, UI_Y + 30);
        tft.print("LVL");
        tft.setCursor(UI_X, UI_Y + 40);
        tft.print(tetris_level);
    }
}

void draw_dynamic_block(Point pos, int rot, int color_idx) {
    for (int i = 0; i < 4; i++) {
        Point p = current_block.square[rot][i];
        draw_cell(pos.x + p.x, pos.y + p.y, color_idx);
    }
}

bool check_collision(Point pos, int rot) {
    for (int i = 0; i < 4; i++) {
        int x = pos.x + current_block.square[rot][i].x;
        int y = pos.y + current_block.square[rot][i].y;

        if (x < 0 || x >= GRID_W || y >= GRID_H) return true;
        if (y >= 0 && screen[x][y] != 0) return true;
    }
    return false;
}

void lock_block() {
    for (int i = 0; i < 4; i++) {
        int x = current_pos.x + current_block.square[current_rotation][i].x;
        int y = current_pos.y + current_block.square[current_rotation][i].y;
        if (x >= 0 && x < GRID_W && y >= 0 && y < GRID_H) {
            screen[x][y] = current_block.color_index;
        }
    }
}

void check_lines() {
    int lines_cleared = 0;
    for (int y = GRID_H - 1; y >= 0; y--) {
        bool full = true;
        for (int x = 0; x < GRID_W; x++) {
            if (screen[x][y] == 0) { full = false; break; }
        }

        if (full) {
            lines_cleared++;
            for (int ky = y; ky > 0; ky--) {
                for (int kx = 0; kx < GRID_W; kx++) {
                    screen[kx][ky] = screen[kx][ky - 1];
                }
            }
            for (int kx = 0; kx < GRID_W; kx++) screen[kx][0] = 0;
            y++; 
        }
    }

    if (lines_cleared > 0) {
        tetris_score += lines_cleared * 100 * lines_cleared;
        tetris_level = 1 + (tetris_score / 1000);
        speed_delay = max(100, 500 - (tetris_level * 30));
        draw_grid(); 
        draw_ui();
    }
}

void new_block() {
    current_block = blocks[random(0, 7)];
    current_pos = {GRID_W / 2 - 1, 0};
    current_rotation = 0;

    if (check_collision(current_pos, current_rotation)) {
        game_over = true;
        game_running = false;
    }
}

// --- MAIN FUNCTION ---
void playTetris() {
    InGameMode = true; 

    uint8_t old_rotation = tft.getRotation();

    if (tft.width() > tft.height()) {
        tft.setRotation(old_rotation + 1);
    }

    tft.fillScreen(TETRIS_BG_COLOR);
    calculate_layout(); 

    memset(screen, 0, sizeof(screen));
    tetris_score = 0;
    tetris_level = 1;
    speed_delay = 500;
    game_over = false;
    game_running = true;

    draw_grid();
    draw_ui();
    new_block();
    draw_dynamic_block(current_pos, current_rotation, current_block.color_index);

    unsigned long last_drop = millis();

    // GAME LOOP
    while (game_running) {
        // Handle Exit
        if (check(EscPress)) {
             game_running = false;
             break;
        }

        Point prev_pos = current_pos;
        int prev_rot = current_rotation;
        bool moved = false;

        // --- ĐỌC VÀ REMAP NÚT ---
        bool raw_Prev = check(PrevPress);
        bool raw_Next = check(NextPress);
        
        bool raw_Down = false;
        #if defined(DownPress)
        raw_Down = check(DownPress);
        #endif
        
        bool raw_Up = false;
        #if defined(UpPress)
        raw_Up = check(UpPress);
        #endif

        // Khởi tạo trạng thái nút thực tế (effective)
        bool eff_Prev = raw_Prev;
        bool eff_Next = raw_Next;
        bool eff_Down = raw_Down;
        bool eff_Up   = raw_Up;

        // Áp dụng cấu hình Game Button Mapping (0: Bình thường, 1: Nút ngược)
        if (bruceConfig.gameButtonMapping == 1) { 
            eff_Next = raw_Prev;
            eff_Prev = raw_Next;
            eff_Down = raw_Up;
            eff_Up   = raw_Down;
        }

        // --- XỬ LÝ INPUT ---
        if (eff_Prev) {
            if (!check_collision({current_pos.x - 1, current_pos.y}, current_rotation)) {
                current_pos.x--; moved = true;
            }
        }
        else if (eff_Next) {
            if (!check_collision({current_pos.x + 1, current_pos.y}, current_rotation)) {
                current_pos.x++; moved = true;
            }
        }
        else if (check(SelPress)) { 
            int new_rot = (current_rotation + 1) % current_block.num_rotations;
            if (!check_collision(current_pos, new_rot)) {
                current_rotation = new_rot; moved = true;
            }
        }
        
        if (eff_Down) { 
            if (!check_collision({current_pos.x, current_pos.y + 1}, current_rotation)) {
                current_pos.y++; moved = true; tetris_score++; 
                draw_ui();
            }
        }

        if (moved) {
            draw_dynamic_block(prev_pos, prev_rot, 0); 
            draw_dynamic_block(current_pos, current_rotation, current_block.color_index);
            delay(50);
        }

        // Gravity
        if (millis() - last_drop > speed_delay) {
            if (!check_collision({current_pos.x, current_pos.y + 1}, current_rotation)) {
                draw_dynamic_block(current_pos, current_rotation, 0); 
                current_pos.y++;
                draw_dynamic_block(current_pos, current_rotation, current_block.color_index); 
            } else {
                lock_block();
                check_lines();
                new_block();
                if (!game_over) {
                    draw_dynamic_block(current_pos, current_rotation, current_block.color_index);
                }
            }
            last_drop = millis();
        }

        // Game Over
        if (game_over) {
            // Make the box taller to fit the score
            tft.fillRect(OFFSET_X, tft.height()/2 - 25, GRID_W * BLOCK_SIZE, 55, TFT_RED);
            tft.setTextColor(TFT_WHITE);

            // "GAME OVER" text
            tft.drawCentreString("GAME OVER", tft.width()/2, tft.height()/2 - 15, 1);

            // Display the final score
            char score_str[16];
            sprintf(score_str, "Score: %d", tetris_score);
            tft.drawCentreString(score_str, tft.width()/2, tft.height()/2, 1);

            // "Press SEL" text
            tft.drawCentreString("Press SEL", tft.width()/2, tft.height()/2 + 15, 1);
            
            while(true) {
                if (check(SelPress)) { 
                    tft.fillScreen(TETRIS_BG_COLOR);
                    memset(screen, 0, sizeof(screen));
                    tetris_score = 0; tetris_level = 1; speed_delay = 500;
                    game_over = false;
                    draw_grid(); draw_ui(); new_block();
                    break; 
                } 
                if (check(EscPress)) {
                    game_running = false;
                    break;
                }
                delay(100);
            }
        }
        
        delay(10);
    }

    InGameMode = false; 

    tft.setRotation(old_rotation);
    tft.fillScreen(bruceConfig.bgColor); 
}
