export module sample.tty.watcher.config;

import std;

export import sample.core.event_loop;
export import sample.core.log;
export import sample.core.file.file_guard;
export import sample.tty.type;

export namespace sample::tty {

/**
 * @brief Configuration parameters for initializing a TTY device \c Watcher.
 *
 * Contains references to the event loop and logging system, the line processing callback,
 * optional uevent filtering criteria (subsystem, device path, USB vendor/product IDs),
 * and serial communication settings.
 */
struct WatcherConfig
{
    /**
     * @brief Callback invoked when a complete line of data is received from a TTY handler.
     *
     * @param line View of the received string data.
     * @param send_cb Reference to a callback for sending responses back to the TTY device.
     *
     * @note Workaround for GCC 15 Internal Compiler Error (ICE).
     * @bug GCC 15 crashes with a segmentation fault during cross-module AST serialization
     *      when \c std::move_only_function is imported transitively from \c sample.tty.type.
     * @todo Remove this duplicated type alias once GCC C++ modules support stabilizes.
     * */
    using ProcessLineCallback = std::move_only_function<void(std::string_view, SendCallback&)>;

    EventLoop& event_loop;                /**< Reference to the asynchronous event loop engine. */
    Log log;                              /**< Logger instance for diagnostic and error logging. */
    ProcessLineCallback process_line_cb;  /**< Callback for processing incoming TTY data lines. */
    std::optional<std::string> subsystem; /**< Filter for the kernel subsystem (e.g., "tty"). */
    std::optional<std::string> path;      /**< Filter for the device path (e.g., "/dev/ttyUSB0"). */
    std::optional<std::uint16_t> vendor_id;  /**< Filter for the 16-bit USB vendor ID. */
    std::optional<std::uint16_t> product_id; /**< Filter for the 16-bit USB product/model ID. */
    SerialInfo serial_info;                  /**< Configuration options for serial port. */
};

} // namespace sample::tty
