![xrdesktop Logo](doc/xrdesktop.png)

xrdesktop
=========

A library for XR interaction with classical desktop compositors.

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
ninja -C build xrdesktop-doc
```

## Run

#### Run the examples
Run the client in the default mode from settings.

```
$ ./build/examples/client
```

Override the default mode.

```
$ ./build/examples/client -m scene
```

Override the default API.

```
$ GXR_API=openxr ./build/examples/client -m scene
```

#### Build gsettings for running without installing
To run the client example without xrdesktop being installed, you need to
build the glib schemas in `res`

```
$ glib-compile-schemas res/
```

And add the `GSETTINGS_SCHEMA_DIR`

```
$ GSETTINGS_SCHEMA_DIR=res/ ./build/examples/client
```


#### Run the tests

Run all tests

```
$ ninja -C build test
```

Don't run tests that require a running XR runtime.

```
meson test -C build/ --no-suite xrdesktop:xr
```

Don't run tests that require a running XR runtime or the installed package.

```
meson test -C build/ --no-suite xrdesktop:xr --no-suite xrdesktop:post-install
```

Since meson `0.46` the project name can be omitted from the test suite:

```
meson test -C build/ --no-suite xr --no-suite post-install
```

## Contact

You can submit issues in our [issue tracker](https://gitlab.freedesktop.org/xrdesktop/xrdesktop/issues).

Join our chats, *#xrdesktop* on freenode or [Discord](https://discord.gg/msETben).

## Documentation

For getting started read the [Howto Guide](https://gitlab.freedesktop.org/xrdesktop/xrdesktop/wikis/howto) in our wiki. We also have a generated [API documentation](https://xrdesktop.freedesktop.org/doc/).

## License

xrdesktop is licensed under MIT.

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

