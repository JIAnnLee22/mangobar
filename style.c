#define _GNU_SOURCE
#include "style.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void apply_rule(Style *dst, const StyleRule *r);

static void skip_ws_comments(const char **p) {
  for (;;) {
    const char *s = *p;
    while (*s && isspace((unsigned char)*s))
      s++;
    if (s[0] == '/' && s[1] == '*') {
      s += 2;
      while (*s && !(s[0] == '*' && s[1] == '/'))
        s++;
      if (*s)
        s += 2;
      *p = s;
    } else {
      *p = s;
      return;
    }
  }
}

// Parse #RGB / #RGBA / #RRGGBB / #AARRGGBB -> ARGB (uint32)
static int parse_hex_color(const char *s, uint32_t *out) {
  if (*s == '#')
    s++;
  size_t n = strlen(s);
  if (n != 3 && n != 4 && n != 6 && n != 8)
    return 0;
  for (size_t i = 0; i < n; i++)
    if (!isxdigit((unsigned char)s[i]))
      return 0;
  char buf[16];
  int b = 0;
  if (n == 3 || n == 4) {
    for (size_t i = 0; i < n; i++) {
      buf[b++] = s[i];
      buf[b++] = s[i];
    }
    buf[b] = '\0';
  } else {
    strcpy(buf, s);
    b = (int)n;
  }
  unsigned long v = strtoul(buf, NULL, 16);
  if (b == 6)
    v = (v << 8) | 0xFFu; // fill alpha when absent
  *out = (uint32_t)v;
  return 1;
}

static int parse_int(const char *s) {
  while (*s && !isdigit((unsigned char)*s))
    s++;
  if (!isdigit((unsigned char)*s))
    return 0;
  int sign = 1;
  const char *q = s;
  while (q > s && (q[-1] == '-' || q[-1] == '+')) {
    if (q[-1] == '-')
      sign = -sign;
    q--;
  }
  return sign * atoi(s);
}

static const char *find_var(const StyleSheet *ss, const char *name) {
  for (int i = 0; i < ss->var_count; i++)
    if (strcmp(ss->var_names[i], name) == 0)
      return ss->var_values[i];
  return NULL;
}

// Resolve a color value to hex:
// supports #hex, @var, linear-gradient(...) (first color), none (transparent)
static int resolve_color_value(const StyleSheet *ss, const char *val,
                               char *out, size_t outsz) {
  const char *p = val;
  while (*p && isspace((unsigned char)*p))
    p++;
  if (strncmp(p, "none", 4) == 0) {
    snprintf(out, outsz, "#00000000");
    return 1;
  }
  if (*p == '#') {
    snprintf(out, outsz, "%s", p);
    return 1;
  }
  if (*p == '@') {
    p++;
    char name[64];
    size_t n = 0;
    while (p[n] && !isspace((unsigned char)p[n]) && p[n] != ';' &&
           p[n] != ')' && n + 1 < sizeof(name))
      {
        name[n] = p[n];
        n++;
      }
    name[n] = '\0';
    const char *v = find_var(ss, name);
    if (v)
      snprintf(out, outsz, "%s", v);
    return v ? 1 : 0;
  }
  // Find first #hex or @var inside parentheses
  while (*p) {
    if (*p == '#') {
      const char *q = p;
      while (*q && !isspace((unsigned char)*q) && *q != ')' && *q != ',')
        q++;
      size_t n = (size_t)(q - p);
      if (n < outsz) {
        memcpy(out, p, n);
        out[n] = '\0';
        return 1;
      }
      return 0;
    }
    if (*p == '@') {
      p++;
      char name[64];
      size_t n = 0;
      while (p[n] && !isspace((unsigned char)p[n]) && p[n] != ')' &&
             p[n] != ',' && n + 1 < sizeof(name))
        {
          name[n] = p[n];
          n++;
        }
      name[n] = '\0';
      const char *v = find_var(ss, name);
      if (v) {
        snprintf(out, outsz, "%s", v);
        return 1;
      }
      return 0;
    }
    p++;
  }
  return 0;
}

