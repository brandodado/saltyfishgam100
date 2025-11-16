#include <math.h>
#include "cprocessing.h"
#include "mainmenu.h"
#include "utils.h"
#include <stdbool.h>
#include "card.h"
#include "levels.h"
#include <stdio.h>
#include "game.h"
#include "reward.h"
#include "deck.h" 
#include "gameover.h" 

// ---------------------------------------------------------
// 1. GLOBAL VARIABLES
// ---------------------------------------------------------

int window_w;
int window_h;
int selected_card_index;

#define SELECT_BTN_W 200
#define SELECT_BTN_H 100
#define MAX_LEVEL 2 

CP_Vector select_btn_pos;
CP_Vector deck_pos;
CP_Vector discard_pos;

int played_cards;
int turn_num;
bool drawn;
int animate_index;
float max_bar_width;

#define MAX_HAND_SIZE 7      
#define MAX_DECK_SIZE 40 

Card hand[MAX_HAND_SIZE];
Card draw_pile[MAX_DECK_SIZE];
int draw_pile_size;
Card discard[MAX_DECK_SIZE];
int discard_size;
int hand_size;

Deck player_deck;
RewardState reward_state;

#define START_ATTACK 7
#define START_DEFENSE 2
#define INITIAL_DECK_SIZE 10

static Player player = { 50, 50, 7, 2 };
static int selected_enemy = 0;
static float hit_flash_timer[16] = { 0.0f };

static Enemy* current_enemies = NULL;
static int current_enemy_count = 0;
static int current_level = 1;

static bool stage_cleared = false;
static float banner_timer = 0.0f;
static bool reward_active = false;

static CP_Font game_font;
static CP_Image game_bg = NULL;

int level_num = 1;

float miss_timer = 0.0f;
float miss_x = 0.0f;     
float miss_y = 0.0f;

typedef enum {
    PHASE_PLAYER,
    PHASE_ENEMY
} BattlePhase;

BattlePhase current_phase = PHASE_PLAYER;
float enemy_turn_timer = 0.0f;
int enemy_action_index = 0;
float enemy_anim_offset_x = 0.0f;
bool enemy_has_hit = false;

// ---------------------------------------------------------
// 2. HELPER FUNCTIONS
// ---------------------------------------------------------

static bool AllEnemiesDefeated(void) {
    for (int i = 0; i < current_enemy_count; i++) {
        if (current_enemies[i].alive && current_enemies[i].health > 0) {
            return false;
        }
    }
    return true;
}

void DrawStageClearBanner(void) {
    CP_Settings_Fill(CP_Color_Create(255, 255, 0, 255));
    CP_Settings_TextSize(48);
    CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
    CP_Font_DrawText("STAGE CLEARED!", CP_System_GetWindowWidth() / 2, CP_System_GetWindowHeight() / 2);
}

void SyncDeckToArray(void) {
    draw_pile_size = 0;
    discard_size = 0;
    hand_size = 0;

    for (int i = 0; i < player_deck.size && i < MAX_DECK_SIZE; i++) {
        draw_pile[i] = player_deck.cards[i];
        draw_pile[i].pos = deck_pos;
    }
    draw_pile_size = player_deck.size;
}

void ResetStageState(void) {
    selected_enemy = -1;
    for (int i = 0; i < 16; i++) hit_flash_timer[i] = 0.0f;

    stage_cleared = false;
    reward_active = false;
    banner_timer = 0.0f;

    current_phase = PHASE_PLAYER;
    enemy_anim_offset_x = 0.0f;
    enemy_has_hit = false;

    ResetReward(&reward_state);
}

void ResetGame(void) {
    player.health = player.max_health;
    player.attack = START_ATTACK;
    player.defense = START_DEFENSE;

    draw_pile_size = INITIAL_DECK_SIZE;
    discard_size = 0;
    hand_size = 0;
    selected_card_index = -1;
    played_cards = 0;
    drawn = false;

    turn_num = 0;
    level_num = 1;
    current_enemy_count = 0;
    current_enemies = NULL;

    ResetStageState();
}

