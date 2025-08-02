//
// Created by charlie on 8/1/25.
//

#include "../Public/InotifyWatcher.h"
#include <unistd.h>
#include <vector>
#include <chrono>

namespace FileWatcher {

    InotifyWatcher::InotifyWatcher()
        : m_fd(inotify_init1(IN_NONBLOCK)), m_running(false)
    {}

    InotifyWatcher::~InotifyWatcher() {
        stop();
        if (m_fd >= 0) close(m_fd);
    }

    bool InotifyWatcher::watch(
        const std::string& path,
        const std::function<void(const std::string&, FileEvent)>& callback
    ) {
        m_callback = callback;
        int wd = inotify_add_watch(m_fd, path.c_str(), IN_CREATE | IN_MODIFY | IN_DELETE);
        if (wd < 0) return false;
        m_watches[wd] = path;
        m_running = true;
        m_thread = std::thread(&InotifyWatcher::eventLoop, this);
        return true;
    }

    void InotifyWatcher::stop() {
        if (m_running.exchange(false) && m_thread.joinable()) {
            m_thread.join();
        }
    }

    void InotifyWatcher::eventLoop() {
        constexpr size_t BUF_LEN = 4096;
        std::vector<char> buffer(BUF_LEN);
        while (m_running) {
            int length = read(m_fd, buffer.data(), BUF_LEN);
            if (length <= 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
            size_t i = 0;
            while (i < static_cast<size_t>(length)) {
                auto* event = reinterpret_cast<inotify_event*>(&buffer[i]);
                auto it = m_watches.find(event->wd);
                if (it != m_watches.end() && event->len > 0) {
                    FileEvent ev = FileEvent::Modified;
                    if (event->mask & IN_CREATE) ev = FileEvent::Added;
                    else if (event->mask & IN_DELETE) ev = FileEvent::Removed;
                    m_callback(it->second + "/" + event->name, ev);
                }
                i += sizeof(inotify_event) + event->len;
            }
        }
    }

} // namespace FileWatcher
