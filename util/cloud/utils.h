// Copyright 2024, Roman Gershman.  All rights reserved.
// See LICENSE for licensing terms.

#pragma once

#include <boost/beast/http/dynamic_body.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/parser.hpp>

#include "util/http/http_client.h"

namespace util::cloud {

namespace detail {
inline std::string_view FromBoostSV(boost::string_view sv) {
  return std::string_view(sv.data(), sv.size());
}

class HttpRequestBase {
 public:
  HttpRequestBase(const HttpRequestBase&) = delete;
  HttpRequestBase& operator=(const HttpRequestBase&) = delete;
  HttpRequestBase() = default;

  virtual ~HttpRequestBase() = default;
  virtual std::error_code Send(http::Client* client) = 0;

  const boost::beast::http::header<true>& GetHeaders() const {
    return const_cast<HttpRequestBase*>(this)->GetHeadersInternal();
  }

  void SetHeader(boost::beast::http::field f, std::string_view value) {
    GetHeadersInternal().set(f, boost::string_view{value.data(), value.size()});
  }

  void SetHeader(std::string_view f, std::string_view value) {
    GetHeadersInternal().set(boost::string_view{f.data(), f.size()},
                             boost::string_view{value.data(), value.size()});
  }

 protected:
  virtual boost::beast::http::header<true>& GetHeadersInternal() = 0;
};

class EmptyRequestImpl : public HttpRequestBase {
  using EmptyRequest = boost::beast::http::request<boost::beast::http::empty_body>;
  EmptyRequest req_;

 public:
  EmptyRequestImpl(boost::beast::http::verb req_verb, std::string_view url);
  EmptyRequestImpl(EmptyRequestImpl&& other) : req_(std::move(other.req_)) {
  }

  void SetUrl(std::string_view url) {
    req_.target(boost::string_view{url.data(), url.size()});
  }

  void Finalize() {
    req_.prepare_payload();
  }

  std::error_code Send(http::Client* client) final;

 protected:
  boost::beast::http::header<true>& GetHeadersInternal() final {
    return req_.base();
  }
};

class DynamicBodyRequestImpl : public HttpRequestBase {
  using DynamicBodyRequest = boost::beast::http::request<boost::beast::http::dynamic_body>;
  DynamicBodyRequest req_;

 public:
  DynamicBodyRequestImpl(DynamicBodyRequestImpl&& other) : req_(std::move(other.req_)) {
  }

  explicit DynamicBodyRequestImpl(std::string_view url)
      : req_(boost::beast::http::verb::post, boost::string_view{url.data(), url.size()}, 11) {
  }

  template <typename BodyArgs> void SetBody(BodyArgs&& body_args) {
    req_.body() = std::forward<BodyArgs>(body_args);
  }

  void Finalize() {
    req_.prepare_payload();
  }

  std::error_code Send(http::Client* client) final;

 protected:
  boost::beast::http::header<true>& GetHeadersInternal() final {
    return req_.base();
  }
};

}  // namespace detail

}  // namespace util::cloud