void LoadLevel(int level) {
    if (level > MAX_LEVEL) {
        ResetGame();
        CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
        return;
    }

    current_enemies = NULL;
    current_enemy_count = 0;
    current_level = level;

    SyncDeckToArray();

    drawn = false;
    turn_num = 0;
    played_cards = 0;
    current_phase = PHASE_PLAYER;

    if (level == 1) {
        current_enemies = level1_enemies;
        current_enemy_count = level1_enemy_count;
    }
    else if (level == 2) {
        current_enemies = level2_enemies;
        current_enemy_count = level2_enemy_count;
    }

    if (current_enemies && current_enemy_count > 0) {
        for (int i = 0; i < current_enemy_count; i++) {
            current_enemies[i].health = current_enemies[i].max_health;
            current_enemies[i].alive = true;
        }
    }

    ResetStageState();
}

static void HandleInput(void) {
    if (CP_Input_KeyTriggered(KEY_LEFT)) {
        selected_enemy--;
        if (selected_enemy < 0) selected_enemy = current_enemy_count - 1;
    }
    if (CP_Input_KeyTriggered(KEY_RIGHT)) {
        selected_enemy++;
        if (selected_enemy >= current_enemy_count) selected_enemy = 0;
    }
    if (CP_Input_KeyTriggered(KEY_SPACE)) {
        if (current_enemy_count > 0 && current_enemies[selected_enemy].alive) {
            current_enemies[selected_enemy].health -= player.attack;
            if (current_enemies[selected_enemy].health <= 0) current_enemies[selected_enemy].alive = false;
            if (selected_enemy >= 0 && selected_enemy < 16) hit_flash_timer[selected_enemy] = 0.2f;
        }
    }
    if (CP_Input_KeyTriggered(KEY_Q)) {
        CP_Engine_SetNextGameState(Main_Menu_Init, Main_Menu_Update, Main_Menu_Exit);
    }
}

static int UpdateStageClear(int level_num) {
    if (!stage_cleared && !reward_active && AllEnemiesDefeated() && current_enemy_count > 0) {
        stage_cleared = true;
        banner_timer = 2.0f;
    }
    if (stage_cleared) {
        if (banner_timer > 0.0f) {
            banner_timer -= CP_System_GetDt();
            DrawStageClearBanner();
            return 1;
        }
        else {
            stage_cleared = false;
            reward_active = true;
            GenerateRewardOptions(&reward_state);
            return 1;
        }
    }
    if (reward_active) {
        UpdateReward(&reward_state, &player_deck);
        DrawReward(&reward_state);
        if (reward_state.reward_claimed) {
            reward_active = false;
            LoadLevel(level_num + 1);
        }
        return 1;
    }
    return 0;
}

static void DrawEntity(const char* name, int health, int max_health,
    int attack, int defense, float x, float y, float w, float h,
    CP_Color model_color, int is_selected)
{
    // 1. Draw Box & Selection
    CP_Settings_RectMode(CP_POSITION_CORNER);
    if (is_selected) {
        CP_Settings_StrokeWeight(4.0f);
        CP_Settings_Stroke(CP_Color_Create(255, 255, 0, 255));
        CP_Graphics_DrawRect(x - 4, y - 4, w + 8, h + 8);
    }
    CP_Settings_StrokeWeight(0);

    int clamped_health = (health < 0) ? 0 : (health > max_health) ? max_health : health;
    float ratio = (max_health > 0) ? (float)clamped_health / (float)max_health : 0.0f;

    CP_Settings_Fill(model_color);
    CP_Graphics_DrawRect(x, y, w, h);

    // 2. Draw HP Bar
    float bar_h = 16.0f;
    float bar_y = y + h + 4.0f;
    CP_Settings_Fill(CP_Color_Create(70, 70, 70, 255));
    CP_Graphics_DrawRect(x, bar_y, w, bar_h);

    CP_Color hp_color = (ratio > 0.6f) ? CP_Color_Create(0, 200, 0, 255)
        : (ratio > 0.3f) ? CP_Color_Create(255, 200, 0, 255)
        : CP_Color_Create(200, 0, 0, 255);
    CP_Settings_Fill(hp_color);
    CP_Graphics_DrawRect(x, bar_y, w * ratio, bar_h);

    // 3. Draw Text (Name & HP)
    CP_Font_Set(game_font);
    CP_Settings_TextSize(20);
    CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);
    CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255));
    CP_Font_DrawText(name, x + w * 0.5f, y - 20.0f);

    char hp_text[32];
    snprintf(hp_text, sizeof(hp_text), "%d/%d", health, max_health);
    CP_Settings_TextSize(16);
    CP_Font_DrawText(hp_text, x + w * 0.5f, bar_y + bar_h * 0.5f + 2);

    // 4. Draw Stats (ATK & DEF)
    CP_Settings_TextSize(18);
    // FORCE ALIGNMENT LEFT for Stats
    CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_LEFT, CP_TEXT_ALIGN_V_TOP);

    float stats_y = bar_y + bar_h + 10.0f;

    // ATK (Red)
    CP_Settings_Fill(CP_Color_Create(255, 80, 80, 255));
    char atk_text[32]; snprintf(atk_text, sizeof(atk_text), "ATK: %d", attack);
    CP_Font_DrawText(atk_text, x, stats_y);

    // DEF (Blue) - Drawn to the right side of the card
    CP_Settings_Fill(CP_Color_Create(80, 120, 255, 255));
    char def_text[32]; snprintf(def_text, sizeof(def_text), "DEF: %d", defense);
    CP_Font_DrawText(def_text, x + (w * 0.5f) + 10, stats_y);
}

