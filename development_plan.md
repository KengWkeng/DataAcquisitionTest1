# Development Plan

I want to design an engine data acquisition architecture based on Qt C++ 6.8.1. Please provide a development guide based on my requirements, proceeding step by step. Each step should result in a runnable and testable (at least partially functional) version, gradually increasing complexity. First, run the complete process using the virtual device example from the config, then gradually add DAQ, ECU, and Modbus protocols.

## Acquisition Process:
The main thread `mainwindow` reads the initialization information and data channel information of the acquisition hardware contained in `config.json` via `ConfigManager`. This information is then injected into the `DeviceManager` in the main thread. Using a factory pattern, based on the different device types and initialization information read, device instances are created following the "interface-class-object" steps. Each device instance is then moved to a child thread. `ConfigManager` also passes the parameters (gain, offset, cubic function calibration parameters) for each hardware channel corresponding to a software channel to the `ChannelManager` in `DataProcessThread` to create and connect the corresponding hardware and software data acquisition channels. The data synchronization thread (`ChannelManager`) occupies a dedicated thread responsible for collecting raw hardware data from all acquisition threads, performing time alignment (achieved through the same sampling frequency), generating data with a unified timeline processed by `DataMangaer`, generating corresponding secondary instruments (data requiring calculation from primary collected quantities using formulas), sending the complete acquisition data (a complete frame includes all primary and secondary instrument quantities) to the main thread for display, and also sending it to the storage thread to save the collected data into CSV format files according to channel and timestamp.

## Initialize Devices:
Design interfaces, classes, and objects based on the `config.json` input. The main thread `mainwindow` reads `config.json` via `ConfigManager`. Using a factory model, `DeviceManager` implements the creation and initialization of multiple acquisition devices and assigns them to corresponding child threads. Each hardware data channel read by each device corresponds to a subsequent data channel. `Device` serves as an overall interface. The configuration method for each different type of device instance is configured as a subclass of `Device`. Depending on the hardware, there will be different input parameters and configuration methods. Leave the implementation for specific hardware blank for now. First, implement the configuration for the virtual device test instance. Run the complete process with the virtual device example from the config, then gradually add DAQ, ECU, and Modbus protocols. All `Device` classes must reserve an interface for filtering the collected data, initially implemented as empty.

## Initialize Channels
Data collected by hardware and filtered is considered raw data. To synchronize hardware devices with different sampling rates, I plan to use a "snapshot" method. The multi-channel data collected in all hardware child threads will update the raw data of the corresponding acquisition channels in real-time. I need to periodically read all channel data at the specified synchronization interval, record the timestamp, and generate synchronized raw data frames before processing. Raw data needs to undergo "gain and offset" calculations to be converted into the corresponding primary instrument quantities. However, due to potential sensor deviations, we need to perform a cubic function calibration on this data before outputting it. Based on the cubic function coefficients specified in the config, generate the final primary instrument quantities. The method for calculating secondary quantities from primary quantities will not be implemented here; specific implementation methods will be determined after all data links are tested.

## Modularization
### Core Architecture Philosophy:

*   **Clear Module Responsibilities:** Each module does one thing and interacts with other modules through well-defined interfaces (signals/slots, abstract classes).
*   **Thread Isolation:** Place time-consuming operations (device I/O, data processing, file storage) in separate threads to ensure the main thread (UI) remains responsive.
*   **Configuration Driven:** The system's behavior (which devices to connect, how to process channels) is determined by an external configuration file.
*   **Interface-Oriented Programming:** Extensive use of abstract base classes and interfaces facilitates extension (e.g., adding new device types) and testing (using virtual devices).

### Module Planning and Responsibilities:

#### Core Module (Basic Core):

*   **Responsibilities:** Define basic data structures, enums, and constants shared across the entire application.
*   **Main Content:**
    *   `struct DeviceConfig`: Contains device type, connection parameters (serial port, baud rate, IP address, etc.), list of hardware channels under the device, etc.
    *   `struct ChannelConfig`: Contains software channel ID, associated device ID and hardware channel index, Gain, Offset, cubic calibration coefficients (a, b, c, d for ax^3 + bx^2 + cx + d), unit, etc.
    *   `struct RawDataPoint`: Contains raw value, timestamp, source device ID, hardware channel index.
    *   `struct ProcessedDataPoint`: Contains processed value (primary instrument quantity), timestamp, software channel ID, status (validity).
    *   `struct SynchronizedDataFrame`: Contains a timestamp and a list or `QMap<ChannelID, ProcessedDataPoint>` of all channel `ProcessedDataPoint`s at that timestamp.
    *   `enum DeviceType`: MODBUS, DAQ, ECU, VIRTUAL.
    *   `enum StatusCode`: Status codes for devices or channels.
