module;
#include <ctime>

export module sample.core.log;

import std;
import :constant;

export import sample.error;

export namespace sample {

/**
 * @class Log
 * @brief A high-performance, thread-safe, and macro-free logging engine.
 *
 * The @c Log class performs all message formatting within a fixed-size stack buffer
 * specified by @c sample::log::max_message_size. This design guarantees absolute
 * immunity to Out-of-Memory (OOM) failures and heap fragmentation in the hot path.
 */
class Log final
{
    static constexpr std::string_view ellipsis{"…"};

public:
    /**
     * @enum Level
     * @brief Defines the severity threshold of log messages.
     */
    enum class Level : std::uint8_t {
        critical, ///< Severe errors causing immediate application termination.
        error,    ///< Runtime failures allowing the server to keep running.
        warning,  ///< Unexpected behaviors or non-fatal anomalies.
        info      ///< General operational messages tracking application state.
    };

    /**
     * @struct ControlBlock
     * @brief Shared runtime configuration state for a specific log instance.
     */
    struct ControlBlock
    {
        std::atomic<Level> level{Level::info}; ///< Current active logging threshold.
        std::atomic<bool> debug{true};         ///< Toggles inclusion of precise source locations.

        /**
         * @brief Constructs a ControlBlock with default configuration.
         * @details Default threshold is `Level::info` and debug locations are enabled (`true`).
         */
        ControlBlock() = default;

        /**
         * @brief Constructs a ControlBlock with a custom logging level.
         * @param level Target logging threshold to set.
         * @note `debug` flag retains its default value (`true`).
         */
        explicit ControlBlock(Level level) noexcept : level{level} {}

        /**
         * @brief Constructs a ControlBlock with custom level and debug flag.
         * @param level Target logging threshold to set.
         * @param debug Enables or disables inclusion of precise source locations.
         */
        explicit ControlBlock(Level level, bool debug) noexcept : level{level}, debug{debug} {}
    };

    template <typename... Args> struct Context
    {
        std::basic_format_string<char, Args...> fmt;
        std::source_location loc;
        template <typename T>
        consteval Context(
            const T& fmt,
            std::source_location loc = std::source_location::current()) noexcept :
            fmt{fmt},
            loc{loc}
        {}
    };

    /**
     * @brief Constructs a new Log instance bound to a shared runtime configuration.
     * @param module_name Unique identifier string for the module.
     * @param control_block Shared pointer to the level and debug configuration flags.
     * @throw sample::error::Logic if @p control_block is @c nullptr.
     */
    explicit Log(std::string_view module_name, std::shared_ptr<ControlBlock> control_block);

    /**
     * @brief Logs an informational message if the active threshold allows it.
     * @note Evaluates threshold using @c std::memory_order_acquire.
     * @param context Bound format string and source location.
     * @param args Variadic arguments matching the format string.
     */
    template <typename... Args>
    void info(std::type_identity_t<Context<Args...>> context, Args&&... args) const noexcept
    {
        if(control_block_->level.load(std::memory_order_acquire) >= Level::info) {
            if(control_block_->debug.load()) {
                force_info_debug(context, std::forward<Args>(args)...);
            } else {
                force_info(context, std::forward<Args>(args)...);
            }
        }
    }

    /**
     * @brief Logs a simple informational message if the active threshold allows it.
     * @note Evaluates threshold using @c std::memory_order_acquire.
     * @param message The text message to be logged.
     * @param loc The source location where the log call originated.
     */
    void info(std::string_view message, std::source_location loc = std::source_location::current())
        const noexcept
    {
        if(control_block_->level.load(std::memory_order_acquire) >= Level::info) {
            if(control_block_->debug.load()) {
                force_info_debug(message, loc);
            } else {
                force_info(message, loc);
            }
        }
    }

    /**
     * @brief Logs a warning message if the active threshold allows it.
     * @note Evaluates threshold using @c std::memory_order_acquire.
     * @param context Bound format string and source location.
     * @param args Variadic arguments matching the format string.
     */
    template <typename... Args>
    void warning(std::type_identity_t<Context<Args...>> context, Args&&... args) const noexcept
    {
        if(control_block_->level.load(std::memory_order_acquire) >= Level::warning) {
            if(control_block_->debug.load()) {
                force_warning_debug(context, std::forward<Args>(args)...);
            } else {
                force_warning(context, std::forward<Args>(args)...);
            }
        }
    }

