#pragma once

#include <blaze/async.h>
#include <blaze/request.h>
#include <blaze/response.h>

#include <functional>

namespace blaze {

using Next = std::function<Async<void>()>;
using Middleware = std::function<Async<void>(Request&, Response&, Next)>;
using Handler = std::function<Async<void>(Request&, Response&)>;

} // namespace blaze
