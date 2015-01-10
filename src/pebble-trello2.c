#include <pebble.h>
#include <pebble-trello2.h>
#include <string.h>

char* strdup(const char* in) {
  char* t = malloc(strlen(in)+1);
  strcpy(t, in);
  return t;
}

char* strndup(const char *s, size_t n) {
  size_t len = strlen(s);
  if (len <= n)
    return strdup(s);
  char* o = malloc(n+1);
  strncpy(o, s, n);
  o[n] = 0;
  return o;
}


//// LIST


enum ElementState{
  STATE_UNCHECKED  = 0,
  // The state has been toggled to unchecked
  STATE_PENDING_UC = 1,

  STATE_CHECKED    = 2,
  // The state has been toggled to checked
  STATE_PENDING_C  = 3,
};

typedef enum ElementState ElementState;


ElementState intStateToState(int state) {
  if(state)
    return STATE_CHECKED;
  return STATE_UNCHECKED;
}

bool isCheckedState(ElementState s) {
  return s == STATE_CHECKED || s == STATE_PENDING_C;
}

bool isPendingState(ElementState s) {
  return s == STATE_PENDING_C || s == STATE_PENDING_UC;
}


typedef struct
{
  char** elements;
  char** subtitles;
  ElementState* elementState;
  int elementCount;
} List;


List* make_list(int elementCount) {
  List* l = malloc(sizeof(List) + 2*sizeof(char*)*elementCount);
  l->elementCount = elementCount;
  l->elements = sizeof(List)+ (void*)l;
  l->subtitles = elementCount * sizeof(char*)+ (void*)l->elements;
  memset(l->subtitles, 0, sizeof(char*)*elementCount);
  l->elementState = NULL;
  return l;
}

void list_load_item(List* list, int index, const char* str) {
  if(strlen(str) > 13) {
    list->elements[index] = strndup(str, 11);
    list->subtitles[index] = strdup(str+11);
  } else {
    list->elements[index] = strdup(str);
  }
}

static void empty_list(List* l) {
  if(l->elements == NULL)
    return;
  for(int i=0; i<l->elementCount; ++i) {
    free(l->elements[i]);
    free(l->subtitles[i]);
  }
  if(l->elementState)
    free(l->elementState);
  l->elementState = NULL;
  l->elements = NULL;
  l->elementCount = 0;
}

static void destroy_list(List* l) {
  empty_list(l);
  free(l);
}


typedef struct
{
  List* list;
  List** sublists;
} SimpleTree;

SimpleTree* make_simple_tree() {
  SimpleTree* tree = malloc(sizeof(SimpleTree));
  memset(tree, 0, sizeof(SimpleTree));
  return tree;
}

static void empty_simple_tree(SimpleTree* tree) {
  if(tree->list == NULL)
    return;
  for(int i=0; i<tree->list->elementCount; ++i) {
    destroy_list(tree->sublists[i]);
  }
  free(tree->sublists);
  tree->sublists = NULL;
  destroy_list(tree->list);
  tree->list = NULL;
}

void destroy_simple_tree(SimpleTree* tree) {
  empty_simple_tree(tree);
  free(tree);
}

// Boards -> lists
static SimpleTree* firstTree;
int selected_board_index, selected_list_index, selected_card_index, selected_checklist_index;


// cards -> checklists
static SimpleTree* secondTree;

static List* checklist = NULL;

static char* checklistID = NULL;


//// CUSTOMWINDOW

typedef struct {
  Window *window;
  List* content;
  SimpleMenuLayer* simplemenu;
  SimpleMenuItem* simpleMenuItem;
} CustomWindow;

void custom_window_create(CustomWindow* window) {
  window->window = window_create();
  window->content = NULL;
  window->simplemenu = NULL;
  window->simpleMenuItem = NULL;
}

void custom_window_destroy(CustomWindow* window) {
  if(window->window)
    window_destroy(window->window);
  window->window = NULL;
}