    /**
     * @brief Logs a simple warning message if the active threshold allows it.
     * @note Evaluates threshold using @c std::memory_order_acquire.
     * @param message The text message to be logged.
     * @param loc The source location where the log call originated.
     */
    void warning(
        std::string_view message,
        std::source_location loc = std::source_location::current()) const noexcept
    {
        if(control_block_->level.load(std::memory_order_acquire) >= Level::warning) {
            if(control_block_->debug.load()) {
                force_warning_debug(message, loc);
            } else {
                force_warning(message, loc);
            }
        }
    }

    /**
     * @brief Logs an error message if the active threshold allows it.
     * @note Evaluates threshold using @c std::memory_order_acquire.
     * @param context Bound format string and source location.
     * @param args Variadic arguments matching the format string.
     */
    template <typename... Args>
    void error(std::type_identity_t<Context<Args...>> context, Args&&... args) const noexcept
    {
        if(control_block_->level.load(std::memory_order_acquire) >= Level::error) {
            if(control_block_->debug.load()) {
                force_error_debug(context, std::forward<Args>(args)...);
            } else {
                force_error(context, std::forward<Args>(args)...);
            }
        }
    }

    /**
     * @brief Logs a simple error message if the active threshold allows it.
     * @note Evaluates threshold using @c std::memory_order_acquire.
     * @param message The text message to be logged.
     * @param loc The source location where the log call originated.
     */
    void error(std::string_view message, std::source_location loc = std::source_location::current())
        const noexcept
    {
        if(control_block_->level.load(std::memory_order_acquire) >= Level::error) {
            if(control_block_->debug.load()) {
                force_error_debug(message, loc);
            } else {
                force_error(message, loc);
            }
        }
    }

    /**
     * @brief Unconditionally outputs a critical failure and terminates control flow.
     * @details Outputs the message with a @c CRITICAL banner and immediately throws.
     * @note This function never returns normally.
     * @param context Bound format string and source location.
     * @param args Variadic arguments matching the format string.
     * @throw sample::error::Logic always thrown to interrupt execution.
     */
    template <typename... Args>
    [[noreturn]] static void
    critical(std::type_identity_t<Context<Args...>> context, Args&&... args)
    {
        force_print_impl(title_critical, false, context, std::forward<Args>(args)...);
        throw error::Logic{"critical error"};
    }

    /**
     * @brief Lightweight static utility for raw log output.
     * @details Bypasses module names, severity filters, and timestamps.
     * @param fmt Validated format string.
     * @param args Arguments matching the format string.
     */
    template <typename... Args> static void print(std::format_string<Args...> fmt, Args&&... args)
    {
        // NOLINTNEXTLINE(*-pro-type-member-init)
        std::array<char, log::max_message_size> buffer;
        const auto [out_it, total_size_signed] = std::format_to_n(
            buffer.data(), log::max_message_size, fmt, std::forward<Args>(args)...);
        if(const auto total_size = static_cast<std::size_t>(total_size_signed);
           total_size <= log::max_message_size)
        {
            std::println("{}", std::string_view(buffer.data(), total_size));
            return;
        }
        constexpr std::size_t ellipsis_size = ellipsis.size();
        static_assert(
            log::max_message_size >= ellipsis_size,
            "max_message_size must be at least as large as ellipsis_size");
        constexpr std::size_t prefix_len = log::max_message_size - ellipsis_size;
        std::memcpy(buffer.data() + prefix_len, ellipsis.data(), ellipsis_size);
        std::println("{}", std::string_view(buffer.data(), log::max_message_size));
    }

    /**
     * @brief Lightweight static utility for raw log output.
     * @param message The text message to be logged.
     */
    static void print(std::string_view message) noexcept
    {
        if(message.size() <= log::max_message_size) {
            std::println("{}", message);
            return;
        }
        constexpr std::size_t ellipsis_size = ellipsis.size();
        static_assert(
            log::max_message_size >= ellipsis_size,
            "max_message_size must be at least as large as ellipsis_size");
        // NOLINTNEXTLINE(*-pro-type-member-init)
        std::array<char, log::max_message_size> buffer;
        constexpr std::size_t prefix_len = log::max_message_size - ellipsis_size;
        std::memcpy(buffer.data(), message.data(), prefix_len);
        std::memcpy(buffer.data() + prefix_len, ellipsis.data(), ellipsis_size);
        std::println("{}", std::string_view(buffer.data(), buffer.size()));
    }

    /**
     * @brief Bypasses severity checks to print a message under a custom title.
     * @param title Custom banner string (e.g., "AUDIT", "SECURITY").
     * @param context Bound format string and source location.
     * @param args Variadic arguments matching the format string.
     */
    template <typename... Args>
    static void force_print(
        std::string_view title,
        std::type_identity_t<Context<Args...>> context,
        Args&&... args) noexcept
    {
        force_print_impl(title, false, context, std::forward<Args>(args)...);
    }

