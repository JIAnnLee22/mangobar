# mangobar

A Wayland status bar for mangowm, built on `wlr-layer-shell`.
The system tray (StatusNotifierItem / DBusMenu) is inspired by
[swaybar](https://github.com/swaywm/sway) and [waybar](https://github.com/Alexays/Waybar).

<img width="1919" height="1079" alt="image" src="https://github.com/user-attachments/assets/6c1ab40e-96fe-41a8-9d8a-89c45fa24db8" />


## Install

### Dependencies

Build tools: `meson`, `ninja`, `wayland-scanner` (part of `wayland`),
`pkg-config`.

Libraries:

- `cjson`
- `wayland`
- `wayland-protocols`
- `fcft`
- `pixman`
- `cairo`
- `pango`
- `libpulse`
- `systemd-libs`
- `gdk-pixbuf2`
- `alsa-lib`

Arch:

```sh
sudo pacman -S --needed meson ninja cjson wayland wayland-protocols \
  fcft pixman cairo pango libpulse systemd-libs gdk-pixbuf2 alsa-lib
```

Debian/Ubuntu:

```sh
sudo apt install meson ninja-build libcjson-dev libwayland-dev \
  wayland-protocols libfcft-dev libpixman-1-dev libcairo2-dev \
  libpango1.0-dev libpulse-dev libsystemd-dev libgdk-pixbuf-2.0-dev \
  libasound2-dev pkg-config
```

### Build

```sh
git clone https://github.com/mangowm/mangobar.git
cd mangobar
meson setup build -Dprefix=/usr
ninja -C build
sudo ninja -C build install
```

If `meson.build` changes after a `git pull`, ninja re-runs meson
automatically; install any newly added dependencies first.

Runtime note: `pamixer` / `brightnessctl` are only needed if your
config's click/scroll actions call them.

## Usage

Run `mangobar` inside a mangowm session. It reads configuration from:

1. `$MANGOBAR_CONFIG`
2. `~/.config/mangobar/config.jsonc`

The JSONC config supports `height`, `layer`, `buffer-scale`, `font`,
`css` (or `style`), and per-module `format`, `interval`, `on-click`,
`on-scroll-*` etc. Unsupported modules are ignored. All runtime settings
come from JSONC and CSS. See [`config.jsonc`](config.jsonc) for a complete
example.

`buffer-scale` is a multiplier on top of the output's Wayland scale
(default `1`); leave it at `1` to follow the display's HiDPI scale
automatically. Text, icons and menus are rendered at the effective scale
so they stay sharp.

Any module with a `format` can also set `format-alt`; a left click toggles
between the two formats (the module's `on-click` command still runs if
configured). The network module additionally supports `{down}` / `{up}` in
its format strings, e.g. `"format-alt": "↓{down} ↑{up}"` for live speeds.

CSS priority is `$MANGOBAR_CSS` > `~/.config/mangobar/style.css`.

## Modules

- `tags` / `layout` / `title` / `keymode` / `keyboardlayout`: from mangowm IPC
- `cpu` / `mem`: read `/proc`
- `brightness`: read `/sys/class/backlight` (auto-detected or the JSONC
  `device` field); updates immediately on external changes via udev
- `volume`: read via the PulseAudio library, with ALSA fallback; shows mute
  state and updates immediately on external changes via PulseAudio events
- `clock`: time (`#clock`) and date (`#clock.date`) with separate CSS
- `network`: shows the active interface name; click toggles up/down speeds
  (KB/s below 1MB/s, MB/s otherwise)
- `tray`: StatusNotifierItem / DBusMenu, with a side-opening submenu
- `custom/<name>`: user-defined modules (see below)

## Custom modules

Add `custom/<name>` to `modules-left` or `modules-right` and define it in the
same JSONC file:

```jsonc
"custom/power": {
    "format": "󰣇",
    "on-click": "wlogout",
    "on-click-right": "swaync-client -t -sw"
},
"custom/uptime": {
    "exec": "uptime -p | sed 's/^up //'",
    "interval": 60,
    "format": "󰅐 {}",
    "on-scroll-up": "brightnessctl s +5%",
    "on-scroll-down": "brightnessctl s 5%-"
}
```

Fields:

- `exec`: command whose stdout becomes the module text (trailing newline trimmed)
- `interval`: refresh interval in seconds; `0`/omitted runs once at startup
- `format`: shown as-is, with `{}` replaced by the exec output; if omitted the
  raw exec output is shown
- `on-click` / `on-click-middle` / `on-click-right` / `on-scroll-up` /
  `on-scroll-down`: shell commands or `@ipc:xxx` commands

Custom modules are styled with `#custom-<name>` selectors, e.g.
`#custom-power { background-color: #cc241d; color: #ffffff; }`.

Right-click a tray item with a menu to open a context menu; clicking outside
closes it.

## Styling

`~/.config/mangobar/style.css` supports a small CSS subset: `color`,
`background`, `padding`, `margin`, `border-radius`, `min-width`,
`font-family/size/weight`, `@define-color`, and `linear-gradient` (first
color). Selectors: `*`, `#module`, `#module.state`, and `#custom-<name>`.

See [`style.css.example`](style.css.example).

## Module actions

Actions are configured through the JSONC config's `on-click` /
`on-scroll-*` fields. Special commands:

- `@view` / `@toggle`: switch / toggle a tag over IPC
- `@ipc:xxx`: send `xxx` verbatim over IPC
- anything else: run via `/bin/sh -c`