enum {
  CWINDOW_LOADING = 0,
  CWINDOW_BOARDS = 1,
  CWINDOW_LISTS = 2,
  CWINDOW_CARDS = 3,
  CWINDOW_CHECKLISTS = 4,
  CWINDOW_CHECKLIST = 5,
  CWINDOW_SIZE = 6
};


static CustomWindow windows[CWINDOW_SIZE];


#define NUMBER_IMAGES  4

typedef struct {
  uint32_t id;
  GBitmap* bitmap;
} LoadedBitmap;


LoadedBitmap loadedBitmaps[NUMBER_IMAGES] = {
  {RESOURCE_ID_TRELLO_BOX, NULL},
  {RESOURCE_ID_TRELLO_CHECKED, NULL},
  {RESOURCE_ID_TRELLO_PENDING, NULL},
  {RESOURCE_ID_TRELLO_LOGO, NULL}
};

enum {
  RES_IDX_TRELLO_BOX,
  RES_IDX_TRELLO_CHECKED,
  RES_IDX_TRELLO_PENDING,
  RES_IDX_TRELLO_LOGO
};


GBitmap* stateToIcon(ElementState s) {
  switch(s) {
    case STATE_PENDING_C:
    case STATE_PENDING_UC:
      return loadedBitmaps[RES_IDX_TRELLO_PENDING].bitmap;
    case STATE_CHECKED:
     return loadedBitmaps[RES_IDX_TRELLO_CHECKED].bitmap;
    case STATE_UNCHECKED:
     return loadedBitmaps[RES_IDX_TRELLO_BOX].bitmap;
  }
  return NULL;
}

static TextLayer *loading_text_layer;
static BitmapLayer *loading_bitmaps_layer;

static void loading_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
/*  GRect bounds = layer_get_bounds(window_layer);

  BitmapLayer* layer = bitmap_layer_create((GRect) { .origin = { 0, 0 }, .size = { bounds.size.w, 20 } });
  bitmap_layer_set_bitmap(layer, logo);
TODO: show logo
  */

  loading_text_layer = text_layer_create((GRect) { .origin = { 0, 28}, .size = { 144, 168-28 } });
  text_layer_set_text(loading_text_layer, (const char*)window_get_user_data(window));
  text_layer_set_overflow_mode(loading_text_layer, GTextOverflowModeWordWrap);
  text_layer_set_text_alignment(loading_text_layer, GTextAlignmentCenter);

  loading_bitmaps_layer = bitmap_layer_create((GRect) { .origin = { 58, 0}, .size = { 28, 28 } });
  bitmap_layer_set_bitmap(loading_bitmaps_layer, loadedBitmaps[RES_IDX_TRELLO_LOGO].bitmap);

  layer_add_child(window_layer, bitmap_layer_get_layer(loading_bitmaps_layer));
  layer_add_child(window_layer, text_layer_get_layer(loading_text_layer));

}

int32_t tuple_get_int(Tuple *t) {
  if(t->type == TUPLE_INT) {
    if(t->length == 2)
      return t->value->int16;
    if(t->length == 4)
      return t->value->int32;
    if(t->length == 1)
      return t->value->int8;
  }
  if(t->type == TUPLE_UINT){
    if(t->length == 2)
      return t->value->uint16;
    if(t->length == 4)
      return t->value->uint32;
    if(t->length == 1)
      return t->value->uint8;
  }
  return -1;
}

char* tuple_get_str(Tuple *t){
  return t->value->cstring;
}


void debug_print_tree(SimpleTree *tree) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "DEBUG TREE %X", (int)tree);
  for(int i=0; i<tree->list->elementCount; ++i) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "tree->list[%i]: %s", i, tree->list->elements[i]);
    for(int j=0; j< tree->sublists[i]->elementCount; ++j) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "tree->sublists[%i]->element[%i]: %s", i, j, tree->sublists[i]->elements[j]);
    }
  }
}

