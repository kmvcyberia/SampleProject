Markdown

# Configuration Documentation

The configuration file is used by the emulator server to set up device filtering parameters, logging rules, dictionary paths, and serial port (`termios`) settings.

By default, the server expects a configuration file in **JSON** format (e.g., `config.json`).

---

## Parameter Reference

### 1. `device` Section (Device Filtering & Path)

Defines parameters for locating the device via `Watcher` (Netlink) or directly connecting to a specified path.

| Parameter | Type | Default | Allowed Values | Description |
| :--- | :--- | :--- | :--- | :--- |
| `vendor_id` | integer | `0` | `0`–`65535` (`0x0000`–`0xFFFF`) | USB Vendor ID of the target device. `0` disables Vendor ID filtering. |
| `product_id` | integer | `0` | `0`–`65535` (`0x0000`–`0xFFFF`) | USB Product ID of the target device. `0` disables Product ID filtering. |
| `subsystem` | string | `"tty"` | `"tty"`, `"usb"` | Linux kernel subsystem to monitor via `udev` events. |
| `path` | string | `"/dev/ttyS0"` | Valid TTY path | Static file system path to the serial port (`/dev/ttyS0`, `/dev/ttyUSB0`, `/dev/pts/1`, etc.). |

---

### 2. General Parameters

| Parameter | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| `dictionary_path` | string | `"dictionary.csv"` | Relative or absolute path to the CSV file containing command patterns and modem responses. |

---

### 3. `log` Section (Logging Settings)

Controls logging granularity and stdout debug output.

| Parameter | Type | Default | Allowed Values | Description |
| :--- | :--- | :--- | :--- | :--- |
| `level` | string | `"info"` | `"debug"`, `"info"`, `"warn"`, `"error"` | Minimum severity level for log output. |
| `debug` | boolean | `true` | `true`, `false` | Enables verbose debug logging to standard output (`stdout`/`stderr`). |

---

### 4. `serial` Section (Serial Port / Termios Settings)

Configures low-level communication parameters for the serial interface.

| Parameter | Type | Default | Allowed Values | Description |
| :--- | :--- | :--- | :--- | :--- |
| `baud_rate` | integer | `115200` | `9600`, `19200`, `38400`, `57600`, `115200`, etc. | Data transmission speed in baud (bits per second). |
| `data_bits` | integer | `8` | `5`, `6`, `7`, `8` | Number of data bits per frame. |
| `stop_bits` | integer | `1` | `1`, `2` | Number of stop bits. |
| `parity` | string | `"none"` | `"none"`, `"even"`, `"odd"` | Parity checking mode (`"none"` for no parity, `"even"`, or `"odd"`). |
| `flow_control` | string | `"none"` | `"none"`, `"hardware"`, `"software"` | Flow control method (`"hardware"` for RTS/CTS, `"software"` for XON/XOFF). |

---

### 5. `virtual_path` Section (Virtual Device Mode)

Enables the server to operate as a device emulator by creating or attaching to virtual TTY ports. This allows the program to simultaneously act as both a controller (for real devices) and an emulator (for testing/development).
Parameter	Type	Default	Allowed Values	Description
virtual_path	string	"" (disabled)	Valid TTY path (e.g., /dev/pts/3, /tmp/virtual_tty0)	Path to a virtual serial port. When set, the server creates a Handler for this path and begins responding to AT commands as if it were a real device.

---

## Configuration Example (`config.json`)

```json
{
  "device": {
    "vendor_id": 43,
    "product_id": 43,
    "subsystem": "tty",
    "path": "/dev/ttyS0"
  },
  "dictionary_path": "dictionary.csv",
  "log": {
    "level": "info",
    "debug": false
  },
  "serial": {
    "baud_rate": 115200,
    "data_bits": 8,
    "stop_bits": 1,
    "parity": "none",
    "flow_control": "none"
  },
  "virtual_path": "/dev/pts/3"
}
