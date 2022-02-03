use glium::{backend::Facade, framebuffer::ToColorAttachment, uniforms::AsUniformValue, Texture2d};
use glutin::platform::{
    unix::{RawHandle, WindowExtUnix},
    ContextTraitExt,
};
use std::{collections::HashMap, ffi::c_void, os::unix::prelude::RawFd, sync::Arc};

use tokio::sync::{mpsc, oneshot};
use x11rb::{connection::Connection, protocol::xproto, rust_connection::RustConnection};
#[derive(Debug)]
pub struct Texture {
    id: usize,
    width: u32,
    height: u32,
}
impl Texture {
    pub fn width(&self) -> u32 {
        self.width
    }
    pub fn height(&self) -> u32 {
        self.height
    }
}
#[derive(Debug)]
enum Request {
    BindTexture {
        pixmap: xproto::Pixmap,
        visual: xproto::Visualid,
        reply: oneshot::Sender<Result<Texture>>,
    },
    ImportFd {
        width: u32,
        height: u32,
        fd: RawFd,
        size: u64,
        reply: oneshot::Sender<Result<Texture>>,
    },
    ReleaseTexture {
        texture: Texture,
        reply: oneshot::Sender<Result<()>>,
    },
    Blit {
        src: usize,
        dst: usize,
        reply: oneshot::Sender<Result<()>>,
    },
    Capture {
        start: bool,
        reply: oneshot::Sender<Result<()>>,
    },
}

const GLX_BIND_TO_TEXTURE_TARGETS_EXT: libc::c_int = 0x20D3;
const GLX_TEXTURE_FORMAT_EXT: libc::c_int = 0x20D5;
const GLX_TEXTURE_TARGET_EXT: libc::c_int = 0x20D6;
const GLX_TEXTURE_FORMAT_RGB_EXT: libc::c_int = 0x20D9;
const GLX_TEXTURE_FORMAT_RGBA_EXT: libc::c_int = 0x20DA;
const GLX_TEXTURE_2D_EXT: libc::c_int = 0x20DC;
const GLX_FRONT_LEFT_EXT: libc::c_int = 0x20DE;

