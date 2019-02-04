#pragma once

#include <string>
#include <vector>

namespace async_redis
{
class reply
{
public:
    enum type
    {
        invalid,
        string,
        arrays,
        integer,
        nil,
        status,
        error,
    };

    reply(void *raw_reply);

    reply(reply &&other) noexcept;
    reply &operator=(reply &&other) noexcept;

    reply() = default;
    reply(const reply &) = default;
    reply &operator=(const reply &) = default;

    operator bool() const;

    ~reply() = default;

    type Type() const;

    bool Ok() const;
    const char * Status() const;

    const std::vector<reply> &GetArray() const;
    int64_t GetInt() const;
    const std::string &GetString() const;

private:
    type reply_type;

    std::vector<reply> rows;
    std::string str_val;
    int64_t int_val;
};
}
