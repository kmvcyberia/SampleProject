module sample.core.console_parser:constant;

import std;

namespace sample::console_parser {

inline constexpr char help_short_key{'h'};
inline constexpr char version_short_key{'v'};
inline constexpr std::string_view help_long_key{"help"};
inline constexpr std::string_view version_long_key{"version"};

} // namespace sample::console_parser
