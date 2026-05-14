#pragma once

typedef enum {
    MENU_ACTION_NONE = 0,
    MENU_ACTION_NEW_GAME
} MenuAction;

void menu_init(void);
MenuAction menu_update(void);
int menu_get_selected_map(void);
