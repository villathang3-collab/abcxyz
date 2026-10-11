#include "tururururu.h"
#include "core/display.h"
#include "core/mykeyboard.h"
#include <globals.h>

// Final version: Restored classic UI rendering (No ghosting for >4 hearts), Fixed Bomb reset.

// --- Sprites (Global to be accessible everywhere) ---
TFT_eSprite red_fish_sprite = TFT_eSprite(&tft);
TFT_eSprite life_heart_sprite = TFT_eSprite(&tft);
TFT_eSprite bonus_food_sprite = TFT_eSprite(&tft);
TFT_eSprite big_shark_sprite = TFT_eSprite(&tft);
TFT_eSprite big_red_shark_sprite = TFT_eSprite(&tft);
TFT_eSprite score_bonus_sprite = TFT_eSprite(&tft);
TFT_eSprite bomb_sprite = TFT_eSprite(&tft);   
TFT_eSprite frenzy_sprite = TFT_eSprite(&tft); 

// --- Game Objects & State ---
enum ObjectType { ORANGE_FISH, RED_FISH, LIFE_HEART, BONUS_FOOD, SCORE_BONUS, BIG_RED_SHARK, BOMB_ITEM, FRENZY_ITEM };

struct GameObject {
    int x, y;
    ObjectType type;
    int width, height;
    int value; 
};

GameObject gameObjects[5];

// Player (Shark) Settings
int sharkX = 40, sharkY = 80;
int sharkSize = 14; 
int normalSharkW = 32, normalSharkH = 30;
int bigSharkW = 42, bigSharkH = 40;
bool sharkUp = false, sharkDown = false;

// Game State
int score = 0, prev_score = -1; 
int lives = 3, prev_lives = -1; 
bool isPoweredUp = false;
unsigned long powerUpStartTime = 0;
const unsigned long powerUpDuration = 8000; 

// Frenzy State
bool isFrenzy = false, prev_isFrenzy = false;
unsigned long frenzyStartTime = 0;
const unsigned long frenzyDuration = 5000; 

#define STEP (tftHeight) / 15 

// --- Functions ---

void initSprites() {
    sprite.deleteSprite();
    sprite.createSprite(normalSharkW, normalSharkH);
    sprite.fillScreen(bruceConfig.bgColor);
    sprite.fillEllipse(19, 17, 10, 5, TFT_DARKGREY); 
    sprite.fillTriangle(0, 10, 0, 22, 9, 17, TFT_DARKGREY); 
    sprite.fillTriangle(17, 6, 17, 14, 22, 14, TFT_DARKGREY); 
    sprite.fillCircle(25, 14, 1, TFT_BLACK); 
    sprite.fillTriangle(23, 18, 29, 18, 24, 21, TFT_RED); 

    big_shark_sprite.deleteSprite();
    big_shark_sprite.createSprite(bigSharkW, bigSharkH);
    big_shark_sprite.fillScreen(bruceConfig.bgColor);
    big_shark_sprite.fillEllipse(25, 23, 14, 7, TFT_DARKGREY);
    big_shark_sprite.fillCircle(23, 32, 7, TFT_LIGHTGREY);
    big_shark_sprite.fillTriangle(0, 14, 0, 30, 12, 23, TFT_DARKGREY);
    big_shark_sprite.fillTriangle(23, 8, 23, 20, 30, 20, TFT_DARKGREY);
    big_shark_sprite.fillCircle(35, 19, 2, TFT_YELLOW);

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

    big_red_shark_sprite.deleteSprite();
    big_red_shark_sprite.createSprite(40, 16); 
    big_red_shark_sprite.fillScreen(bruceConfig.bgColor);
    big_red_shark_sprite.fillEllipse(12, 8, 12, 6, TFT_RED);
    big_red_shark_sprite.fillTriangle(32, 0, 32, 16, 22, 10, TFT_RED);
    big_red_shark_sprite.fillCircle(6, 6, 2, TFT_BLACK);

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

    bomb_sprite.deleteSprite();
    bomb_sprite.createSprite(12, 12);
    bomb_sprite.fillScreen(bruceConfig.bgColor);
    bomb_sprite.fillCircle(6, 7, 5, TFT_DARKGREY); 
    bomb_sprite.fillCircle(6, 7, 2, TFT_RED);      
    bomb_sprite.fillRect(5, 0, 2, 3, TFT_ORANGE);  

    frenzy_sprite.deleteSprite();
    frenzy_sprite.createSprite(12, 12);
    frenzy_sprite.fillScreen(bruceConfig.bgColor);
    frenzy_sprite.fillTriangle(6, 0, 12, 6, 0, 6, TFT_MAGENTA);
    frenzy_sprite.fillTriangle(6, 12, 12, 6, 0, 6, TFT_MAGENTA);
    frenzy_sprite.fillCircle(6, 6, 2, TFT_WHITE); 

    score_bonus_sprite.deleteSprite();
    score_bonus_sprite.createSprite(20, 10);
}

