#include "mouse_binder.hpp"

#include <X11/X.h>
#include <iostream>

MouseBinder::MouseBinder() = default;

MouseBinder::~MouseBinder() {
    stop();
}

void MouseBinder::bind(int button, Callback onPress, Callback onRelease) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        bindings_[button] = Binding{std::move(onPress), std::move(onRelease)};
    }
    if (!started_) {
        start();
    }
}

void MouseBinder::unbind(int button) {
    std::lock_guard<std::mutex> lock(mutex_);
    bindings_.erase(button);
}

void MouseBinder::start() {
    recordDisplay_ = XOpenDisplay(nullptr);
    controlDisplay_ = XOpenDisplay(nullptr);
    if (!recordDisplay_ || !controlDisplay_) {
        std::cerr << "Unable to open X display for mouse binder\n";
        return;
    }

    int major = 0;
    int minor = 0;
    if (!XRecordQueryVersion(recordDisplay_, &major, &minor)) {
        std::cerr << "XRecord extension is unavailable for mouse binder\n";
        return;
    }

    XRecordRange *range = XRecordAllocRange();
    if (!range) {
        std::cerr << "Unable to allocate XRecord range for mouse binder\n";
        return;
    }
    range->device_events.first = ButtonPress;
    range->device_events.last = ButtonRelease;

    XRecordClientSpec clients = XRecordAllClients;
    context_ = XRecordCreateContext(recordDisplay_, 0, &clients, 1, &range, 1);
    XFree(range);
    if (!context_) {
        std::cerr << "Unable to create XRecord context for mouse binder\n";
        return;
    }

    running_.store(true);
    started_ = true;
    worker_ = std::thread(&MouseBinder::run, this);
}

void MouseBinder::run() {
    XRecordEnableContext(recordDisplay_, context_, &MouseBinder::recordCallback, reinterpret_cast<XPointer>(this));
}

void MouseBinder::stop() {
    if (!started_) {
        return;
    }
    running_.store(false);

    if (controlDisplay_ && context_) {
        XRecordDisableContext(controlDisplay_, context_);
        XFlush(controlDisplay_);
    }

    if (worker_.joinable()) {
        worker_.join();
    }

    if (recordDisplay_ && context_) {
        XRecordFreeContext(recordDisplay_, context_);
    }

    if (recordDisplay_) {
        XCloseDisplay(recordDisplay_);
        recordDisplay_ = nullptr;
    }
    if (controlDisplay_) {
        XCloseDisplay(controlDisplay_);
        controlDisplay_ = nullptr;
    }

    context_ = 0;
    started_ = false;
}

void MouseBinder::recordCallback(XPointer self, XRecordInterceptData *data) {
    auto *binder = reinterpret_cast<MouseBinder *>(self);
    if (binder) {
        binder->handleRecordData(data);
    }
    if (data) {
        XRecordFreeData(data);
    }
}

void MouseBinder::handleRecordData(XRecordInterceptData *data) {
    if (!running_.load()) {
        return;
    }
    if (!data || data->category != XRecordFromServer || !data->data || data->data_len < 2) {
        return;
    }

    const unsigned char *raw = reinterpret_cast<const unsigned char *>(data->data);
    for (int offset = 0; offset + 1 < data->data_len; offset += 32) {
        const int eventType = raw[offset] & 0x7F;
        const int button = raw[offset + 1];

        Binding binding;
        bool found = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = bindings_.find(button);
            if (it != bindings_.end()) {
                binding = it->second;
                found = true;
            }
        }
        if (!found) {
            continue;
        }

        if (eventType == ButtonPress && binding.onPress) {
            binding.onPress(button);
        } else if (eventType == ButtonRelease && binding.onRelease) {
            binding.onRelease(button);
        }
    }
}
