// Generated by gir (https://github.com/gtk-rs/gir @ 2358cc24efd2)
// from ../../gir-files (@ eab91ba8f88b)
// from ../../xrd-gir-files (@ 2f1ef32fd867)
// DO NOT EDIT

#![cfg(target_os = "linux")]

use gxr_sys::*;
use std::mem::{align_of, size_of};
use std::env;
use std::error::Error;
use std::ffi::OsString;
use std::path::Path;
use std::process::Command;
use std::str;
use tempfile::Builder;

static PACKAGES: &[&str] = &["gxr-0.16"];

#[derive(Clone, Debug)]
struct Compiler {
    pub args: Vec<String>,
}

impl Compiler {
    pub fn new() -> Result<Self, Box<dyn Error>> {
        let mut args = get_var("CC", "cc")?;
        args.push("-Wno-deprecated-declarations".to_owned());
        // For _Generic
        args.push("-std=c11".to_owned());
        // For %z support in printf when using MinGW.
        args.push("-D__USE_MINGW_ANSI_STDIO".to_owned());
        args.extend(get_var("CFLAGS", "")?);
        args.extend(get_var("CPPFLAGS", "")?);
        args.extend(pkg_config_cflags(PACKAGES)?);
        Ok(Self { args })
    }

    pub fn compile(&self, src: &Path, out: &Path) -> Result<(), Box<dyn Error>> {
        let mut cmd = self.to_command();
        cmd.arg(src);
        cmd.arg("-o");
        cmd.arg(out);
        let status = cmd.spawn()?.wait()?;
        if !status.success() {
            return Err(format!("compilation command {cmd:?} failed, {status}").into());
        }
        Ok(())
    }

    fn to_command(&self) -> Command {
        let mut cmd = Command::new(&self.args[0]);
        cmd.args(&self.args[1..]);
        cmd
    }
}

fn get_var(name: &str, default: &str) -> Result<Vec<String>, Box<dyn Error>> {
    match env::var(name) {
        Ok(value) => Ok(shell_words::split(&value)?),
        Err(env::VarError::NotPresent) => Ok(shell_words::split(default)?),
        Err(err) => Err(format!("{name} {err}").into()),
    }
}

fn pkg_config_cflags(packages: &[&str]) -> Result<Vec<String>, Box<dyn Error>> {
    if packages.is_empty() {
        return Ok(Vec::new());
    }
    let pkg_config = env::var_os("PKG_CONFIG")
        .unwrap_or_else(|| OsString::from("pkg-config"));
    let mut cmd = Command::new(pkg_config);
    cmd.arg("--cflags");
    cmd.args(packages);
    let out = cmd.output()?;
    if !out.status.success() {
        return Err(format!("command {cmd:?} returned {}", out.status).into());
    }
    let stdout = str::from_utf8(&out.stdout)?;
    Ok(shell_words::split(stdout.trim())?)
}


#[derive(Copy, Clone, Debug, Eq, PartialEq)]
struct Layout {
    size: usize,
    alignment: usize,
}

#[derive(Copy, Clone, Debug, Default, Eq, PartialEq)]
struct Results {
    /// Number of successfully completed tests.
    passed: usize,
    /// Total number of failed tests (including those that failed to compile).
    failed: usize,
}

impl Results {
    fn record_passed(&mut self) {
        self.passed += 1;
    }
    fn record_failed(&mut self) {
        self.failed += 1;
    }
    fn summary(&self) -> String {
        format!("{} passed; {} failed", self.passed, self.failed)
    }
    fn expect_total_success(&self) {
        if self.failed == 0 {
            println!("OK: {}", self.summary());
        } else {
            panic!("FAILED: {}", self.summary());
        };
    }
}

