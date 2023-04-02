#[cfg(feature = "dox")]
fn main() {} // prevent linking libraries to avoid documentation failure

#[cfg(not(feature = "dox"))]
fn main() {
    // build gulkan
    use std::path::Path;
    let out_dir = Path::new(&std::env::var("OUT_DIR").unwrap()).to_owned();
    let buildtype = format!("--buildtype={}", std::env::var("PROFILE").unwrap());
    let manifest_dir = Path::new(&std::env::var("CARGO_MANIFEST_DIR").unwrap()).to_owned();
    std::fs::remove_dir_all(out_dir.join("gulkan_build")).unwrap_or(());
    let rc = std::process::Command::new("meson")
        .arg("setup")
        .args([&buildtype, "--prefix"])
        .arg(out_dir.join("gulkan"))
        .arg(out_dir.join("gulkan_build"))
        .arg(manifest_dir.join("gulkan"))
        .status()
        .unwrap();
    assert!(rc.success());
    std::process::Command::new("meson")
        .args(["compile", "-C"])
        .arg(out_dir.join("gulkan_build"))
        .status()
        .unwrap();
    assert!(rc.success());
    std::process::Command::new("meson")
        .args(["install", "-C"])
        .arg(out_dir.join("gulkan_build"))
        .status()
        .unwrap();
    assert!(rc.success());
    println!("cargo:rerun-if-changed=gulkan");
    println!(
        "cargo:rustc-link-search=native={}",
        out_dir.join("gulkan").join("lib").display()
    );
    println!("cargo:OUT_DIR={}", out_dir.join("gulkan").display());
}
