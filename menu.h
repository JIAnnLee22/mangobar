#ifndef MANGOBAR_MENU_H
#define MANGOBAR_MENU_H

#include "sd-bus.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct MangobarMenu MangobarMenu;
typedef struct MangobarMenuNode MangobarMenuNode;

struct MangobarMenuNode {
  int id;
  char *label;
  char *type; // "item" "separator" "submenu" "toggle" "radio" ...
  char *toggle_type; // DBusMenu toggle-type: "checkmark" / "radio"
  bool enabled;
  bool visible;
  int toggle_state; // current state for toggle/radio items
  MangobarMenuNode **children;
  int child_count;
};

// Create a DBusMenu client (service/path from SNI Menu property).
// on_layout: called when the menu layout refreshes.
MangobarMenu *menu_init(sd_bus *bus, const char *service, const char *path,
                        void (*on_layout)(void *data), void *userdata);
void menu_destroy(MangobarMenu *menu);

// Async refresh menu (AboutToShow + GetLayout)
void menu_refresh(MangobarMenu *menu);

// Whether the menu is ready and has visible items
bool menu_has_items(MangobarMenu *menu);

// Visible top-level item count / item at index
int menu_visible_count(MangobarMenu *menu);
const MangobarMenuNode *menu_visible_node(MangobarMenu *menu, int idx);

// Visible child count / child at index for any node
int menu_node_visible_count(const MangobarMenuNode *node);
const MangobarMenuNode *menu_node_visible_node(const MangobarMenuNode *node,
                                               int idx);

// Enter submenu / go back to parent; true on success
bool menu_enter_child(MangobarMenu *menu, const MangobarMenuNode *node);
bool menu_go_back(MangobarMenu *menu);

// Trigger a menu item click
void menu_activate(MangobarMenu *menu, const MangobarMenuNode *node,
                   uint32_t timestamp);

// Whether a submenu is shown (renders a back item)
bool menu_in_submenu(MangobarMenu *menu);

#endif