#[test]
fn cross_validate_constants_with_c() {
    let mut c_constants: Vec<(String, String)> = Vec::new();

    for l in get_c_output("constant").unwrap().lines() {
        let (name, value) = l.split_once(';').expect("Missing ';' separator");
        c_constants.push((name.to_owned(), value.to_owned()));
    }

    let mut results = Results::default();

    for ((rust_name, rust_value), (c_name, c_value)) in
        RUST_CONSTANTS.iter().zip(c_constants.iter())
    {
        if rust_name != c_name {
            results.record_failed();
            eprintln!("Name mismatch:\nRust: {rust_name:?}\nC:    {c_name:?}");
            continue;
        }

        if rust_value != c_value {
            results.record_failed();
            eprintln!(
                "Constant value mismatch for {rust_name}\nRust: {rust_value:?}\nC:    {c_value:?}",
            );
            continue;
        }

        results.record_passed();
    }

    results.expect_total_success();
}

#[test]
fn cross_validate_layout_with_c() {
    let mut c_layouts = Vec::new();

    for l in get_c_output("layout").unwrap().lines() {
        let (name, value) = l.split_once(';').expect("Missing first ';' separator");
        let (size, alignment) = value.split_once(';').expect("Missing second ';' separator");
        let size = size.parse().expect("Failed to parse size");
        let alignment = alignment.parse().expect("Failed to parse alignment");
        c_layouts.push((name.to_owned(), Layout { size, alignment }));
    }

    let mut results = Results::default();

    for ((rust_name, rust_layout), (c_name, c_layout)) in
        RUST_LAYOUTS.iter().zip(c_layouts.iter())
    {
        if rust_name != c_name {
            results.record_failed();
            eprintln!("Name mismatch:\nRust: {rust_name:?}\nC:    {c_name:?}");
            continue;
        }

        if rust_layout != c_layout {
            results.record_failed();
            eprintln!(
                "Layout mismatch for {rust_name}\nRust: {rust_layout:?}\nC:    {c_layout:?}",
            );
            continue;
        }

        results.record_passed();
    }

    results.expect_total_success();
}

fn get_c_output(name: &str) -> Result<String, Box<dyn Error>> {
    let tmpdir = Builder::new().prefix("abi").tempdir()?;
    let exe = tmpdir.path().join(name);
    let c_file = Path::new("tests").join(name).with_extension("c");

    let cc = Compiler::new().expect("configured compiler");
    cc.compile(&c_file, &exe)?;

    let mut abi_cmd = Command::new(exe);
    let output = abi_cmd.output()?;
    if !output.status.success() {
        return Err(format!("command {abi_cmd:?} failed, {output:?}").into());
    }

    Ok(String::from_utf8(output.stdout)?)
}

const RUST_LAYOUTS: &[(&str, Layout)] = &[
    ("GxrActionClass", Layout {size: size_of::<GxrActionClass>(), alignment: align_of::<GxrActionClass>()}),
    ("GxrActionManifestEntry", Layout {size: size_of::<GxrActionManifestEntry>(), alignment: align_of::<GxrActionManifestEntry>()}),
    ("GxrActionSetClass", Layout {size: size_of::<GxrActionSetClass>(), alignment: align_of::<GxrActionSetClass>()}),
    ("GxrActionType", Layout {size: size_of::<GxrActionType>(), alignment: align_of::<GxrActionType>()}),
    ("GxrAnalogEvent", Layout {size: size_of::<GxrAnalogEvent>(), alignment: align_of::<GxrAnalogEvent>()}),
    ("GxrBinding", Layout {size: size_of::<GxrBinding>(), alignment: align_of::<GxrBinding>()}),
    ("GxrBindingComponent", Layout {size: size_of::<GxrBindingComponent>(), alignment: align_of::<GxrBindingComponent>()}),
    ("GxrBindingManifest", Layout {size: size_of::<GxrBindingManifest>(), alignment: align_of::<GxrBindingManifest>()}),
    ("GxrBindingMode", Layout {size: size_of::<GxrBindingMode>(), alignment: align_of::<GxrBindingMode>()}),
    ("GxrBindingPath", Layout {size: size_of::<GxrBindingPath>(), alignment: align_of::<GxrBindingPath>()}),
    ("GxrBindingType", Layout {size: size_of::<GxrBindingType>(), alignment: align_of::<GxrBindingType>()}),
    ("GxrContextClass", Layout {size: size_of::<GxrContextClass>(), alignment: align_of::<GxrContextClass>()}),
    ("GxrControllerClass", Layout {size: size_of::<GxrControllerClass>(), alignment: align_of::<GxrControllerClass>()}),
    ("GxrDevice", Layout {size: size_of::<GxrDevice>(), alignment: align_of::<GxrDevice>()}),
    ("GxrDeviceClass", Layout {size: size_of::<GxrDeviceClass>(), alignment: align_of::<GxrDeviceClass>()}),
    ("GxrDeviceManagerClass", Layout {size: size_of::<GxrDeviceManagerClass>(), alignment: align_of::<GxrDeviceManagerClass>()}),
    ("GxrDigitalEvent", Layout {size: size_of::<GxrDigitalEvent>(), alignment: align_of::<GxrDigitalEvent>()}),
    ("GxrEye", Layout {size: size_of::<GxrEye>(), alignment: align_of::<GxrEye>()}),
    ("GxrManifestClass", Layout {size: size_of::<GxrManifestClass>(), alignment: align_of::<GxrManifestClass>()}),
    ("GxrOverlayEvent", Layout {size: size_of::<GxrOverlayEvent>(), alignment: align_of::<GxrOverlayEvent>()}),
    ("GxrPose", Layout {size: size_of::<GxrPose>(), alignment: align_of::<GxrPose>()}),
    ("GxrPoseEvent", Layout {size: size_of::<GxrPoseEvent>(), alignment: align_of::<GxrPoseEvent>()}),
    ("GxrStateChange", Layout {size: size_of::<GxrStateChange>(), alignment: align_of::<GxrStateChange>()}),
    ("GxrStateChangeEvent", Layout {size: size_of::<GxrStateChangeEvent>(), alignment: align_of::<GxrStateChangeEvent>()}),
];