*   **Thread:** No specific thread; referenced by other modules.

#### Config Module (Configuration Management):

*   **Responsibilities:** Read, parse `config.json`, validate configuration, and provide structured configuration data to other modules.
*   **Main Class:** `ConfigManager` (usually a singleton or an instance managed by the main application).
*   **Methods:**
    *   `bool loadConfig(const QString& filePath)`: Load and parse JSON.
    *   `QList<DeviceConfig> getDeviceConfigs() const`: Get the list of device configurations.
    *   `QMap<QString, ChannelConfig> getChannelConfigs() const`: Get channel configurations (with channel ID as Key).
    *   `int getSynchronizationIntervalMs() const`: Get the data synchronization (snapshot) interval.
*   **Thread:** Typically initialized and called in the main thread, passing configuration data to other modules that need it (usually during the initialization phase).

#### Device Module (Device Communication):

*   **Responsibilities:** Manage device connections, data acquisition, and emission of raw data. Each physical device runs in its own separate child thread.
*   **Main Classes:**
    *   `DeviceManager`: (Runs in the main thread)
        *   Responsible for creating specific device objects (`VirtualDevice`, `ModbusDevice`, etc.) using a factory pattern based on the `DeviceConfig` list provided by `ConfigManager`.
        *   Creates a `QThread` for each device instance.
        *   Moves the device object to its corresponding child thread using `moveToThread()`.
        *   Manages the start and stop of device threads.
        *   Connects signals emitted by devices to slots in `DataSynchronizer`.
        *   Provides control interfaces (`startAllDevices()`, `stopAllDevices()`, `getDeviceStatus()`).
    *   `AbstractDevice` (Abstract base class, `QObject`):
        *   Defines common interfaces: `virtual bool connectDevice() = 0;`, `virtual bool disconnectDevice() = 0;`, `virtual void startAcquisition() = 0;`, `virtual void stopAcquisition() = 0;`, `virtual QString getDeviceID() const = 0;`.
        *   Defines common signals: `void rawDataPointReady(QString deviceId, int hardwareChannelIndex, double rawValue, qint64 timestamp);`, `void deviceStatusChanged(QString deviceId, StatusCode status, QString message);`, `void errorOccurred(QString deviceId, QString errorMsg);`.
        *   Reserves a filtering interface: `virtual double applyFilter(double rawValue) { return rawValue; }` (Subclasses can override this method and call it before emitting `rawDataPointReady`).
    *   `VirtualDevice`, `ModbusDevice`, `DAQDevice`, `ECUDevice` (Concrete device classes, inherit `AbstractDevice`):
        *   Implement the pure virtual functions of `AbstractDevice`.
        *   In their `startAcquisition`-related logic (usually a loop or a timer-triggered slot), read hardware data, call `applyFilter`, then emit the `rawDataPointReady` signal.
        *   The constructor receives the corresponding `DeviceConfig` for initialization.
*   **Thread:** `DeviceManager` is created and managed in the main thread, but each `AbstractDevice` instance runs in its own independent `QThread` child thread.

#### Processing Module (Data Synchronization & Processing):

