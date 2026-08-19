#include "http/client.hpp"

#include <array>
#include <stdexcept>
#include <utility>

namespace {

class CurlGlobal {
public:
    CurlGlobal() {
        const auto result =
            curl_global_init(CURL_GLOBAL_DEFAULT);

        if (result != CURLE_OK) {
            throw std::runtime_error{
                "Failed to initialise libcurl"
            };
        }
    }

    ~CurlGlobal() {
        curl_global_cleanup();
    }
};

void ensure_curl_initialised() {
    static CurlGlobal curl_global;
    static_cast<void>(curl_global);
}

std::size_t write_callback(
    char* data,
    const std::size_t size,
    const std::size_t count,
    void* user_data
) {
    const auto total_size = size * count;

    auto& body =
        *static_cast<std::string*>(user_data);

    body.append(data, total_size);

    return total_size;
}

}

namespace http {

Client::Client() {
    ensure_curl_initialised();

    handle_ = curl_easy_init();

    if (handle_ == nullptr) {
        throw std::runtime_error{
            "Failed to create libcurl easy handle"
        };
    }
}

Client::~Client() {
    if (handle_ != nullptr) {
        curl_easy_cleanup(handle_);
    }
}

Client::Client(Client&& other) noexcept
    : handle_{std::exchange(other.handle_, nullptr)} {
}

Client& Client::operator=(Client&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (handle_ != nullptr) {
        curl_easy_cleanup(handle_);
    }

    handle_ =
        std::exchange(other.handle_, nullptr);

    return *this;
}

Response Client::get(
    const std::string& url
) {
    if (handle_ == nullptr) {
        throw std::runtime_error{
            "HTTP client has no curl handle"
        };
    }

    curl_easy_reset(handle_);

    Response response;

    std::array<char, CURL_ERROR_SIZE> error_buffer{};

    curl_easy_setopt(
        handle_,
        CURLOPT_URL,
        url.c_str()
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_HTTPGET,
        1L
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_FOLLOWLOCATION,
        1L
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_WRITEFUNCTION,
        write_callback
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_WRITEDATA,
        &response.body
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_ERRORBUFFER,
        error_buffer.data()
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_TIMEOUT,
        30L
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_USERAGENT,
        "publish-scraper/0.1"
    );

    const auto result =
        curl_easy_perform(handle_);

    if (result != CURLE_OK) {
        const std::string message =
            error_buffer[0] != '\0'
                ? error_buffer.data()
                : curl_easy_strerror(result);

        throw std::runtime_error{
            "HTTP request failed: " + message
        };
    }

    curl_easy_getinfo(
        handle_,
        CURLINFO_RESPONSE_CODE,
        &response.status_code
    );

    return response;
}

Response Client::post(
    const std::string& url
) {
    if (handle_ == nullptr) {
        throw std::runtime_error{
            "HTTP client has no curl handle"
        };
    }

    curl_easy_reset(handle_);

    Response response;

    std::array<char, CURL_ERROR_SIZE> error_buffer{};

    curl_easy_setopt(
        handle_,
        CURLOPT_URL,
        url.c_str()
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_POST,
        1L
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_POSTFIELDS,
        ""
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_POSTFIELDSIZE,
        0L
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_FOLLOWLOCATION,
        1L
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_WRITEFUNCTION,
        write_callback
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_WRITEDATA,
        &response.body
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_ERRORBUFFER,
        error_buffer.data()
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_TIMEOUT,
        30L
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_USERAGENT,
        "publish-scraper/0.1"
    );

    const auto result =
        curl_easy_perform(handle_);

    if (result != CURLE_OK) {
        const std::string message =
            error_buffer[0] != '\0'
                ? error_buffer.data()
                : curl_easy_strerror(result);

        throw std::runtime_error{
            "HTTP request failed: " + message
        };
    }

    curl_easy_getinfo(
        handle_,
        CURLINFO_RESPONSE_CODE,
        &response.status_code
    );

    return response;
}

Response Client::post_json(
    const std::string& url,
    const std::string& body
) {
    if (handle_ == nullptr) {
        throw std::runtime_error{
            "HTTP client has no curl handle"
        };
    }

    curl_easy_reset(handle_);

    Response response;

    std::array<char, CURL_ERROR_SIZE>
        error_buffer{};

    curl_slist* headers = nullptr;

    headers = curl_slist_append(
        headers,
        "Content-Type: application/json"
    );

    if (headers == nullptr) {
        throw std::runtime_error{
            "Failed to create HTTP headers"
        };
    }

    curl_easy_setopt(
        handle_,
        CURLOPT_URL,
        url.c_str()
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_POST,
        1L
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_POSTFIELDS,
        body.c_str()
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_POSTFIELDSIZE,
        static_cast<long>(body.size())
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_HTTPHEADER,
        headers
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_FOLLOWLOCATION,
        1L
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_WRITEFUNCTION,
        write_callback
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_WRITEDATA,
        &response.body
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_ERRORBUFFER,
        error_buffer.data()
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_TIMEOUT,
        30L
    );

    curl_easy_setopt(
        handle_,
        CURLOPT_USERAGENT,
        "publish-scraper/0.1"
    );

    const auto result =
        curl_easy_perform(handle_);

    curl_slist_free_all(headers);

    if (result != CURLE_OK) {
        const std::string message =
            error_buffer[0] != '\0'
                ? error_buffer.data()
                : curl_easy_strerror(result);

        throw std::runtime_error{
            "HTTP request failed: "
            + message
        };
    }

    curl_easy_getinfo(
        handle_,
        CURLINFO_RESPONSE_CODE,
        &response.status_code
    );

    return response;
}

}
