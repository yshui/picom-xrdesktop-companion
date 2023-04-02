#[cfg(feature = "dox")]
fn main() {} // prevent linking libraries to avoid documentation failure

#[cfg(not(feature = "dox"))]
fn main() {
    // build xrdesktop
    use std::path::Path;
    let out_dir = Path::new(&std::env::var("OUT_DIR").unwrap()).to_owned();
    let buildtype = format!("--buildtype={}", std::env::var("PROFILE").unwrap());
    let manifest_dir = Path::new(&std::env::var("CARGO_MANIFEST_DIR").unwrap()).to_owned();
    std::fs::remove_dir_all(out_dir.join("xrd_build")).unwrap_or(());

    let gulkan_dir = Path::new(&std::env::var("DEP_GULKAN_0.15_OUT_DIR").unwrap()).to_owned();
    let gxr_dir = Path::new(&std::env::var("DEP_GXR_0.15_OUT_DIR").unwrap()).to_owned();
    let pkg_config_path = format!(
        "{}:{}:{}",
        gulkan_dir.join("lib").join("pkgconfig").display(),
        gxr_dir.join("lib").join("pkgconfig").display(),
        std::env::var("PKG_CONFIG_PATH").unwrap_or_default()
    );
    std::env::set_var("PKG_CONFIG_PATH", &pkg_config_path);
    let rc = std::process::Command::new("meson")
        .arg("setup")
        .args([&buildtype, "--prefix"])
        .arg(out_dir.join("xrd"))
        .arg(out_dir.join("xrd_build"))
        .arg(manifest_dir.join("xrdesktop"))
        .status()
        .unwrap();
    assert!(rc.success());
    std::process::Command::new("meson")
        .args(["compile", "-C"])
        .arg(out_dir.join("xrd_build"))
        .status()
        .unwrap();
    assert!(rc.success());
    std::process::Command::new("meson")
        .args(["install", "-C"])
        .arg(out_dir.join("xrd_build"))
        .status()
        .unwrap();
    assert!(rc.success());
    let pkg_config_path = format!(
        "{}:{}",
        out_dir.join("xrd").join("lib").join("pkgconfig").display(),
        pkg_config_path,
    );
    std::env::set_var("PKG_CONFIG_PATH", pkg_config_path);
    println!("cargo:rerun-if-changed=xrdesktop");
    println!("cargo:rerun-if-env-changed=DEP_GULKAN_0.15_OUT_DIR");
    println!("cargo:rerun-if-env-changed=DEP_GXR_OUT_0.15_DIR");
    println!("cargo:OUT_DIR={}", out_dir.join("xrd").display());

    println!("cargo:rustc-link-search=native={}", out_dir.join("xrd").join("lib").display());
}
