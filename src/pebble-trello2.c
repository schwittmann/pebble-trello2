#include <pebble.h>
#include <pebble-trello2.h>
#include <string.h>

void* iso_realloc(void* ptr, size_t size) {
  if (ptr != NULL) {
    return realloc(ptr, size);
  } else {
    return malloc(size);
  }
}

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


/*
Deployment builts:


#undef APP_LOG
#define APP_LOG(args...)

*/

enum ApplicationState {
  APPSTATE_DISPLAY_ERROR, // dead end, no transitions

  APPSTATE_LOADING_BOARDS,
  APPSTATE_DISPLAY_BOARDS,
  APPSTATE_DISPLAY_LISTS,

  APPSTATE_LOADING_CARDS,
  APPSTATE_DISPLAY_CARDS,
  APPSTATE_DISPLAY_CHECKLISTS,

  APPSTATE_LOADING_CHECKLIST,
  APPSTATE_DISPLAY_CHECKLIST
};

typedef enum ApplicationState ApplicationState;


static ApplicationState applicationState = APPSTATE_LOADING_BOARDS;


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

GBitmap* stateToIcon(ElementState s);

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
  ElementState* elementState;
  int elementCount;
} List;


List* make_list(int elementCount) {
  List* l = malloc(sizeof(List) + sizeof(char*)*elementCount);
  l->elementCount = elementCount;
  l->elements = sizeof(List)+ (void*)l;
  l->elementState = NULL;
  return l;
}

void list_load_item(List* list, int index, const char* str) {
    list->elements[index] = strdup(str);
}