const RUST_CONSTANTS: &[(&str, &str)] = &[
    ("(gint) GXR_ACTION_DIGITAL", "0"),
    ("(gint) GXR_ACTION_DIGITAL_FROM_FLOAT", "1"),
    ("(gint) GXR_ACTION_FLOAT", "3"),
    ("(gint) GXR_ACTION_HAPTIC", "5"),
    ("(gint) GXR_ACTION_POSE", "4"),
    ("(gint) GXR_ACTION_VEC2F", "2"),
    ("(gint) GXR_BINDING_COMPONENT_CLICK", "2"),
    ("(gint) GXR_BINDING_COMPONENT_FORCE", "6"),
    ("(gint) GXR_BINDING_COMPONENT_NONE", "0"),
    ("(gint) GXR_BINDING_COMPONENT_POSITION", "4"),
    ("(gint) GXR_BINDING_COMPONENT_PULL", "3"),
    ("(gint) GXR_BINDING_COMPONENT_TOUCH", "5"),
    ("(gint) GXR_BINDING_COMPONENT_UNKNOWN", "1"),
    ("(gint) GXR_BINDING_MODE_ANALOG_STICK", "4"),
    ("(gint) GXR_BINDING_MODE_BUTTON", "2"),
    ("(gint) GXR_BINDING_MODE_NONE", "0"),
    ("(gint) GXR_BINDING_MODE_TRACKPAD", "3"),
    ("(gint) GXR_BINDING_MODE_UNKNOWN", "1"),
    ("(gint) GXR_BINDING_TYPE_BOOLEAN", "2"),
    ("(gint) GXR_BINDING_TYPE_FLOAT", "3"),
    ("(gint) GXR_BINDING_TYPE_HAPTIC", "5"),
    ("(gint) GXR_BINDING_TYPE_POSE", "1"),
    ("(gint) GXR_BINDING_TYPE_UNKNOWN", "0"),
    ("(gint) GXR_BINDING_TYPE_VEC2", "4"),
    ("GXR_DEVICE_INDEX_MAX", "64"),
    ("(gint) GXR_EYE_LEFT", "0"),
    ("(gint) GXR_EYE_RIGHT", "1"),
    ("(gint) GXR_STATE_FRAMECYCLE_START", "0"),
    ("(gint) GXR_STATE_FRAMECYCLE_STOP", "1"),
    ("(gint) GXR_STATE_RENDERING_START", "2"),
    ("(gint) GXR_STATE_RENDERING_STOP", "3"),
    ("(gint) GXR_STATE_SHUTDOWN", "4"),
];


