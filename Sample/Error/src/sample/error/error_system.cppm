module;
#include <cerrno>

export module sample.error.system;

import std;

export namespace sample::error {

enum class SystemCode : int {
    interrupted = EINTR,
    connection_aborted = ECONNABORTED,
    invalid = EINVAL,
    bad_file_descriptor = EBADF,
    again = EAGAIN,
    would_block = EWOULDBLOCK,
    max_file = EMFILE,
    new_file = ENFILE,
    in_progress = EINPROGRESS
};

/**
 * @brief Captures the current thread's system error state from \c errno.
 *
 * @return \c std::error_code constructed using \c errno and \c std::system_category().
 */
auto get_last_system_error() noexcept -> std::error_code
{
    return {errno, std::system_category()};
}

/**
 * @brief Evaluates whether a given \c std::error_code corresponds to a specific \c SystemCode.
 *
 * @param error_code The error code to check.
 * @param system_code Target system error code enumeration value.
 * @return \c true if integer value matches; \c false otherwise.
 */
auto is_system_error(std::error_code error_code, SystemCode system_code) noexcept -> bool
{
    return error_code.value() == static_cast<int>(system_code);
}

/**
 * @brief Evaluates whether the current thread's active \c errno matches a \c SystemCode.
 *
 * @param system_code Target system error code enumeration value.
 * @return \c true if current \c errno matches; \c false otherwise.
 */
auto is_system_error(SystemCode system_code) noexcept -> bool
{
    const std::error_code error_code = get_last_system_error();
    return is_system_error(error_code, system_code);
}

} // namespace sample::error