void deserialize_checklist(DictionaryIterator *iter, List** listRef) {
  Tuple *numElTuple = dict_find(iter, MESSAGE_NUMEL_DICT_KEY);
  if(!numElTuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Message without numels!");
    return;
  }
  int numberElements = tuple_get_int(numElTuple);


  if(*listRef != NULL) {
    destroy_list(*listRef);
  }

  *listRef = make_list(numberElements);
  (*listRef)->elementState = malloc(sizeof(bool)*numberElements);

  List* list = *listRef;
  for(int i=0; i<numberElements; ++i) {
    list_load_item(list, i, tuple_get_str(dict_find(iter, 2*i)));
    list->elementState[i] = intStateToState(tuple_get_int(dict_find(iter, 2*i+1)));
  }
  if(checklistID)
    free(checklistID);
  checklistID = strdup(tuple_get_str(dict_find(iter, MESSAGE_CHECKLISTID_DICT_KEY)));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Checklistid: %s", checklistID);
}


void deserialize_simple_tree(DictionaryIterator *iter, SimpleTree* tree) {
  empty_simple_tree(tree);
  Tuple *numElTuple = dict_find(iter, MESSAGE_NUMEL_DICT_KEY);
  if(!numElTuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Message without numels!");
    return;
  }

  int numberElements = tuple_get_int(numElTuple);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Number of elements in first list: %i", numberElements);
  tree->list = make_list(numberElements);
  tree->sublists = malloc(sizeof(List*)*numberElements);
  int msgPtr = 0;
  for(int i=0; i<tree->list->elementCount; ++i) {
    list_load_item(tree->list, i, tuple_get_str(dict_find(iter, msgPtr++)));
    int sublistSize = tuple_get_int(dict_find(iter, msgPtr++));
    tree->sublists[i] = make_list(sublistSize);
    for(int j=0; j<sublistSize; j++){
      list_load_item(tree->sublists[i], j, tuple_get_str(dict_find(iter, msgPtr++)));
    }
  }

  //debug_print_tree(tree);
}


static void list_window_unload(Window *window) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "list_window_unload: %X", (int)window);
  CustomWindow* cwindow = NULL;
  for(int i = CWINDOW_LOADING; i< CWINDOW_SIZE;++i) {
    if(windows[i].window == window) {
      cwindow = &windows[i];
      break;
    }
  }

  if(!cwindow) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "list_window_unload: Can't find custom window");
    return;
  }

  layer_remove_from_parent(simple_menu_layer_get_layer(cwindow->simplemenu));
  simple_menu_layer_destroy(cwindow->simplemenu);
  cwindow->simplemenu = NULL;

  if(cwindow->simpleMenuItem) {
    free(cwindow->simpleMenuItem);
    cwindow->simpleMenuItem = NULL;
  }
}

void createListWindow(CustomWindow *window, SimpleMenuLayerSelectCallback callback, const char* title) {

  window_set_window_handlers(window->window, (WindowHandlers) {
  .unload = list_window_unload,
  });

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Creating list window with %i Elements", window->content->elementCount);
  int numberElements = window->content->elementCount;

  window->simpleMenuItem = malloc(sizeof(SimpleMenuSection) + sizeof(SimpleMenuItem)* numberElements);
  SimpleMenuItem* boardMenuItems = window->simpleMenuItem;
  memset(boardMenuItems, 0, sizeof(SimpleMenuItem)*numberElements);


  SimpleMenuSection* boardSection = ((void*)window->simpleMenuItem)+sizeof(SimpleMenuItem)* numberElements;
  boardSection->items = boardMenuItems;
  boardSection->num_items = numberElements;
  boardSection->title = title;

  for(int i =0; i< numberElements ;++i ) {
    const char* element = window->content->elements[i];
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Element %i: %s", i, element);
    // No need to dup here. window->content will be destroyed AFTER boardmenuitems
    boardMenuItems[i].title = element;
    boardMenuItems[i].subtitle = window->content->subtitles[i];
    boardMenuItems[i].callback = callback;
    if(window->content->elementState)
      boardMenuItems[i].icon = stateToIcon(window->content->elementState[i]);
  }
  window->simplemenu = simple_menu_layer_create(layer_get_frame(window_get_root_layer(window->window)), window->window, boardSection, 1, window);
  Layer *window_layer = window_get_root_layer(window->window);
  layer_add_child(window_layer, simple_menu_layer_get_layer(window->simplemenu));
}

