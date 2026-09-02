export module sample.tty.dispatcher;

import std;

export import sample.tty.type;

export namespace sample::tty {

/**
 * @brief Matches incoming TTY lines against registered command patterns and executes callbacks.
 *
 * The \c Dispatcher maintains a list of registered patterns (supporting `*` and `.` wildcards).
 * When processing a line, it automatically trims leading/trailing whitespace and control
 * characters, finds the first matching pattern, and invokes the associated callback with a reply
 * handler.
 */
class Dispatcher final
{
public:
    /**
     * @brief Represents a registered command pattern and its handler callback.
     */
    struct Command
    {
        using Callback = std::move_only_function<void(SendCallback&)>;
        std::string pattern;
        Callback callback;
    };

    /**
     * @brief Registers a new command pattern and its handler callback.
     *
     * @param pattern Pattern string to match (e.g., "AT.").
     * @param callback Handler function invoked when an incoming line matches \p pattern.
     */
    void add_command(std::string pattern, Command::Callback callback);

    /**
     * @brief Trims an incoming text line, checks it against patterns, and executes.
     *
     * Leading/trailing spaces, tabs, carriage returns, newlines, and null bytes are trimmed.
     *
     * @param line Raw incoming text line to evaluate.
     * @param send_cb Reference to a callback for sending responses back to the TTY device.
     * @return \c true if a matching command was found and executed; \c false otherwise.
     */
    [[nodiscard]] auto process_line(std::string_view line, SendCallback& send_cb) -> bool;

private:
    [[nodiscard]] static constexpr auto trim(std::string_view data) noexcept -> std::string_view;
    [[nodiscard]] static constexpr auto
    match_pattern(std::string_view text, std::string_view pattern) noexcept -> bool;
    std::vector<Command> dictionary;
};

} // namespace sample::tty