static void empty_list(List* l) {
  if(l->elements == NULL)
    return;
  for(int i=0; i<l->elementCount; ++i) {
    free(l->elements[i]);
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

struct CustomMenuLayer;

typedef struct CustomMenuLayer CustomMenuLayer;

typedef struct {
  Window *window;
  List* content;
  struct CustomMenuLayer* customMenu;
} CustomWindow;

void custom_window_create(CustomWindow* window) {
  window->window = window_create();
  window->content = NULL;
  window->customMenu = NULL;
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



void display_message_failed(AppMessageResult reason) {
  char* err = malloc(53);
  snprintf(err, 53, "Failed to send message to phone. Errorcode: %d", reason);
  window_set_user_data(windows[CWINDOW_LOADING].window, err);
  window_stack_pop_all(false);
  window_stack_push(windows[CWINDOW_LOADING].window, false);
  applicationState =  APPSTATE_DISPLAY_ERROR;
  return;
}


#define CUSTOM_MENU_LIST_FONT fonts_get_system_font(FONT_KEY_GOTHIC_24)

struct CustomMenuLayer{
  CustomWindow* cwindow;
  MenuLayer* menuLayer;
  const char* title;
  MenuLayerCallbacks callbacks;
  SimpleMenuLayerSelectCallback callback;
};

GRect custom_menu_layer_get_text_rect(ElementState* s) {
  GRect ret = (GRect){.origin =  (GPoint){.x  = 5, .y = 0}, .size = (GSize){.w = 144-5, .h= 255}};
  if(s) {
    ret.origin.x += 12;
    ret.size.w -= 12;
  }

  return ret;
}

void custom_menu_layer_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
  CustomMenuLayer *this = (CustomMenuLayer*) callback_context;
  List * content = this->cwindow->content;
  graphics_context_set_text_color(ctx, GColorBlack);
  GRect s = custom_menu_layer_get_text_rect(content->elementState? content->elementState+cell_index->row : NULL);
  graphics_draw_text(ctx, content->elements[cell_index->row], CUSTOM_MENU_LIST_FONT, s,
    GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  if(content->elementState) {
    GBitmap *icon = stateToIcon(content->elementState[cell_index->row]);
    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
    graphics_draw_bitmap_in_rect(ctx, icon, (GRect){.origin = (GPoint){.x = 1, .y = 10}, .size = icon->bounds.size});
  }
}

uint16_t custom_menu_layer_num_rows(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
  CustomMenuLayer *this = (CustomMenuLayer*) callback_context;
  return this->cwindow->content->elementCount;
}

uint16_t custom_menu_layer_num_sections(struct MenuLayer *menu_layer, void *callback_context) {
  return 1;
}

void custom_menu_set_select_callback(CustomMenuLayer* this, SimpleMenuLayerSelectCallback callback) {
  this->callback = callback;
}

int16_t custom_menu_layer_cell_height(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
    CustomMenuLayer *this = (CustomMenuLayer*) callback_context;
    List * content = this->cwindow->content;
    GRect text_bounds = custom_menu_layer_get_text_rect(content->elementState? content->elementState+cell_index->row : NULL);
    GSize s = graphics_text_layout_get_content_size(this->cwindow->content->elements[cell_index->row],
      CUSTOM_MENU_LIST_FONT,
      text_bounds,
      GTextOverflowModeTrailingEllipsis,
      GTextAlignmentLeft);
    return s.h + 5;
}

int16_t custom_menu_layer_header_height(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
  return 18;
}

void custom_menu_layer_draw_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context) {
  CustomMenuLayer *this = (CustomMenuLayer*) callback_context;
  menu_cell_basic_header_draw(ctx, cell_layer,  this->title);
}

void custom_menu_layer_destroy(CustomMenuLayer* this) {
  menu_layer_destroy(this->menuLayer);
  free(this);
}

void custom_menu_layer_select(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
  CustomMenuLayer *this = (CustomMenuLayer*) callback_context;
  if(this->callback)
    this->callback(cell_index->row, this);
}

CustomMenuLayer* custom_menu_layer_create(CustomWindow* cwindow) {
  CustomMenuLayer* this = malloc(sizeof(CustomMenuLayer));
  memset(this, 0, sizeof(CustomMenuLayer));
  this->cwindow = cwindow;
  this->menuLayer = menu_layer_create(layer_get_bounds(window_get_root_layer(this->cwindow->window)));
  this->callbacks = (MenuLayerCallbacks){
    .draw_header = custom_menu_layer_draw_header,
    .draw_row = custom_menu_layer_draw_row,
    .get_cell_height = custom_menu_layer_cell_height,
    .get_header_height = custom_menu_layer_header_height,
    .get_num_rows = custom_menu_layer_num_rows,
    .get_num_sections = custom_menu_layer_num_sections,
    .select_click = custom_menu_layer_select
  };
  menu_layer_set_callbacks(this->menuLayer, this, this->callbacks);
  menu_layer_set_click_config_onto_window(this->menuLayer, cwindow->window);
  return this;
}

typedef struct {
  int index;
  bool state;
} SelectCheckitemMessage;

typedef struct {
  int count;
  SelectCheckitemMessage *messages;
} SelectCheckitemMessageList;

static AppTimer* resendTimer = NULL;
static SelectCheckitemMessageList resendList = {0,  NULL};



void resend_timer_callback(void* data) {
  resendTimer = NULL;
  if(resendList.count == 0) {
    return;
  }
  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (iter == NULL) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "null iter at resending. Result: %u", result);
    resendTimer = app_timer_register(500, resend_timer_callback, NULL);
    return;
  }

  SelectCheckitemMessage* msg = resendList.messages + (resendList.count -1);

  Tuplet tuple = TupletInteger(MESSAGE_TYPE_DICT_KEY, MESSAGE_TYPE_SELECTED_ITEM);
  dict_write_tuplet(iter, &tuple);

  Tuplet tuple2 = TupletInteger(MESSAGE_ITEMIDX_KEY, msg->index);
  dict_write_tuplet(iter, &tuple2);

  Tuplet tuple3 = TupletInteger(MESSAGE_ITEMSTATE_KEY, msg->state);
  dict_write_tuplet(iter, &tuple3);

  dict_write_end(iter);

  result = app_message_outbox_send();

  if(result == APP_MSG_OK) {
    resendList.count--;
    if (resendList.count == 0) {
      free(resendList.messages);
      resendList.messages = NULL;
      return;
    }
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "null iter at finishing resending. Result: %u", result);
  resendTimer = app_timer_register(500, resend_timer_callback, NULL);

}

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

  if (cwindow == windows+ CWINDOW_CHECKLIST) {
    // left checklist. pending messages are no longer relevant
    if(resendList.count) {
      free(resendList.messages);
      resendList.messages = NULL;
      resendList.count = 0;
    }
  }
  layer_remove_from_parent(menu_layer_get_layer(cwindow->customMenu->menuLayer));
  custom_menu_layer_destroy(cwindow->customMenu);
  cwindow->customMenu = NULL;
}