static void DrawPlayer(Player* p, float ww, float wh) {
    DrawEntity("Player", p->health, p->max_health,
        p->attack, p->defense,
        100, wh / 2 - 100, 150, 200,
        CP_Color_Create(50, 50, 150, 255),
        selected_enemy == -1);
}

// --- UPDATED DRAWHUD ---
static void DrawHUD(float ww, float wh) {
    char debug[128];
    snprintf(debug, sizeof(debug), "Level %d | Enemies %d", current_level, current_enemy_count);

    // RESET ALIGNMENT to LEFT so text doesn't draw off-screen
    CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_LEFT, CP_TEXT_ALIGN_V_TOP);

    CP_Settings_Fill(CP_Color_Create(255, 255, 0, 255));
    CP_Font_Set(game_font);
    CP_Settings_TextSize(30);
    CP_Font_DrawText(debug, 20, 35);
}

void UpdateEnemyTurn(void) {
    float dt = CP_System_GetDt();
    enemy_turn_timer += dt;

    if (enemy_action_index >= current_enemy_count) {
        current_phase = PHASE_PLAYER;
        turn_num++;
        played_cards = 0;
        drawn = false;
        enemy_anim_offset_x = 0.0f;
        enemy_has_hit = false;
        return;
    }

    Enemy* e = &current_enemies[enemy_action_index];

    if (!e->alive) {
        enemy_action_index++;
        enemy_turn_timer = 0.0f;
        return;
    }

    // Lunge Animation
    if (enemy_turn_timer < 0.3f) {
        float ratio = enemy_turn_timer / 0.3f;
        enemy_anim_offset_x = -200.0f * ratio;
    }
    else if (!enemy_has_hit) {
        int damage = e->attack - player.defense;
        if (damage < 0) damage = 0;
        player.health -= damage;

        enemy_has_hit = true;
        enemy_anim_offset_x = -200.0f;
    }
    else if (enemy_turn_timer < 0.6f) {
        float ratio = (enemy_turn_timer - 0.3f) / 0.3f;
        enemy_anim_offset_x = -200.0f * (1.0f - ratio);
    }
    else {
        enemy_action_index++;
        enemy_turn_timer = 0.0f;
        enemy_anim_offset_x = 0.0f;
        enemy_has_hit = false;
    }
}

// ---------------------------------------------------------
// 3. INITIALIZATION
// ---------------------------------------------------------

void Game_Init(void)
{
    game_bg = CP_Image_Load("Assets/background.jpg");
    game_font = CP_Font_Load("Assets/Roboto-Regular.ttf");
    if (game_font != 0) { CP_Font_Set(game_font); CP_Settings_TextSize(24); }

    deck_pos = CP_Vector_Set(50, 600);
    discard_pos = CP_Vector_Set(200, 600);

    // Initialize Master Deck
    Card attack = { deck_pos,CP_Vector_Set(0, 0), Attack, None, 5, "Deal 5 attack damage",CARD_W_INIT, CARD_H_INIT, false };
    Card heal = { deck_pos,CP_Vector_Set(0, 0), Heal, 7, None, "Heal 7 health points",CARD_W_INIT,CARD_H_INIT, false };

    player_deck.size = 0;
    int index = 0;
    for (int i = 0; i < 6; i++) player_deck.cards[index++] = attack;
    for (int i = 0; i < 4; i++) player_deck.cards[index++] = heal;
    player_deck.size = index;

    // Load Level 1
    current_level = 1;
    LoadLevel(current_level);
    InitReward(&reward_state);
}