*   **Responsibilities:** Collect raw data points from all device threads, perform "snapshot" synchronization, apply gain/offset and calibration, and generate synchronized data frames.
*   **Main Classes:**
    *   `DataSynchronizer` (`QObject`):
        *   **Channel Management:** Internally maintains a `QMap<QString, Channel>` (mapping software channel ID to `Channel` object). Creates `Channel` objects based on `ChannelConfig` provided by `ConfigManager` during initialization.
        *   **Raw Data Cache:** Internally maintains a thread-safe cache (`QMap<QPair<QString, int>, RawDataPoint> latestRawData_`, where Key is `{deviceId, hardwareChannelIndex}`) to store the latest raw data point for each hardware channel. Use `QMutex` for access protection.
        *   **Snapshot Timer:** Contains a `QTimer` whose interval is configured by `ConfigManager`.
        *   **Slot `onRawDataPointReady(QString deviceId, int hardwareChannelIndex, double rawValue, qint64 timestamp)`:** (Connection type `Qt::QueuedConnection`) Updates the `latestRawData_` cache.
        *   **Slot `takeSnapshot()`:** Triggered by `QTimer`.
            *   Get the current timestamp `syncTimestamp`.
            *   Lock access to `latestRawData_`.
            *   Create a `SynchronizedDataFrame`.
            *   Iterate through all configured `Channel` objects.
            *   For each `Channel`, find the latest value of its associated hardware channel (`deviceId`, `hardwareChannelIndex`) in `latestRawData_`.
            *   If data is found, call the corresponding `Channel` object's `process(rawValue)` method to perform gain/offset and calibration calculations.
            *   Add the calculated `ProcessedDataPoint` to the `SynchronizedDataFrame`.
            *   Unlock.
            *   Emit the `synchronizedFrameReady(SynchronizedDataFrame frame)` signal.
        *   **Signal:** `synchronizedFrameReady(SynchronizedDataFrame frame);`
    *   `Channel` (Helper class, possibly not a `QObject`):
        *   Holds its own `ChannelConfig`.
        *   **Method `ProcessedDataPoint process(double rawValue, qint64 timestamp)`:**
            *   Apply gain and offset: `calibratedValue = rawValue * gain + offset;`
            *   Apply cubic calibration: `finalValue = a * pow(calibratedValue, 3) + b * pow(calibratedValue, 2) + c * calibratedValue + d;`
            *   Return a `ProcessedDataPoint` containing `finalValue` and the timestamp.
        *   (Future: can be extended to calculate secondary instrument quantities).
*   **Thread:** The `DataSynchronizer` object runs entirely in a dedicated independent `QThread` child thread.

#### Storage Module (Data Storage):

*   **Responsibilities:** Receive synchronized data frames and write them asynchronously to CSV files.
*   **Main Classes:**
    *   `StorageManager` (`QObject`):
        *   **Slot `onSynchronizedFrameReady(SynchronizedDataFrame frame)`:** (Connection type `Qt::QueuedConnection`) Receives the data frame, puts it into an internal thread-safe queue (`QQueue<SynchronizedDataFrame>`). Triggers or notifies the writing logic.
        *   **Internal Writing Logic:** (Possibly driven by a separate function or `QTimer`, checks the queue) Takes data frames from the queue, calls `CsvStorageStrategy` to write.
        *   Manages file handles, filename generation (possibly splitting by date or size).
    *   `AbstractStorageStrategy` (Interface): `virtual bool open(QString path) = 0;`, `virtual bool write(const SynchronizedDataFrame& frame) = 0;`, `virtual bool close() = 0;`
    *   `CsvStorageStrategy` (Concrete strategy): Implements reading and writing in CSV format. When writing, flattens the `SynchronizedDataFrame` into a CSV-friendly format (e.g., timestamp as the first column, followed by the value of each channel).
*   **Thread:** The `StorageManager` object runs entirely in a dedicated independent `QThread` child thread to prevent file I/O from blocking other parts.

#### UI Module (User Interface):

*   **Responsibilities:** Display real-time data, device status, provide user control interfaces (start/stop, configuration).
*   **Main Classes:**
    *   `MainWindow` (`QWidget` or `QMainWindow`):
        *   Holds instances (or references) of `ConfigManager`, `DeviceManager`.
        *   Creates `DataSynchronizer` and `StorageManager` and moves them to their dedicated threads.
        *   Connects `DataSynchronizer`'s `synchronizedFrameReady` signal to its own slot function for updating the UI (charts, tables).
        *   Connects `DeviceManager`'s `deviceStatusChanged` signal to update the device status display.
        *   Provides buttons to call `DeviceManager`'s `startAllDevices()` and `stopAllDevices()`.
        *   (Future) Provides an interface to modify configuration and notify `ConfigManager` to save.
    *   `PlotWidget`, `TableViewWidget`, etc.: Custom or standard Qt controls for data display.
*   **Thread:** Main thread. All UI updates must be done in the main thread.

### Thread Planning Summary:

*   **Main Thread:**
    *   Runs the `QApplication` event loop.
    *   Runs `MainWindow` and all UI controls.
    *   Runs `ConfigManager` (mainly used during initialization).
    *   Runs `DeviceManager` (responsible for creating devices and threads, managing lifecycle).
*   **Device Threads:**
    *   One thread per `AbstractDevice` instance.
    *   Responsible for blocking or time-consuming communication with specific hardware.
    *   Number = Number of active devices.
*   **Data Sync/Processing Thread:**
    *   Runs the `DataSynchronizer` object.
    *   Responsible for receiving raw data, performing snapshot synchronization, and calibration calculations.
    *   1 dedicated thread.
