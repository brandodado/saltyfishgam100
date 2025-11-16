#pragma once
// ---------------- Player Struct ----------------
typedef struct {
    int health;
    int max_health;
    int attack;
    int defense;
} Player;

#ifndef GAME_H
#define GAME_H

void LoadLevel(int level);   // Declaration

void ResetGame(void);        // If you call ResetGame from menu

void ResetStageState(void);

void Game_Init(void);
void Game_Update(void);
void Game_Exit(void);

#endif