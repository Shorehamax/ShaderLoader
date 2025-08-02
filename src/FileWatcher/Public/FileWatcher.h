//
// Created by charlie on 8/1/25.
//


#pragma once

#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>

namespace FileWatcher {

    enum class FileEvent {
        Added,
        Modified,
        Removed
    };

    class IFileWatcher {
    public:
        virtual ~IFileWatcher() = default;
        virtual bool watch(
            const std::string& path,
            const std::function<void(const std::string&, FileEvent)>& callback
        ) = 0;
        virtual void stop() = 0;
    };

    class FileWatcher {
    public:
        FileWatcher();
        ~FileWatcher();

        bool addWatch(const std::string& path);
        bool removeWatch(const std::string& path);
        void start();
        void stop();
        void setCallback(const std::function<void(const std::string&, FileEvent)>& callback);

    private:
        std::unique_ptr<IFileWatcher>                           m_impl;
        std::vector<std::string>                                m_paths;
        std::function<void(const std::string&, FileEvent)>      m_callback;
    };

} // namespace FileWatcher