void createListWindow(CustomWindow *window, SimpleMenuLayerSelectCallback callback, const char* title) {

  window_set_window_handlers(window->window, (WindowHandlers) {
  .unload = list_window_unload,
  });

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Creating list window with %i Elements", window->content->elementCount);

  window->customMenu = custom_menu_layer_create(window);
  window->customMenu->title = title;
  custom_menu_set_select_callback(window->customMenu, callback);

  Layer *window_layer = window_get_root_layer(window->window);
  layer_add_child(window_layer, menu_layer_get_layer(window->customMenu->menuLayer));
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
  applicationState = APPSTATE_LOADING_CARDS;

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
    display_message_failed(result);
    return;
  }


  Tuplet tuples[3] = {
    TupletInteger(MESSAGE_TYPE_DICT_KEY, MESSAGE_TYPE_SELECTED_LIST),
    TupletInteger(MESSAGE_BOARDIDX_DICT_KEY, selected_board_index),

  TupletInteger(MESSAGE_LISTIDX_DICT_KEY, selected_list_index)};

  for(int i=0 ;i<3;++i)
    dict_write_tuplet(iter, &tuples[i]);

  dict_write_end(iter);

  result = app_message_outbox_send();
  if(result != APP_MSG_OK) {
    display_message_failed(result);
  }
}

