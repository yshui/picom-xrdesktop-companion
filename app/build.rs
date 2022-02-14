use std::path::Path;

use gl_generator::{Api, Fallbacks, Profile, Registry, StructGenerator};
fn main() {
    let dest = std::env::var("OUT_DIR").unwrap();
    let mut file = std::fs::File::create(&Path::new(&dest).join("gl_bindings.rs")).unwrap();
    Registry::new(Api::Gl, (4, 5), Profile::Core, Fallbacks::None, [])
        .write_bindings(StructGenerator, &mut file)
        .unwrap();
}
