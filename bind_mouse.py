import threading
from Xlib import X, display
from Xlib.ext import record


class MouseBinder:
    """Passively monitors global mouse button events via XRecord
    and dispatches to user-registered press/release callbacks."""

    def __init__(self):
        self._bindings = {}
        self._lock = threading.Lock()
        self._record_dpy = None
        self._ctrl_dpy = None
        self._ctx = None
        self._thread = None
        self._running = False

    def bind(self, button, on_press=None, on_release=None):
        """Register callbacks for a mouse button.

        Args:
            button: X11 button number (1=left, 2=middle, 3=right,
                    4-7=scroll, 8=back, 9=forward, ...).
            on_press: Called with (button,) when the button is pressed.
            on_release: Called with (button,) when the button is released.
        """
        with self._lock:
            self._bindings[button] = (on_press, on_release)
        if not self._running:
            self._start()

    def unbind(self, button):
        """Remove callbacks for a mouse button."""
        with self._lock:
            self._bindings.pop(button, None)

    def stop(self):
        """Stop the listener thread and free X resources."""
        if not self._running:
            return
        self._running = False
        self._ctrl_dpy.record_disable_context(self._ctx)
        self._ctrl_dpy.flush()
        self._thread.join()
        self._record_dpy.record_free_context(self._ctx)
        self._record_dpy.close()
        self._ctrl_dpy.close()
        self._record_dpy = None
        self._ctrl_dpy = None
        self._ctx = None
        self._thread = None

    def _start(self):
        self._record_dpy = display.Display()
        self._ctrl_dpy = display.Display()

        if not self._record_dpy.has_extension("RECORD"):
            raise RuntimeError("X RECORD extension not available")

        self._ctx = self._record_dpy.record_create_context(
            0,
            [record.AllClients],
            [{
                'core_requests': (0, 0),
                'core_replies': (0, 0),
                'ext_requests': (0, 0, 0, 0),
                'ext_replies': (0, 0, 0, 0),
                'delivered_events': (0, 0),
                'device_events': (X.ButtonPress, X.ButtonRelease),
                'errors': (0, 0),
                'client_started': False,
                'client_died': False,
            }]
        )

        self._running = True
        self._thread = threading.Thread(target=self._listen, daemon=True)
        self._thread.start()

    def _listen(self):
        self._record_dpy.record_enable_context(self._ctx, self._handle)

    def _handle(self, reply):
        if reply.category != record.FromServer or reply.client_swapped:
            return

        data = reply.data
        while len(data) >= 32:
            event_type = data[0] & 0x7F
            button = data[1]

            with self._lock:
                binding = self._bindings.get(button)

            if binding:
                on_press, on_release = binding
                if event_type == X.ButtonPress and on_press:
                    on_press(button)
                elif event_type == X.ButtonRelease and on_release:
                    on_release(button)

            data = data[32:]

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.stop()