static void very_short_vibe() {
  static const uint32_t const segments[] = { 50 };
  VibePattern pat = {
    .durations = segments,
    .num_segments = ARRAY_LENGTH(segments),
  };
  vibes_enqueue_custom_pattern(pat);
}

static void menu_list_select_callback(int index, void *ctx) {
  very_short_vibe();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Menu list: selected index %i", index);
  window_set_user_data(windows[CWINDOW_LOADING].window, "Loading Cards...");
  window_stack_push(windows[CWINDOW_LOADING].window, true);

  // remove boards and lists from stack: memory constraints
  window_stack_remove(windows[CWINDOW_BOARDS].window, false);
  window_stack_remove(windows[CWINDOW_LISTS].window, false);
  custom_window_destroy(&windows[CWINDOW_BOARDS]);
  custom_window_destroy(&windows[CWINDOW_LISTS]);
  destroy_simple_tree(firstTree);
  firstTree = NULL;
  selected_list_index = index;


  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (iter == NULL) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "null iter at sending. Result: %u", result);
    return;
  }


  Tuplet tuples[3] = {
    TupletInteger(MESSAGE_TYPE_DICT_KEY, MESSAGE_TYPE_SELECTED_LIST),
    TupletInteger(MESSAGE_BOARDIDX_DICT_KEY, selected_board_index),

  TupletInteger(MESSAGE_LISTIDX_DICT_KEY, selected_list_index)};

  for(int i=0 ;i<3;++i)
    dict_write_tuplet(iter, &tuples[i]);

  dict_write_end(iter);

  app_message_outbox_send();
}

static void menu_checklists_select_callback(int index, void *ctx) {
  very_short_vibe();
  selected_checklist_index = index;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Checklists: selected index %i", index);

  window_set_user_data(windows[CWINDOW_LOADING].window, "Loading Checklist...");
  window_stack_push(windows[CWINDOW_LOADING].window, true);

  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (iter == NULL) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "null iter at sending. Result: %u", result);
    return;
  }


  Tuplet tuple = TupletInteger(MESSAGE_TYPE_DICT_KEY, MESSAGE_TYPE_SELECTED_CHECKLIST);
  dict_write_tuplet(iter, &tuple);

  Tuplet tuple2 = TupletInteger(MESSAGE_BOARDIDX_DICT_KEY, selected_board_index);
  dict_write_tuplet(iter, &tuple2);

  Tuplet tuple3 = TupletInteger(MESSAGE_LISTIDX_DICT_KEY, selected_list_index);
  dict_write_tuplet(iter, &tuple3);

  Tuplet tuple4 = TupletInteger(MESSAGE_CARDIDX_DICT_KEY, selected_card_index);
  dict_write_tuplet(iter, &tuple4);

  Tuplet tuple5 = TupletInteger(MESSAGE_CHECKLISTIDX_DICT_KEY, selected_checklist_index);
  dict_write_tuplet(iter, &tuple5);

  dict_write_end(iter);

  app_message_outbox_send();
}


ElementState toggleState(ElementState oldState) {
  if (oldState == STATE_CHECKED || oldState == STATE_PENDING_C)
    return STATE_PENDING_UC;
  if (oldState == STATE_UNCHECKED || oldState == STATE_PENDING_UC )
    return STATE_PENDING_C;
  return STATE_UNCHECKED;
}

