#include <pebble.h>
#include <pebble-trello2.h>
#include <string.h>

char* strdup(const char* in) {
  char* t = malloc(strlen(in)+1);
  strcpy(t, in);
  return t;
}

//// LIST

typedef struct
{
  char** elements;
  int elementCount;
} List;


List* make_list(int elementCount) {
  List* l = malloc(sizeof(List));
  l->elementCount = elementCount;
  l->elements = malloc(sizeof(char*)*elementCount);
  return l;
}

static void empty_list(List* l) {
  if(l->elements == NULL)
    return;
  for(int i=0; i<l->elementCount; ++i)
    free(l->elements[i]);
  free(l->elements);
  l->elements = NULL;
  l->elementCount = 0;
}

static void delete_list(List* l) {
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
    delete_list(tree->sublists[i]);
  }
  free(tree->sublists);
  tree->sublists = NULL;
  delete_list(tree->list);
  tree->list = NULL;
}

// Boards -> lists
static SimpleTree* firstTree;

// cards -> checklists
static SimpleTree* secondTree;


//// CUSTOMWINDOW

typedef struct {
  Window *window;
  List* content;
  SimpleMenuLayer* simplemenu;
} CustomWindow;

void custom_window_create(CustomWindow* window) {
  window->window = window_create();
  window->content = NULL;
  window->simplemenu = NULL;
}

void custom_window_destroy(CustomWindow* window) {
  window_destroy(window->window);
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


static TextLayer *loading_text_layer;
static bool wasFirstMsg;

static void loading_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
/*
  BitmapLayer* layer = bitmap_layer_create((GRect) { .origin = { 0, 0 }, .size = { bounds.size.w, 20 } });
  bitmap_layer_set_bitmap(layer, logo);
TODO: show logo
  */

  loading_text_layer = text_layer_create((GRect) { .origin = { 0, 0}, .size = { bounds.size.w, 60 } });
  text_layer_set_text(loading_text_layer, "Loading Boards....");
  text_layer_set_overflow_mode(loading_text_layer, GTextOverflowModeWordWrap);
  text_layer_set_text_alignment(loading_text_layer, GTextAlignmentCenter);
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

void list_window_free(CustomWindow *window) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "TODO: implement freeing");
}

void deserialize_simple_tree(DictionaryIterator *iter, SimpleTree* tree) {
  empty_simple_tree(tree);
  Tuple *numElTuple = dict_find(iter, MESSAGE_NUMEL1_DICT_KEY);
  if(!numElTuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Message without numels!");
    return;
  }

  int numberElements = tuple_get_int(numElTuple);
  tree->list = make_list(numberElements);
  tree->sublists = malloc(sizeof(List*)*numberElements);
  int msgPtr = 0;
  for(int i=0; i<tree->list->elementCount; ++i) {
    tree->list->elements[i] = strdup(tuple_get_str(dict_find(iter, msgPtr++)));
    int sublistSize = tuple_get_int(dict_find(iter, msgPtr++));
    tree->sublists[i] = make_list(sublistSize);
    for(int j=0; j<sublistSize; j++){
      tree->sublists[i]->elements[i] = strdup(tuple_get_str(dict_find(iter, msgPtr++)));
    }
  }
}

void createListWindow(CustomWindow *window, SimpleMenuLayerSelectCallback callback) {
  list_window_free(window);
/*
  SimpleMenuItem* boardMenuItems = malloc(sizeof(SimpleMenuItem)*numberElements);
  memset(boardMenuItems, 0, sizeof(SimpleMenuItem)*numberElements);


  SimpleMenuSection* boardSection = malloc(sizeof(SimpleMenuSection));
  boardSection->items = boardMenuItems;
  boardSection->num_items = numberElements;
  boardSection->title = NULL;

  for(int i =0; ;++i ) {
    Tuple *boardTuple = dict_find(iter, i);
    if(!boardTuple)
      break;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Board %i: %s", i, boardTuple->value->cstring);
    boardMenuItems[i].title = strdup(boardTuple->value->cstring);
    boardMenuItems[i].callback = callback;
  }
  window->simplemenu = simple_menu_layer_create(layer_get_frame(window_get_root_layer(window->window)), window->window, boardSection, 1, NULL);
  Layer *window_layer = window_get_root_layer(window->window);
  layer_add_child(window_layer, simple_menu_layer_get_layer(window->simplemenu));
  */
}

static void menu_board_select_callback(int index, void *ctx) {
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
      windows[CWINDOW_LOADING].content = firstTree->list;
      createListWindow(&windows[CWINDOW_BOARDS], menu_board_select_callback);
      window_stack_push(windows[CWINDOW_BOARDS].window, true);
      window_stack_remove(windows[CWINDOW_LOADING].window, false);
      break;
    case MESSAGE_TYPE_INIT:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Got type init!");
      if(tuple_get_int(value) != 1) {
        text_layer_set_text(loading_text_layer, "Token missing. Please open settings on phone.");
      }
      break;
  }
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped!");
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  if (wasFirstMsg ) {
  }
  else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Failed to Send!");
  }
  wasFirstMsg = false;
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
  app_message_init();

  firstTree = make_simple_tree();
  secondTree = make_simple_tree();
  for(int i=0; i<CWINDOW_SIZE;++i) {
    custom_window_create(&windows[i]);
  }
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
}

int main(void) {
  wasFirstMsg = false;
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", windows[CWINDOW_LOADING].window);

  app_event_loop();
  deinit();
}
