picom-xrdesktop-companion
========================

* [picom-xrdesktop-companion](#picom-xrdesktop-companion)
   * [What is this](#what-is-this)
   * [How to use](#how-to-use)
   * [Installation](#installation)
      * [Dependencies](#dependencies)
      * [Building](#building)
   * [Limitations](#limitations)
      * [Bugs](#bugs)
      * [Window stacking](#window-stacking)
      * [Scene mode](#scene-mode)
   * [Questions](#questions)
   * [Acknowledgements](#acknowledgements)

## What is this

`picom-xrdesktop-companion` is a program that runs alongside [`picom`](/yshui/picom), that mirrors your desktop windows to the VR/XR space.

To see what it would look like, you can watch this video by the developers of xrdesktop:

[![xrdesktop](https://img.youtube.com/vi/RXQnWJpMLn4/sddefault.jpg)](https://www.youtube.com/watch?v=RXQnWJpMLn4)

There are also a bunch of example videos in [this article](https://www.collabora.com/news-and-blog/news-and-events/moving-the-linux-desktop-to-another-reality.html).

## How to use

First, you need to [build this program](#building)

To use this program, you must have `picom` installed on your system. For now, only the latest git version of `picom` is supported. ([AUR](https://aur.archlinux.org/packages/picom-git)).

First, make sure `picom` is running with dbus support enabled:

```
picom --dbus
```

Then, make sure SteamVR is running. And after that, start this program. You should see your windows mirrored.

## Installation

### Dependencies

* Rust (Nightly channel) [installation options](https://www.rust-lang.org/tools/install)
* [xrdesktop](https://gitlab.freedesktop.org/xrdesktop/xrdesktop)([AUR](https://aur.archlinux.org/packages/xrdesktop)) and its dependencies:
  * [gxr](https://gitlab.freedesktop.org/xrdesktop/gxr)([AUR](https://aur.archlinux.org/packages/gxr))
  * [gulkan](https://gitlab.freedesktop.org/xrdesktop/gulkan)([AUR](https://aur.archlinux.org/packages/gulkan))
* [libinputsynth](https://gitlab.freedesktop.org/xrdesktop/libinputsynth)([AUR](https://aur.archlinux.org/packages/libinputsynth-git))
* Vulkan
* OpenGL
* gtk3

### Building

Simply run

```
cargo build --release
```

The resulting binary will be at `./target/release/app`

## Limitations

### Bugs

Both this program and xrdesktop are in their early stages, so bugs and crashes can often happen. Feel free to open an issue here for problems you have encountered.

### Window stacking

When you move your pointer over a window in VR, the corresponding window must to raised to the top of the window stack to make sure it is not obscured and is able to receive input. This program makes best effort attempt to do that, but it might not work well with all window managers or programs.

It also changes how your windows are stacked and doesn't attempt to restore it after you stop window mirroring.

### Scene mode

Scene mode isn't supported currently. Please change default mode to "overlay" in xrdesktop settings.

## Questions

**Why `picom`?**

Believe it or not, this has nothing to do with `picom` being a compositor. Let me explain.

X has a very low level concept of "windows" and is not at all what we users would consider "windows". A set of conventions have developed over the decades to establish what we would consider as a window. These conventions are used by window managers to establish their internal knowledge of windows. Unfortunately this knowledge is only internal to the window manager, and if you want to, say, know what windows are open at the moment, you need to replicate at least part of what the window manager is doing.

And believe me, what it does is arcane. Besides that, your program also has to interact with the window manager and interpret its manipulations of windows correctly. The whole thing is really complex and complicated and you really don't want to do it. That's why xrdesktop developers chose to only integrate xrdesktop as part of the window manager (i.e. as a [plugin](https://gitlab.freedesktop.org/xrdesktop/kwin-effect-xrdesktop), or by [patching the window manager](https://gitlab.freedesktop.org/xrdesktop/gnome-shell.git)), instead of making it standalone. (BTW, please stop using X and switch to a wayland compositor if you can).

OTOH, `picom`, as a compositor, has already implemented the arcane window managing functionality just to function as a compositor. And fortunately, it also has a dbus interface that exposes its knowledge regarding windows. And that's what this program uses to mirror your windows. That's the only part of `picom` this program needs, so you don't even have to run `picom` as your compositor. If you want, you can run:

```
picom --dbus --experimental-backends --backend dummy
```

this way picom will provide the window information but not run as a compositor.

## Acknowledgements

Thanks to the good people at [Collabora](https://www.collabora.com) that made xrdesktop. They also make a ton of interesting stuff and you have probably already heard of them if you are into Linux gaming, so check them out.
