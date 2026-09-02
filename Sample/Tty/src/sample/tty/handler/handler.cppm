module;
#include <termios.h>

export module sample.tty.handler;

import std;
import sample.core.event_listener;
import sample.core.file.file_guard;
export import sample.core.event_loop;
export import sample.core.log;
export import sample.tty.type;
export import sample.tty.handler.config;

export namespace sample::tty {

/**
 * @brief Manages asynchronous I/O operations and serial line configuration for an open TTY device.
 *
 * The \c Handler class owns a serial port file descriptor, handles non-blocking read and write
 * events integrated into the \c EventLoop, buffers outgoing data, and forwards received data
 * chunks to the provided line callback.
 *
 * Inherits from \c EventListener to receive read/write readiness notifications from the event loop.
 */
class Handler final : public EventListener, public std::enable_shared_from_this<Handler>
{
public:
    class Key
    {
        Key() = default;
        friend class Handler;
    };
    explicit Handler(Key, HandlerConfig config, FileGuard descriptor);
    ~Handler() override;
    Handler(const Handler&) = delete;
    auto operator=(const Handler&) -> Handler& = delete;
    Handler(Handler&& other) = delete;
    auto operator=(Handler&& other) -> Handler& = delete;

    /**
     * @brief Factory method to open, configure, instantiate, and register a TTY \c Handler.
     *
     * Opens the device node at \c config.dev_path in non-blocking mode (\c O_RDWR | \c O_NOCTTY |
     * \c O_NONBLOCK), applies initial \c termios settings, instantiates the \c Handler, and
     * registers it with the \c EventLoop.
     *
     * @param config Configuration options for the handler.
     * @return \c std::shared_ptr<Handler> on success, or a system \c std::error_code on failure.
     */
    static auto create(HandlerConfig config)
        -> std::expected<std::shared_ptr<Handler>, std::error_code>;

    /**
     * @brief Queues data to be asynchronously written to the serial device.
     *
     * Appends data to the internal write buffer. If the buffer was empty prior to this call,
     * it activates write event listening in the event loop for the underlying file descriptor.
     *
     * @param data View of the byte sequence to be sent.
     */
    void send(std::string_view data);

    /**
     * @brief Handles read readiness notifications from the event loop.
     *
     * Reads pending bytes from the serial descriptor. On success, passes data to \c
     * process_line_cb_. On EOF or unrecoverable error, logs the failure and triggers \c remove_cb_.
     */
    void read() override;

    /**
     * @brief Handles write readiness notifications from the event loop.
     *
     * Flushes queued outgoing data to the serial descriptor. Disables write readiness notifications
     * in the event loop once the write buffer is drained. Triggers \c remove_cb_ on write errors.
     */
    void write() override;

    /**
     * @brief Reconfigures serial communication parameters for the active file descriptor on the
     * fly.
     *
     * Compares the new parameters against the active \c serial_info_ and applies changes to the
     * underlying \c termios structure using \c TCSADRAIN.
     *
     * @param serial_info Desired serial port settings.
     * @return Empty \c std::expected on success, or a system \c std::error_code if \c termios
     * updates fail.
     */
    auto reconfigure_serial(SerialInfo serial_info) noexcept
        -> std::expected<void, std::error_code>;

private:
    auto start() -> std::expected<void, std::error_code>;

    static auto init_serial(int descriptor, SerialInfo serial_info) noexcept
        -> std::expected<void, std::error_code>;
    static void apply_serial_info(termios& tty, SerialInfo serial_info) noexcept;

    EventLoop& event_loop_;
    Log log_;
    ProcessLineCallback& process_line_cb_;
    FileGuard descriptor_;
    std::string dev_path_;
    std::move_only_function<void()> remove_cb_;
    std::deque<std::string> write_buffer_;
    std::size_t write_offset_{0};
    SerialInfo serial_info_;
};

} // namespace sample::tty