// Parse 1-2 ints: one value -> all sides; two -> vertical, horizontal
static void parse_int_pair(const char *s, int *a, int *b) {
  *a = parse_int(s);
  const char *p = s;
  while (*p && !isdigit((unsigned char)*p))
    p++;
  if (!*p)
    return;
  while (*p && (isdigit((unsigned char)*p) || *p == '-' || *p == '+'))
    p++;
  while (*p && !isdigit((unsigned char)*p))
    p++;
  if (*p)
    *b = parse_int(p);
  else
    *b = *a;
}

static void apply_decl(StyleSheet *ss, Style *st, const char *prop,
                       const char *val) {
  uint32_t c;
  char color_buf[64];
  if (strcmp(prop, "color") == 0 &&
      resolve_color_value(ss, val, color_buf, sizeof(color_buf)) &&
      parse_hex_color(color_buf, &c)) {
    st->color = c;
    st->color_set = true;
  } else if ((strcmp(prop, "background") == 0 ||
              strcmp(prop, "background-color") == 0) &&
             resolve_color_value(ss, val, color_buf, sizeof(color_buf)) &&
             parse_hex_color(color_buf, &c)) {
    st->background = c;
    st->background_set = true;
  } else if (strcmp(prop, "border-color") == 0 &&
             resolve_color_value(ss, val, color_buf, sizeof(color_buf)) &&
             parse_hex_color(color_buf, &c)) {
    st->border_color = c;
    st->border_color_set = true;
  } else if (strcmp(prop, "border-radius") == 0) {
    st->radius = parse_int(val);
    st->radius_set = true;
  } else if (strcmp(prop, "min-width") == 0) {
    st->min_width = parse_int(val);
    st->min_width_set = true;
  } else if (strcmp(prop, "padding") == 0) {
    int a, b;
    parse_int_pair(val, &a, &b);
    st->padding_left = st->padding_right = b;
    st->padding_left_set = st->padding_right_set = true;
  } else if (strcmp(prop, "padding-left") == 0) {
    st->padding_left = parse_int(val);
    st->padding_left_set = true;
  } else if (strcmp(prop, "padding-right") == 0) {
    st->padding_right = parse_int(val);
    st->padding_right_set = true;
  } else if (strcmp(prop, "margin") == 0) {
    int a, b;
    parse_int_pair(val, &a, &b);
    st->margin_top = a;
    st->margin_top_set = true;
    st->margin_left = st->margin_right = b;
    st->margin_left_set = st->margin_right_set = true;
  } else if (strcmp(prop, "margin-top") == 0) {
    st->margin_top = parse_int(val);
    st->margin_top_set = true;
  } else if (strcmp(prop, "margin-left") == 0) {
    st->margin_left = parse_int(val);
    st->margin_left_set = true;
  } else if (strcmp(prop, "margin-right") == 0) {
    st->margin_right = parse_int(val);
    st->margin_right_set = true;
  } else if (strcmp(prop, "font-family") == 0) {
    if (val[0]) {
      // Take only the first family name, strip quotes and fallbacks
      const char *p = val;
      while (*p && isspace((unsigned char)*p))
        p++;
      char fam[sizeof(ss->font_family)];
      size_t n = 0;
      bool quoted = false;
      if (*p == '"' || *p == '\'') {
        quoted = true;
        p++;
      }
      while (*p && n + 1 < sizeof(fam)) {
        if (quoted && (*p == '"' || *p == '\'')) {
          p++;
          break;
        }
        if (!quoted && *p == ',')
          break;
        fam[n++] = *p++;
      }
      while (n > 0 && isspace((unsigned char)fam[n - 1]))
        n--;
      fam[n] = '\0';
      if (fam[0]) {
        strncpy(ss->font_family, fam, sizeof(ss->font_family) - 1);
        ss->font_family[sizeof(ss->font_family) - 1] = '\0';
        ss->font_set = true;
      }
    }
  } else if (strcmp(prop, "font-size") == 0) {
    ss->font_size = parse_int(val);
    if (ss->font_size > 0)
      ss->font_set = true;
  } else if (strcmp(prop, "font-weight") == 0) {
    if (val[0]) {
      strncpy(ss->font_weight, val, sizeof(ss->font_weight) - 1);
      ss->font_weight[sizeof(ss->font_weight) - 1] = '\0';
      ss->font_set = true;
    }
  }
  // Ignore: font shorthand, border, border-radius, opacity, etc.
}

