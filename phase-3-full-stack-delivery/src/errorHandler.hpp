#pragma once
// errorHandler.hpp — port of backend/src/middleware/errorHandler.js
// JS threw a custom HttpError with a .status; caught centrally by Express
// middleware. C++ has real exceptions built into the language, so this
// is a genuine try/catch, not a simulated one.
#include <stdexcept>
#include <string>

class HttpError : public std::runtime_error {
public:
    int status;
    HttpError(int status, const std::string& message) : std::runtime_error(message), status(status) {}
};
