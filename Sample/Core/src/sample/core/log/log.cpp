module sample.core.log;

import std;

namespace sample {

Log::Log(std::string_view module_name, std::shared_ptr<ControlBlock> control_block) :
    module_name_{module_name},
    control_block_{std::move(control_block)}
{
    if(!control_block_) {
        throw error::Logic{"Control block is not initialized. "
                           "You probable trying create Log outside kernel's manager"};
    }
}

void Log::force_print_impl(
    std::string_view title,
    bool debug,
    std::string_view message,
    std::source_location loc) noexcept
{
    std::array<char, log::max_message_size> buffer{};
    const std::size_t original_size = message.size();
    const std::size_t size = std::min(message.size(), log::max_message_size);
    std::memcpy(buffer.data(), message.data(), size);
    force_print_impl_internal(title, debug, buffer.data(), original_size, loc);
}

void Log::force_print_impl_internal(
    std::string_view title,
    bool debug,
    char* buffer,
    std::size_t original_size,
    std::source_location loc) noexcept
{
    const auto actual_len = std::min(original_size, log::max_message_size);
    if(original_size > log::max_message_size) {
        if constexpr(constexpr std::size_t ellipsis_size = ellipsis.size();
                     log::max_message_size > ellipsis_size)
        {
            std::memcpy(
                buffer + log::max_message_size - ellipsis_size, ellipsis.data(), ellipsis_size);
        }
    }
    const auto now = std::chrono::system_clock::now();
    const long usec =
        std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count() %
        1000000;
    std::string_view buffer_view(buffer, actual_len);
    std::lock_guard lock{mutex_};
    std::println("[{0} {1:%F %T}.{2:06}]: {3}", title, now, usec, buffer_view);
    if(debug) {
        std::println(
            "    Location: {0}:{1}\n"
            "    Function: {2}",
            loc.file_name(),
            loc.line(),
            loc.function_name());
    }
}

} // namespace sample