#[derive(thiserror::Error, Debug)]
pub enum Error {
    #[error("{0}")]
    DisplayCreation(#[from] glium::backend::glutin::DisplayCreationError),
    #[error("{0}")]
    TextureCreation(#[from] glium::texture::TextureCreationError),
    #[error("{0}")]
    IncompatibleOpenGl(#[from] glium::IncompatibleOpenGl),
    #[error("{0}")]
    Recv(#[from] tokio::sync::oneshot::error::RecvError),
    #[error("send error")]
    Send,
    #[error("can't load libGLX {0}")]
    Loading(#[from] libloading::Error),
    #[error("{0}")]
    GlutinOs(#[from] glutin::error::OsError),
    #[error("{0}")]
    TextureImport(#[from] glium::texture::TextureImportError),
    #[error("{0}")]
    FramebufferValidation(#[from] glium::framebuffer::ValidationError),
    #[error("{0}")]
    Draw(#[from] glium::DrawError),
    #[error("{0}")]
    X11Reply(#[from] x11rb::errors::ReplyError),
    #[error("{0}")]
    X11Connection(#[from] x11rb::errors::ConnectionError),

    #[error("visual {0} is invalid")]
    InvalidVisual(xproto::Visualid),
    #[error("no FBConfig found for visual {0}")]
    NoFbConfig(xproto::Visualid, #[backtrace] std::backtrace::Backtrace),
    #[error("failed to create GLXPixmap")]
    PixmapCreation,
}

type Result<T> = std::result::Result<T, Error>;

enum AnyTexture2d {
    Srgb(SrgbTexture2d),
    Linear(Texture2d),
}

impl<'a> ToColorAttachment<'a> for &'a AnyTexture2d {
    fn to_color_attachment(self) -> glium::framebuffer::ColorAttachment<'a> {
        match self {
            AnyTexture2d::Srgb(t) => t.to_color_attachment(),
            AnyTexture2d::Linear(t) => t.to_color_attachment(),
        }
    }
}
impl<'a> AsUniformValue for &'a AnyTexture2d {
    fn as_uniform_value(&self) -> glium::uniforms::UniformValue<'_> {
        match self {
            AnyTexture2d::Srgb(t) => t.as_uniform_value(),
            AnyTexture2d::Linear(t) => t.as_uniform_value(),
        }
    }
}

struct TextureInner {
    texture: AnyTexture2d,
    glxpixmap: Option<libc::c_int>,
}

struct GlInner {
    x11: Arc<RustConnection>,
    screen: u32,
    x11depths: Vec<xproto::Depth>,
    glium: glium::Display,
    glx: glutin_glx_sys::glx::Glx,
    bind_tex_image: unsafe extern "C" fn(*mut c_void, libc::c_int, libc::c_int, *const c_void),
    gl: ffi::Gl,
    textures: HashMap<usize, TextureInner>,
    blit_shader: glium::Program,
}

impl Drop for GlInner {
    fn drop(&mut self) {
        if self.textures.is_empty() {
            return;
        }
        let raw_display = self.glium.gl_window().window().xlib_display().unwrap();
        for (_, TextureInner { glxpixmap, .. }) in self.textures.drain() {
            if let Some(glxpixmap) = glxpixmap {
                unsafe { self.glx.DestroyPixmap(raw_display as _, glxpixmap as _) };
            }
        }
    }
}

use glium::{implement_vertex, texture::srgb_texture2d::SrgbTexture2d, Surface};
#[derive(Copy, Clone)]
struct Vertex {
    position: [f32; 2],
}
implement_vertex!(Vertex, position);

impl GlInner {
    fn new(x11: Arc<RustConnection>, screen: u32) -> Result<Self> {
        use glutin::platform::{unix::EventLoopExtUnix, ContextTraitExt};
        let el = glutin::event_loop::EventLoop::<()>::new_any_thread();
        let wb = glutin::window::WindowBuilder::new().with_visible(false);
        let cb = glutin::ContextBuilder::new()
            .with_vsync(false)
            .with_multisampling(0);
        let display = glium::Display::new(wb, cb, &el)?;
        assert!(unsafe { display.gl_window().get_egl_display().is_none() });
        let glx = unsafe {
            let libglx = libloading::Library::new("libGL.so")
                .or_else(|_| libloading::Library::new("libGL.so.1"))?;
            glutin_glx_sys::glx::Glx::load_with(|s| {
                libglx
                    .get(
                        std::ffi::CString::new(s.as_bytes())
                            .unwrap()
                            .as_bytes_with_nul(),
                    )
                    .map(|sym| *sym)
                    .unwrap_or(std::ptr::null_mut())
            })
        };
        use glium::program;
        let blit_shader = program!(&display,
            330 => {
                vertex: "
                    #version 330
                    in vec2 position;
                    out vec2 tex_coord;
                    void main() {
                        gl_Position = vec4(position, 0, 1);
                        tex_coord = position / 2.0 + vec2(0.5);
                    }
                ",
                fragment: "
                    #version 330
                    uniform sampler2D tex;
                    in vec4 gl_FragCoord;
                    in vec2 tex_coord;
                    out vec4 color;
                    void main() {
                        color = texture(tex, tex_coord);
                    }
                ",
                outputs_srgb: true,
            }
        )
        .unwrap();
        Ok(Self {
            x11depths: x11.setup().roots[screen as usize].allowed_depths.clone(),
            gl: ffi::Gl::load_with(|s| display.gl_window().get_proc_address(s)),
            glium: display,
            blit_shader,
            bind_tex_image: unsafe {
                std::mem::transmute(
                    glx.GetProcAddress(
                        std::ffi::CString::new("glXBindTexImageEXT")
                            .unwrap()
                            .as_bytes_with_nul()
                            .as_ptr(),
                    ),
                )
            },
            screen,
            glx,
            x11,
            textures: Default::default(),
        })
    }
    fn find_visual(&self, visual: xproto::Visualid) -> Option<(u8, &xproto::Visualtype)> {
        for d in &self.x11depths {
            for v in &d.visuals {
                if v.visual_id == visual {
                    return Some((d.depth, v));
                }
            }
        }
        None
    }
    fn find_fbconfig(&self, depth: u8, visual: &xproto::Visualtype) -> Result<*const libc::c_void> {
        use glutin_glx_sys::glx;
        let raw_display = self.glium.gl_window().window().xlib_display().unwrap();
        if visual.class != xproto::VisualClass::TRUE_COLOR {
            return Err(Error::InvalidVisual(visual.visual_id));
        }

        // 2. find FBConfig
        let mut num_config: libc::c_int = 0;
        let [red_bits, green_bits, blue_bits] =
            [visual.red_mask, visual.green_mask, visual.blue_mask].map(|x| x.count_ones());
        let attrs = [
            glx::RED_SIZE,
            red_bits,
            glx::GREEN_SIZE,
            green_bits,
            glx::BLUE_SIZE,
            blue_bits,
            glx::ALPHA_SIZE,
            depth as u32 - red_bits - green_bits - blue_bits,
            glx::BUFFER_SIZE,
            depth as _,
            glx::RENDER_TYPE,
            glx::RGBA_BIT,
            glx::DRAWABLE_TYPE,
            glx::PIXMAP_BIT,
            glx::X_VISUAL_TYPE,
            glx::TRUE_COLOR,
            glx::X_RENDERABLE,
            1,
            0,
        ]
        .map(|x| x as libc::c_int);
        let config = unsafe {
            self.glx.ChooseFBConfig(
                raw_display as _,
                self.screen as _,
                attrs.as_ptr(),
                &mut num_config as *mut _,
            )
        };
        if num_config == 0 {
            return Err(Error::NoFbConfig(
                visual.visual_id,
                std::backtrace::Backtrace::capture(),
            ));
        }
        let configs = unsafe { std::slice::from_raw_parts(config, num_config as _) };
        for &config in configs {
            let mut texture_targets = 0;
            let mut fb_visual = 0;
            unsafe {
                self.glx.GetFBConfigAttrib(
                    raw_display as _,
                    config,
                    glx::VISUAL_ID as _,
                    &mut fb_visual,
                );
            }
            if fb_visual == 0 {
                continue;
            }
            unsafe {
                self.glx.GetFBConfigAttrib(
                    raw_display as _,
                    config,
                    GLX_BIND_TO_TEXTURE_TARGETS_EXT,
                    &mut texture_targets,
                );
            }

            if texture_targets & 0x2 == 0 {
                // 0x2 is GLX_TEXTURE_2D_BIT_EXT
                continue;
            }

            let (mut fb_red_bits, mut fb_green_bits, mut fb_blue_bits, mut fb_bufsize) =
                (0, 0, 0, 0);
            unsafe {
                self.glx.GetFBConfigAttrib(
                    raw_display as _,
                    config,
                    glx::RED_SIZE as _,
                    &mut fb_red_bits,
                );
                self.glx.GetFBConfigAttrib(
                    raw_display as _,
                    config,
                    glx::GREEN_SIZE as _,
                    &mut fb_green_bits,
                );
                self.glx.GetFBConfigAttrib(
                    raw_display as _,
                    config,
                    glx::BLUE_SIZE as _,
                    &mut fb_blue_bits,
                );
                self.glx.GetFBConfigAttrib(
                    raw_display as _,
                    config,
                    glx::BUFFER_SIZE as _,
                    &mut fb_bufsize,
                );
            }
            if fb_red_bits != red_bits as _
                || fb_green_bits != green_bits as _
                || fb_blue_bits != blue_bits as _
                || fb_bufsize != depth as _
            {
                continue;
            }
            let (fb_depth, _) = self.find_visual(fb_visual as _).unwrap();
            if fb_depth != depth as _ {
                continue;
            }
            return Ok(config);
        }
        Err(Error::NoFbConfig(
            visual.visual_id,
            std::backtrace::Backtrace::capture(),
        ))
    }
    fn bind_texture(
        &mut self,
        pixmap: xproto::Pixmap,
        visual: xproto::Visualid,
    ) -> Result<Texture> {
        // TODO: handle y_inverted property
        let raw_display = self.glium.gl_window().window().xlib_display().unwrap();
        let (depth, visual) = self
            .find_visual(visual)
            .ok_or(Error::InvalidVisual(visual))?;
        let fbconfig = self.find_fbconfig(depth, visual)?;
        log::info!("{:p}", raw_display);

        let geometry = xproto::get_geometry(self.x11.as_ref(), pixmap)?.reply()?;
        let attrs = [
            GLX_TEXTURE_FORMAT_EXT,
            if depth == 32 {
                GLX_TEXTURE_FORMAT_RGBA_EXT
            } else {
                GLX_TEXTURE_FORMAT_RGB_EXT
            },
            GLX_TEXTURE_TARGET_EXT,
            GLX_TEXTURE_2D_EXT,
            0,
        ];
        let pixmap = unsafe {
            self.glx
                .CreatePixmap(raw_display as _, fbconfig, pixmap.into(), attrs.as_ptr())
        };
        if pixmap == 0 {
            return Err(Error::PixmapCreation);
        }
        let mut texture_id = 0;
        unsafe {
            // Remember the current state to restore it later.
            let mut old_texture_2d = 0;
            self.gl
                .GetIntegerv(ffi::TEXTURE_BINDING_2D, &mut old_texture_2d);

            self.gl.GenTextures(1, &mut texture_id);
            self.gl.BindTexture(ffi::TEXTURE_2D, texture_id);
            self.gl
                .TexParameteri(ffi::TEXTURE_2D, ffi::TEXTURE_MIN_FILTER, ffi::NEAREST as _);
            self.gl
                .TexParameteri(ffi::TEXTURE_2D, ffi::TEXTURE_MAG_FILTER, ffi::NEAREST as _);
            self.gl
                .TexParameteri(ffi::TEXTURE_2D, ffi::TEXTURE_WRAP_S, ffi::REPEAT as _);
            self.gl
                .TexParameteri(ffi::TEXTURE_2D, ffi::TEXTURE_WRAP_T, ffi::REPEAT as _);
            self.gl
                .TexParameteri(ffi::TEXTURE_2D, ffi::TEXTURE_BASE_LEVEL, 0);
            self.gl
                .TexParameteri(ffi::TEXTURE_2D, ffi::TEXTURE_MAX_LEVEL, 0);
            (self.bind_tex_image)(
                raw_display,
                pixmap as _,
                GLX_FRONT_LEFT_EXT,
                std::ptr::null(),
            );
            self.gl.BindTexture(ffi::TEXTURE_2D, old_texture_2d as _);
        }
        let texture = unsafe {
            glium::Texture2d::from_id(
                &self.glium,
                if depth == 32 {
                    glium::texture::UncompressedFloatFormat::U8U8U8U8
                } else {
                    glium::texture::UncompressedFloatFormat::U8U8U8
                },
                texture_id,
                true,
                glium::texture::MipmapsOption::NoMipmap,
                glium::texture::Dimensions::Texture2d {
                    width: geometry.width as _,
                    height: geometry.height as _,
                },
            )
        };
        self.glium.assert_no_error(None);
        self.textures.insert(
            texture_id as _,
            TextureInner {
                texture: AnyTexture2d::Linear(texture),
                glxpixmap: Some(pixmap as _),
            },
        );
        Ok(Texture {
            id: texture_id as _,
            width: geometry.width as _,
            height: geometry.height as _,
        })
    }
    fn release_pixmap(&mut self, tex: Texture) -> Result<()> {
        let raw_display = self.glium.gl_window().window().xlib_display().unwrap();
        if let Some(TextureInner {
            glxpixmap: Some(pixmap),
            ..
        }) = self.textures.remove(&(tex.id as _))
        {
            unsafe { self.glx.DestroyPixmap(raw_display as _, pixmap as _) };
        }
        Ok(())
    }
    fn capture(&mut self, start: bool) -> Result<()> {
        let mut rd = crate::RENDERDOC.lock().unwrap();
        if let Some(rd) = rd.as_mut() {
            let glxcontext =
                if let RawHandle::Glx(glx) = unsafe { self.glium.gl_window().raw_handle() } {
                    glx
                } else {
                    panic!()
                };
            if start {
                rd.start_frame_capture(glxcontext, std::ptr::null());
            } else {
                rd.end_frame_capture(glxcontext, std::ptr::null());
            }
        }
        Ok(())
    }
    fn blit(&mut self, src: usize, dst: usize) -> Result<()> {
        use glium::uniform;
        let src = self.textures.get(&src).unwrap();
        let dst = self.textures.get(&dst).unwrap();
        let mut fb = glium::framebuffer::SimpleFrameBuffer::new(&self.glium, &dst.texture)?;
        let uniform = uniform! {
            tex: &src.texture
        };
        let vbo = glium::VertexBuffer::new(
            &self.glium,
            &[
                Vertex {
                    position: [-1.0, -1.0],
                },
                Vertex {
                    position: [-1.0, 1.0],
                },
                Vertex {
                    position: [1.0, 1.0],
                },
                Vertex {
                    position: [1.0, -1.0],
                },
            ],
        )
        .unwrap();
        let indices = glium::IndexBuffer::new(
            &self.glium,
            glium::index::PrimitiveType::TriangleStrip,
            &[1 as u16, 2, 0, 3],
        )
        .unwrap();
        //let time = std::time::SystemTime::now()
        //    .duration_since(std::time::SystemTime::UNIX_EPOCH)
        //    .unwrap()
        //    .as_secs_f64();
        //let color = f64::sin(time * 2.0);
        //log::info!("!!{} {}", color, time);
        //fb.clear_color(color as f32, 0.0, 1.0, 1.0);
        fb.draw(
            &vbo,
            &indices,
            &self.blit_shader,
            &uniform,
            &Default::default(),
        )?;
        self.glium.get_context().finish();
        Ok(())
    }
    fn import_fd(&mut self, width: u32, height: u32, fd: RawFd, size: u64) -> Result<Texture> {
        use glium::texture::{
            Dimensions, ExternalTilingMode, ImportParameters, MipmapsOption, SrgbFormat,
        };
        use glium::GlObject;
        use std::os::unix::io::FromRawFd;
        let texture = unsafe {
            SrgbTexture2d::new_from_fd(
                &self.glium,
                SrgbFormat::U8U8U8U8,
                MipmapsOption::NoMipmap,
                Dimensions::Texture2d { width, height },
                ImportParameters {
                    dedicated_memory: true,
                    size,
                    offset: 0,
                    tiling: ExternalTilingMode::Optimal,
                },
                std::fs::File::from_raw_fd(fd),
            )
        }?;
        let id = texture.get_id() as _;
        self.textures.insert(
            id,
            TextureInner {
                texture: AnyTexture2d::Srgb(texture),
                glxpixmap: None,
            },
        );
        Ok(Texture { id, width, height })
    }
    fn run(&mut self, mut rx: mpsc::UnboundedReceiver<Request>) -> Result<()> {
        while let Some(req) = rx.blocking_recv() {
            use Request::*;
            match req {
                BindTexture {
                    pixmap,
                    visual,
                    reply,
                } => {
                    let _: std::result::Result<_, _> =
                        reply.send(self.bind_texture(pixmap, visual));
                }
                ReleaseTexture { texture, reply } => {
                    let _: std::result::Result<_, _> = reply.send(self.release_pixmap(texture));
                }
                ImportFd {
                    width,
                    height,
                    fd,
                    size,
                    reply,
                } => {
                    let _: std::result::Result<_, _> =
                        reply.send(self.import_fd(width, height, fd, size));
                }
                Blit { src, dst, reply } => {
                    let _: std::result::Result<_, _> = reply.send(self.blit(src, dst));
                }
                Capture { start, reply } => {
                    let _: std::result::Result<_, _> = reply.send(self.capture(start));
                }
            }
        }
        Ok(())
    }
}

#[derive(Clone, Debug)]
pub struct Gl {
    inner: mpsc::UnboundedSender<Request>,
}

#[allow(clippy::all)]
mod ffi {
    include!(concat!(env!("OUT_DIR"), "/gl_bindings.rs"));
}

use crate::gen_proxy;
#[allow(dead_code)]
impl Gl {
    pub fn new(x11: Arc<RustConnection>, screen: u32) -> Result<Self> {
        let (tx, rx) = mpsc::unbounded_channel();
        let x11 = x11.clone();
        std::thread::spawn(move || {
            let mut inner = GlInner::new(x11, screen)?;
            inner.run(rx)
        });
        Ok(Self { inner: tx })
    }
    gen_proxy!(
        bind_texture,
        BindTexture,
        Texture,
        pixmap: xproto::Pixmap,
        visual: xproto::Visualid
    );
    gen_proxy!(release_texture, ReleaseTexture, (), texture: Texture);
    gen_proxy!(
        import_fd,
        ImportFd,
        Texture,
        width: u32,
        height: u32,
        fd: RawFd,
        size: u64
    );
    gen_proxy!(capture, Capture, (), start: bool);

    pub async fn blit(&self, src: &Texture, dst: &Texture) -> Result<()> {
        let (tx, rx) = oneshot::channel();
        self.inner
            .send(Request::Blit {
                src: src.id,
                dst: dst.id,
                reply: tx,
            })
            .map_err(|_| self::Error::Send)?;
        rx.await?
    }
}
