# OLAMP (Open Lamp)

A voice-controlled smart lamp/device based on ESP32, powered by AI and MCP protocol. Built on top of the [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) open-source project.

## Features

- **Voice Interaction**: Real-time ASR (Automatic Speech Recognition) + LLM (Large Language Model) + TTS (Text-to-Speech)
- **Camera Support**: Photo capture with AI visual analysis
- **Display**: LCD/OLED screen support with emoji display
- **Device Control**: Volume, LED, servo motor, GPIO control via MCP protocol
- **Modular Testing Framework**: Independent test modules for screen, camera, and voice

## Prerequisites

- **Operating System**: Windows with WSL2 (Ubuntu 20.04)
- **ESP-IDF**: Version 5.4 or later
- **Hardware**: ESP32-S3 development board
- **USB to Serial Driver**: CP210x, CH341, or CDC-ACM compatible

## Environment Setup

### 1. Install ESP-IDF

Follow the official [ESP-IDF Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/) to install ESP-IDF 5.4 or later.

### 2. Serial Port Configuration (WSL2 Ubuntu 20.04)

#### Check Serial Port Devices

```bash
ls -l /dev/ttyUSB*
```

#### Set Serial Port Permissions

```bash
sudo chmod 666 /dev/ttyUSB*
```

**Note**: You may need to run this command each time you reconnect the USB device, or you can create a udev rule to make it permanent.

#### Load USB Serial Drivers

```bash
sudo modprobe cp210x
sudo modprobe ch341
sudo modprobe cdc_acm
```

If the drivers are not available, you may need to install them:

```bash
sudo apt-get update
sudo apt-get install linux-modules-extra-$(uname -r)
```

## Quick Start

### 1. Clone the Repository

```bash
git clone https://github.com/SpaceEnstar01/OLAMP.git
cd OLAMP
```

### 2. Configure Serial Port and Drivers

Before building and flashing, ensure your serial port is properly configured:

```bash
# Check available serial ports
ls -l /dev/ttyUSB*

# Set permissions (replace ttyUSB0 with your actual port)
sudo chmod 666 /dev/ttyUSB0

# Load USB serial drivers
sudo modprobe cp210x
sudo modprobe ch341
sudo modprobe cdc_acm
```

### 3. Build and Flash

```bash
# Navigate to project directory
cd /home/zexuan/X/lab/xiaozhi-esp32

# Source ESP-IDF environment
source $HOME/esp/esp-idf/export.sh

# Set target chip (ESP32-S3)
idf.py set-target esp32s3

# Build the project
idf.py build

# Flash to device (replace /dev/ttyUSB0 with your actual port)
idf.py -p /dev/ttyUSB0 flash

# Monitor serial output
idf.py -p /dev/ttyUSB0 monitor
```

**Note**: Replace `/dev/ttyUSB0` with your actual serial port device. Common ports are `/dev/ttyUSB0`, `/dev/ttyUSB1`, or `/dev/ttyACM0`.

## First Time Setup

### Wi-Fi Configuration Mode

After the device boots up, it will enter Wi-Fi configuration mode:

1. **Connect to the device's hotspot** using your phone or computer
   - SSID format: `Xiaozhi-XXXX` (e.g., `Xiaozhi-0D9D`)
   - No password required

2. **Open a web browser** and navigate to:
   ```
   http://192.168.4.1
   ```

3. **Configure your Wi-Fi network**:
   - Select your Wi-Fi network from the list
   - Enter your Wi-Fi password
   - Save the configuration

4. The device will restart and connect to your Wi-Fi network.

**Serial Output Example**:
```
W (1409) Application: Alert 配网模式: 手机连接热点 Xiaozhi-0D9D，浏览器访问 http://192.168.4.1
```

## Testing

The project includes a modular testing framework for independent testing of different components.

### Test Modules

- **Screen Test** (`main/TEST/screen_test/`): Test LCD/OLED display functionality
- **Camera Test** (`main/TEST/camera_test/`): Test camera capture and image processing
- **Voice Test** (`main/TEST/voice_test/`): Test audio input/output and voice recognition

### Running Tests

For detailed information about the testing framework, see [main/TEST/README.md](main/TEST/README.md).

**Note**: In test mode, the original `main.cc` is replaced by the test module's `main.cc` during compilation. The test code is completely isolated from the main application code.

## Project Structure

```
OLAMP/
├── main/                    # Main application code
│   ├── application.cc      # Application entry point
│   ├── mcp_server.cc       # MCP protocol implementation
│   ├── audio/              # Audio processing modules
│   ├── boards/             # Hardware board configurations
│   ├── display/            # Display drivers
│   ├── led/                # LED control
│   ├── servo/              # Servo motor control
│   ├── protocols/          # Communication protocols
│   └── TEST/               # Testing framework
│       ├── screen_test/    # Screen testing module
│       ├── camera_test/    # Camera testing module
│       └── voice_test/     # Voice testing module
├── docs/                   # Documentation
├── scripts/                # Utility scripts
└── partitions/             # Partition table configurations
```

## Supported Hardware

This project supports a wide range of ESP32 development boards. Board configurations are located in `main/boards/`. Some examples include:

- ESP32-S3 based boards
- Various LCD/OLED displays
- Camera modules
- Audio codecs

To configure for your specific board, use `idf.py menuconfig` and select the appropriate board configuration.

## Acknowledgments

This project is built on top of the excellent [xiaozhi-esp32](https://github.com/78/xiaozhi-esp32) open-source project. We extend our sincere thanks to the xiaozhi project contributors for their valuable work.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

