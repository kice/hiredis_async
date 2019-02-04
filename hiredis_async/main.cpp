extern "C" {
#ifdef _WIN32
#define WIN32_INTEROP_APIS_H
#define NO_QFORKIMPL
#include <Win32_Interop/win32fixes.h>

#pragma comment(lib, "hiredis.lib")
#pragma comment(lib, "Win32_Interop.lib")
#endif
}

#include <hiredis/hiredis.h>

#include "client.h"

void c()
{
    unsigned int j, isunix = 0;
    redisContext *c;
    redisReply *reply;
    const char *hostname = "127.0.0.1";

    int port = 6379;

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds

    c = redisConnectWithTimeout(hostname, port, timeout);
    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    reply = (redisReply *)redisCommand(c, "AUTH foobared233");

    auto auth = "AUTH foobared233";
    reply = (redisReply *)redisCommandArgv(c, 1, &auth, 0);

    /* PING server */
    reply = (redisReply *)redisCommand(c, "PING");
    printf("PING: %s\n", reply->str);
    freeReplyObject(reply);

    /* Set a key */
    reply = (redisReply *)redisCommand(c, "SET %s %s", "foo", "hello world");
    printf("SET: %s\n", reply->str);
    freeReplyObject(reply);

    /* Set a key using binary safe API */
    reply = (redisReply *)redisCommand(c, "SET %b %b", "bar", (size_t)3, "hello", (size_t)5);
    printf("SET (binary API): %s\n", reply->str);
    freeReplyObject(reply);

    /* Try a GET and two INCR */
    reply = (redisReply *)redisCommand(c, "GET foo");
    printf("GET foo: %s\n", reply->str);
    freeReplyObject(reply);

    reply = (redisReply *)redisCommand(c, "INCR counter");
    printf("INCR counter: %lld\n", reply->integer);
    freeReplyObject(reply);
    /* again ... */
    reply = (redisReply *)redisCommand(c, "INCR counter");
    printf("INCR counter: %lld\n", reply->integer);
    freeReplyObject(reply);

    /* Create a list of numbers, from 0 to 9 */
    reply = (redisReply *)redisCommand(c, "DEL mylist");
    freeReplyObject(reply);
    for (j = 0; j < 10; j++) {
        char buf[64];

        snprintf(buf, 64, "%u", j);
        reply = (redisReply *)redisCommand(c, "LPUSH mylist element-%s", buf);
        freeReplyObject(reply);
    }

    /* Let's check what we have inside the list */
    reply = (redisReply *)redisCommand(c, "LRANGE mylist 0 -1");
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (j = 0; j < reply->elements; j++) {
            printf("%u) %s\n", j, reply->element[j]->str);
        }
    }
    freeReplyObject(reply);

    /* Disconnects and frees the context */
    redisFree(c);
}

void test(int id)
{
    async_redis::client client(0, 0);

    client.Connect("127.0.0.1", 6379, 1500);

    auto auth = client.Send({ "AUTH", "foobared233" });
    client.Commit();

    {
        auto reply = auth.get();
        if (!reply || reply->Type() != async_redis::reply::type::status) {
            __debugbreak();
        }
    }

    client.Send({ "PING" }, [=](async_redis::reply *reply) {
        if (reply) {
            printf("[%d]PING reply type: %d, string: %s\n", id, reply->Type(), reply->Status());
        }
        delete reply;
    }).Send({ "SET", "foo" + std::to_string(id), "233 2333" }, [=](auto reply) {
        if (reply) {
            printf("[%d]SET reply type: %d, string: %s\n", id, reply->Type(), reply->Status());
        }
        delete reply;
    }).Send({ "GET", "foo" + std::to_string(id) }, [=](auto reply) {
        if (reply) {
            printf("[%d]GET reply type: %d, string: %s\n", id, reply->Type(), reply->GetString().c_str());
        }
        delete reply;
    }).Send({ "INCR", "counter" }, [=](auto reply) {
        if (reply) {
            printf("[%d]GET INCR type: %d, int: %d\n", id, reply->Type(), (int)reply->GetInt());
        }
        delete reply;
    }).Commit();

    std::string mylist = "mylist-" + std::to_string(id);

    auto del = client.Send({ "DEL", mylist });
    client.Commit();

    {
        auto reply = del.get();
        if (!reply || reply->Type() != async_redis::reply::type::integer) {
            __debugbreak();
        }
    }

    for (int i = 0; i < 10; i++) {
        char buf[64];
        snprintf(buf, 64, "element-%u", i);

        client.Send({ "LPUSH", mylist, buf }, [=](async_redis::reply *reply) {
            if (reply) {
                printf("[%d]LPUSH reply type: %d, string: %d\n", id, reply->Type(), (int)reply->GetInt());
            }
            delete reply;
        });
    }
    client.Commit();

    client.Send({ "LRANGE", mylist, "0", "-1" }, [=](async_redis::reply *reply) {
        if (!reply) {
            printf("[%d]LRANGE reply for \"LRANGE mylist\" is invalid\n", id);
            return;
        }

        if (reply->Type() == async_redis::reply::type::arrays) {
            for (const auto &element : reply->GetArray()) {
                printf("[%d]LRANGE reply type: %d, string: %s\n", id, element.Type(), element.GetString().c_str());
            }
        }
        delete reply;
    }).Commit();

    auto ping = client.Send({ "PING" });
    client.Commit();

    {
        auto reply = ping.get();
        if (!reply || reply->Type() != async_redis::reply::type::status) {
            __debugbreak();
        }
    }

    printf("[%d] Done.\n", id);
}

