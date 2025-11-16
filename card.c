#include "cprocessing.h"
#include "card.h"
#include "utils.h"
#include <stdlib.h>
#include "levels.h"
#include "game.h"	
#include <stdio.h>

//CP_Image card_type_icon[3];
//CP_Image card_effect_icon[5];

void DrawCard(Card* hand, CP_Image* type, CP_Image * effect) {
	// check type for color
	switch (hand -> type) {
	case Attack:
		CP_Settings_Fill(CP_Color_Create(255, 0, 0, 255));
		break;

	case Heal:
		CP_Settings_Fill(CP_Color_Create(80, 172, 85, 255));
		break;

	case Shield:
		CP_Settings_Fill(CP_Color_Create(0, 0, 255, 255));
		break;

	default:
		CP_Settings_Fill(CP_Color_Create(100, 100, 100, 255));
		break;
	}
	// set rect to draw from center
	CP_Settings_RectMode(CP_POSITION_CENTER);
	CP_Graphics_DrawRect(hand->pos.x, hand->pos.y, hand->card_w, hand->card_h);

	/*CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255));
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_CENTER, CP_TEXT_ALIGN_H_CENTER);
	CP_Font_DrawText(hand->description, hand->pos.x, hand->pos.y);
	CP_Settings_TextAlignment(CP_TEXT_ALIGN_H_LEFT, CP_TEXT_ALIGN_H_CENTER);	*/
	char power_str[4];
	char effect_str[4];
	CP_Settings_Fill(CP_Color_Create(255, 255, 255, 255));
	CP_Settings_TextSize(30); // see what font size team wants first
	snprintf(power_str, 4, "%d", (int)hand->power);
	/*snprintf(effect_str, 4, "%d", hand->effect_pow);*/
	CP_Image_Draw(type[hand->type], hand->pos.x - 20, hand->pos.y - 30, 35, 35, 255);
	CP_Font_DrawText(power_str, hand->pos.x, hand->pos.y - 25);
	if (hand->effect != None) {
		CP_Image_Draw(effect[hand->effect], hand->pos.x - 20, hand->pos.y + 30, 35, 35, 255);
		/*CP_Font_DrawText(effect_str, hand->pos.x, hand->pos.y);*/
	}
	CP_Settings_TextSize(25); // default
}

// Maybe switch to pointer for select index
void SelectCard(int index, int *selected) {
	// if selected card is selected again, turn select flag to point to no card
	if (*selected == index) {
		*selected = -1;
		return;
	}
	// else change index
	// change selected index
	*selected = index;

}

// calculate hand width for hand position
float HandWidth(Card hand[], int size, float margin) {
	float width = 0;
	//loop througn the hand and sum up card with with margin
	for (int i = 0; i < size; ++i) {
		width += (i == size - 1) ? hand[i].card_w : hand[i].card_w + margin; // if last card just add card width to the sum
	}
	return width;
}

void SetHandPos(Card *hand, int hand_size) {
	float rectdelta = 0; // distance of the x coord of the middle of the first card
	float hand_margin = 20;
	float total_hand_width = HandWidth(hand, hand_size, hand_margin); 
	// calculate x coord of start of hand by taking the window width - width of hand - 2 to get starting of the hand coord
	// we add hand[0].card_w to get the middle of first card
	float hand_x = ((CP_System_GetWindowWidth() - total_hand_width) / 2.0f) + (hand[0].card_w / 2); 
	float hand_y = 600;

	for (int i = 0; i < hand_size; ++i) {
		hand[i].target_pos = CP_Vector_Set(hand_x + rectdelta, hand_y); 
		hand[i].is_animating = true; // enable animation for the card to move to target pos
		rectdelta += hand_margin + hand[i].card_w; // set new distance from middle of first card
	}
}

void ShuffleDeck(Card *deck, int deck_size) {
	// loop through deck from the back
	for (int i = deck_size -1; i>0; i--){
		// get swap index.
		// get value [0, i]. so swapped indexes does not get swapped again
		// special : it can choose itself
		int j = rand() % (i + 1); 
		// swap
		Card temp = deck[i];
		deck[i] = deck[j];
		deck[j] = temp;
	}
}