static void menu_checklist_item_select_callback(int index, void* ctx) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Checklist: selected item %i", index);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "heap_bytes_used: %u, heap_bytes_free: %u", heap_bytes_used(), heap_bytes_free());
  
  if(isPendingState(checklist->elementState[index])) {
    vibes_double_pulse();
    return;
  }

  very_short_vibe();
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (iter == NULL) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "null iter at sending. Result: %u", result);
    return;
  }

  // set to waiting state

  checklist->elementState[index] = toggleState(checklist->elementState[index]);

  CustomWindow *cwindow = ctx;
  SimpleMenuItem* items = cwindow->simpleMenuItem;
  items[index].icon = stateToIcon(checklist->elementState[index]);
  layer_mark_dirty(simple_menu_layer_get_layer(windows[CWINDOW_CHECKLIST].simplemenu));


  Tuplet tuple = TupletInteger(MESSAGE_TYPE_DICT_KEY, MESSAGE_TYPE_SELECTED_ITEM);
  dict_write_tuplet(iter, &tuple);

  Tuplet tuple2 = TupletInteger(MESSAGE_ITEMIDX_KEY, index);
  dict_write_tuplet(iter, &tuple2);

  Tuplet tuple3 = TupletInteger(MESSAGE_ITEMSTATE_KEY, isCheckedState(checklist->elementState[index]));
  dict_write_tuplet(iter, &tuple3);

  dict_write_end(iter);

  app_message_outbox_send();
}

static void menu_cards_select_callback(int index, void *ctx) {
  very_short_vibe();
  selected_card_index = index;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Cards: selected index %i", index);
  windows[CWINDOW_CHECKLISTS].content = secondTree->sublists[index];
  createListWindow(&windows[CWINDOW_CHECKLISTS], menu_checklists_select_callback, secondTree->list->elements[index]);
  window_stack_push(windows[CWINDOW_CHECKLISTS].window, true);
  if(secondTree->sublists[index]->elementCount == 1) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Only one checklist, selecting...");
    window_stack_remove(windows[CWINDOW_CHECKLISTS].window, true);
    menu_checklists_select_callback(0, NULL);
  }
}

