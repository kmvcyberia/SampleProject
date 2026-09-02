export module at_service.core.utility.file;

import std;
import sample.error;

export namespace at_service::utility {

/**
 * @brief Represents an AT command specification consisting of a match pattern and response.
 */
struct CommandSpec
{
    std::string pattern;
    std::string response;
};

/**
 * @brief Replaces text escape sequences with their corresponding ASCII control characters.
 *
 * Converts escaped sequences like `\r`, `\n`, `\t`, and `\\` into actual control bytes.
 * Any unrecognized escape sequence retains its original backslash.
 *
 * @param input View of the string containing text escape sequences.
 * @return A new \c std::string with converted control characters.
 */
[[nodiscard]] std::string unescape(std::string_view input)
{
    std::string result;
    result.reserve(input.size());
    for(std::size_t i{0}; i < input.size(); ++i) {
        if(input[i] == '\\' && i + 1 < input.size()) {
            switch(input[i + 1]) {
            case 'r':
                result += '\r';
                ++i;
                break;
            case 'n':
                result += '\n';
                ++i;
                break;
            case 't':
                result += '\t';
                ++i;
                break;
            case '\\':
                result += '\\';
                ++i;
                break;
            default:
                result += input[i];
                break;
            }
        } else {
            result += input[i];
        }
    }
    return result;
}

/**
 * @brief Loads and parses a semicolon-separated command dictionary file from disk.
 *
 * Reads a text file line-by-line where entries are defined as `pattern;response`.
 * Trims trailing `\r` characters, ignores empty lines or `#` comments, and converts
 * text escape sequences in both pattern and response fields.
 *
 * @param dictionary_path Path to the CSV/DSV dictionary file on disk.
 * @return \c std::vector<CommandSpec> on success, or \c std::error_code if the file cannot
 * be opened.
 */
[[nodiscard]] auto load_dictionary(const std::filesystem::path& dictionary_path)
    -> std::expected<std::vector<CommandSpec>, std::error_code>
{
    std::ifstream file(dictionary_path);
    if(!file.is_open()) {
        return std::unexpected{std::make_error_code(std::errc::io_error)};
    }
    std::vector<CommandSpec> result;
    std::string line;
    while(std::getline(file, line)) {
        if(const std::size_t sep_pos = line.find(';'); sep_pos != std::string::npos) {
            result.emplace_back(line.substr(0, sep_pos), line.substr(sep_pos + 1));
        }
    }
    return result;
}

} // namespace at_service::utility