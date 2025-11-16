#include "deck.h"
#include "cprocessing.h"
#include <stdlib.h>
#include <string.h>

void InitDeck(Deck* deck) {
    if (!deck) return;

    deck->size = 0;
    deck->capacity = MAX_DECK_SIZE;

    // Initialize with starter cards
    CP_Vector deck_pos = CP_Vector_Set(50, 600);

    // Create starter cards
    Card attack = {
        deck_pos,
        CP_Vector_Set(0, 0),
        Attack,
        None,
        5,
        "Deal 5 attack damage",
        CARD_W_INIT,
        CARD_H_INIT,
        false
    };

    Card heal = {
        deck_pos,
        CP_Vector_Set(0, 0),
        Heal,
        None,
        7,
        "Heal 7 health points",
        CARD_W_INIT,
        CARD_H_INIT,
        false
    };

    // Add starter cards
    for (int i = 0; i < 6; i++) {
        AddCardToDeck(deck, attack);
    }
    for (int i = 0; i < 4; i++) {
        AddCardToDeck(deck, heal);
    }
}

bool AddCardToDeck(Deck* deck, Card card) {
    if (!deck || deck->size >= deck->capacity) {
        return false;
    }

    deck->cards[deck->size] = card;
    deck->size++;
    return true;
}

bool RemoveCardFromDeck(Deck* deck, int index) {
    if (!deck || index < 0 || index >= deck->size) {
        return false;
    }

    // Shift all cards after the removed card
    for (int i = index; i < deck->size - 1; i++) {
        deck->cards[i] = deck->cards[i + 1];
    }

    deck->size--;
    return true;
}

Card GetDeckCard(Deck* deck, int index) {
    if (!deck || index < 0 || index >= deck->size) {
        // Return a default card if invalid
        Card default_card = {
            CP_Vector_Set(0, 0),
            CP_Vector_Set(0, 0),
            Attack,
            None,
            0,
            "Invalid",
            CARD_W_INIT,
            CARD_H_INIT,
            false
        };
        return default_card;
    }

    return deck->cards[index];
}

bool IsDeckFull(Deck* deck) {
    if (!deck) return true;
    return deck->size >= deck->capacity;
}

bool IsDeckEmpty(Deck* deck) {
    if (!deck) return true;
    return deck->size == 0;
}

int GetDeckSize(Deck* deck) {
    if (!deck) return 0;
    return deck->size;
}

void ClearDeck(Deck* deck) {
    if (!deck) return;
    deck->size = 0;
}