#pragma once

// Declares the function from utils.c so other files can use it
int IsAreaClicked(float area_center_x, float area_center_y,
    float area_width, float area_height,
    float click_x, float click_y);

void Main_Menu_Init(void);
void Main_Menu_Update(void);
void Main_Menu_Exit(void);