export module at_service.core.app.config;

import std;

export import sample.core.file.json;
export import sample.core.log;
export import sample.tty.type;

namespace at_service {

inline constexpr char console_parser_config_filepath_short_key{'c'};
inline constexpr std::string_view console_parser_config_filepath_long_key{"config-filepath"};
inline constexpr std::string_view console_parser_config_filepath_key{"config-filename"};
inline constexpr std::string_view console_parser_config_filepath_description{
    "Filepath to configuration file"};

} // namespace at_service

export namespace at_service {

/**
 * @brief Main configuration Data Transfer Object (DTO) for the AT service application.
 *
 * Encapsulates device discovery filters, serial port parameters, logging options,
 * and the filesystem path to the command dictionary.
 */
struct AppConfig
{
    struct DeviceInfo
    {
        std::optional<std::string> subsystem;
        std::optional<std::string> path;
        std::optional<std::uint16_t> vendor_id;
        std::optional<std::uint16_t> product_id;
    };

    struct LogInfo
    {
        std::optional<sample::Log::Level> level;
        std::optional<bool> debug;
    };

    using SerialInfo = sample::tty::SerialInfo;

    /**
     * @brief Execution status or error details returned by CLI parsing.
     */
    struct ErrorStatus
    {
        int code;            ///< Process exit code (e.g., EXIT_SUCCESS or EXIT_FAILURE).
        std::string message; ///< Detailed error description, help text, or version info.
    };

    DeviceInfo device_info;
    SerialInfo serial_info;
    LogInfo log_info;
    std::string dictionary_path;
    std::optional<std::string> virtual_path;

    /**
     * @brief Parses CLI arguments, resolves configuration file path, and loads settings.
     *
     * Evaluates command-line flags (falling back to "config.json" if omitted) and reads
     * the target JSON configuration file.
     *
     * @param argc Command-line argument count from \c main().
     * @param argv Array of command-line argument strings from \c main().
     * @return \c std::pair containing the resolved config path and \c AppConfig on success,
     *         or \c ErrorStatus with exit code and message on error/help request.
     */
    static auto from_console(int argc, char** argv)
        -> std::expected<std::pair<std::string, AppConfig>, ErrorStatus>;

    /**
     * @brief Deserializes application configuration settings from a JSON file.
     *
     * @param config_path Path to the JSON configuration file on disk.
     * @return \c AppConfig instance on success, or \c ErrorStatus detailing read/parse failures.
     */
    static auto from_config_file(const std::filesystem::path& config_path)
        -> std::expected<AppConfig, ErrorStatus>;
};

inline auto
from_json_node(const sample::Json::Node& node, std::type_identity<AppConfig::DeviceInfo>)
    -> std::optional<AppConfig::DeviceInfo>
{
    return AppConfig::DeviceInfo{
        .subsystem = node.get_child("subsystem").as<std::string>(),
        .path = node.get_child("path").as<std::string>(),
        .vendor_id = node.get_child("vendor_id").as<std::uint16_t>(),
        .product_id = node.get_child("product_id").as<std::uint16_t>()};
}

inline auto from_json_node(const sample::Json::Node& node, std::type_identity<AppConfig::LogInfo>)
    -> std::optional<AppConfig::LogInfo>
{
    using Level = sample::Log::Level;
    const auto level_str = node.get_child("level").as<std::string>();
    std::optional<Level> level;
    if(level_str == "info") {
        level = Level::info;
    } else if(level_str == "warning") {
        level = Level::warning;
    } else if(level_str == "error") {
        level = Level::error;
    } else if(level_str == "critical") {
        level = Level::critical;
    }
    return AppConfig::LogInfo{.level = level, .debug = node.get_child("debug").as<bool>()};
}

} // namespace at_service

export namespace sample::tty {

inline auto from_json_node(const Json::Node& node, std::type_identity<SerialInfo>)
    -> std::optional<SerialInfo>
{
    SerialInfo serial_info;
    if(const auto parity_opt = node.get_child("parity").as<std::string>()) {
        if(std::string_view parity_str = *parity_opt; parity_str == "none") {
            serial_info.parity = Parity::none;
        } else if(parity_str == "even") {
            serial_info.parity = Parity::even;
        } else if(parity_str == "odd") {
            serial_info.parity = Parity::odd;
        }
    }
    if(const auto flow_control_opt = node.get_child("flow_control").as<std::string>()) {
        if(std::string_view flow_control_str = *flow_control_opt; flow_control_str == "none") {
            serial_info.flow_control = FlowControl::none;
        } else if(flow_control_str == "hardware") {
            serial_info.flow_control = FlowControl::hardware;
        } else if(flow_control_str == "software") {
            serial_info.flow_control = FlowControl::software;
        }
    }
    if(const auto baud_rate = node.get_child("baud_rate").as<std::uint32_t>()) {
        serial_info.baud_rate = *baud_rate;
    }
    if(const auto data_bits = node.get_child("data_bits").as<std::uint8_t>()) {
        serial_info.data_bits = *data_bits;
    }
    if(const auto stop_bits = node.get_child("stop_bits").as<std::uint8_t>()) {
        serial_info.stop_bits = *stop_bits;
    }
    return serial_info.sanitized();
}

} // namespace sample::tty