void drawShark() {
    if (isPoweredUp) {
        big_shark_sprite.pushSprite(sharkX - 7, sharkY - 6);
    } else {
        sprite.pushSprite(sharkX - 14, sharkY - 7);
    }
}

void drawObject(GameObject &obj) {
    switch (obj.type) {
        case ORANGE_FISH:   draw.pushSprite(obj.x, obj.y); break;
        case RED_FISH:      red_fish_sprite.pushSprite(obj.x, obj.y); break;
        case LIFE_HEART:    life_heart_sprite.pushSprite(obj.x, obj.y); break;
        case BONUS_FOOD:    bonus_food_sprite.pushSprite(obj.x, obj.y); break;
        case BIG_RED_SHARK: big_red_shark_sprite.pushSprite(obj.x, obj.y); break;
        case BOMB_ITEM:     bomb_sprite.pushSprite(obj.x, obj.y); break;
        case FRENZY_ITEM:   frenzy_sprite.pushSprite(obj.x, obj.y); break;
        case SCORE_BONUS: {
            score_bonus_sprite.fillScreen(bruceConfig.bgColor);
            score_bonus_sprite.setTextDatum(MC_DATUM); 
            score_bonus_sprite.setTextColor(TFT_CYAN, bruceConfig.bgColor);
            score_bonus_sprite.drawString(String(obj.value), 10, 5, 1);
            score_bonus_sprite.pushSprite(obj.x, obj.y);
            break;
        }
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
    int drawX = isPoweredUp ? (sharkX - 7) : (sharkX - 14);
    int drawY = isPoweredUp ? (sharkY - 6) : (sharkY - 7);
    int sW = isPoweredUp ? bigSharkW : normalSharkW;
    int sH = isPoweredUp ? bigSharkH : normalSharkH;

    if (sharkDown) { 
        tft.fillRect(drawX, drawY, sW, STEP, bruceConfig.bgColor);
        sharkY += STEP; 
        sharkDown = false; 
    }
    if (sharkUp)   { 
        tft.fillRect(drawX, drawY + sH - STEP, sW, STEP, bruceConfig.bgColor);
        sharkY -= STEP; 
        sharkUp = false; 
    }

    int currentSharkSize = isPoweredUp ? (sharkSize * 2) : sharkSize;
    if (sharkY < 14) { sharkY = 14; } // Đẩy giới hạn Y xuống dưới thanh UI
    if (sharkY > tftHeight - currentSharkSize) { sharkY = tftHeight - currentSharkSize; }
}

void resetObject(GameObject &obj) {
    obj.x = tftWidth + random(20, 100);
    obj.y = random(14, tftHeight - 20); // Đẩy vật thể sinh ra thấp hơn thanh UI
    obj.value = 0; 

    int spawnRoll = random(100);
    
    if (isFrenzy) {
        if (spawnRoll < 30) { 
            obj.type = SCORE_BONUS; obj.width = 20; obj.height = 10; obj.value = 10; 
        } else { 
            obj.type = ORANGE_FISH; obj.width = 20; obj.height = 8; 
        }
        return;
    }

    if (spawnRoll < 2) {         
        obj.type = BOMB_ITEM; obj.width = 12; obj.height = 12;
    } else if (spawnRoll < 4) {  
        obj.type = FRENZY_ITEM; obj.width = 12; obj.height = 12;
    } else if (spawnRoll < 6) {  
        obj.type = LIFE_HEART; obj.width = 10; obj.height = 10;
    } else if (spawnRoll < 10) { 
        obj.type = BONUS_FOOD; obj.width = 12; obj.height = 12;
    } else if (spawnRoll < 14) { 
        obj.type = BIG_RED_SHARK; obj.width = 40; obj.height = 16;
    } else if (spawnRoll < 22) { 
        obj.type = SCORE_BONUS; obj.width = 20; obj.height = 10; obj.value = (random(4) + 1) * 5; 
    } else if (spawnRoll < 61) { 
        obj.type = RED_FISH; obj.width = 20; obj.height = 8;
    } else {                     
        obj.type = ORANGE_FISH; obj.width = 20; obj.height = 8;
    }
}

void moveObject(GameObject &obj) {
    tft.fillRect(obj.x + obj.width - 2, obj.y, 2, obj.height, bruceConfig.bgColor);
    
    obj.x -= 2;
    if (obj.x < -40) { 
        tft.fillRect(obj.x, obj.y, obj.width, obj.height, bruceConfig.bgColor);
        resetObject(obj);
    }
}

void checkCollisions() {
    int currentSharkSize = isPoweredUp ? (sharkSize * 2) : sharkSize;

    for (int i = 0; i < 5; i++) {
        bool collision = 
            (sharkX < gameObjects[i].x + gameObjects[i].width) && 
            (sharkX + currentSharkSize > gameObjects[i].x) &&
            (sharkY < gameObjects[i].y + gameObjects[i].height) && 
            (sharkY + currentSharkSize > gameObjects[i].y);

        if (collision) {
            // 1. XÓA NGAY đồ vật bị ăn khỏi màn hình
            tft.fillRect(gameObjects[i].x, gameObjects[i].y, gameObjects[i].width, gameObjects[i].height, bruceConfig.bgColor);
            
            // 2. Lưu lại Type & Value trước khi xóa gốc mảng
            ObjectType eatenType = gameObjects[i].type;
            int eatenVal = gameObjects[i].value;
            
            // 3. RESET ĐỂ MÓN ĐỒ BIẾN MẤT HOÀN TOÀN KHỎI MẢNG
            resetObject(gameObjects[i]);

            // 4. Mới chạy hiệu ứng
            switch (eatenType) {
                case ORANGE_FISH:   score++; break;
                case RED_FISH:      if (!isPoweredUp) lives--; break;
                case LIFE_HEART:    lives++; break;
                case BIG_RED_SHARK: if (!isPoweredUp) lives -= 2; break;
                case SCORE_BONUS:   score += eatenVal; break;
                case BONUS_FOOD:    
                    if (!isPoweredUp) { 
                        tft.fillRect(sharkX - 14, sharkY - 7, normalSharkW, normalSharkH, bruceConfig.bgColor);
                        isPoweredUp = true; 
                        powerUpStartTime = millis(); 
                    } 
                    break;
                case FRENZY_ITEM:
                    isFrenzy = true;
                    frenzyStartTime = millis();
                    for (int j = 0; j < 5; j++) {
                        if (gameObjects[j].type == RED_FISH || gameObjects[j].type == BIG_RED_SHARK) {
                            tft.fillRect(gameObjects[j].x, gameObjects[j].y, gameObjects[j].width, gameObjects[j].height, bruceConfig.bgColor);
                            gameObjects[j].type = ORANGE_FISH;
                            gameObjects[j].width = 20; gameObjects[j].height = 8;
                        }
                    }
                    break;
                case BOMB_ITEM:
                    // CHỈ CHỚP Ở VÙNG GAMEPLAY (chừa lại thanh UI 14px ở trên)
                    tft.fillRect(0, 14, tftWidth, tftHeight - 14, TFT_WHITE);
                    delay(30);
                    tft.fillRect(0, 14, tftWidth, tftHeight - 14, bruceConfig.bgColor);
                    
                    for (int j = 0; j < 5; j++) {
                        if (gameObjects[j].type == RED_FISH || gameObjects[j].type == BIG_RED_SHARK) {
                            score += 10;
                            resetObject(gameObjects[j]);
                        }
                    }
                    
                    // Buộc vẽ lại khu vực cá mập do màn hình nháy
                    tft.fillRect(sharkX - bigSharkW/2, sharkY - bigSharkH/2, bigSharkW, bigSharkH, bruceConfig.bgColor);
                    break;
            }
        }
    }
}

void displayUI() {
    // Bản gốc xịn xò: Chỉ gọi lại khi thay đổi, xử lý từng mảng rời rạc
    
    // UI Điểm
    if (score != prev_score) {
        tft.fillRect(0, 0, 80, 10, bruceConfig.bgColor);
        tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
        tft.setTextSize(FM);
        tft.setCursor(0, 0);
        tft.printf("Score: %d", score);
        prev_score = score;
    }

    // UI Frenzy
    if (isFrenzy != prev_isFrenzy) {
        tft.fillRect(tftWidth / 2 - 35, 0, 70, 12, bruceConfig.bgColor); // Xóa sạch vùng chứa chữ
        if (isFrenzy) {
            tft.setTextColor(TFT_MAGENTA, bruceConfig.bgColor);
            tft.drawCentreString("FRENZY!", tftWidth / 2, 0, 1);
        }
        prev_isFrenzy = isFrenzy;
    }

    // UI Tim
    if (lives != prev_lives) {
        // FIXED: TĂNG CHIỀU DÀI VÙNG XÓA LÊN 80 ĐỂ KHÔNG CHÉM TIM THỨ 5
        tft.fillRect(tftWidth - 80, 0, 80, 12, bruceConfig.bgColor);
        if (lives > 5) {
            life_heart_sprite.pushSprite(tftWidth - 30, 2);
            tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
            tft.setTextDatum(ML_DATUM);
            tft.drawString("x" + String(lives), tftWidth - 18, 6, 1);
        } else {
            for (int i = 0; i < lives; i++) {
                life_heart_sprite.pushSprite(tftWidth - 15 - (i * 12), 2);
            }
        }
        prev_lives = lives;
    }
}

void shark_setup() {
    score = 0; prev_score = -1;
    lives = 3; prev_lives = -1;
    sharkY = 80;
    isPoweredUp = false;
    isFrenzy = false; prev_isFrenzy = false;
    
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
            if (isPoweredUp && (millis() - powerUpStartTime > powerUpDuration)) {
                tft.fillRect(sharkX - 7, sharkY - 6, bigSharkW, bigSharkH, bruceConfig.bgColor);
                isPoweredUp = false;
            }

            if (isFrenzy && (millis() - frenzyStartTime > frenzyDuration)) {
                isFrenzy = false;
            }

            moveShark();
            for (int i = 0; i < 5; i++) { moveObject(gameObjects[i]); }

            checkCollisions(); 
            drawShark(); 
            for (int i = 0; i < 5; i++) { drawObject(gameObjects[i]); } 
            displayUI(); 
            
            time = millis();
        }

        if (isFrenzy) {
            downTime = 8; 
        } else {
            if (score < 10) downTime = 50; 
            else if (score < 20) downTime = 40; 
            else if (score < 30) downTime = 35;
            else if (score < 40) downTime = 30; 
            else if (score < 50) downTime = 25; 
            else downTime = 15; 
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
    sprite.deleteSprite();
    red_fish_sprite.deleteSprite();
    life_heart_sprite.deleteSprite();
    bonus_food_sprite.deleteSprite();
    big_shark_sprite.deleteSprite();
    big_red_shark_sprite.deleteSprite();
    score_bonus_sprite.deleteSprite();
    bomb_sprite.deleteSprite();
    frenzy_sprite.deleteSprite();
    delay(150);
}