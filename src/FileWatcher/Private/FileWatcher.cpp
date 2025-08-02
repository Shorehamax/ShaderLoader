//
// Created by charlie on 8/1/25.
//

#include "../Public/FileWatcher.h"
#include "../Public/InotifyWatcher.h"
#include <algorithm>

namespace FileWatcher {

    FileWatcher::FileWatcher()
        : m_impl(std::make_unique<InotifyWatcher>())
    {}

    FileWatcher::~FileWatcher() {
        stop();
    }

    bool FileWatcher::addWatch(const std::string& path) {
        m_paths.push_back(path);
        return m_impl->watch(path, [this](const std::string& file, FileEvent ev) {
            if (m_callback) m_callback(file, ev);
        });
    }

    bool FileWatcher::removeWatch(const std::string& path) {
        auto it = std::find(m_paths.begin(), m_paths.end(), path);
        if (it == m_paths.end()) return false;
        m_paths.erase(it);
        m_impl->stop();
        return true;
    }

    void FileWatcher::start() {
        // Implementation-specific (InotifyWatcher starts on first watch)
    }

    void FileWatcher::stop() {
        m_impl->stop();
    }

    void FileWatcher::setCallback(const std::function<void(const std::string&, FileEvent)>& callback) {
        m_callback = callback;
    }

} // namespace FileWatcher



