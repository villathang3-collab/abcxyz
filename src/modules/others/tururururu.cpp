#include "tururururu.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include <globals.h>

// Final version, respecting the original high-performance rendering logic.
// The "magic" is partial screen updates: erase only the trails, then draw with pushSprite.

// --- Sprites (Global to be accessible everywhere) ---
TFT_eSprite red_fish_sprite = TFT_eSprite(&tft);
TFT_eSprite life_heart_sprite = TFT_eSprite(&tft);
TFT_eSprite bonus_food_sprite = TFT_eSprite(&tft);
TFT_eSprite big_shark_sprite = TFT_eSprite(&tft);

// --- Game Objects & State ---
enum ObjectType { ORANGE_FISH, RED_FISH, LIFE_HEART, BONUS_FOOD };

struct GameObject {
    int x, y;
    ObjectType type;
    int width, height;
};

GameObject gameObjects[5];

// Player (Shark) Settings
int sharkX = 40, sharkY = 80;
int sharkW = 32, sharkH = 30; // Current dimensions
int normalSharkW = 32, normalSharkH = 30;
int bigSharkW = 42, bigSharkH = 40;
bool sharkUp = false, sharkDown = false;

// Game State
int score = 0, prev_score = -1; // Track score changes
int lives = 3, prev_lives = -1; // Track lives changes
bool isPoweredUp = false;
unsigned long powerUpStartTime = 0;
const unsigned long powerUpDuration = 8000; // 8 seconds

#define STEP (tftHeight) / 22 // Increased step for more responsive movement

// --- Functions ---

void initSprites() {
    // Normal Shark Sprite
    sprite.deleteSprite();
    sprite.createSprite(normalSharkW, normalSharkH);
    sprite.fillScreen(bruceConfig.bgColor);
    sprite.fillEllipse(19, 17, 10, 5, TFT_DARKGREY); // Body
    sprite.fillTriangle(0, 10, 0, 22, 9, 17, TFT_DARKGREY); // Tail
    sprite.fillTriangle(17, 6, 17, 14, 22, 14, TFT_DARKGREY); // Fin
    sprite.fillCircle(25, 14, 1, TFT_BLACK); // Eye
    sprite.fillTriangle(23, 18, 29, 18, 24, 21, TFT_RED); // Mouth

    // Big (Powered-up) Shark Sprite
    big_shark_sprite.deleteSprite();
    big_shark_sprite.createSprite(bigSharkW, bigSharkH);
    big_shark_sprite.fillScreen(bruceConfig.bgColor);
    big_shark_sprite.fillEllipse(25, 23, 14, 7, TFT_DARKGREY);
    big_shark_sprite.fillCircle(23, 32, 7, TFT_LIGHTGREY);
    big_shark_sprite.fillTriangle(0, 14, 0, 30, 12, 23, TFT_DARKGREY);
    big_shark_sprite.fillTriangle(23, 8, 23, 20, 30, 20, TFT_DARKGREY);
    big_shark_sprite.fillCircle(35, 19, 2, TFT_YELLOW);

    // Orange & Red Fish Sprites
    draw.deleteSprite();
    draw.createSprite(20, 8);
    draw.fillScreen(bruceConfig.bgColor);
    draw.fillEllipse(6, 4, 6, 3, TFT_ORANGE);
    draw.fillTriangle(16, 0, 16, 8, 11, 5, TFT_ORANGE);
    draw.fillCircle(3, 3, 1, TFT_BLACK);

    red_fish_sprite.deleteSprite();
    red_fish_sprite.createSprite(20, 8);
    red_fish_sprite.fillScreen(bruceConfig.bgColor);
    red_fish_sprite.fillEllipse(6, 4, 6, 3, TFT_RED);
    red_fish_sprite.fillTriangle(16, 0, 16, 8, 11, 5, TFT_RED);
    red_fish_sprite.fillCircle(3, 3, 1, TFT_BLACK);

    // Life Heart & Bonus Food Sprites
    life_heart_sprite.deleteSprite();
    life_heart_sprite.createSprite(10, 10);
    life_heart_sprite.fillScreen(bruceConfig.bgColor);
    life_heart_sprite.fillCircle(3, 3, 3, TFT_RED);
    life_heart_sprite.fillCircle(7, 3, 3, TFT_RED);
    life_heart_sprite.fillTriangle(0, 3, 10, 3, 5, 9, TFT_RED);

    bonus_food_sprite.deleteSprite();
    bonus_food_sprite.createSprite(12, 12);
    bonus_food_sprite.fillScreen(bruceConfig.bgColor);
    bonus_food_sprite.fillCircle(6, 6, 5, TFT_YELLOW);
    bonus_food_sprite.fillCircle(6, 6, 3, TFT_GOLD);
}

