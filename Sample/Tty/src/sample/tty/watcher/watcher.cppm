export module sample.tty.watcher;

import std;
import sample.core.log;
import sample.core.file.file_guard;
import sample.tty.handler;
import sample.core.string_unordered_map;
export import sample.tty.watcher.config;
export import sample.core.event_loop;

export namespace sample::tty {

/**
 * @brief Monitors Linux uevent notifications via Netlink socket for TTY device hotplug events.
 *
 * The \c Watcher class listens for kernel uevents over a \c NETLINK_KOBJECT_UEVENT socket.
 * When matching TTY devices are connected ("add") or disconnected ("remove"), it dynamically
 * manages the lifecycle of corresponding \c Handler instances.
 *
 * Inherits from \c EventListener to integrate directly into an asynchronous \c EventLoop.
 */
class Watcher final : public EventListener, public std::enable_shared_from_this<Watcher>
{
public:
    class Key
    {
        Key() = default;
        friend class Watcher;
    };

    explicit Watcher(Key key, WatcherConfig config, FileGuard netlink_descriptor);
    ~Watcher() override;
    Watcher(const Watcher&) = delete;
    auto operator=(const Watcher&) -> Watcher& = delete;
    Watcher(Watcher&& other) = delete;
    auto operator=(Watcher&& other) -> Watcher& = delete;

    /**
     * @brief Factory method to initialize, start, and register a new \c Watcher.
     *
     * Creates a non-blocking uevent Netlink socket, binds it to kernel broadcast groups,
     * instantiates the watcher, and registers it with the event loop provided in \p config.
     *
     * @param config Configuration options containing event loop, logger, and filters.
     * @return \c std::shared_ptr<Watcher> on success, or a system \c std::error_code on failure.
     */
    static auto create(WatcherConfig config)
        -> std::expected<std::shared_ptr<Watcher>, std::error_code>;

    /**
     * @brief Handles write readiness notifications from the event loop.
     *
     * @note No-op for the uevent Netlink watcher as it only receives kernel messages.
     */
    void write() override {}

    /**
     * @brief Handles read readiness notifications when incoming uevent messages arrive.
     *
     * Reads uevent payloads from the Netlink socket, filters them by configured criteria
     * (subsystem, device path, USB vendor/product IDs), and spawns or destroys TTY handlers.
     */
    void read() override;

private:
    auto start() -> std::expected<void, std::error_code>;
    static auto create_netlink_descriptor() -> std::expected<FileGuard, std::error_code>;

    EventLoop& event_loop_;
    Log log_;
    ProcessLineCallback process_line_cb_;
    FileGuard netlink_descriptor_;
    std::optional<std::string> subsystem_;
    std::optional<std::string> path_;
    std::optional<std::uint16_t> vendor_id_;
    std::optional<std::uint16_t> product_id_;
    string_unordered_map<std::shared_ptr<Handler>> handler_map_;
    SerialInfo serial_info_;
};

} // namespace sample::tty
