#pragma once

#include "FDE/Export.hpp"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace FDE
{

/// Background file reads; invoke callbacks on main thread via ProcessMainThread.
class FDE_API AssetLoadQueue
{
  public:
    AssetLoadQueue();
    ~AssetLoadQueue();

    AssetLoadQueue(const AssetLoadQueue&) = delete;
    AssetLoadQueue& operator=(const AssetLoadQueue&) = delete;

    void EnqueueReadFile(const std::string& absolutePath,
                         std::function<void(std::vector<uint8_t>)> onComplete);

    void ProcessMainThread();
    void Shutdown();

  private:
    struct PendingJob
    {
        std::string path;
        std::function<void(std::vector<uint8_t>)> onComplete;
    };
    struct CompletedJob
    {
        std::vector<uint8_t> bytes;
        std::function<void(std::vector<uint8_t>)> onComplete;
    };

    void WorkerLoop();

    std::mutex m_pendingMutex;
    std::queue<PendingJob> m_pending;
    std::condition_variable m_cv;
    std::mutex m_completedMutex;
    std::queue<CompletedJob> m_completed;

    std::unique_ptr<std::thread> m_worker;
    std::atomic<bool> m_stop{false};
};

} // namespace FDE
