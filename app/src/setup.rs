use std::io::Write;

/// Do prerequisite setups
///
/// Right now this is just installing settings schema for xrdesktop
pub fn setup() {
    let schema_source =
        gio::SettingsSchemaSource::default().expect("Failed to get default schema source");
    if schema_source.lookup("org.xrdesktop", true).is_some() {
        return;
    }

    log::info!("Installing xrdesktop settings schema");
    let data_dir = dirs::data_local_dir().expect("Failed to get data dir");
    let schema_dir = data_dir.join("glib-2.0").join("schemas");
    std::fs::create_dir_all(&schema_dir).expect("Failed to create schema dir");
    {
        let mut file = std::fs::File::create(schema_dir.join("org.xrdesktop.gschema.xml"))
            .expect("Failed to create schema file");

        write!(file, "{}", xrd::sys::SCHEMA).expect("Failed to write schema file");
    }

    let output = std::process::Command::new("glib-compile-schemas")
        .arg(schema_dir)
        .output()
        .expect("Failed to compile schema");
    log::info!("glib-compile-schemas output: {}", String::from_utf8_lossy(&output.stdout));
}
