#pragma once
#include <stdbool.h>

bool AreCirclesIntersecting(float x1, float y1, float r1,
    float x2, float y2, float r2);

// <-- FIX: Added IsAreaClicked declaration here -->
int IsAreaClicked(float area_center_x, float area_center_y,
    float area_width, float area_height,
    float click_x, float click_y);