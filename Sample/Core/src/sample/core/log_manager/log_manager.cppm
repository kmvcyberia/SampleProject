export module sample.core.log_manager;

import std;
import sample.core.string_unordered_map;
import sample.core.thread_safe_data;
export import sample.core.log;

export namespace sample {

/**
 * @class LogManager
 * @brief Centralized registry and lifetime manager for module log configurations.
 * The @c LogManager acts as a thread-safe factory and administration point for @c Log * instances.
 * It manages shared configuration states (@c ControlBlock) using a highly efficient, contiguous @c
 * std::flat_map wrapped in a thread-safe data container.
 * @note This class is both non-copyable and non-movable due to its underlying synchronization
 * primitives.
 */
class LogManager final
{
    using ControlBlockMap =
        std::flat_map<std::string, std::weak_ptr<Log::ControlBlock>, std::less<>>;

public:
    /**
     * @brief Retrieves an existing log handle or instantiates a new one for a specific module.
     * Performs an efficient lookup in the internal registry. If the module's control block
     * is active, a new lightweight @c Log handle is returned. If the module does not exist,
     * or its previous logger has expired, a new @c ControlBlock is allocated and registered.
     * @param module_name The unique string identifier for the target module.
     * @return Log A lightweight, thread-safe log handle bound to the module's runtime state.
     */
    [[nodiscard]] auto get_logger(std::string_view module_name) -> Log;

    /**
     * @brief Atomically updates the logging severity threshold for a specific module on the fly.
     * Locates the active configuration block for the specified module and updates its threshold
     * level. If the module is not found or its log instances have been entirely destroyed,
     * the dead entry is automatically pruned from the registry.
     * @param module_name The unique string identifier of the module to modify.
     * @param level The new @c Log::Level severity threshold to enforce.
     * @return true If the module was found and its active level was updated.
     * @return false If the module does not exist or has no active @c Log references.
     */
    auto set_module_level(std::string_view module_name, Log::Level level) noexcept -> bool;

    /**
     * @brief Atomically enables or disables debug logging for a specific module on the fly.
     * Locates the active configuration block for the specified module and updates its debug mode
     * state. If the module is not found or its log instances have been entirely destroyed, the dead
     * entry is automatically pruned from the registry.
     * @param module_name The unique string identifier of the module to modify.
     * @param debug @c true to enable debug logging, or @c false to disable it.
     * @return true If the module was found and its debug state was updated.
     * @return false If the module does not exist or has no active @c Log references.
     */
    auto set_module_debug(std::string_view module_name, bool debug) noexcept -> bool;

private:
    ThreadSafeData<ControlBlockMap> control_block_map_;
};

} // namespace sample