static void menu_checklists_select_callback(int index, void *ctx) {
  very_short_vibe();
  selected_checklist_index = index;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Checklists: selected index %i", index);

  window_set_user_data(windows[CWINDOW_LOADING].window, "Loading Checklist...");
  window_stack_push(windows[CWINDOW_LOADING].window, true);
  applicationState = APPSTATE_LOADING_CHECKLIST;

  DictionaryIterator *iter;
  AppMessageResult result = app_message_outbox_begin(&iter);
  if (iter == NULL) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "null iter at sending. Result: %u", result);
    display_message_failed(result);
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

  result = app_message_outbox_send();
  if(result != APP_MSG_OK)
    display_message_failed(result);
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
    if (result == APP_MSG_BUSY) {
      // set to waiting state
      checklist->elementState[index] = toggleState(checklist->elementState[index]);
      CustomMenuLayer *cmenu = ctx;
      menu_layer_reload_data(cmenu->menuLayer);

      APP_LOG(APP_LOG_LEVEL_DEBUG, "Queued message");
      resendList.messages = iso_realloc(resendList.messages, (++resendList.count) * sizeof(SelectCheckitemMessage));
      resendList.messages[resendList.count-1] = (SelectCheckitemMessage) {.index = index, .state = isCheckedState(checklist->elementState[index])};
      if (!resendTimer) {
        resendTimer = app_timer_register(500, resend_timer_callback, NULL);
      }
    }
    return;
  }

  // set to waiting state

  checklist->elementState[index] = toggleState(checklist->elementState[index]);
  CustomMenuLayer *cmenu = ctx;
  menu_layer_reload_data(cmenu->menuLayer);

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
  applicationState = APPSTATE_DISPLAY_CHECKLISTS;
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
  applicationState = APPSTATE_DISPLAY_LISTS;
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *type = dict_find(iter, MESSAGE_TYPE_DICT_KEY);
  Tuple *value = dict_find(iter, MESSAGE_VALUE_DICT_KEY);
  if(!type) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Message without type!");
    return;
  }
  int intType = tuple_get_int(type);

  const char* failmsg = "Received msg type %d in application state %d. Ignore.";

  switch(intType) {
    case MESSAGE_TYPE_BOARDS:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Got type boards!");
      if(applicationState != APPSTATE_LOADING_BOARDS) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, failmsg, intType, applicationState);
        return;
      }
      deserialize_simple_tree(iter, firstTree);
      windows[CWINDOW_BOARDS].content = firstTree->list;
      createListWindow(&windows[CWINDOW_BOARDS], menu_board_select_callback, "Boards");
      window_stack_remove(windows[CWINDOW_LOADING].window, false);
      window_stack_push(windows[CWINDOW_BOARDS].window, true);
      applicationState = APPSTATE_DISPLAY_BOARDS;
      if(firstTree->list->elementCount == 1) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Only one board, selecting...");
        window_stack_remove(windows[CWINDOW_BOARDS].window, true);
        menu_board_select_callback(0, NULL);
      }
      break;
    case MESSAGE_TYPE_CARDS:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Got type cards!");
      if(applicationState != APPSTATE_LOADING_CARDS) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, failmsg, intType, applicationState);
        return;
      }
      deserialize_simple_tree(iter, secondTree);
      windows[CWINDOW_CARDS].content = secondTree->list;
      createListWindow(&windows[CWINDOW_CARDS], menu_cards_select_callback, "Cards");
      window_stack_remove(windows[CWINDOW_LOADING].window, false);
      window_stack_push(windows[CWINDOW_CARDS].window, true);
      applicationState = APPSTATE_DISPLAY_CARDS;
      if(secondTree->list->elementCount == 1) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Only one card, selecting...");
        window_stack_remove(windows[CWINDOW_CARDS].window, true);
        menu_cards_select_callback(0, NULL);
      }
      break;
    case MESSAGE_TYPE_CHECKLIST:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Got type checklist!");
      if(applicationState != APPSTATE_LOADING_CHECKLIST) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, failmsg, intType, applicationState);
        return;
      }
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Heap used: %u, Heap free: %u", heap_bytes_used(), heap_bytes_free ());
      deserialize_checklist(iter, &checklist);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Heap used2: %u, Heap free: %u", heap_bytes_used(), heap_bytes_free ());
      windows[CWINDOW_CHECKLIST].content = checklist;
      createListWindow(&windows[CWINDOW_CHECKLIST], menu_checklist_item_select_callback, windows[CWINDOW_CHECKLISTS].content->elements[selected_checklist_index]);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Heap used3: %u, Heap free: %u", heap_bytes_used(), heap_bytes_free ());
      window_stack_push(windows[CWINDOW_CHECKLIST].window, true);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Heap used4: %u, Heap free: %u", heap_bytes_used(), heap_bytes_free ());
      window_stack_remove(windows[CWINDOW_LOADING].window, false);
      applicationState = APPSTATE_DISPLAY_CHECKLIST;
      break;
    case MESSAGE_TYPE_INIT:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Got type init!");
      if(tuple_get_int(value) != 1) {
        text_layer_set_text(loading_text_layer, "Token missing. Please open settings on phone and restart watchapp.");
        applicationState = APPSTATE_DISPLAY_ERROR;
      }
      break;
    case MESSAGE_TYPE_ITEM_STATE_CHANGED: {
      if(applicationState != APPSTATE_DISPLAY_CHECKLIST) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, failmsg, intType, applicationState);
        return;
      }
      char* receivedChecklistId = tuple_get_str(dict_find(iter, MESSAGE_CHECKLISTID_DICT_KEY));
      if(strcmp(receivedChecklistId, checklistID) != 0) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Got outdated checklist update. %s, expected %s", receivedChecklistId, checklistID);
        return;
      }

      if(windows[CWINDOW_CHECKLIST].customMenu == NULL) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Got item state changed although no checklist is loaded");
        return;
      }
      int itemidx = tuple_get_int(dict_find(iter, MESSAGE_ITEMIDX_KEY));
      int state = tuple_get_int(dict_find(iter, MESSAGE_ITEMSTATE_KEY));
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Raw state %u", state);
      checklist->elementState[itemidx] = intStateToState(state);
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Set element state to %u", checklist->elementState[itemidx]);

      menu_layer_reload_data(windows[CWINDOW_CHECKLIST].customMenu->menuLayer);
      break;
    }
    case MESSAGE_TYPE_HTTP_FAIL: {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Got type http fail!");
      char* text = tuple_get_str(dict_find(iter, MESSAGE_FAILTEXT_DICT_KEY));
      window_set_user_data(windows[CWINDOW_LOADING].window, strdup(text));
      window_stack_pop_all(false);
      window_stack_push(windows[CWINDOW_LOADING].window, false);
      applicationState = APPSTATE_DISPLAY_ERROR;
    }
  }
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
  // no need for special handling here. phone will get NACK and resend
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped!");
}


static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Failed to Send! Reason:%u", reason);

  if (applicationState == APPSTATE_LOADING_BOARDS) {
    display_message_failed(reason);
    return;
  }

  // TODO: implement resending

}

static void app_message_init(void) {
  // Register message handlers
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_failed(out_failed_handler);
  // Init buffers
  app_message_open(app_message_inbox_size_maximum(), APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
}


static void loading_window_unload(Window *window) {
  if(loading_text_layer != NULL)
    text_layer_destroy(loading_text_layer);
}


void initial_ping_callback(void* data) {
  if(applicationState != APPSTATE_LOADING_BOARDS)
    return;

  DictionaryIterator* iter;

  AppMessageResult result = app_message_outbox_begin(&iter);
  if(result != APP_MSG_OK) {
    display_message_failed(result);
    return;
  }
  Tuplet tuple = TupletInteger(MESSAGE_TYPE_DICT_KEY, MESSAGE_TYPE_PING);
  dict_write_tuplet(iter, &tuple);
  dict_write_end(iter);

  result = app_message_outbox_send();
  if(result != APP_MSG_OK) {
    display_message_failed(result);
    return;
  }
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

  // ping phone, causes send_fail if no app is running
  app_timer_register(5000, initial_ping_callback, NULL);
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