void drawShark() {
    if (isPoweredUp) {
        big_shark_sprite.pushSprite(sharkX - bigSharkW/2, sharkY - bigSharkH/2);
    } else {
        sprite.pushSprite(sharkX - normalSharkW/2, sharkY - normalSharkH/2);
    }
}

void drawObject(GameObject &obj) {
    switch (obj.type) {
        case ORANGE_FISH: draw.pushSprite(obj.x, obj.y); break;
        case RED_FISH:    red_fish_sprite.pushSprite(obj.x, obj.y); break;
        case LIFE_HEART:  life_heart_sprite.pushSprite(obj.x, obj.y); break;
        case BONUS_FOOD:  bonus_food_sprite.pushSprite(obj.x, obj.y); break;
    }
}

void detectInputs() {
#if defined(ARDUINO_M5STICK_C_PLUS) || defined(ARDUINO_M5STICK_C_PLUS2)
    if (check(SelPress))
#else
    if (check(PrevPress) || check(UpPress))
#endif
        sharkUp = true;
    if (check(NextPress) || check(DownPress)) sharkDown = true;
}

void moveShark() {
    // Chỉ xóa phần vệt dư (trail) khi cá mập di chuyển lên/xuống
    if (sharkDown) { 
        // Xóa dải màu ở đỉnh đầu cá
        tft.fillRect(sharkX - sharkW/2, sharkY - sharkH/2, sharkW, STEP, bruceConfig.bgColor);
        sharkY += STEP; 
        sharkDown = false; 
    }
    if (sharkUp)   { 
        // Xóa dải màu ở dưới đáy cá
        tft.fillRect(sharkX - sharkW/2, sharkY + sharkH/2 - STEP, sharkW, STEP, bruceConfig.bgColor);
        sharkY -= STEP; 
        sharkUp = false; 
    }

    if (sharkY < sharkH/2) sharkY = sharkH/2;
    if (sharkY > tftHeight - sharkH/2) sharkY = tftHeight - sharkH/2;
}

void resetObject(GameObject &obj) {
    obj.x = tftWidth + random(20, 100);
    obj.y = random(10, tftHeight - 20);
    int spawnRoll = random(70);
    if (spawnRoll == 0)      { obj.type = LIFE_HEART;  obj.width = 10; obj.height = 10; }
    else if (spawnRoll < 4)  { obj.type = BONUS_FOOD;  obj.width = 12; obj.height = 12; }
    else if (spawnRoll < 18) { obj.type = RED_FISH;    obj.width = 20; obj.height = 8;  }
    else                     { obj.type = ORANGE_FISH; obj.width = 20; obj.height = 8;  }
}

void moveObject(GameObject &obj) {
    // Chỉ xóa 2 pixel viền đuôi phía sau vật thể (tránh giật màn hình)
    tft.fillRect(obj.x + obj.width - 2, obj.y, 2, obj.height, bruceConfig.bgColor);
    
    obj.x -= 2;
    if (obj.x < -20) {
        // Xóa dứt điểm khi vật thể trôi hẳn ra khỏi rìa trái màn hình
        tft.fillRect(obj.x, obj.y, obj.width, obj.height, bruceConfig.bgColor);
        resetObject(obj);
    }
}

