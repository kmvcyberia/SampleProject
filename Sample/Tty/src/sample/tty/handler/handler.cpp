module;
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

module sample.tty.handler;

import sample.error;

namespace sample::tty {

Handler::Handler(Key, HandlerConfig config, FileGuard descriptor) :
    event_loop_{config.event_loop},
    log_{config.log},
    process_line_cb_{config.process_line_cb},
    descriptor_{std::move(descriptor)},
    dev_path_{config.dev_path.string()},
    remove_cb_{std::move(config.remove_cb)},
    serial_info_{config.serial_info}
{}

Handler::~Handler()
{
    event_loop_.remove_listener(descriptor_.get());
}

auto Handler::create(HandlerConfig config)
    -> std::expected<std::shared_ptr<Handler>, std::error_code>
{
    FileGuard descriptor{::open(config.dev_path.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK)};
    if(!descriptor) {
        return std::unexpected{error::get_last_system_error()};
    }

    std::expected<void, std::error_code> result = init_serial(descriptor.get(), config.serial_info);
    if(!result) {
        return std::unexpected{result.error()};
    }
    config.log.info(
        "Starting handler...\n"
        "\tdev path `{0}`\n\tsubsystem `{1}`\n\tvendor_id `{2:04x}`\n\tproduct_id `{3:04x}`",
        config.dev_path.string(),
        config.subsystem,
        config.vendor_id,
        config.product_id);
    auto handler = std::make_shared<Handler>(Key{}, std::move(config), std::move(descriptor));
    result = handler->start();
    if(!result) {
        return std::unexpected{result.error()};
    }
    return handler;
}

void Handler::send(std::string_view data)
{
    if(data.empty()) {
        return;
    }
    const bool is_empty = write_buffer_.empty();
    write_buffer_.emplace_back(data);
    if(is_empty && !event_loop_.enable_write_listener(descriptor_.get())) {
        log_.error(
            "Cannot enable write, device path `{0}`, descriptor `{1}`",
            dev_path_,
            descriptor_.get());
        remove_cb_();
    }
}

void Handler::read()
{
    // NOLINTNEXTLINE(*-pro-type-member-init)
    std::array<char, 4096> buffer;
    while(true) {
        const std::ptrdiff_t byte_count = ::read(descriptor_.get(), buffer.data(), buffer.size());
        if(byte_count <= 0) {
            if(byte_count == -1) {
                const auto ec = error::get_last_system_error();
                if(error::is_system_error(ec, error::SystemCode::again) ||
                   error::is_system_error(ec, error::SystemCode::would_block))
                {
                    break;
                }
                if(error::is_system_error(ec, error::SystemCode::interrupted)) {
                    continue;
                }
                log_.error(
                    "Read error, device path `{0}`, descriptor `{1}`, reason `{2}`",
                    dev_path_,
                    descriptor_.get(),
                    ec.message());
                remove_cb_();
                return;
            }
            log_.info(
                "Device disconnected or EOF reached, device path `{0}`, descriptor `{1}`",
                dev_path_,
                descriptor_.get());
            remove_cb_();
            return;
        }
        const std::string_view chunk{buffer.data(), static_cast<std::size_t>(byte_count)};
        SendCallback send_cb = [this](std::string_view data) { send(data); };
        process_line_cb_(chunk, send_cb);
    }
}

void Handler::write()
{
    while(!write_buffer_.empty()) {
        const std::string& chunk = write_buffer_.front();
        const char* data_ptr = chunk.data() + write_offset_;
        const std::size_t remaining_bytes = chunk.size() - write_offset_;

        const std::ptrdiff_t byte_count = ::write(descriptor_.get(), data_ptr, remaining_bytes);

        if(byte_count <= 0) {
            if(byte_count == -1) {
                const std::error_code ec = error::get_last_system_error();
                if(error::is_system_error(ec, error::SystemCode::again) ||
                   error::is_system_error(ec, error::SystemCode::would_block))
                {
                    return;
                }
                if(error::is_system_error(ec, error::SystemCode::interrupted)) {
                    continue;
                }
                log_.error(
                    "Write error, device path `{0}`, descriptor `{1}`, reason `{2}`",
                    dev_path_,
                    descriptor_.get(),
                    ec.message());
                remove_cb_();
                return;
            }
            log_.error(
                "Write returned 0 bytes, device path `{0}`, descriptor `{1}`",
                dev_path_,
                descriptor_.get());
            remove_cb_();
            return;
        }

        write_offset_ += static_cast<std::size_t>(byte_count);
        if(write_offset_ >= chunk.size()) {
            write_buffer_.pop_front();
            write_offset_ = 0;
        }
    }
    if(!event_loop_.disable_write_listener(descriptor_.get())) {
        log_.error(
            "Cannot disable write, device path `{0}`, descriptor `{1}`",
            dev_path_,
            descriptor_.get());
        remove_cb_();
    }
}

auto Handler::reconfigure_serial(SerialInfo serial_info) noexcept
    -> std::expected<void, std::error_code>
{
    const SerialInfo serial_info_clean{serial_info.sanitized()};
    if(serial_info_clean == serial_info_) {
        return {};
    }
    termios tty{};
    if(::tcgetattr(descriptor_.get(), &tty) != 0) {
        return std::unexpected{error::get_last_system_error()};
    }
    apply_serial_info(tty, serial_info_clean);
    if(::tcsetattr(descriptor_.get(), TCSADRAIN, &tty) != 0) {
        return std::unexpected{error::get_last_system_error()};
    }
    serial_info_ = serial_info_clean;
    return {};
}

auto Handler::start() -> std::expected<void, std::error_code>
{
    return event_loop_.add_listener(descriptor_.get(), shared_from_this());
}

auto Handler::init_serial(int descriptor, SerialInfo serial_info) noexcept
    -> std::expected<void, std::error_code>
{
    termios tty{};
    if(::tcgetattr(descriptor, &tty) != 0) {
        return std::unexpected{error::get_last_system_error()};
    }
    ::cfmakeraw(&tty);
    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_iflag |= IGNBRK;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 0;
    apply_serial_info(tty, serial_info.sanitized());
    ::tcflush(descriptor, TCIOFLUSH);
    if(::tcsetattr(descriptor, TCSANOW, &tty) != 0) {
        return std::unexpected{error::get_last_system_error()};
    }
    return {};
}

void Handler::apply_serial_info(termios& tty, SerialInfo serial_info) noexcept
{
    speed_t speed{B115200};
    switch(serial_info.baud_rate) {
    case 9600:
        speed = B9600;
        break;
    case 19200:
        speed = B19200;
        break;
    case 38400:
        speed = B38400;
        break;
    case 57600:
        speed = B57600;
        break;
    case 115200:
        speed = B115200;
        break;
    case 230400:
        speed = B230400;
        break;
    default:
        speed = B115200;
        break;
    }
    ::cfsetispeed(&tty, speed);
    ::cfsetospeed(&tty, speed);
    tty.c_cflag &= ~CSIZE;
    switch(serial_info.data_bits) {
    case 5:
        tty.c_cflag |= CS5;
        break;
    case 6:
        tty.c_cflag |= CS6;
        break;
    case 7:
        tty.c_cflag |= CS7;
        break;
    case 8:
    default:
        tty.c_cflag |= CS8;
        break;
    }
    tty.c_cflag &= ~(PARENB | PARODD);
    if(serial_info.parity == Parity::even) {
        tty.c_cflag |= PARENB;
    } else if(serial_info.parity == Parity::odd) {
        tty.c_cflag |= (PARENB | PARODD);
    }
    if(serial_info.stop_bits == 2) {
        tty.c_cflag |= CSTOPB;
    } else {
        tty.c_cflag &= ~CSTOPB;
    }
    if(serial_info.flow_control == FlowControl::hardware) {
        tty.c_cflag |= CRTSCTS;
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    } else if(serial_info.flow_control == FlowControl::software) {
        tty.c_cflag &= ~CRTSCTS;
        tty.c_iflag |= (IXON | IXOFF);
    } else {
        tty.c_cflag &= ~CRTSCTS;
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    }
}

} // namespace sample::tty
