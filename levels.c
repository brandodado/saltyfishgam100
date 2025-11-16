#include "levels.h"

// ---------------- Level 1 ----------------
Enemy level1_enemies[] = {
    { "Goblin", 20, 20, 5, 1, true },
    { "Slime",  15, 15, 3, 0, true }
};
int level1_enemy_count = sizeof(level1_enemies) / sizeof(level1_enemies[0]);

// ---------------- Level 2 ----------------
Enemy level2_enemies[] = {
    { "Orc",   30, 30, 8, 2, true },
    { "Slime", 15, 15, 3, 0, true },
    { "Slime", 15, 15, 3, 0, true }
};
int level2_enemy_count = sizeof(level2_enemies) / sizeof(level2_enemies[0]);