void style_sheet_init(StyleSheet *ss) {
  memset(ss, 0, sizeof(*ss));
}

// Parse a selector string (with optional state) into module / state
static void parse_selector(const char *sel, char *module, size_t msz,
                           char *state, size_t ssz) {
  module[0] = state[0] = '\0';
  const char *p = sel;
  while (*p && isspace((unsigned char)*p))
    p++;
  if (*p == '*') {
    strncpy(module, "*", msz - 1);
    p++;
  } else if (*p == '#') {
    p++;
    const char *q = p;
    while (*q && *q != '.' && !isspace((unsigned char)*q) && *q != '{')
      q++;
    size_t n = (size_t)(q - p);
    if (n >= msz)
      n = msz - 1;
    memcpy(module, p, n);
    module[n] = '\0';
    p = q;
  } else if (strncmp(p, "menuitem", 8) == 0) {
    strncpy(module, "menuitem", msz - 1);
    p += 8;
    while (*p && isspace((unsigned char)*p))
      p++;
    if (*p == ':') {
      p++;
      const char *q = p;
      while (*q && !isspace((unsigned char)*q) && *q != '{' && *q != '}')
        q++;
      size_t n = (size_t)(q - p);
      if (n >= ssz)
        n = ssz - 1;
      memcpy(state, p, n);
      state[n] = '\0';
    }
    return;
  } else if (strncmp(p, "menu", 4) == 0) {
    strncpy(module, "menu", msz - 1);
    return;
  }
  // Skip child prefixes like "button"
  while (*p && isspace((unsigned char)*p))
    p++;
  if (*p == '>') {
    // Child rules like #tray > .passive don't apply to the module
    module[0] = '\0';
    return;
  }
  if (strncmp(p, "button", 6) == 0)
    p += 6;
  while (*p && isspace((unsigned char)*p))
    p++;
  if (*p == '.') {
    p++;
    const char *q = p;
    while (*q && !isspace((unsigned char)*q) && *q != '{' && *q != '}')
      q++;
    size_t n = (size_t)(q - p);
    if (n >= ssz)
      n = ssz - 1;
    memcpy(state, p, n);
    state[n] = '\0';
  } else if (*p == ':') {
    p++;
    const char *q = p;
    while (*q && !isspace((unsigned char)*q) && *q != '{' && *q != '}')
      q++;
    size_t n = (size_t)(q - p);
    if (n >= ssz)
      n = ssz - 1;
    memcpy(state, p, n);
    state[n] = '\0';
  }
}

