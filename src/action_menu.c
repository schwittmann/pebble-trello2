#include <pebble.h>
#include "action_menu.h"

ActionMenu *actionMenu = NULL;


void action_forward(ActionMenu *action_menu, const ActionMenuItem *action, void *context)  {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "clicked action, context:%X", (int)context );
  ActionCallback k = (ActionCallback)action_menu_item_get_action_data(action);
  k();
}

void add_action_menu(ActionCallback refresh, ActionCallback record, ActionCallback delete) {
	ActionMenuLevel *level = action_menu_level_create(4);
	ActionMenuConfig c = {NULL};
	c.root_level = level;
	action_menu_level_add_action(level, "Refresh", action_forward, refresh);
	if(PBL_IF_MICROPHONE_ELSE(true, false))
		action_menu_level_add_action(level, "Add item", action_forward, record);
	action_menu_level_add_action(level, "Delete checked items", action_forward, delete);
	actionMenu = action_menu_open(&c); 
}
