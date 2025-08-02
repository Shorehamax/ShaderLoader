//
// Created by charlie on 8/1/25.
//


#pragma once

#include "FileWatcher.h"
#include <string>
#include <functional>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <sys/inotify.h>

namespace FileWatcher {

    class InotifyWatcher : public IFileWatcher {
    public:
        InotifyWatcher();
        ~InotifyWatcher() override;

        bool watch(
            const std::string& path,
            const std::function<void(const std::string&, FileEvent)>& callback
        ) override;
        void stop() override;

    private:
        int                                                 m_fd;
        std::unordered_map<int, std::string>               m_watches;
        std::function<void(const std::string&, FileEvent)> m_callback;
        std::thread                                        m_thread;
        std::atomic<bool>                                  m_running;

        void eventLoop();
    };

} // namespace FileWatcher




