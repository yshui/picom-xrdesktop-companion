#[allow(unused_imports)]
use log::*;
use gio::prelude::*;
fn main() {
    env_logger::init();
    if !xrd::settings_is_schema_installed() {
        panic!("xrdesktop GSettings Schema not installed");
    }

    let settings = xrd::settings_get_instance().unwrap();
    let mode: xrd::ClientMode = unsafe { glib::translate::from_glib(settings.enum_("default-mode")) };
    println!("{}", mode);

    let client = xrd::Client::with_mode(mode);
}
