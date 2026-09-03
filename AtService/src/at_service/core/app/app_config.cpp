module;
#include <cstdlib>

module at_service.core.app.config;

import sample.core.console_parser;

namespace at_service {

auto AppConfig::from_console(int argc, char** argv)
    -> std::expected<std::pair<std::string, AppConfig>, ErrorStatus>
{
    using Status = sample::ConsoleParser::Status;
    sample::ConsoleParser console_parser;
    (void) console_parser.add_parameter(
        std::string{console_parser_config_filepath_key},
        console_parser_config_filepath_short_key,
        std::string{console_parser_config_filepath_long_key},
        std::string{console_parser_config_filepath_description},
        false);
    switch(auto [status, message] = console_parser.parse(argc, argv); status) {
    case Status::success:
        break;
    case Status::error:
        return std::unexpected{ErrorStatus{.code = EXIT_FAILURE, .message = std::move(message)}};
    case Status::help_request:
    case Status::version_request:
        return std::unexpected{ErrorStatus{.code = EXIT_SUCCESS, .message = std::move(message)}};
    }
    const std::string config_path{
        console_parser.get_parameter(console_parser_config_filepath_key).value_or("config.json")};
    std::expected<AppConfig, ErrorStatus> result = from_config_file(config_path);
    if(!result) {
        return std::unexpected{result.error()};
    }
    return std::pair{config_path, *result};
}
auto AppConfig::from_config_file(const std::filesystem::path& config_path)
    -> std::expected<AppConfig, ErrorStatus>
{
    try {
        const sample::Json json{config_path};
        return AppConfig{
            .device_info = json.value<DeviceInfo>("device").value_or(DeviceInfo{}),
            .serial_info = json.value<SerialInfo>("serial").value_or(SerialInfo{}),
            .log_info = json.value<LogInfo>("log").value_or(LogInfo{}),
            .dictionary_path =
                json.value<std::string>("dictionary_path").value_or("dictionary.csv"),
            .virtual_path = json.value<std::string>("virtual_path")};
    }
    catch(const sample::error::FileException& e) {
        return std::unexpected{ErrorStatus{.code = EXIT_FAILURE, .message = e.what()}};
    }
}

} // namespace at_service