int style_sheet_parse(StyleSheet *ss, const char *buf) {
  const char *p = buf;
  for (;;) {
    skip_ws_comments(&p);
    if (!*p)
      break;

    // @define-color name value;
    if (strncmp(p, "@define-color", 13) == 0) {
      p += 13;
      while (*p && isspace((unsigned char)*p))
        p++;
      char name[64];
      size_t nl = 0;
      while (*p && !isspace((unsigned char)*p) && *p != ';' &&
             nl + 1 < sizeof(name))
        name[nl++] = *p++;
      name[nl] = '\0';
      while (*p && isspace((unsigned char)*p))
        p++;
      char val[64];
      size_t vl = 0;
      while (*p && *p != ';' && vl + 1 < sizeof(val))
        val[vl++] = *p++;
      val[vl] = '\0';
      if (*p == ';')
        p++;
      if (nl && vl && ss->var_count < MANGOBAR_MAX_VARS) {
        snprintf(ss->var_names[ss->var_count], sizeof(ss->var_names[0]), "%s",
                 name);
        snprintf(ss->var_values[ss->var_count], sizeof(ss->var_values[0]),
                 "%s", val);
        ss->var_count++;
      }
      continue;
    }

    // Skip @keyframes ... { ... } blocks
    if (strncmp(p, "@keyframes", 10) == 0) {
      while (*p && *p != '{')
        p++;
      if (*p == '{') {
        int depth = 0;
        while (*p) {
          if (*p == '{')
            depth++;
          else if (*p == '}' && --depth <= 0) {
            p++;
            break;
          }
          p++;
        }
      }
      continue;
    }

    // Selector list
    char selector[2048];
    size_t sl = 0;
    while (*p && *p != '{' && sl < sizeof(selector) - 1)
      selector[sl++] = *p++;
    selector[sl] = '\0';
    if (*p == '{')
      p++;
    bool is_menu_rule = strstr(selector, "menu") != NULL;

    // Declaration block
    Style decl = {0};
    for (;;) {
      skip_ws_comments(&p);
      if (!*p)
        break;
      if (*p == '}') {
        p++;
        break;
      }
      char prop[64];
      size_t pl = 0;
      while (*p && *p != ':' && *p != ';' && *p != '}' &&
             pl < sizeof(prop) - 1)
        prop[pl++] = *p++;
      prop[pl] = '\0';
      while (*p && *p != ':' && *p != ';' && *p != '}')
        p++;
      if (*p == ':')
        p++;
      skip_ws_comments(&p);
      char val[512];
      size_t vl = 0;
      while (*p && *p != ';' && *p != '}' && vl < sizeof(val) - 1)
        val[vl++] = *p++;
      val[vl] = '\0';
      char *vp = val;
      while (*vp && isspace((unsigned char)*vp))
        vp++;
      char *ve = val + vl;
      while (ve > vp && isspace((unsigned char)ve[-1]))
        ve--;
      *ve = '\0';
      if (*p == ';')
        p++;
      if (prop[0]) {
        if (is_menu_rule && strcmp(prop, "font-size") == 0)
          ss->menu_font_size = parse_int(vp);
        else
          apply_decl(ss, &decl, prop, vp);
      }
    }

    // Split comma-separated selectors
    char *cs = strdup(selector);
    if (!cs)
      continue;
    char *tok = strtok(cs, ",");
    while (tok && ss->rule_count < MANGOBAR_MAX_STYLE_RULES) {
      StyleRule *r = &ss->rules[ss->rule_count++];
      parse_selector(tok, r->module, sizeof(r->module), r->state,
                     sizeof(r->state));
      r->style = decl;
      if (strcmp(r->module, "menu") == 0 && r->state[0] == '\0') {
        apply_rule(&ss->menu, r);
        if (r->style.radius_set)
          ss->menu_radius = r->style.radius;
      } else if (strcmp(r->module, "menuitem") == 0 &&
                 r->state[0] == '\0') {
        apply_rule(&ss->menuitem, r);
      } else if (strcmp(r->module, "menuitem") == 0 &&
                 strcmp(r->state, "hover") == 0) {
        apply_rule(&ss->menuitem_hover, r);
      }
      tok = strtok(NULL, ",");
    }
    free(cs);
  }
  ss->loaded = true;
  return 0;
}

int style_sheet_load(StyleSheet *ss, const char *path) {
  FILE *f = fopen(path, "r");
  if (!f)
    return -1;
  fseek(f, 0, SEEK_END);
  long sz = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (sz < 0) {
    fclose(f);
    return -1;
  }
  char *buf = malloc((size_t)sz + 1);
  if (!buf) {
    fclose(f);
    return -1;
  }
  size_t rd = fread(buf, 1, (size_t)sz, f);
  buf[rd] = '\0';
  fclose(f);
  int ret = style_sheet_parse(ss, buf);
  free(buf);
  return ret;
}

