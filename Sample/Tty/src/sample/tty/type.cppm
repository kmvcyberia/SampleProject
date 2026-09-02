export module sample.tty.type;

import std;

export namespace sample::tty {

/**
 * @brief Callback invoked to send raw string data back to a TTY device.
 *
 * @param data View of the byte sequence to be transmitted.
 */
using SendCallback = std::move_only_function<void(std::string_view)>;

/**
 * @brief Callback invoked to process an incoming line of TTY data.
 *
 * @param line View of the received data line.
 * @param send_cb Reference to a callback for sending responses back to the TTY device.
 */
using ProcessLineCallback = std::move_only_function<void(std::string_view, SendCallback&)>;

/**
 * @brief Parity check options for serial communication.
 */
enum class Parity : std::uint8_t { none, even, odd };

/**
 * @brief Flow control modes for serial communication.
 */
enum class FlowControl : std::uint8_t { none, hardware, software };

[[nodiscard]] constexpr auto parity_to_string(Parity parity) noexcept -> std::string_view
{
    switch(parity) {
    case Parity::none:
        return "none";
    case Parity::even:
        return "even";
    case Parity::odd:
        return "odd";
    }
    return "unknown";
}

[[nodiscard]] constexpr auto flow_control_to_string(FlowControl flow_control) noexcept
    -> std::string_view
{
    switch(flow_control) {
    case FlowControl::none:
        return "none";
    case FlowControl::hardware:
        return "hardware";
    case FlowControl::software:
        return "software";
    }
    return "unknown";
}

/**
 * @brief Configuration parameters for serial port hardware settings.
 */
struct SerialInfo
{
    std::uint32_t baud_rate{115200};
    std::uint8_t data_bits{8};
    std::uint8_t stop_bits{1};
    Parity parity{Parity::none};
    FlowControl flow_control{FlowControl::none};
    auto operator<=>(const SerialInfo&) const = default;

    /**
     * @brief Validates and sanitizes serial port settings, substituting defaults for invalid
     * values.
     *
     * Ensures \c baud_rate is a standard supported value, \c data_bits is within [5, 8],
     * \c stop_bits is 1 or 2, and enum values fall within valid ranges.
     *
     * @return A sanitized copy of \c SerialInfo with guaranteed valid configuration values.
     */
    [[nodiscard]] constexpr auto sanitized() const -> SerialInfo
    {
        SerialInfo clean = *this;
        switch(clean.baud_rate) {
        case 9600:
        case 19200:
        case 38400:
        case 57600:
        case 115200:
        case 230400:
            break;
        default:
            clean.baud_rate = 115200;
            break;
        }
        if(clean.data_bits < 5 || clean.data_bits > 8) {
            clean.data_bits = 8;
        }
        if(clean.stop_bits != 1 && clean.stop_bits != 2) {
            clean.stop_bits = 1;
        }
        switch(clean.parity) {
        case Parity::none:
        case Parity::even:
        case Parity::odd:
            break;
        default:
            clean.parity = Parity::none;
            break;
        }
        switch(clean.flow_control) {
        case FlowControl::none:
        case FlowControl::hardware:
        case FlowControl::software:
            break;
        default:
            clean.flow_control = FlowControl::none;
            break;
        }
        return clean;
    }
};

} // namespace sample::tty