    /** @name Explicit Force Overrides
     *  Static methods to bypass active runtime atomic severity checks.
     * @param context Bound format string and source location.
     * @param args Variadic arguments matching the format string.
     *  @{ */
    template <typename... Args>
    static void force_info(std::type_identity_t<Context<Args...>> context, Args&&... args) noexcept
    {
        force_print_impl(title_info, false, context, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void
    force_info_debug(std::type_identity_t<Context<Args...>> context, Args&&... args) noexcept
    {
        force_print_impl(title_info, true, context, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void
    force_warning(std::type_identity_t<Context<Args...>> context, Args&&... args) noexcept
    {
        force_print_impl(title_warning, false, context, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void
    force_warning_debug(std::type_identity_t<Context<Args...>> context, Args&&... args) noexcept
    {
        force_print_impl(title_warning, true, context, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void force_error(std::type_identity_t<Context<Args...>> context, Args&&... args) noexcept
    {
        force_print_impl(title_error, false, context, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void
    force_error_debug(std::type_identity_t<Context<Args...>> context, Args&&... args) noexcept
    {
        force_print_impl(title_error, true, context, std::forward<Args>(args)...);
    }
    /** @} */

    /** @name Explicit Force Overrides
     *  Static methods to bypass active runtime atomic severity checks.
     * @param message The text message to be logged.
     * @param loc The source location where the log call originated.
     *  @{ */
    static void force_info(
        std::string_view message,
        std::source_location loc = std::source_location::current()) noexcept
    {
        force_print_impl(title_info, false, message, loc);
    }
    static void force_info_debug(
        std::string_view message,
        std::source_location loc = std::source_location::current()) noexcept
    {
        force_print_impl(title_info, true, message, loc);
    }

    static void force_warning(
        std::string_view message,
        std::source_location loc = std::source_location::current()) noexcept
    {
        force_print_impl(title_warning, false, message, loc);
    }

    static void force_warning_debug(
        std::string_view message,
        std::source_location loc = std::source_location::current()) noexcept
    {
        force_print_impl(title_warning, true, message, loc);
    }

    static void force_error(
        std::string_view message,
        std::source_location loc = std::source_location::current()) noexcept
    {
        force_print_impl(title_error, false, message, loc);
    }

    static void force_error_debug(
        std::string_view message,
        std::source_location loc = std::source_location::current()) noexcept
    {
        force_print_impl(title_error, true, message, loc);
    }
    /** @} */

    /**
     * @brief Retrieves the active logging severity threshold level.
     * @return Current active @c Level.
     */
    [[nodiscard]] auto get_level() const noexcept -> Level
    {
        return control_block_->level.load(std::memory_order_acquire);
    }

    /**
     * @brief Checks if source-location debug logging is enabled.
     * @return @c true if debug printing is active, @c false otherwise.
     */
    [[nodiscard]] auto get_debug() const noexcept -> bool
    {
        return control_block_->debug.load(std::memory_order_acquire);
    }

    /**
     * @brief Retrieves the module identifier name handle.
     * @return Non-owning @c std::string_view mapping to the module name.
     */
    [[nodiscard]] auto get_module_name() const noexcept -> std::string_view
    {
        return module_name_;
    }

private:
    template <typename... Args>
    static void force_print_impl(
        std::string_view title,
        bool debug,
        std::type_identity_t<Context<Args...>> context,
        Args&&... args) noexcept
    {

        std::array<char, log::max_message_size> buffer{};
        auto [out_it, original_size] = std::format_to_n(
            buffer.data(), log::max_message_size, context.fmt, std::forward<Args>(args)...);
        force_print_impl_internal(
            title, debug, buffer.data(), static_cast<std::size_t>(original_size), context.loc);
    }

    static void force_print_impl(
        std::string_view title,
        bool debug,
        std::string_view message,
        std::source_location loc) noexcept;
    static void force_print_impl_internal(
        std::string_view title,
        bool debug,
        char* buffer,
        std::size_t original_size,
        std::source_location loc) noexcept;

    std::string_view module_name_;
    std::shared_ptr<ControlBlock> control_block_;
    inline static std::mutex mutex_;
    static constexpr std::string_view title_info{"INFO"};
    static constexpr std::string_view title_warning{"WARNING"};
    static constexpr std::string_view title_error{"ERROR"};
    static constexpr std::string_view title_critical{"CRITICAL ERROR"};
};

} // namespace sample