void test2(int id)
{
    async_redis::client client(0);

    client.Connect("127.0.0.1", 6379, 1500);

    auto auth = client.Send({ "AUTH", "foobared233" });
    {
        auto reply = auth.get();
        if (!reply || reply->Type() != async_redis::reply::type::status) {
            __debugbreak();
        }
    }

    client.Send({ "PING" }, [=](async_redis::reply *reply) {
        if (reply) {
            printf("[%d]PING reply type: %d, string: %s\n", id, reply->Type(), reply->Status());
        }
        delete reply;
    }).Send({ "SET", "foo" + std::to_string(id), "233 2333" }, [=](auto reply) {
        if (reply) {
            printf("[%d]SET reply type: %d, string: %s\n", id, reply->Type(), reply->Status());
        }
        delete reply;
    }).Send({ "GET", "foo" + std::to_string(id) }, [=](auto reply) {
        if (reply) {
            printf("[%d]GET reply type: %d, string: %s\n", id, reply->Type(), reply->GetString().c_str());
        }
        delete reply;
    }).Send({ "INCR", "counter" }, [=](auto reply) {
        if (reply) {
            printf("[%d]GET INCR type: %d, int: %d\n", id, reply->Type(), (int)reply->GetInt());
        }
        delete reply;
    });

    std::string mylist = "mylist-" + std::to_string(id);

    auto del = client.Send({ "DEL", mylist });
    {
        auto reply = del.get();
        if (!reply || reply->Type() != async_redis::reply::type::integer) {
            __debugbreak();
        }
    }

    for (int i = 0; i < 10; i++) {
        char buf[64];
        snprintf(buf, 64, "element-%u", i);

        client.Send({ "LPUSH", mylist, buf }, [=](async_redis::reply *reply) {
            if (reply) {
                printf("[%d]LPUSH reply type: %d, string: %d\n", id, reply->Type(), (int)reply->GetInt());
            }
            delete reply;
        });
    }

    client.Send({ "LRANGE", mylist, "0", "-1" }, [=](async_redis::reply *reply) {
        if (!reply) {
            printf("[%d]LRANGE reply for \"LRANGE mylist\" is invalid\n", id);
            return;
        }

        if (reply->Type() == async_redis::reply::type::arrays) {
            for (const auto &element : reply->GetArray()) {
                printf("[%d]LRANGE reply type: %d, string: %s\n", id, element.Type(), element.GetString().c_str());
            }
        }
        delete reply;
    });

    auto ping = client.Send({ "PING" });
    {
        auto reply = ping.get();
        if (!reply || reply->Type() != async_redis::reply::type::status) {
            __debugbreak();
        }
    }

    printf("[%d] Done.\n", id);
}

int main(int argc, char **argv)
{
    std::vector<std::thread> workers;
    for (int i = 0; i < 200; ++i) {
        workers.emplace_back([i] {
            for (int j = 0; j < 10; ++j) {
                test(i);
            }
        });
    }

    Sleep(1000);

    for (auto &worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    return 0;
}