*   **Storage Thread:**
    *   Runs the `StorageManager` object.
    *   Responsible for writing processed data to files.
    *   1 dedicated thread.

### Data Flow:

1.  `ConfigManager` (Main Thread) reads `config.json`.
2.  `MainWindow` (Main Thread) gets configuration, initializes `DeviceManager` and `DataSynchronizer`.
3.  `DeviceManager` (Main Thread) creates `Device` objects, moves them to their respective device threads, and starts them.
4.  `Device` (Device Thread) collects data -> `applyFilter` -> emits `rawDataPointReady` signal.
5.  `DataSynchronizer`'s (Processing Thread) `onRawDataPointReady` slot (via `Qt::QueuedConnection`) receives the signal, updates the internal cache.
6.  `DataSynchronizer`'s (Processing Thread) `QTimer` triggers the `takeSnapshot` slot.
7.  `takeSnapshot` (Processing Thread) reads the cache, calls `Channel::process` for calculations, generates `SynchronizedDataFrame`.
8.  `DataSynchronizer` (Processing Thread) emits `synchronizedFrameReady` signal.
9.  `MainWindow`'s (Main Thread) slot (via `Qt::QueuedConnection`) receives `synchronizedFrameReady` signal, updates UI.
10. `StorageManager`'s (Storage Thread) slot (via `Qt::QueuedConnection`) receives `synchronizedFrameReady` signal, puts the data frame into the queue, asynchronously writes to a CSV file.

### Key Implementation Notes:

*   **Thread Safety:** Access to `DataSynchronizer`'s `latestRawData_` cache must be protected using `QMutex`. `StorageManager`'s internal queue needs to be thread-safe (`QQueue` with `QMutex` or use QtConcurrent queues).
*   **Signal/Slot Connection Types:** Cross-thread signal/slot connections must use `Qt::QueuedConnection` (usually automatic, but explicit specification is safer) to ensure the slot function executes in the receiver object's thread.
*   **Object Lifetime:** Ensure proper management of `QThread` lifecycles. Typically, stop and wait for all child threads to finish in the `MainWindow` destructor. Ensure objects moved to child threads are correctly deleted before the thread finishes, or destroyed along with the thread (if the parent object is the `QThread` itself, though not recommended).
*   **Error Handling:** Implement robust error handling and logging in various modules (especially `Device`, `Config`, `Storage`). Pass error information to the UI for display via signals.
*   **Secondary Instrument Quantities:** The current architecture reserves space for adding the calculation of secondary instrument quantities in `DataSynchronizer` or `Channel` in the future. Calculation logic can be added after generating the `SynchronizedDataFrame` in `takeSnapshot`.

## Conceptual Folder Structure
```
EngineDAQ/
├── EngineDAQ.pro                 # qmake project file (or CMakeLists.txt)
├── main.cpp                      # Application entry point
│
├── Core/                         # Core Module: Basic data structures and constants
│   ├── DataTypes.h               # Define DeviceConfig, ChannelConfig, RawDataPoint, ProcessedDataPoint, SynchronizedDataFrame, etc.
│   └── Constants.h               # Define enums (DeviceType, StatusCode), constants, etc.
│
├── Config/                       # Configuration Module
│   └── ConfigManager.h           # ConfigManager header file
│   └── ConfigManager.cpp         # ConfigManager implementation file
│
├── Device/                       # Device Communication Module
│   ├── DeviceManager.h           # DeviceManager header file
│   ├── DeviceManager.cpp         # DeviceManager implementation file
│   ├── AbstractDevice.h          # AbstractDevice interface header file
│   ├── AbstractDevice.cpp        # AbstractDevice interface implementation file (may be empty or contain common logic)
│   ├── DeviceFactory.h           # (Optional) DeviceFactory header file (for creating concrete devices)
│   ├── DeviceFactory.cpp         # (Optional) DeviceFactory implementation file
│   │
│   └── ConcreteDevices/          # (Optional subdirectory) or directly under Device/
│       ├── VirtualDevice.h       # VirtualDevice header file
│       ├── VirtualDevice.cpp     # VirtualDevice implementation file
│       ├── ModbusDevice.h        # (Future) ModbusDevice header file
│       ├── ModbusDevice.cpp      # (Future) ModbusDevice implementation file
│       ├── DAQDevice.h           # (Future) DAQDevice header file
│       ├── DAQDevice.cpp         # (Future) DAQDevice implementation file
│       └── ECUDevice.h           # (Future) ECUDevice header file
│       └── ECUDevice.cpp         # (Future) ECUDevice implementation file
│
├── Processing/                   # Data Synchronization & Processing Module
│   ├── DataSynchronizer.h        # DataSynchronizer header file
│   ├── DataSynchronizer.cpp      # DataSynchronizer implementation file
│   └── Channel.h                 # Channel processing class header file (might not need .cpp)
│   └── Channel.cpp               # Channel processing class implementation file (if logic is complex)
│
├── Storage/                      # Data Storage Module
│   ├── StorageManager.h          # StorageManager header file
│   ├── StorageManager.cpp        # StorageManager implementation file
│   ├── AbstractStorageStrategy.h # AbstractStorageStrategy interface header file
│   │
│   └── Strategies/               # (Optional subdirectory) or directly under Storage/
│       └── CsvStorageStrategy.h  # CsvStorageStrategy header file
│       └── CsvStorageStrategy.cpp# CsvStorageStrategy implementation file
│       # (Future: can add BinaryStorageStrategy.h/.cpp etc.)
│
├── UI/                           # User Interface Module
│   ├── MainWindow.h              # MainWindow header file
│   ├── MainWindow.cpp            # MainWindow implementation file
│   ├── MainWindow.ui             # MainWindow UI design file (if using Qt Designer)
│   │
│   └── Widgets/                  # (Optional subdirectory) Custom UI widgets
│       ├── PlotWidget.h          # PlotWidget header file
│       ├── PlotWidget.cpp        # PlotWidget implementation file
│       └── TableViewWidget.h     # TableViewWidget header file
│       └── TableViewWidget.cpp   # TableViewWidget implementation file
│
└── Resources/                    # Resource files
    └── config.json               # Default or example configuration file
    └── icons/                    # Application icons, etc.
        └── app_icon.png
```

