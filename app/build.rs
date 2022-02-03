use std::path::Path;

use gl_generator::{Api, Fallbacks, StructGenerator, Profile, Registry};
fn main() {
    let dest = std::env::var("OUT_DIR").unwrap();
    let lib = pkg_config::Config::new()
        .atleast_version("0.16")
        .probe("libinputsynth-0.16")
        .unwrap();
    bindgen::builder()
        .header("import.h")
        .clang_args(
            lib.include_paths
                .iter()
                .map(|p| format!("-I{}", p.display())),
        )
        .rustified_enum(".*")
        .allowlist_function("input_synth.*")
        .allowlist_type("Input.*")
        .blocklist_type("_?InputSynthClass.*")
        .generate()
        .unwrap()
        .write_to_file(Path::new(&dest).join("inputsynth.rs"))
        .unwrap();
    println!("cargo:rustc-link-lib=inputsynth-0.16");
    let mut file = std::fs::File::create(&Path::new(&dest).join("gl_bindings.rs")).unwrap();
    Registry::new(Api::Gl, (4, 5), Profile::Core, Fallbacks::None, [])
        .write_bindings(StructGenerator, &mut file)
        .unwrap();
}
