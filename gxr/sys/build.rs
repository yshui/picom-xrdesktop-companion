#[cfg(feature = "dox")]
fn main() {} // prevent linking libraries to avoid documentation failure

#[cfg(not(feature = "dox"))]
fn main() {
    // build gxr
    use std::path::Path;

    use pkg_config::Config as PkgConfig;
    let out_dir = Path::new(&std::env::var("OUT_DIR").unwrap()).to_owned();
    let buildtype = format!("--buildtype={}", std::env::var("PROFILE").unwrap());
    let manifest_dir = Path::new(&std::env::var("CARGO_MANIFEST_DIR").unwrap()).to_owned();
    std::fs::remove_dir_all(out_dir.join("gxr_build")).unwrap_or(());

    let gulkan_dir = Path::new(&std::env::var("DEP_GULKAN_0.15_OUT_DIR").unwrap()).to_owned();
    let pkg_config_path = format!(
        "{}:{}",
        gulkan_dir.join("lib").join("pkgconfig").display(),
        std::env::var("PKG_CONFIG_PATH").unwrap_or_default()
    );
    std::env::set_var("PKG_CONFIG_PATH", &pkg_config_path);
    let rc = std::process::Command::new("meson")
        .arg("setup")
        .args([&buildtype, "-Dbackends=openvr,openxr", "--prefix"])
        .arg(out_dir.join("gxr"))
        .arg(out_dir.join("gxr_build"))
        .arg(manifest_dir.join("gxr"))
        .status()
        .unwrap();
    assert!(rc.success());
    std::process::Command::new("meson")
        .args(["compile", "-C"])
        .arg(out_dir.join("gxr_build"))
        .status()
        .unwrap();
    assert!(rc.success());
    std::process::Command::new("meson")
        .args(["install", "-C"])
        .arg(out_dir.join("gxr_build"))
        .status()
        .unwrap();
    assert!(rc.success());
    println!("cargo:rerun-if-changed=gxr");
    println!("cargo:rerun-if-env-changed=DEP_GULKAN_OUT_DIR");
    println!("cargo:rustc-link-search=native={}", out_dir.join("gxr").join("lib").display());
    println!("cargo:OUT_DIR={}", out_dir.join("gxr").display());

    let mut pc = PkgConfig::new();
    pc.atleast_version("3.22").probe("gdk-3.0").unwrap();
    pc.atleast_version("1.4.18").probe("openvr").unwrap();
    pc.range_version(..);
    pc.probe("json-glib-1.0").unwrap();
    pc.probe("openxr").unwrap();
}