## Configuration File
`config.json` content:
```json
{
  "modbus_devices": [
    {
      "instance_name": "SerialPort1_Modbus",
      "serial_config": { "port": "/dev/ttyS0", "baudrate": 9600, "databits": 8, "stopbits": 1, "parity": "N" },
      "read_cycle_ms": 1000,
      "slaves": [
        {
          "slave_id": 1,
          "operation_command": 3,
          "registers": [
            {
              "register_address": 40001,
              "channel_name": "Temperature_Sensor_1",
              "channel_params": { "gain": 1.0, "offset": 0.0, "calibration_params": { "a": 0.0, "b": 0.0, "c": 1.0, "d": 0.0 } }
            },
            {
              "register_address": 40002,
              "channel_name": "Pressure_Sensor_1",
              "channel_params": { "gain": 0.1, "offset": 5.0, "calibration_params": { "a": 0.001, "b": 0.05, "c": 0.95, "d": 0.2 } }
            }
          ]
        },
        {
          "slave_id": 2,
          "operation_command": 3,
          "registers": [
            {
              "register_address": 40101,
              "channel_name": "Flow_Rate_1",
              "channel_params": { "gain": 1.0, "offset": 0.0, "calibration_params": { "a": 0.0, "b": 0.0, "c": 1.0, "d": 0.0 } }
            }
          ]
        }
      ]
    },
    {
      "instance_name": "SerialPort2_Modbus",
      "serial_config": { "port": "/dev/ttyS1", "baudrate": 19200, "databits": 8, "stopbits": 1, "parity": "E" },
      "read_cycle_ms": 500,
      "slaves": [
        {
          "slave_id": 10,
          "operation_command": 4,
          "registers": [
            {
              "register_address": 30001,
              "channel_name": "Valve_Status_1",
              "channel_params": { "gain": 1.0, "offset": 0.0, "calibration_params": { "a": 0.0, "b": 0.0, "c": 1.0, "d": 0.0 } }
            }
          ]
        }
      ]
    }
  ],
  "daq_devices": [
    {
      "device_id": "dev1",
      "sample_rate": 10000,
      "channels": [
        {
          "channel_id": 1,
          "channel_name": "Vibration_X",
          "channel_params": { "gain": 1.0, "offset": 0.0, "calibration_params": { "a": 0.0, "b": 0.0, "c": 1.0, "d": 0.0 } }
        },
        {
          "channel_id": 2,
          "channel_name": "Vibration_Y",
          "channel_params": { "gain": 1.0, "offset": 0.0, "calibration_params": { "a": 0.0, "b": 0.0, "c": 1.0, "d": 0.0 } }
        },
        {
          "channel_id": 5,
          "channel_name": "Strain_Gauge_1",
          "channel_params": { "gain": 0.005, "offset": -10.0, "calibration_params": { "a": 0.0, "b": 0.002, "c": 0.98, "d": 0.5 } }
        },
        {
          "channel_id": 6,
          "channel_name": "Strain_Gauge_2",
          "channel_params": { "gain": 0.005, "offset": -10.0, "calibration_params": { "a": 0.0, "b": 0.002, "c": 0.98, "d": 0.5 } }
        },
        {
          "channel_id": 7,
          "channel_name": "Temperature_TC1",
          "channel_params": { "gain": 1.0, "offset": 0.0, "calibration_params": { "a": 0.0, "b": 0.0, "c": 1.0, "d": 0.0 } }
        }
      ]
    },
    {
      "device_id": "dev2",
      "sample_rate": 5000,
      "channels": [
        {
          "channel_id": 0,
          "channel_name": "Voltage_Input_1",
          "channel_params": { "gain": 2.0, "offset": 0.1, "calibration_params": { "a": 0.0, "b": 0.0, "c": 1.0, "d": 0.0 } }
        }
      ]
    }
  ],
  "ecu_devices": [
    {
      "instance_name": "Engine_ECU",
      "serial_config": { "port": "/dev/ttyUSB0", "baudrate": 115200, "databits": 8, "stopbits": 1, "parity": "N" },
      "read_cycle_ms": 100,
      "channels": {
        "speed": { "channel_params": { "gain": 1.0, "offset": 0.0, "calibration_params": { "a": 0.0, "b": 0.0, "c": 1.0, "d": 0.0 } } },
        "throttle_position": { "channel_params": { "gain": 1.0, "offset": 0.0, "calibration_params": { "a": 0.0, "b": 0.0, "c": 1.0, "d": 0.0 } } },
        "cylinder_temp": { "channel_params": { "gain": 0.5, "offset": -10.0, "calibration_params": { "a": 0.0, "b": 0.01, "c": 0.9, "d": 2.0 } } },
        "exhaust_temp": { "channel_params": { "gain": 0.5, "offset": -10.0, "calibration_params": { "a": 0.0, "b": 0.01, "c": 0.9, "d": 2.0 } } },
        "fuel_pressure": { "channel_params": { "gain": 1.0, "offset": 0.0, "calibration_params": { "a": 0.0, "b": 0.0, "c": 1.0, "d": 0.0 } } },
        "rotor_temp": { "channel_params": { "gain": 1.0, "offset": 0.0, "calibration_params": { "a": 0.0, "b": 0.0, "c": 1.0, "d": 0.0 } } },
        "intake_temp": { "channel_params": { "gain": 1.0, "offset": -273.15, "calibration_params": { "a": 0.0, "b": 0.0, "c": 1.0, "d": 0.0 } } },
        "intake_pressure": { "channel_params": { "gain": 1.0, "offset": 0.0, "calibration_params": { "a": 0.0, "b": 0.0, "c": 1.0, "d": 0.0 } } },
        "supply_voltage": { "channel_params": { "gain": 1.0, "offset": 0.0, "calibration_params": { "a": 0.0, "b": 0.0, "c": 1.0, "d": 0.0 } } }
      }
    }
  ],
  "virtual_devices": [ // Added virtual device type
    {
      "instance_name": "Sine_Wave_Generator",
      "signal_type": "sine", // Optional: "sine", "square", "triangle", "random"
      "amplitude": 5.0,
      "frequency": 10.0, // Hz (might not be needed for random type)
      "channel_params": { "gain": 1.0, "offset": 0.0, "calibration_params": { "a": 0.0, "b": 0.0, "c": 1.0, "d": 0.0 } }
    },
    {
      "instance_name": "Random_Noise_Source",
      "signal_type": "random",
      "amplitude": 1.0, // For random numbers, this might represent range or standard deviation
      "frequency": 0.0, // Frequency is meaningless for random numbers, can be set to 0 or omitted
      "channel_params": { "gain": 2.0, "offset": 0.5, "calibration_params": { "a": 0.0, "b": 0.0, "c": 1.0, "d": 0.0 } }
    },
    {
      "instance_name": "Square_Wave_Test",
      "signal_type": "square",
      "amplitude": 2.5,
      "frequency": 5.0,
      "channel_params": { "gain": 1.0, "offset": 0.0, "calibration_params": { "a": 0.0, "b": 0.0, "c": 1.0, "d": 0.0 } }
    }
    // ... can add more virtual device instances
  ]
}
```