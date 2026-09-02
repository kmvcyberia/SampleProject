module;
#include <linux/netlink.h>
#include <sys/socket.h>

module sample.tty.watcher;

import std;
import sample.error;

namespace sample::tty {
Watcher::Watcher(Key, WatcherConfig config, FileGuard netlink_descriptor) :
    event_loop_{config.event_loop},
    log_{std::move(config.log)},
    process_line_cb_{std::move(config.process_line_cb)},
    netlink_descriptor_{std::move(netlink_descriptor)},
    subsystem_{std::move(config.subsystem)},
    path_{std::move(config.path)},
    vendor_id_{config.vendor_id},
    product_id_{config.product_id},
    serial_info_{config.serial_info}
{}

Watcher::~Watcher()
{
    event_loop_.remove_listener(netlink_descriptor_.get());
}

auto Watcher::create(WatcherConfig config)
    -> std::expected<std::shared_ptr<Watcher>, std::error_code>
{
    config.log.info(
        "Starting watcher...\n"
        "\t\tDevice settings\n"
        "\tsubsystem `{0}`\n\tpath `{1}`\n\tvendor_id `{2}`\n\tproduct_id `{3}`\n"
        "\t\tSerial port settings\n"
        "\tBaud rate `{4}`\n\tData bits `{5}`\n\tStop bits `{6}`\n"
        "\tParity `{7}`\n\tFlowControl `{8}`",
        config.subsystem.value_or("*"),
        config.path.value_or("*"),
        config.vendor_id ? std::format("{:04x}", *config.vendor_id) : "*",
        config.product_id ? std::format("{:04x}", *config.product_id) : "*",
        config.serial_info.baud_rate,
        config.serial_info.data_bits,
        config.serial_info.stop_bits,
        parity_to_string(config.serial_info.parity),
        flow_control_to_string(config.serial_info.flow_control));
    std::expected<FileGuard, std::error_code> netlink_descriptor = create_netlink_descriptor();
    if(!netlink_descriptor) {
        return std::unexpected{netlink_descriptor.error()};
    }
    auto watcher =
        std::make_shared<Watcher>(Key{}, std::move(config), *std::move(netlink_descriptor));
    if(const std::expected<void, std::error_code> result = watcher->start(); !result) {
        return std::unexpected{result.error()};
    }
    return watcher;
}

void Watcher::read()
{
    std::array<char, 8192> buffer; // NOLINT(*-pro-type-member-init)
    while(true) {
        const std::ptrdiff_t byte_count =
            ::recv(netlink_descriptor_.get(), buffer.data(), buffer.size(), MSG_DONTWAIT);

        if(byte_count <= 0) {
            if(byte_count == -1) {
                const std::error_code ec = error::get_last_system_error();
                if(error::is_system_error(ec, error::SystemCode::again) ||
                   error::is_system_error(ec, error::SystemCode::would_block))
                {
                    break;
                }
                if(error::is_system_error(ec, error::SystemCode::interrupted)) {
                    continue;
                }
            }
            break;
        }
        std::string_view payload(buffer.data(), static_cast<std::size_t>(byte_count));
        if(!payload.starts_with("libudev")) {
            continue;
        }
        std::string_view dev_action;
        std::string_view dev_name;
        std::string_view subsystem;
        std::uint16_t vendor_id{0};
        std::uint16_t product_id{0};
        std::size_t pos{0};
        const std::size_t payload_size{payload.size()};
        while(pos < payload_size) {
            const std::size_t null_pos{payload.find('\0', pos)};
            if(null_pos == std::string_view::npos) {
                break;
            }
            std::string_view entry = payload.substr(pos, null_pos - pos);
            pos = null_pos + 1;
            if(entry.empty()) {
                continue;
            }
            if(entry.starts_with("ACTION=")) {
                dev_action = entry.substr(7);
            } else if(entry.starts_with("DEVNAME=")) {
                dev_name = entry.substr(8);
            } else if(entry.starts_with("SUBSYSTEM=")) {
                subsystem = entry.substr(10);
            } else if(entry.starts_with("ID_VENDOR_ID=")) {
                auto hex_str = entry.substr(13);
                std::from_chars(hex_str.data(), hex_str.data() + hex_str.size(), vendor_id, 16);
            } else if(entry.starts_with("ID_MODEL_ID=")) {
                auto hex_str = entry.substr(12);
                std::from_chars(hex_str.data(), hex_str.data() + hex_str.size(), product_id, 16);
            }
        }

        if(vendor_id_ && *vendor_id_ != vendor_id || product_id_ && *product_id_ != product_id ||
           subsystem_ && *subsystem_ != subsystem || path_ && *path_ != dev_name)
        {
            continue;
        }
        if(dev_action == "add") {
            if(!handler_map_.contains(dev_name)) {
                std::expected<std::shared_ptr<Handler>, std::error_code> handler_exp =
                    Handler::create(
                        HandlerConfig{
                            .event_loop = event_loop_,
                            .log = log_,
                            .process_line_cb = process_line_cb_,
                            .dev_path = dev_name,
                            .subsystem = std::string{subsystem},
                            .vendor_id = vendor_id,
                            .product_id = product_id,
                            .remove_cb =
                                [this, dev_name = std::string{dev_name}] {
                                    if(const auto it = handler_map_.find(dev_name);
                                       it != handler_map_.end()) {
                                        handler_map_.erase(it);
                                    }
                                },
                            .serial_info = serial_info_});
                if(handler_exp) {
                    handler_map_.emplace(std::string{dev_name}, *std::move(handler_exp));
                } else {
                    log_.error(
                        "Cannot create handler, reason `{0}`", handler_exp.error().message());
                }
            }
        } else if(dev_action == "remove") {
            if(auto it = handler_map_.find(dev_name); it != handler_map_.end()) {
                handler_map_.erase(it);
            }
        }
    }
}

auto Watcher::start() -> std::expected<void, std::error_code>
{
    return event_loop_.add_listener(netlink_descriptor_.get(), shared_from_this());
}

auto Watcher::create_netlink_descriptor() -> std::expected<FileGuard, std::error_code>
{
    FileGuard netlink_descriptor{
        ::socket(AF_NETLINK, SOCK_RAW | SOCK_NONBLOCK | SOCK_CLOEXEC, NETLINK_KOBJECT_UEVENT)};
    if(!netlink_descriptor) {
        return std::unexpected{error::get_last_system_error()};
    }
    sockaddr_nl addr{};
    addr.nl_family = AF_NETLINK;
    addr.nl_groups = 2;
    const int status = ::bind(
        netlink_descriptor.get(), reinterpret_cast<const sockaddr*>(&addr), sizeof(sockaddr_nl));
    if(status == -1) {
        return std::unexpected{error::get_last_system_error()};
    }
    return netlink_descriptor;
}

} // namespace sample::tty