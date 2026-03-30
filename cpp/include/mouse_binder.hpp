#pragma once

#include <X11/Xlib.h>
#include <X11/extensions/record.h>
#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>

class MouseBinder {
public:
    using Callback = std::function<void(int)>;

    MouseBinder();
    ~MouseBinder();

    void bind(int button, Callback onPress, Callback onRelease);
    void unbind(int button);
    void stop();

private:
    static void recordCallback(XPointer self, XRecordInterceptData *data);
    void handleRecordData(XRecordInterceptData *data);
    void start();
    void run();

    struct Binding {
        Callback onPress;
        Callback onRelease;
    };

    std::unordered_map<int, Binding> bindings_;
    std::mutex mutex_;
    std::atomic<bool> running_{false};
    bool started_{false};

    Display *recordDisplay_{nullptr};
    Display *controlDisplay_{nullptr};
    XRecordContext context_{0};
    std::thread worker_;
};
