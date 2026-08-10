#ifndef MANGOBAR_TRAY_H
#define MANGOBAR_TRAY_H

#include <pixman.h>
#include <stdbool.h>
#include <stdint.h>

#define MANGOBAR_TRAY_MAX_ITEMS 32

typedef struct MangobarTray MangobarTray;
typedef struct MangobarTrayItem MangobarTrayItem;

// Init tray (DBus connection, watcher/host registration).
// set_dirty: called when tray content changes to trigger redraw.
MangobarTray *tray_init(void (*set_dirty)(void));
void tray_destroy(MangobarTray *tray);

// DBus fd for select() event loop
int tray_get_fd(MangobarTray *tray);
// Events the DBus fd currently needs (POLLIN/POLLOUT mask)
int tray_get_events(MangobarTray *tray);
// Process DBus events when fd is readable
void tray_dispatch(MangobarTray *tray);

// Return currently visible (non-Passive, ready) items
MangobarTrayItem **tray_visible_items(MangobarTray *tray, int *count);

const char *tray_item_id(MangobarTrayItem *item);
// Icon (ARGB32 premultiplied, owned by caller)
pixman_image_t *tray_item_icon(MangobarTrayItem *item);
// Icon natural size
int tray_item_icon_size(MangobarTrayItem *item);
// Remove a tray item (e.g. stale service)
void tray_remove_item(MangobarTray *tray, MangobarTrayItem *item);
// Re-register hosts and re-pull RegisteredStatusNotifierItems
void tray_refresh(MangobarTray *tray);
// Verify service names are still owned; drop stale items
void tray_prune(MangobarTray *tray);

// Whether the item exposes a DBusMenu (AppIndicator style)
bool tray_item_has_menu(MangobarTrayItem *item);
const char *tray_item_service(MangobarTrayItem *item);
const char *tray_item_menu_path(MangobarTrayItem *item);
// Tray D-Bus connection (for DBusMenu client)
void *tray_get_bus(MangobarTray *tray);

// Handle click (x,y are guessed global coordinates)
void tray_handle_click(MangobarTray *tray, MangobarTrayItem *item, double x,
                       double y, uint32_t button);

#endif
