import threading
from Xlib import X, display
from Xlib.ext import record
from Xlib.XK import keysym_to_string

class KeyboardBinder:
    """Passively monitors all keyboard events via XRecord
    and dispatches key press/release information."""
    
    def __init__(self):
        self._bindings = {}
        self._lock = threading.Lock()
        self._record_dpy = None
        self._ctrl_dpy = None
        self._ctx = None
        self._thread = None
        self._running = False

    def bind(self, key, on_press=None, on_release=None):
        """Register callbacks for specific keyboard keycodes or key names."""
        with self._lock:
            self._bindings[key] = (on_press, on_release)
        if not self._running:
            self._start()

    def unbind(self, key):
        """Remove callbacks for a specific keyboard key."""
        with self._lock:
            self._bindings.pop(key, None)

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
                'device_events': (X.KeyPress, X.KeyRelease),
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
            keycode = data[1]

            with self._lock:
                binding = self._bindings.get(keycode)
            if binding:
                on_press, on_release = binding
                if event_type == X.KeyPress and on_press:
                    on_press(keycode)
                elif event_type == X.KeyRelease and on_release:
                    on_release(keycode)

            data = data[32:]

    def _get_keyname(self, keycode):
        """Get the human-readable key name from the keycode."""
        key = self._record_dpy.keycode_to_keysym(keycode, 0)
        return keysym_to_string(key)

    def __enter__(self):
        return self

    def __exit__(self, *_):
        self.stop()