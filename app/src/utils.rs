use tokio::sync::{
    mpsc::{unbounded_channel, UnboundedReceiver, UnboundedSender},
    oneshot::{channel as oneshot_channel, error as oneshot_error, Receiver, Sender},
};
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("{0}")]
    OneshotRecv(#[from] oneshot_error::RecvError),
    #[error("Failed to send request to thread")]
    MpscSend,
}

type Result<T, E = Error> = std::result::Result<T, E>;

use std::any::Any;
struct Request<T>(
    Box<dyn FnOnce(&mut T) -> Box<dyn Any + Send> + Send>,
    Sender<Box<dyn Any + Send>>,
);

// feature: downcast_unchecked
trait UnsafeAny<T> {
    unsafe fn downcast_unchecked(self) -> Box<T>;
}

impl<T: Any> UnsafeAny<T> for Box<dyn Any> {
    unsafe fn downcast_unchecked(self) -> Box<T> {
        Box::from_raw(Box::into_raw(self) as *mut T)
    }
}

struct RemoteInner<T>(T, UnboundedReceiver<Request<T>>);
impl<T> RemoteInner<T> {
    fn new<E>(
        f: impl FnOnce() -> Result<T, E>,
        rx: UnboundedReceiver<Request<T>>,
    ) -> Result<Self, E> {
        Ok(Self(f()?, rx))
    }
    fn run(&mut self) {
        while let Some(req) = self.1.blocking_recv() {
            let _: Result<_, _> = req.1.send((req.0)(&mut self.0));
        }
    }
}

pub struct Remote<T>(UnboundedSender<Request<T>>);

impl<T> Clone for Remote<T> {
    fn clone(&self) -> Self {
        Self(self.0.clone())
    }
}

impl<T> std::fmt::Debug for Remote<T> {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        self.0.fmt(f)
    }
}

impl<T: 'static> Remote<T> {
    pub async fn new<E: Send + std::fmt::Debug + 'static>(
        f: impl FnOnce() -> Result<T, E> + Send + 'static,
    ) -> Result<Self, E> {
        let (tx, rx) = unbounded_channel();
        let (init_tx, init_rx) = oneshot_channel();
        std::thread::spawn(move || {
            let inner = RemoteInner::new(f, rx);
            match inner {
                Ok(mut inner) => {
                    init_tx.send(Ok(())).unwrap();
                    inner.run();
                }
                Err(e) => {
                    init_tx.send(Err(e)).unwrap();
                }
            }
        });
        init_rx.await.unwrap()?;
        Ok(Self(tx))
    }
    fn call_inner<R: 'static + Send>(
        &self,
        f: impl FnOnce(&mut T) -> R + Send + 'static,
    ) -> Result<Receiver<Box<dyn Any + Send>>> {
        let boxed = Box::new(|v: &mut T| Box::new(f(v)) as Box<dyn Any + Send>)
            as Box<dyn FnOnce(&mut T) -> Box<dyn Any + Send> + Send>;
        let (tx, rx) = oneshot_channel();
        self.0
            .send(Request(boxed, tx))
            .map_err(|_| Error::MpscSend)?;
        Ok(rx)
    }
    pub async fn call<R: 'static + Send>(
        &self,
        f: impl FnOnce(&mut T) -> R + Send + 'static,
    ) -> Result<R> {
        // We know that `f` will return type `R`
        Ok(*(unsafe { UnsafeAny::downcast_unchecked(self.call_inner(f)?.await? as Box<dyn Any>) }))
    }
    pub fn call_sync<R: 'static + Send>(
        &self,
        f: impl FnOnce(&mut T) -> R + Send + 'static,
    ) -> Result<R> {
        // We know that `f` will return type `R`
        Ok(*(unsafe {
            UnsafeAny::downcast_unchecked(self.call_inner(f)?.blocking_recv()? as Box<dyn Any>)
        }))
    }
}

#[macro_export]
macro_rules! gen_remote_fn {
    ($name:ident($($arg:ident : $ty:path),*) -> $reply:ty) => {
        pub async fn $name(&self, $($arg: $ty),*) -> Result<$reply> {
            self.inner.call(move |inner| {
                inner.$name($($arg),*)
            }).await?
        }
        paste::paste! {
            pub fn [<$name _sync>](&self, $($arg: $ty),*) -> Result<$reply> {
                self.inner.call_sync(move |inner| {
                    inner.$name($($arg),*)
                })?
            }
        }
    }
}

#[allow(dead_code)]
extern "C" {
    fn a_type_that_should_not_be_dropped_is_dropped_Y7soaRCSQz();
}

#[derive(Default, Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
pub struct CompileDropBomb;

impl Drop for CompileDropBomb {
    #[inline(always)]
    fn drop(&mut self) {
        unsafe {
            a_type_that_should_not_be_dropped_is_dropped_Y7soaRCSQz();
        };
    }
}
