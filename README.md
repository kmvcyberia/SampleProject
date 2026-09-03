## Building

**Requirements:** GCC 15+, CMake 3.31+, Ninja 1.11+, nlohmann_json 3.12+

```bash
mkdir build && cd build
cmake .. -G Ninja
cmake --build .
```

## TTY Device Emulation (Testing with `socat`)

For testing without physical hardware, you can emulate a TTY device using `socat`.

### 1. Create a virtual serial port pair

```bash
socat -d -d pty,raw,echo=0 pty,raw,echo=0
```

### 2. Configure the program

In your config.json, set the virtual_device field to the first PTY:

```JSON
{
  "virtual_device": "/dev/pts/3"
}
```

### 3. Start the program

Run the service with your configuration file:

```bash
./AtService config.json
```

### 4. Expected behavior

    Commands typed into /dev/pts/4 are forwarded by socat to /dev/pts/3.
    AtService reads the line from /dev/pts/3, processes it, and writes the response back.
    You see the response on the terminal attached to /dev/pts/4
