#include "FDE/pch.hpp"
#include "FDE/Asset/AssetLoadQueue.hpp"
#include <fstream>

namespace FDE
{

AssetLoadQueue::AssetLoadQueue()
{
    m_worker = std::make_unique<std::thread>(&AssetLoadQueue::WorkerLoop, this);
}

AssetLoadQueue::~AssetLoadQueue()
{
    Shutdown();
}

void AssetLoadQueue::Shutdown()
{
    m_stop = true;
    m_cv.notify_all();
    if (m_worker && m_worker->joinable())
        m_worker->join();
    m_worker.reset();
}

void AssetLoadQueue::EnqueueReadFile(const std::string& absolutePath,
                                     std::function<void(std::vector<uint8_t>)> onComplete)
{
    if (m_stop)
        return;
    {
        std::lock_guard<std::mutex> lock(m_pendingMutex);
        m_pending.push(PendingJob{absolutePath, std::move(onComplete)});
    }
    m_cv.notify_one();
}

void AssetLoadQueue::WorkerLoop()
{
    while (!m_stop)
    {
        PendingJob job;
        {
            std::unique_lock<std::mutex> lock(m_pendingMutex);
            m_cv.wait(lock, [this] { return m_stop || !m_pending.empty(); });
            if (m_stop && m_pending.empty())
                return;
            if (m_pending.empty())
                continue;
            job = std::move(m_pending.front());
            m_pending.pop();
        }

        std::vector<uint8_t> bytes;
        std::ifstream file(job.path, std::ios::binary);
        if (file)
            bytes.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        {
            std::lock_guard<std::mutex> lock(m_completedMutex);
            m_completed.push(CompletedJob{std::move(bytes), std::move(job.onComplete)});
        }
    }
}

void AssetLoadQueue::ProcessMainThread()
{
    std::queue<CompletedJob> local;
    {
        std::lock_guard<std::mutex> lock(m_completedMutex);
        std::swap(local, m_completed);
    }
    while (!local.empty())
    {
        CompletedJob job = std::move(local.front());
        local.pop();
        if (job.onComplete)
            job.onComplete(std::move(job.bytes));
    }
}

} // namespace FDE
