export module at_service.core.app;

import std;
import sample.core.log_manager;
import sample.core.event_loop;
import sample.tty.watcher;
import sample.tty.dispatcher;
export import at_service.core.app.config;

export namespace at_service {

/**
 * @brief Orchestrates the TTY monitoring service lifecycle, event loop, and command processing.
 *
 * The \c App class integrates logger configuration, asynchronous event handling, TTY device
 * detection via \c Watcher, and command matching via \c Dispatcher. It supports thread-safe,
 * dynamic runtime reconfiguration and command dictionary updates.
 */
class App final
{
public:
    /**
     * @brief Constructs and initializes the service infrastructure using the provided
     * configuration.
     *
     * @param config Initialization options containing device specs, log levels, and dictionary
     * path.
     * @throws sample::error::SystemException If creating the initial device watcher fails.
     * @throws sample::error::FileException If the command dictionary file cannot be loaded or
     * opened.
     */
    explicit App(AppConfig config);

    /**
     * @brief Starts the asynchronous event loop engine and initiates TTY device monitoring.
     *
     * @throws sample::error::Logic If the service event loop has already been started.
     */
    void start();

    /**
     * @brief Dynamically reconfigures log levels, TTY device watcher, and command dictionary at
     * runtime.
     *
     * Loads the updated dictionary and builds a new watcher. If successful, safely posts a task
     * to the event loop thread to swap active components without dropping pending events.
     *
     * @param config New application configuration parameters.
     * @return \c true if reconfiguration succeeded; \c false if dictionary loading or watcher setup
     * failed.
     */
    [[nodiscard]] auto reconfigure(AppConfig config) -> bool;

    /**
     * @brief Atomically updates the active AT command dictionary at runtime.
     *
     * Parses the new dictionary file and queues a task in the event loop thread to swap the active
     * \c Dispatcher instance without interrupting device monitoring.
     *
     * @param dictionary_path Path to the updated dictionary file on disk.
     */
    void update_dictionary(const std::filesystem::path& dictionary_path);

private:
    sample::tty::WatcherConfig make_watcher_config(AppConfig config);
    static auto
    create_dispatcher(const std::filesystem::path& dictionary_path, const sample::Log& log)
        -> std::unique_ptr<sample::tty::Dispatcher>;
    sample::LogManager log_manager_;
    sample::Log log_;
    sample::EventLoop event_loop_;
    std::unique_ptr<sample::tty::Dispatcher> dispatcher_;

    std::shared_ptr<sample::tty::Watcher> watcher_;

    bool running_{false};
};

} // namespace at_service
