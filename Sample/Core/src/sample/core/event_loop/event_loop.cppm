export module sample.core.event_loop;

import std;
import sample.core.file.file_guard;
import sample.core.thread_safe_data;
export import sample.core.event_listener;

export namespace sample {

/**
 * @brief High-performance Linux epoll-based reactor event loop.
 *
 * Manages asynchronous I/O multiplexing via Edge-Triggered \c epoll and non-blocking
 * inter-thread signaling via \c eventfd. Executes non-blocking event handling and deferred tasks
 * within a dedicated background thread managed by \c std::jthread.
 */
class EventLoop final
{
public:
    EventLoop();
    ~EventLoop();
    EventLoop(const EventLoop&) = delete;
    auto operator=(const EventLoop&) -> EventLoop& = delete;
    EventLoop(EventLoop&&) = delete;
    auto operator=(EventLoop&&) -> EventLoop& = delete;

    /**
     * @brief Spawns the worker thread and begins asynchronous event processing.
     *
     * @throws sample::error::Logic If the event loop worker thread is already running.
     */
    void start();

    /**
     * @brief Enables write readiness (\c EPOLLOUT) monitoring for a file descriptor.
     *
     * @param descriptor Active file descriptor previously registered with \c add_listener.
     * @return \c std::expected containing empty \c void value on success, or OS \c std::error_code
     * on failure.
     */
    auto enable_write_listener(int descriptor) noexcept -> std::expected<void, std::error_code>;

    /**
     * @brief Disables write readiness (\c EPOLLOUT) monitoring for a file descriptor.
     *
     * @param descriptor Active file descriptor monitored by the epoll instance.
     * @return \c std::expected containing empty \c void value on success, or OS \c std::error_code
     * on failure.
     */
    auto disable_write_listener(int descriptor) noexcept -> std::expected<void, std::error_code>;

    /**
     * @brief Registers a file descriptor and its event listener callback with the reactor.
     *
     * Registers the descriptor with \c epoll using Edge-Triggered mode (\c EPOLLET | \c EPOLLIN)
     * and stores a \c std::weak_ptr to the listener interface in a thread-safe map.
     *
     * @param descriptor Target non-blocking file descriptor (e.g., socket, TTY).
     * @param listener Shared pointer to the event listener implementation.
     * @return \c std::expected containing empty \c void value on success, or OS \c std::error_code
     * on failure.
     * @throws sample::error::Logic If \c listener is null or \c descriptor is already registered.
     */
    auto add_listener(int descriptor, const std::shared_ptr<EventListener>& listener)
        -> std::expected<void, std::error_code>;

    /**
     * @brief Unregisters a file descriptor from epoll monitoring and internal tracking.
     *
     * @param descriptor Target file descriptor to remove.
     */
    void remove_listener(int descriptor) noexcept;

    /**
     * @brief Thread-safely posts an arbitrary callable task to be executed on the event loop
     * thread.
     *
     * Enqueues the callable object and writes a signal byte to the internal \c eventfd
     * descriptor to interrupt \c epoll_wait if idling.
     *
     * @param task Move-only callable object accepting no arguments and returning void.
     */
    void call_task(std::move_only_function<void()> task);

private:
    void wakeup() const noexcept;
    void process_loop(std::stop_token stop_token);
    auto modify_epoll(int descriptor, std::uint32_t events) noexcept
        -> std::expected<void, std::error_code>;
    FileGuard epoll_descriptor_;
    FileGuard wakeup_descriptor_;
    ThreadSafeData<std::unordered_map<int, std::weak_ptr<EventListener>>> event_listener_map_;
    ThreadSafeData<std::vector<std::move_only_function<void()>>> task_vector_;
    std::jthread thread_;
};

} // namespace sample