void checkCollisions() {
    for (int i = 0; i < 5; i++) {
        if (sharkX - sharkW/2 < gameObjects[i].x + gameObjects[i].width &&
            sharkX + sharkW/2 > gameObjects[i].x &&
            sharkY - sharkH/2 < gameObjects[i].y + gameObjects[i].height &&
            sharkY + sharkH/2 > gameObjects[i].y) {
            
            // Xóa ngay lập tức vật thể bị ăn
            tft.fillRect(gameObjects[i].x, gameObjects[i].y, gameObjects[i].width, gameObjects[i].height, bruceConfig.bgColor);
            
            switch (gameObjects[i].type) {
                case ORANGE_FISH: score++; break;
                case RED_FISH:    if (!isPoweredUp) lives--; break;
                case LIFE_HEART:  if (lives < 3) lives++; break;
                case BONUS_FOOD:  if (!isPoweredUp) { isPoweredUp = true; powerUpStartTime = millis(); } break;
            }
            resetObject(gameObjects[i]);
        }
    }
}

void displayUI() {
    // Only update score if it has changed
    if (score != prev_score) {
        tft.fillRect(0, 0, 80, 10, bruceConfig.bgColor);
        tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
        tft.setTextSize(FM);
        tft.setCursor(0, 0);
        tft.printf("Score: %d", score);
        prev_score = score;
    }
    // Only update lives if they have changed
    if (lives != prev_lives) {
        tft.fillRect(tft.width() - 45, 0, 45, 12, bruceConfig.bgColor);
        for (int i = 0; i < lives; i++) {
            life_heart_sprite.pushSprite(tft.width() - 15 - (i * 12), 2);
        }
        prev_lives = lives;
    }
}

void shark_setup() {
    score = 0; prev_score = -1;
    lives = 3; prev_lives = -1;
    sharkY = 80;
    isPoweredUp = false;
    
    initSprites();

    for (int i = 0; i < 5; i++) {
        resetObject(gameObjects[i]);
    }

    tft.fillScreen(bruceConfig.bgColor);
    shark_loop();
}

void shark_loop() {
    int downTime = 50;
    unsigned long time = 0;

    for (;;) {
        detectInputs();

        if (millis() - time > downTime) {
            // --- LOGIC ---
            bool justPoweredDown = false;
            if (isPoweredUp && (millis() - powerUpStartTime > powerUpDuration)) {
                isPoweredUp = false;
                justPoweredDown = true;
            }

            // --- ERASE ---
            if(justPoweredDown) {
                // Xóa con cá mập to một lần duy nhất khi hết hiệu ứng
                 tft.fillRect(sharkX - bigSharkW/2, sharkY - bigSharkH/2, bigSharkW, bigSharkH, bruceConfig.bgColor);
            }
            
            // --- MOVEMENT ---
            moveShark();
            for (int i = 0; i < 5; i++) { moveObject(gameObjects[i]); }

            // --- UPDATE SHARK SIZE ---
            sharkW = isPoweredUp ? bigSharkW : normalSharkW;
            sharkH = isPoweredUp ? bigSharkH : normalSharkH;

            // --- COLLISION & DRAWING ---
            checkCollisions();
            displayUI();
            drawShark();
            for (int i = 0; i < 5; i++) { drawObject(gameObjects[i]); }
            
            time = millis();
        }

        if (lives <= 0) {
            tft.fillScreen(bruceConfig.bgColor);
            tft.setTextColor(TFT_RED, bruceConfig.bgColor);
            tft.drawCentreString("GAME OVER", tftWidth / 2, tftHeight / 2 - 20, 2);
            char score_str[20];
            sprintf(score_str, "Score: %d", score);
            tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
            tft.drawCentreString(score_str, tftWidth / 2, tftHeight / 2 + 10, 1);
            tft.drawCentreString("SEL to Restart", tftWidth / 2, tftHeight / 2 + 30, 1);
            
            while (true) {
                if (check(SelPress)) { shark_setup(); return; }
                if (check(EscPress)) { returnToMenu = true; goto Exit; }
                delay(100);
            }
        }

        if (check(EscPress)) { returnToMenu = true; goto Exit; }
    }

Exit:
    // Dọn dẹp RAM trước khi thoát về BW16 menu
    sprite.deleteSprite();
    red_fish_sprite.deleteSprite();
    life_heart_sprite.deleteSprite();
    bonus_food_sprite.deleteSprite();
    big_shark_sprite.deleteSprite();
    delay(150);
}