static void menu_board_select_callback(int index, void *ctx) {
  very_short_vibe();
  selected_board_index = index;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Menu board: selected index %i", index);
  windows[CWINDOW_LISTS].content = firstTree->sublists[index];
  createListWindow(&windows[CWINDOW_LISTS], menu_list_select_callback, firstTree->list->elements[index]);
  window_stack_push(windows[CWINDOW_LISTS].window, true);
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *type = dict_find(iter, MESSAGE_TYPE_DICT_KEY);
  Tuple *value = dict_find(iter, MESSAGE_VALUE_DICT_KEY);
  if(!type) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Message without type!");
    return;
  }
  switch(tuple_get_int(type)) {
    case MESSAGE_TYPE_BOARDS:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Got type boards!");
      deserialize_simple_tree(iter, firstTree);
      windows[CWINDOW_BOARDS].content = firstTree->list;
      createListWindow(&windows[CWINDOW_BOARDS], menu_board_select_callback, "Boards");
      window_stack_remove(windows[CWINDOW_LOADING].window, false);
      window_stack_push(windows[CWINDOW_BOARDS].window, true);
      if(firstTree->list->elementCount == 1) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Only one board, selecting...");
        window_stack_remove(windows[CWINDOW_BOARDS].window, true);
        menu_board_select_callback(0, NULL);
      }
      break;
    case MESSAGE_TYPE_CARDS:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Got type cards!");
      deserialize_simple_tree(iter, secondTree);
      windows[CWINDOW_CARDS].content = secondTree->list;
      createListWindow(&windows[CWINDOW_CARDS], menu_cards_select_callback, "Cards");
      window_stack_remove(windows[CWINDOW_LOADING].window, false);
      window_stack_push(windows[CWINDOW_CARDS].window, true);
      if(secondTree->list->elementCount == 1) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Only one card, selecting...");
        window_stack_remove(windows[CWINDOW_CARDS].window, true);
        menu_cards_select_callback(0, NULL);
      }
      break;
    case MESSAGE_TYPE_CHECKLIST:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Got type checklist!");
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Heap used: %u, Heap free: %u", heap_bytes_used(), heap_bytes_free ());
      deserialize_checklist(iter, &checklist);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Heap used2: %u, Heap free: %u", heap_bytes_used(), heap_bytes_free ());
      windows[CWINDOW_CHECKLIST].content = checklist;
      createListWindow(&windows[CWINDOW_CHECKLIST], menu_checklist_item_select_callback, "Checklist");
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Heap used3: %u, Heap free: %u", heap_bytes_used(), heap_bytes_free ());
      window_stack_push(windows[CWINDOW_CHECKLIST].window, true);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Heap used4: %u, Heap free: %u", heap_bytes_used(), heap_bytes_free ());
      window_stack_remove(windows[CWINDOW_LOADING].window, false);
      break;
    case MESSAGE_TYPE_INIT:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Got type init!");
      if(tuple_get_int(value) != 1) {
        text_layer_set_text(loading_text_layer, "Token missing. Please open settings on phone.");
      }
      break;
    case MESSAGE_TYPE_ITEM_STATE_CHANGED: {
      char* receivedChecklistId = tuple_get_str(dict_find(iter, MESSAGE_CHECKLISTID_DICT_KEY));
      if(strcmp(receivedChecklistId, checklistID) != 0) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Got outdated checklist update. %s, expected %s", receivedChecklistId, checklistID);
        return;
      }

      int itemidx = tuple_get_int(dict_find(iter, MESSAGE_ITEMIDX_KEY));
      int state = tuple_get_int(dict_find(iter, MESSAGE_ITEMSTATE_KEY));
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Raw state %u", state);
      checklist->elementState[itemidx] = intStateToState(state);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Set element state to %u", checklist->elementState[itemidx]);

      windows[CWINDOW_CHECKLIST].simpleMenuItem[itemidx].icon = stateToIcon(checklist->elementState[itemidx]);
      layer_mark_dirty(simple_menu_layer_get_layer(windows[CWINDOW_CHECKLIST].simplemenu));
      break;
    }
    case MESSAGE_TYPE_HTTP_FAIL: {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Got type http fail!");
      char* text = tuple_get_str(dict_find(iter, MESSAGE_FAILTEXT_DICT_KEY));
      window_set_user_data(windows[CWINDOW_LOADING].window, strdup(text));
      window_stack_pop_all(false);
      window_stack_push(windows[CWINDOW_LOADING].window, false);
    }
  }
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped!");
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Failed to Send! Reason:%u", reason);
}

static void app_message_init(void) {
  // Register message handlers
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_failed(out_failed_handler);
  // Init buffers
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}


static void loading_window_unload(Window *window) {
  if(loading_text_layer != NULL)
    text_layer_destroy(loading_text_layer);
}

static void init(void) {
  for(int i=0; i< NUMBER_IMAGES; ++i) {
    loadedBitmaps[i].bitmap = gbitmap_create_with_resource(loadedBitmaps[i].id);
  }
  app_message_init();

  firstTree = make_simple_tree();
  secondTree = make_simple_tree();
  for(int i=0; i<CWINDOW_SIZE;++i) {
    custom_window_create(&windows[i]);
  }
  window_set_user_data(windows[CWINDOW_LOADING].window, "Loading Boards...");
  window_set_window_handlers(windows[CWINDOW_LOADING].window, (WindowHandlers) {
    .load = loading_window_load,
    .unload = loading_window_unload,
  });
  const bool animated = true;
  window_stack_push(windows[CWINDOW_LOADING].window, animated);
}

static void deinit(void) {
  for(int i=0; i<CWINDOW_SIZE;++i) {
    custom_window_destroy(&windows[i]);
  }

  if(firstTree)
    destroy_simple_tree(firstTree);
  if(secondTree)
    destroy_simple_tree(secondTree);
  if(checklist)
    destroy_list(checklist);
  if(checklistID)
    free(checklistID);

  for(int i=0; i< NUMBER_IMAGES; ++i) {
    gbitmap_destroy(loadedBitmaps[i].bitmap);
  }
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", windows[CWINDOW_LOADING].window);

  app_event_loop();
  deinit();
}
