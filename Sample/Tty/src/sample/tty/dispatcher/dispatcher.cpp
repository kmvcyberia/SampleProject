module;
#include <sample/compiler_fix.h>

module sample.tty.dispatcher;

namespace sample::tty {

void Dispatcher::add_command(std::string pattern, Command::Callback callback)
{
    dictionary.emplace_back(std::move(pattern), std::move(callback));
}

bool Dispatcher::process_line(std::string_view line, SendCallback& send_cb)
{
    const std::string_view line_clean = trim(line);
    if(line_clean.empty()) {
        return false;
    }
    // NOLINTNEXTLINE(readability-use-*)
    for(auto& [pattern, callback] : dictionary) {
        if(match_pattern(line_clean, pattern)) {
            callback(send_cb);
            return true;
        }
    }
    return false;
}

constexpr auto Dispatcher::trim(std::string_view data) noexcept -> std::string_view
{
    constexpr std::string_view targets{" \t\r\n\0", 5};
    const std::size_t first = data.find_first_not_of(targets);
    if(first == std::string_view::npos) {
        return {};
    }
    data.remove_prefix(first);
    const std::size_t last = data.find_last_not_of(targets);
    data.remove_suffix(data.size() - last - 1);
    return data; // NOLINT
}

constexpr auto Dispatcher::match_pattern(std::string_view text, std::string_view pattern) noexcept
    -> bool
{
    std::size_t text_idx{0};
    std::size_t pattern_idx{0};
    std::size_t star_idx{std::string_view::npos};
    std::size_t match_idx{0};
    while(text_idx < text.size()) {
        if(pattern_idx < pattern.size() &&
           (pattern[pattern_idx] == '.' || pattern[pattern_idx] == text[text_idx]))
        {
            ++text_idx;
            ++pattern_idx;
        } else if(pattern_idx < pattern.size() && pattern[pattern_idx] == '*') {
            star_idx = pattern_idx;
            match_idx = text_idx;
            ++pattern_idx;
        } else if(star_idx != std::string_view::npos) {
            pattern_idx = star_idx + 1;
            ++match_idx;
            text_idx = match_idx;
        } else {
            return false;
        }
    }
    while(pattern_idx < pattern.size() && pattern[pattern_idx] == '*') {
        ++pattern_idx;
    }
    return pattern_idx == pattern.size();
}

} // namespace sample::tty