void DealFromDeck(Card *deck, Card *hand, int* deck_size, int* hand_size) {
	// wait i shuffle each draw??? why
	// comment out maybe
	if (*deck_size > 1) {
		ShuffleDeck(deck, *deck_size);
	}

	// pass first card in deck to hand
	hand[*hand_size] = deck[0];
	// move rest of elements up one slot
	for (int i = 0; i < *deck_size-1; ++i) {
		deck[i] = deck[i + 1];
	}
	
	// decrement deck size and increment hand size
	--(*deck_size);
	++(*hand_size);
}

void UseCard(Card* hand, int* selected_index, int* hand_size, Player* player_ptr, Enemy* enemy_ptr ,Card *deck, int* deck_size) {
	/*hand[*selected_index].is_animating = true;*/
	// maybe change to switch case?
	// check type of card and do action
	if (hand[*selected_index].type == Attack) {
		// if enemy is alive deal damage
		if (enemy_ptr->alive) {
			enemy_ptr->health -= hand[*selected_index].power;
		}
		// if health goes below 0 set enemy to not alive
		if (enemy_ptr->health <= 0) {
			enemy_ptr->alive = false;
			// set health to 0
			enemy_ptr->health = 0;
		}
	}
	if (hand[*selected_index].type == Heal) {
		// heal player health
		player_ptr->health += 7;

		// todo: handle overheal
		if (player_ptr->health > player_ptr->max_health) {
			player_ptr->health = player_ptr->max_health;
		}
	}
	/*if (hand[*selected_index].type == Shield) {
		player_hp += 4;
	}*/
	
	// todo: check if card effect if != None. then do card effect 
	switch (hand[*selected_index].effect) {
	case Draw:
		DealFromDeck(deck, hand, deck_size, hand_size);
		SetHandPos(hand, *hand_size);
		break;
	case DOT:
		enemy_ptr->dot_timing = 3;
		break;
	default:
		break;
	}

	
}

void DiscardCard(Card *hand, int* selected_index, int* hand_size, Card *discard, int* discard_size) {
	// add selected card to discard pile
	discard[*discard_size] = hand[*selected_index];
	// increment discard size
	++(*discard_size);

	// remove card from hand array
	for (int i = *selected_index; i < *hand_size - 1; ++i) {
		// move card up to replace the used card
		hand[i] = hand[i + 1];
	}


	// decrement hand size
	--(*hand_size);
	// reset select index
	*selected_index = -1;
}

// assume deck size 0
// can use pointers. or pass to deck randomly using rand()
void RecycleDeck(Card *discard, Card *deck, int* discard_size, int*deck_size) {
	for (int i = 0; i < *discard_size; ++i) {
		deck[i] = discard[i];
		deck[i].pos = CP_Vector_Set(50, 600);
		++(*deck_size);
	}
	*discard_size = 0;
	ShuffleDeck(deck, *deck_size);
}

//todo: card animation
void AnimateMoveCard(Card *hand) {
	// init dt for frame based animation
	float dt = CP_System_GetDt();
	// speed card move for animation
	float speed = 900;
	// get the direction vector in terms of how far the card is from the target position
	CP_Vector direction = CP_Vector_Subtract(hand->target_pos, hand->pos);
	// get euclidean distance of card from pos through the direction vector
	float distance = CP_Vector_Length(direction);

	// if very close to  target pos, snap to target pos
	if (distance < 1.0f) {
		hand->pos = hand->target_pos;
		// set animating flag to no
		hand->is_animating = false;
		return;
	}

	// init movement vector to the normalized direction vector  scaled to speed * dt to move with frame rate
	CP_Vector move = CP_Vector_Scale(CP_Vector_Normalize(direction), speed * dt);

	// if movement vector will move pass the distance snap to target pos
	if (CP_Vector_Length(move) > distance) {
		hand->pos = hand->target_pos;
	}
	else { // move the card
		hand->pos = CP_Vector_Add(hand->pos, move);
	}
}

//void LoadCardIcon(void) {
//	card_type_icon[Attack] = CP_Image_Load("Assets/sword.png");
//	card_type_icon[Heal] = CP_Image_Load("Assets/suit_hearts.png");
//
//	card_effect_icon[DOT] = CP_Image_Load("Assets/fire.png");
//	card_effect_icon[Draw] = CP_Image_Load("Assets/card_add.png");
//}

