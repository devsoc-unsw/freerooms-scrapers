#pragma once

#include <curl/curl.h>

#include <string>

namespace http {

struct Response {
    long status_code = 0;
    std::string body;
};

class Client {
public:
    Client();
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    Client(Client&& other) noexcept;
    Client& operator=(Client&& other) noexcept;

    Response get(const std::string& url);

    Response post(const std::string& url);

    Response post_json(
        const std::string& url,
        const std::string& body
    );

private:
    CURL* handle_ = nullptr;
};

}
