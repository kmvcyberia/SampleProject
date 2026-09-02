export module sample.error;

import std;

export import sample.error.system;

export namespace sample::error {

class Exception : public std::runtime_error
{
public:
    [[nodiscard]] auto where() const -> const std::source_location&
    {
        return loc_;
    }

protected:
    explicit Exception(
        const std::string& message,
        std::source_location loc = std::source_location::current()) :
        std::runtime_error{message},
        loc_{loc}
    {}

private:
    std::source_location loc_;
};

class Logic : public Exception
{
public:
    explicit Logic(
        const std::string& message,
        std::source_location loc = std::source_location::current()) :
        Exception{message, loc}
    {}
};

class SystemException : public Exception
{
public:
    explicit SystemException(
        const std::string& message,
        std::source_location loc = std::source_location::current()) :
        Exception{message, loc}
    {}
};

class FileException : public SystemException
{
public:
    explicit FileException(
        std::string filepath,
        const std::string& message,
        std::source_location loc = std::source_location::current()) :
        SystemException{message, loc},
        filepath_{std::move(filepath)}
    {}
    [[nodiscard]] auto filepath() const -> const std::string&
    {
        return filepath_;
    }

private:
    std::string filepath_;
};

class FileInvalidFormat : public FileException
{
public:
    explicit FileInvalidFormat(
        std::string filepath,
        const std::string& message,
        std::source_location loc = std::source_location::current()) :
        FileException{std::move(filepath), message, loc}
    {}
};

class FileNotFound : public FileException
{
public:
    explicit FileNotFound(
        std::string filepath,
        std::source_location loc = std::source_location::current()) :
        FileException{std::move(filepath), "File not found", loc}
    {}
};

class FileAccessDenied : public FileException
{
public:
    explicit FileAccessDenied(
        std::string filepath,
        std::source_location loc = std::source_location::current()) :
        FileException{std::move(filepath), "Access denied", loc}
    {}
};

/**
 * @brief Temporary helper to work around a C++ Modules code generation bug in GCC 15.
 *
 * @details This function isolates exception throwing within a non-templated context (.cpp file).
 * This prevents a GCC 15 compiler bug (`-fmodules-ts`) that generates incorrect local
 * assembler labels (`.Lsrc_loc0` / `R_X86_64_PC32`) when instantiating a `throw` statement
 * (or using `std::source_location`) inside templates for shared libraries (`.so`).
 *
 * @attention ⚠️ WORKAROUND
 * @todo Remove this helper entirely after upgrading to GCC 16. All invocations of
 * `error::throw_logic("...")` must be replaced back with a direct `throw error::Logic{"..."}`.
 *
 * @param message The error message text.
 */
[[noreturn]] void throw_logic(std::string_view message)
{
    throw Logic{std::string{message}};
}

} // namespace sample::error
