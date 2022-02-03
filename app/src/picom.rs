use zbus::dbus_proxy;

/// # DBus interface proxy for: `com.github.chjj.compton`
#[dbus_proxy(interface = "com.github.chjj.compton", default_path = "/com/github/chjj/compton")]
trait Picom {
    /// repaint method
    fn repaint(&self) -> zbus::Result<()>;

    /// reset method
    fn reset(&self) -> zbus::Result<()>;

    /// win_added signal
    #[dbus_proxy(signal)]
    fn win_added(&self, wid: u32) -> zbus::Result<()>;

    /// win_destroyed signal
    #[dbus_proxy(signal)]
    fn win_destroyed(&self, wid: u32) -> zbus::Result<()>;

    /// win_focusin signal
    #[dbus_proxy(signal)]
    fn win_focusin(&self, wid: u32) -> zbus::Result<()>;

    /// win_focusout signal
    #[dbus_proxy(signal)]
    fn win_focusout(&self, wid: u32) -> zbus::Result<()>;

    /// win_mapped signal
    #[dbus_proxy(signal)]
    fn win_mapped(&self, wid: u32) -> zbus::Result<()>;

    /// win_unmapped signal
    #[dbus_proxy(signal)]
    fn win_unmapped(&self, wid: u32) -> zbus::Result<()>;
}

#[dbus_proxy(interface = "picom.Window")]
trait Window {
    /// ClientWin property
    #[dbus_proxy(property)]
    fn client_win(&self) -> zbus::Result<u32>;

    /// Id property
    #[dbus_proxy(property)]
    fn id(&self) -> zbus::Result<u32>;

    /// Leader property
    #[dbus_proxy(property)]
    fn leader(&self) -> zbus::Result<u32>;

    /// Mapped property
    #[dbus_proxy(property)]
    fn mapped(&self) -> zbus::Result<bool>;

    /// Name property
    #[dbus_proxy(property)]
    fn name(&self) -> zbus::Result<String>;

    /// Next property
    #[dbus_proxy(property)]
    fn next(&self) -> zbus::Result<u32>;

    /// RawFocused property
    #[dbus_proxy(property)]
    fn raw_focused(&self) -> zbus::Result<bool>;
}
