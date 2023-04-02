gxr
===

A GLib XR library utilizing the OpenXR and OpenVR APIs.

## Build

#### Configure the project
```
$ meson build
```

#### Compile the project
```
$ ninja -C build
```

#### Build the docs
```
meson build -Dapi_doc=true
$ ninja -C build gxr-doc
```

## Run

#### Run the cube locally
```
$ GXR_BACKEND_DIR=build/src/ GXR_API=openxr ./build/examples/cube/gxr-cube
```

#### Run the overlay examples (OpenVR only)
```
$ GXR_BACKEND_DIR=build/src/ GXR_API=openvr ./build/examples/overlay_pixbuf
$ GXR_BACKEND_DIR=build/src/ GXR_API=openvr ./build/examples/overlay_cairo_vulkan_animation
$ GXR_BACKEND_DIR=build/src/ GXR_API=openvr ./build/examples/overlay_pixbuf_vulkan_multi
```

#### Run the tests
Run all tests.
```
$ ninja -C build test
```

Don't run tests that require a running XR runtime.
```
meson test -C build/ --no-suite gxr:xr
```

Since meson `0.46` the project name can be omitted from the test suite:
```
meson test -C build/ --no-suite xr

```

## Code of Conduct

Please note that this project is released with a Contributor Code of Conduct.
By participating in this project you agree to abide by its terms.

We follow the standard freedesktop.org code of conduct,
available at <https://www.freedesktop.org/wiki/CodeOfConduct/>,
which is based on the [Contributor Covenant](https://www.contributor-covenant.org).

Instances of abusive, harassing, or otherwise unacceptable behavior may be
reported by contacting:

* First-line project contacts:
  * Lubosz Sarnecki <lubosz.sarnecki@collabora.com>
  * Christoph Haag <christoph.haag@collabora.com>
* freedesktop.org contacts: see most recent list at <https://www.freedesktop.org/wiki/CodeOfConduct/>

