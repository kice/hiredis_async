#pragma once

#include "concurrentqueue.h"
#include "blockingconcurrentqueue.h"

#include <thread>
#include <vector>
#include <functional>
#include <future>
#include <atomic>

#include "reply.h"

struct redisContext;

namespace async_redis
{
class client
{
public:
    /**
     * Create a new redis client
     *
     * @param piped_cache   The number of command put to pipeline before commit
     *                      0: manully commit, 1: disable pipeline (commit when available)
     *
     * @param pipeline_timeout  The maximum time to spend waiting for pipeline to be filled in ms
     *                          0: wait for piped_cache number of command to be filled.
     *                          DONT recommend set time to 0, since the worker will wait forever
     *                          until receive enough command (if you use future you will get a deadlock)
     *
     */
    client(size_t piped_cache = 0, uint32_t pipeline_timeout = 0);
    ~client();

    bool Connect(const std::string &host, int port, uint32_t timeout_ms);

    bool IsConnected() const;

    void Disconnect();

    // bool: if successfully sent and receive, reply: actual reply content
    typedef std::function<void(reply *)> reply_callback;

    client &Send(const std::vector<std::string> &redis_cmd, const reply_callback &callback);

    std::future<std::unique_ptr<reply>> Send(const std::vector<std::string> &redis_cmd)
    {
        auto prms = std::make_shared<std::promise<std::unique_ptr<reply>>>();

        auto f = [=](const reply_callback &cb) -> client & { return Send(redis_cmd, cb); };

        f([prms](auto r) {
            prms->set_value(std::make_unique<reply>(r));
        });

        return prms->get_future();
    }

    // This function will do nothing when auto pipeline enabled
    // If pipeline was enable, it will force worker to commit existing command
    client &Commit();

private:
    struct command_request
    {
        std::string command;
        reply_callback callback;
    };

    static std::string formatCommand(const std::vector<std::string> &redis_cmd);

    moodycamel::BlockingConcurrentQueue<command_request> m_commands;

    moodycamel::details::mpmc_sema::LightweightSemaphore pipeline_sem;

    size_t cache_size;
    uint32_t pipe_timeout;

    std::atomic<bool> flush_pipeline;
    std::atomic<bool> stopped;
    std::thread worker;

    redisContext *ctx;
};
}
