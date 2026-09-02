export module sample.core.console_parser;

import std;

export namespace sample {

/**
 * @class ConsoleParser
 * @brief Non-throwing command-line argument parser.
 * Provides a reliable mechanism to register, parse, and validate command-line options.
 */
class ConsoleParser final
{
public:
    /**
     * @enum Status
     * @brief Represents the precise outcome of the argument parsing process.
     */
    enum class Status : std::uint8_t {
        success,         ///< Arguments were parsed successfully and invariants are satisfied.
        help_request,    ///< Help flag was intercepted; message contains usage manual.
        version_request, ///< Version flag was intercepted; message contains version of application.
        error            ///< Parsing failed due to invalid syntax
    };

    /**
     * @struct Result
     * @brief Capsule holding the parsing execution status and accompanying textual data.
     */
    struct Result
    {
        Status status;       ///< Execution state of the parser operation.
        std::string message; ///< Error diagnostics, generated help text, or version specification.

        /**
         * @brief Contextual conversion to boolean to seamlessly verify parsing success.
         * @return true if the arguments were processed without errors or meta-requests, false
         * otherwise.
         */
        explicit operator bool() const noexcept
        {
            return status == Status::success;
        }
    };

    /**
     * @struct Parameter
     * @brief Layout configuration and state of an individual command-line option.
     */
    struct Parameter
    {
        char short_key;          ///< One-character short key (e.g., 'v'). Use '\0' if none.
        std::string long_key;    ///< Multi-character option key (e.g., "version").
        std::string description; ///< Narrative explaining the option's objective.
        std::optional<std::string> value; ///< Parsed content string if the option was provided.
        bool flag;     ///< True if value-free switch; false if value is mandatory.
        bool required; ///< True if validation fails when parameter is omitted.
        /**
         * @brief Constructs a fully validated immutable configuration schema for a CLI parameter.
         * @param short_key Alphanumeric shorthand key character. Use '\0' if only a long key is
         * desired.
         * @param long_key String representing the long-form identifier (without leading dashes).
         * @param description Clarifying usage string that populates the help list.
         * @param flag Specifies whether this option is a value-less flag toggle.
         * @param required Sets whether this parameter must be present in the processed payload.
         */
        explicit Parameter(
            char short_key,
            std::string long_key,
            std::string description,
            bool flag = true,
            bool required = false) noexcept :
            short_key{short_key},
            long_key{std::move(long_key)},
            description{std::move(description)},
            flag{flag},
            required{required}
        {}
    };

    /**
     * @brief Initializes a new console interface configuration parser.
     * @param program_name The moniker of the binary executable, utilized for diagnostic output
     * context.
     */
    explicit ConsoleParser(std::string program_name = "Sample") noexcept;

    /**
     * @brief Evaluates and extracts the command-line payload string array.
     * Iterates over arguments starting from index 1. Successfully maps short options,
     * compound short options (grouping), and long options while verifying logical contracts.
     * @param argc Total argument counter provided directly from the program main entrance.
     * @param argv Array of null-terminated character strings signifying command options.
     * @return A Result instance indicating parsing success or detailed context regarding
     * errors/requests.
     */
    auto parse(int argc, char* argv[]) -> Result;

    /**
     * @brief Registers an expected configuration option template inside the internal map.
     * @param name Semantic inner system identifier used to query the option state afterwards.
     * @param short_key Single character short key command trigger.
     * @param long_key Multi-character long key string command trigger.
     * @param description Explanatory text that maps to the built-in help guide.
     * @param flag Designates whether the option is a value-less standalone toggle switch.
     * @param required Mandates the option presence in the input payload array.
     * @return true if the parameter template was successfully added; false if the name or key binds
     * conflict.
     */
    [[nodiscard]] auto add_parameter(
        std::string name,
        char short_key,
        std::string long_key,
        std::string description,
        bool flag = true,
        bool required = false) -> bool;

    /**
     * @brief Queries the parsed state values of a registered parameter via its identifier.
     * @param name The semantic system identifier used upon option registration.
     * @return An std::optional object containing the string payload value if set; otherwise
     * std::nullopt.
     */
    [[nodiscard]] auto get_parameter(std::string_view name) const noexcept
        -> std::optional<std::string>;

private:
    [[nodiscard]] auto print_version() const -> std::string;
    [[nodiscard]] auto print_help() const -> std::string;
    [[nodiscard]] auto print_error(std::string_view message) const -> std::string;
    std::flat_map<std::string, Parameter, std::less<>> parameter_map_;
    std::string program_name_;
};

} // namespace sample
