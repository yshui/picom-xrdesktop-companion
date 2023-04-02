# Gulkan

A GLib library for Vulkan abstraction. It provides classes for handling Vulkan instances, devices, shaders and initialize textures GDK Pixbufs, Cairo surfaces and DMA buffers.

## Build

#### Configure the project
```
$ meson build
```

#### Compile the project
```
$ ninja -C build
```

## Run

#### Run the examples
```
$ ./build/examples/glfw_pixbuf
```

#### Run the tests
```
$ ninja -C build test
```

### Build documentation
```
meson build -Dapi_doc=true
ninja -C build gulkan-doc
```

### Vulkan validation

The vulkan loader supports turning on layers with an environment variable.

For recent Vulkan SDKS (>= 1.1.106): `VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation`.

For older Vulkan SDKs: `VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_core_validation`.

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

