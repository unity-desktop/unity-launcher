## unity-launcher

A library for the Unity launcher and dash, for use in unity-shell.

It builds on [Astal](https://github.com/unity-desktop/astal) and gives you two windows.

- **`UnityLauncher`**: the vertical panel. It shows pinned and running apps, tracks Wayland toplevels, and launches or focuses them on click.
- **`UnityDash`**: the overlay. It browses installed apps in a grid and filters them as you type.

The app list is modelled on its own, so the launcher and the dash share one source.

- **`UnityAppList`**: a `GListModel` of `UnityAppEntry` objects. It starts from the pinned app ids and updates as toplevels come and go.
- **`UnityAppEntry`**: one application. It holds the `GAppInfo`, the toplevels, and the pinned, running, and activated state.

### build

Install the dependencies.

- `gtk4` (>= 4.22)
- `libadwaita-1` (>= 1.8)
- `gio-2.0`
- `gio-unix-2.0`
- `graphene-1.0`
- `wayland-client`
- `astal-4-4.0`
- `astal-apps-0.1`
- `astal-wlr-0.1`
- `meson`

Then build and install.

```sh
meson setup build
ninja -C build
sudo meson install -C build
```
