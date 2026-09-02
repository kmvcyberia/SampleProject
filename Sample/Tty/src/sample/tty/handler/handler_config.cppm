export module sample.tty.handler.config;

import std;

export import sample.core.log;
export import sample.core.event_loop;
export import sample.tty.type;

export namespace sample::tty {

/**
 * @brief Configuration parameters for initializing a TTY device \c Handler.
 *
 * Holds references to runtime infrastructure dependencies (event loop, logger, line processor),
 * the filesystem path of the target device node, a teardown callback, and initial serial line
 * options.
 */
struct HandlerConfig
{
    EventLoop& event_loop;                 /**< Reference to the asynchronous event loop engine. */
    const Log& log;                        /**< Reference to the logger instance. */
    ProcessLineCallback& process_line_cb;  /**< Reference to the line processing callback. */
    const std::filesystem::path& dev_path; /**< Path to the target device (e.g., "/dev/ttyUSB0"). */
    const std::string& subsystem;
    std::uint16_t vendor_id;
    std::uint16_t product_id;
    std::move_only_function<void()> remove_cb; /**< Teardown callback executed when the device
                                                  disconnects or encounters a fatal I/O error. */
    SerialInfo serial_info;                    /**< Serial port communication settings. */
};

} // namespace sample::tty