// ---------------------------------------------------------
// 4. GAME UPDATE LOOP
// ---------------------------------------------------------

void Game_Update(void) {
    CP_Graphics_ClearBackground(CP_Color_Create(20, 25, 28, 255));
    float ww = (float)CP_System_GetWindowWidth();
    float wh = (float)CP_System_GetWindowHeight();
    float mx = CP_Input_GetMouseX();
    float my = CP_Input_GetMouseY();
    int mouse_clicked = CP_Input_MouseClicked();

    if (game_bg) CP_Image_Draw(game_bg, ww * 0.5f, wh * 0.5f, ww, wh, 255);

    // 1. STAGE CLEAR / REWARD CHECK
    if (UpdateStageClear(current_level) == 1) {
        return;
    }

    // 2. GAME OVER CHECK (Lose Condition)
    if (player.health <= 0) {
        ResetGame();
        CP_Engine_SetNextGameState(GameOver_Init, GameOver_Update, GameOver_Exit);
        return; // <--- IMPORTANT: Must return so we don't draw the game on top!
    }

    // 3. PHASE LOGIC
    if (current_phase == PHASE_ENEMY) {
        UpdateEnemyTurn();
    }
    else {
        HandleInput();
    }

    // 4. DRAWING
    DrawPlayer(&player, ww, wh);

    if (current_enemies && current_enemy_count > 0) {
        float enemy_width = 120.0f;
        float enemy_height = 160.0f;
        float spacing = 40.0f;
        float total_width = current_enemy_count * enemy_width + (current_enemy_count - 1) * spacing;
        float start_x = ww - total_width - 200;
        float y = wh / 2.0f - enemy_height / 2.0f;

        for (int i = 0; i < current_enemy_count; i++) {
            Enemy* e = &current_enemies[i];
            if (e->alive) {
                float x = start_x + i * (enemy_width + spacing);

                // Animation Offset
                if (current_phase == PHASE_ENEMY && i == enemy_action_index) {
                    x += enemy_anim_offset_x;
                }

                // Hit Flash
                CP_Color col = CP_Color_Create(120, 120, 120, 255);
                if (hit_flash_timer[i] > 0) {
                    hit_flash_timer[i] -= CP_System_GetDt();
                    col = CP_Color_Create(255, 0, 0, 255);
                }
                DrawEntity(e->name, e->health, e->max_health, e->attack, e->defense, x, y, enemy_width, enemy_height, col, selected_enemy == i);
            }
        }
    }

    DrawHUD(ww, wh);

    // --- MISS TEXT VISUAL ---
    if (miss_timer > 0.0f) {
        miss_timer -= CP_System_GetDt();

        // Text settings
        CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255)); // White text
        CP_Settings_TextSize(40);
        CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_V_MIDDLE);

        // Draw text, making it float up slightly
        CP_Font_DrawText("MISS!", miss_x, miss_y - (1.0f - miss_timer) * 50.0f);
    }

    // 5. CARD LOGIC
    CP_Settings_Stroke(CP_Color_Create(0, 0, 0, 255));
    CP_Settings_Fill(CP_Color_Create(145, 145, 145, 255));
    CP_Graphics_DrawRect(deck_pos.x, deck_pos.y, CARD_W_INIT, CARD_H_INIT);
    CP_Graphics_DrawRect(discard_pos.x, discard_pos.y, CARD_W_INIT, CARD_H_INIT);

    if (!drawn) {
        if (turn_num == 0) {
            for (int i = 0; i < 3; ++i) DealFromDeck(draw_pile, &hand[hand_size], &draw_pile_size, &hand_size);
        }
        else if (draw_pile_size > 0) {
            DealFromDeck(draw_pile, &hand[hand_size], &draw_pile_size, &hand_size);
        }
        SetHandPos(hand, hand_size);
        drawn = true;
    }

    if (draw_pile_size == 0) RecycleDeck(discard, draw_pile, &discard_size, &draw_pile_size);

    for (int i = 0; i < hand_size; ++i) {
        if (hand[i].is_animating) AnimateMoveCard(&hand[i]);
        if (i == selected_card_index) {
            CP_Settings_Stroke(CP_Color_Create(255, 255, 0, 255));
            CP_Settings_StrokeWeight(3.0f);
        }
        else {
            CP_Settings_Stroke(CP_Color_Create(0, 0, 0, 255));
            CP_Settings_StrokeWeight(1.0f);
        }
        DrawCard(&hand[i]);
    }

    float select_btn_x = ww - 150;
    float select_btn_y = wh - 100;
    if (selected_card_index >= 0 || played_cards > 0) {
        CP_Settings_RectMode(CP_POSITION_CENTER);
        CP_Settings_Fill(CP_Color_Create(100, 100, 100, 255));
        CP_Graphics_DrawRect(select_btn_x, select_btn_y, SELECT_BTN_W, SELECT_BTN_H);
    }

    if (selected_card_index >= 0) {
        float dx = hand[selected_card_index].pos.x - discard_pos.x;
        float dy = hand[selected_card_index].pos.y - discard_pos.y;
        if ((dx * dx + dy * dy) < 100.0f) {
            DiscardCard(hand, &selected_card_index, &hand_size, discard, &discard_size);
            selected_card_index = -1;
        }
    }

    // 6. CLICK LOGIC
    if (current_phase == PHASE_PLAYER && mouse_clicked) {

        // ... (Card Selection loop remains the same) ...
        for (int i = 0; i < hand_size; ++i) {
            if (IsAreaClicked(hand[i].pos.x, hand[i].pos.y, hand[i].card_w, hand[i].card_h, mx, my)) {
                SelectCard(i, &selected_card_index, hand);
                break;
            }
        }

        // Button Click
        if (IsAreaClicked(select_btn_x, select_btn_y, SELECT_BTN_W, SELECT_BTN_H, mx, my)) {

            // CASE 1: PLAY CARD
            if (selected_card_index >= 0) {
                if (selected_enemy >= 0 && selected_enemy < current_enemy_count && current_enemies[selected_enemy].alive) {

                    // --- MISS LOGIC START ---
                    bool is_miss = false;

                    // 1. Check for Miss (Only Attack cards can miss)
                    if (hand[selected_card_index].type == Attack) {
                        int chance = CP_Random_RangeInt(0, 99);
                        if (chance < 35) is_miss = true; // 35% Chance
                    }

                    if (is_miss) {
                        // 2. Calculate Position Manually (Since Enemy has no .pos)
                        float enemy_w = 120.0f;
                        float spacing = 40.0f;
                        float total_w = current_enemy_count * enemy_w + (current_enemy_count - 1) * spacing;
                        float start_x = ww - total_w - 200;

                        // Set "MISS" text location above the specific enemy
                        miss_x = start_x + selected_enemy * (enemy_w + spacing) + (enemy_w / 2.0f);
                        miss_y = wh / 2.0f - 100.0f;

                        miss_timer = 1.0f; // Start the text timer

                        // 3. Skip Damage (Do NOT call UseCard)
                        printf("ATTACK MISSED!\n");
                    }
                    else {
                        // HIT: Apply effects normally
                        UseCard(hand, &selected_card_index, &hand_size, &player, &current_enemies[selected_enemy]);
                    }

                    // 4. Discard Card (Happens regardless of Hit or Miss)
                    ++played_cards;
                    hand[selected_card_index].target_pos = discard_pos;
                    // --- MISS LOGIC END ---
                }
            }
            // CASE 2: END TURN
            else if (played_cards > 0) {
                played_cards = 3;
                SetHandPos(hand, hand_size);
            }
        }
    }

    if (current_phase == PHASE_PLAYER && played_cards == 3) {
        current_phase = PHASE_ENEMY;
        enemy_action_index = 0;
        enemy_turn_timer = 0.0f;
        enemy_anim_offset_x = 0.0f;
        selected_card_index = -1;
    }
}

void Game_Exit(void)
{
    CP_Image_Free(&game_bg);
}