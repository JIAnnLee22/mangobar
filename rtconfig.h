#ifndef MANGOBAR_RTCONFIG_H
#define MANGOBAR_RTCONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MANGOBAR_MAX_TAGS 31
#define MANGOBAR_MAX_ACTIONS 32
#define MANGOBAR_MAX_MODULES 16
#define MANGOBAR_MAX_CUSTOM 16
#define MANGOBAR_MAX_ALTS 48

enum MangoModule {
  M_NONE = 0,
  M_TAGS,
  M_LAYOUT,
  M_TITLE,
  M_TRAY,
  M_CPU,
  M_MEM,
  M_BRIGHTNESS,
  M_VOLUME,
  M_CLOCK_TIME,
  M_CLOCK_DATE,
  M_KEYMODE,
  M_KBLAYOUT,
  M_NETWORK,
  M_HIDE_CLIENTS,
  M_CUSTOM = 100, // + index into g_cfg.customs
};

typedef struct {
  char name[64]; // e.g. "power" -> CSS #custom-power
  char exec[256]; // command that prints the module text
  char format[256]; // format string; "{}" is replaced with exec output
  int interval; // refresh interval in seconds
  char output[256]; // latest command output
  uint64_t last_run_ms;
  bool enabled;
} MangoCustomModule;

typedef struct {
  char module[32];
  char left[256];
  char middle[256];
  char right[256];
  char scroll_up[256];
  char scroll_down[256];
} MangoAction;

typedef struct {
  char module[32]; /* internal module name, e.g. "cpu", "custom-power" */
  char fmt[256];   /* format-alt string */
} MangoAltFormat;

typedef struct {
  int bar_height;
  int buffer_scale;
  int radius_default;
  int layer;
  int max_title_len;
  int sys_interval;
  int tag_count;
  char font[256]; // fallback font when CSS sets none
  char tag_names[MANGOBAR_MAX_TAGS][8];
  char overview_label[64];
  bool only_occupied;
  char separator[16];
  int tray_pad;
  int tray_gap;
  int tray_icon_size; // 0 = auto from bar height
  char brightness_dev[64];
  char brightness_fmt[64];
  char volume_ctrl[32];
  int volume_mix_index;
  char volume_fmt[64];
  char volume_fmt_muted[64];
  char layout_format[64];
  char title_format[128];
  char cpu_format[64];
  char mem_format[64];
  char clock_time_format[128];
  char clock_date_format[128];
  char keymode_format[64];
  char keyboardlayout_format[64];
  char network_format[64];
  char hide_clients_format[64];
  int left_order[MANGOBAR_MAX_MODULES];
  int left_count;
  int center_order[MANGOBAR_MAX_MODULES];
  int center_count;
  int right_order[MANGOBAR_MAX_MODULES];
  int right_count;
  MangoCustomModule customs[MANGOBAR_MAX_CUSTOM];
  int custom_count;
  MangoAction actions[MANGOBAR_MAX_ACTIONS];
  int action_count;
  MangoAltFormat alts[MANGOBAR_MAX_ALTS];
  int alt_count;
  char css_path[512];
} MangoConfig;

extern MangoConfig g_cfg;

void mango_config_defaults(void);
// Load config from JSONC; returns 0 on success
int mango_config_load(const char *path);
// Search order: $MANGOBAR_CONFIG, ~/.config/mangobar/config.jsonc
const char *mango_config_find_default(char *buf, size_t sz);

#endif