static void apply_rule(Style *dst, const StyleRule *r) {
  const Style *s = &r->style;
  if (s->color_set)
    dst->color = s->color;
  if (s->background_set)
    dst->background = s->background;
  if (s->border_color_set)
    dst->border_color = s->border_color;
  if (s->padding_left_set)
    dst->padding_left = s->padding_left;
  if (s->padding_right_set)
    dst->padding_right = s->padding_right;
  if (s->margin_top_set)
    dst->margin_top = s->margin_top;
  if (s->margin_left_set)
    dst->margin_left = s->margin_left;
  if (s->margin_right_set)
    dst->margin_right = s->margin_right;
  if (s->radius_set)
    dst->radius = s->radius;
  if (s->min_width_set)
    dst->min_width = s->min_width;
  dst->color_set |= s->color_set;
  dst->background_set |= s->background_set;
  dst->border_color_set |= s->border_color_set;
  dst->padding_left_set |= s->padding_left_set;
  dst->padding_right_set |= s->padding_right_set;
  dst->margin_top_set |= s->margin_top_set;
  dst->margin_left_set |= s->margin_left_set;
  dst->margin_right_set |= s->margin_right_set;
  dst->radius_set |= s->radius_set;
  dst->min_width_set |= s->min_width_set;
}

Style style_resolve(const StyleSheet *ss, const char *module,
                    const char *state) {
  Style s = {0};
  // 1. "*" rules
  for (int i = 0; i < ss->rule_count; i++) {
    const StyleRule *r = &ss->rules[i];
    if (strcmp(r->module, "*") == 0 &&
        (r->state[0] == '\0' ||
         (state && strcmp(r->state, state) == 0)))
      apply_rule(&s, r);
  }
  // 2. "#module"
  for (int i = 0; i < ss->rule_count; i++) {
    const StyleRule *r = &ss->rules[i];
    if (strcmp(r->module, module) == 0 && r->state[0] == '\0')
      apply_rule(&s, r);
  }
  // 3. "#module.state"
  if (state && state[0]) {
    for (int i = 0; i < ss->rule_count; i++) {
      const StyleRule *r = &ss->rules[i];
      if (strcmp(r->module, module) == 0 &&
          strcmp(r->state, state) == 0)
        apply_rule(&s, r);
    }
  }
  return s;
}

const char *style_font_string(const StyleSheet *ss, const char *fallback) {
  if (!ss->font_set || !ss->font_family[0])
    return fallback;
  static char buf[256];
  const char *weight = ss->font_weight[0] ? ss->font_weight : "Regular";
  int size = ss->font_size > 0 ? ss->font_size : 12;
  // fcft format: family:style=weight:size=N
  snprintf(buf, sizeof(buf), "%s:style=%s:size=%d", ss->font_family, weight,
           size);
  return buf;
}

int style_find_default_path(char *out, size_t outsz) {
  // 1. $MANGOBAR_CSS
  const char *env = getenv("MANGOBAR_CSS");
  if (env && *env && access(env, R_OK) == 0) {
    strncpy(out, env, outsz - 1);
    out[outsz - 1] = '\0';
    return 0;
  }
  // 2. $XDG_CONFIG_HOME/mangobar/style.css
  const char *xdg = getenv("XDG_CONFIG_HOME");
  char path[512];
  if (xdg && *xdg) {
    snprintf(path, sizeof(path), "%s/mangobar/style.css", xdg);
    if (access(path, R_OK) == 0) {
      strncpy(out, path, outsz - 1);
      out[outsz - 1] = '\0';
      return 0;
    }
  }
  // 3. ~/.config/mangobar/style.css
  const char *home = getenv("HOME");
  if (home && *home) {
    snprintf(path, sizeof(path), "%s/.config/mangobar/style.css", home);
    if (access(path, R_OK) == 0) {
      strncpy(out, path, outsz - 1);
      out[outsz - 1] = '\0';
      return 0;
    }
  }
  return -1;
}
