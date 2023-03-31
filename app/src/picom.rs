use zbus::dbus_proxy;

/// # DBus interface proxy for: `com.github.chjj.compton`
#[dbus_proxy(
    interface = "com.github.chjj.compton",
    default_path = "/com/github/chjj/compton"
)]
trait Picom {
    /// repaint method
    fn repaint(&self) -> zbus::Result<()>;

    /// reset method
    fn reset(&self) -> zbus::Result<()>;
}

#[dbus_proxy(
    interface = "picom.Compositor",
    default_path = "/com/github/chjj/compton"
)]
trait Compositor {
    /// WinAdded signal
    #[dbus_proxy(signal)]
    fn win_added(&self, wid: u32) -> zbus::Result<()>;

    /// WinDestroyed signal
    #[dbus_proxy(signal)]
    fn win_destroyed(&self, wid: u32) -> zbus::Result<()>;

    /// WinMapped signal
    #[dbus_proxy(signal)]
    fn win_mapped(&self, wid: u32) -> zbus::Result<()>;

    /// WinUnmapped signal
    #[dbus_proxy(signal)]
    fn win_unmapped(&self, wid: u32) -> zbus::Result<()>;
}

#[dbus_proxy(interface = "picom.Window", assume_defaults = false)]
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

    /// Type property
    #[dbus_proxy(property)]
    fn type_(&self) -> zbus::Result<String>;
}
