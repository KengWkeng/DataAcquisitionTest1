Guiding Principles:

Iterative: Build functionality in small, manageable chunks.
Testable: Each Step should result in code that can be tested, even if only partially.
Incremental Complexity: Start with the simplest end-to-end flow and add features/hardware progressively.
Virtual First: Ensure the entire architecture (data flow, threading, processing, storage, UI) works correctly using the VirtualDevice before tackling hardware specifics.
Iterative Development Steps:

# Phase 1: Core Framework and Virtual Device Implementation

 ## Step 1: Project Setup and Core Data Structures

Goal: Establish the basic project structure and define fundamental data types.
Tasks:
Set up the Qt C++ project (using qmake or CMake).
Create the conceptual folder structure (Core, Config, Device, Processing, Storage, UI).
Implement the Core module: Define all structs (DeviceConfig, ChannelConfig, RawDataPoint, ProcessedDataPoint, SynchronizedDataFrame) and enums (DeviceType, StatusCode) in DataTypes.h and Constants.h.
Testing: Compile the project. No runtime tests yet, but ensure headers are correct and includable.
Runnable: No.
 ## Step 2: Configuration Loading

Goal: Read and parse the configuration file.
Tasks:
Implement ConfigManager in the Config module.
Implement loadConfig() to parse config.json (initially focus only on the virtual_devices section and maybe the synchronization interval). Use QJsonDocument.
Implement getter methods like getDeviceConfigs() (returning only virtual devices for now) and getSynchronizationIntervalMs().
Testing: Write a small test in main.cpp or a separate test function to load config.json, create a ConfigManager, call loadConfig(), and print the retrieved virtual device configurations and interval to verify parsing.
Runnable: Yes (basic console application for testing config loading).
 ## Step 3: Basic Device Framework and Virtual Device

Goal: Create the device abstraction, manager, and a functional virtual device running in its own thread.
Tasks:
Implement AbstractDevice interface (QObject-based) in the Device module with pure virtual functions and the rawDataPointReady, deviceStatusChanged signals. Include the empty applyFilter method.
Implement DeviceManager (QObject-based) in the Device module:
Method to take QList<DeviceConfig> (from ConfigManager).
Basic factory logic (hardcoded for VIRTUAL type for now) to create VirtualDevice instances.
Logic to create a QThread for each device, move the device object to the thread using moveToThread().
Basic startAllDevices() and stopAllDevices() methods that start/stop the threads and call device-specific start/stop methods (via signals/slots or QMetaObject::invokeMethod).
Implement VirtualDevice inheriting AbstractDevice:
Constructor takes DeviceConfig.
Implement connectDevice, disconnectDevice, startAcquisition, stopAcquisition.
In startAcquisition, start a QTimer within the device's thread.
The timer's timeout slot should generate a sample value (e.g., based on signal_type from config), get the current timestamp, and emit rawDataPointReady.
Testing: In main.cpp, create ConfigManager, load config, create DeviceManager, pass configs, call startAllDevices(). Connect rawDataPointReady from DeviceManager (or directly from devices if needed for testing) to a simple lambda function that prints the received raw data to the console. Verify that data is being generated periodically.
Runnable: Yes (console application showing raw data from virtual device).
 ## Step 4: Data Synchronization and Processing (Basic)

Goal: Implement the core data synchronization and processing logic for the virtual device data.
Tasks:
Implement the Channel helper class (in Processing) to store ChannelConfig and the process() method implementing gain, offset, and cubic calibration.
Implement DataSynchronizer (QObject-based) in the Processing module:
Initialize Channel objects based on ChannelConfig from ConfigManager.
Implement the latestRawData_ cache (QMap<QPair<QString, int>, RawDataPoint>) with QMutex protection.
Implement the public slot onRawDataPointReady (ensure Qt::QueuedConnection when connecting across threads) to update the cache.
Set up the snapshot QTimer using the interval from ConfigManager.
Implement the takeSnapshot slot: lock mutex, get timestamp, iterate configured channels, retrieve latest raw data from cache, call Channel::process(), create ProcessedDataPoint, build SynchronizedDataFrame, unlock mutex, emit synchronizedFrameReady(SynchronizedDataFrame).
In main.cpp (or MainWindow later): Create DataSynchronizer, create its dedicated QThread, move DataSynchronizer to the thread, and start the thread.
Connect AbstractDevice::rawDataPointReady signals (likely via DeviceManager for simplicity) to DataSynchronizer::onRawDataPointReady.
Testing: Connect DataSynchronizer::synchronizedFrameReady to a lambda that prints the content of the synchronized frame (timestamp and processed values) to the console. Verify that processed data frames are generated at the expected interval.
Runnable: Yes (console application showing processed data frames).
 ## Step 5: Basic UI Display

Goal: Create a minimal UI to display the processed data from the virtual device.
Tasks:
Create MainWindow (QMainWindow or QWidget) in the UI module.
Add basic widgets (e.g., QLabel or QTextEdit) to display the timestamp and a few channel values.
In MainWindow's constructor or an initialization function:
Instantiate ConfigManager, DeviceManager, DataSynchronizer (and its thread).
Perform the necessary connections (Device -> Synchronizer).
Connect DataSynchronizer::synchronizedFrameReady to a slot in MainWindow.
The MainWindow slot should update the UI widgets with data from the received SynchronizedDataFrame.
Modify main.cpp to create and show MainWindow instead of just running console logic.
Testing: Run the application. Verify that the UI updates periodically with processed data from the virtual device.
Runnable: Yes (GUI application showing live data from virtual device).
 ## Step 6: Basic Data Storage (CSV)

Goal: Implement saving the processed data frames to a CSV file.
Tasks:
Implement AbstractStorageStrategy interface in Storage.
Implement CsvStorageStrategy inheriting from the abstract strategy: Implement open, write (formatting SynchronizedDataFrame to a CSV line), and close.
Implement StorageManager (QObject-based) in Storage:
Internal thread-safe queue (QQueue + QMutex) for SynchronizedDataFrame.
Slot onSynchronizedFrameReady (ensure Qt::QueuedConnection) to add frames to the queue.
Internal logic (e.g., triggered by a timer or signal after queueing) to dequeue frames and call CsvStorageStrategy::write. Handle file opening/closing.
In MainWindow: Create StorageManager, its dedicated QThread, move it to the thread, start it.
Connect DataSynchronizer::synchronizedFrameReady to StorageManager::onSynchronizedFrameReady.
Testing: Run the application. Check if a CSV file is created and populated with data rows corresponding to the processed frames. Verify the format.
Runnable: Yes (GUI application displaying data and saving it to CSV).
 ## Step 7: Integration and Control

Goal: Integrate basic controls and status display.
Tasks:
Add Start/Stop buttons to MainWindow. Connect their clicked signals to slots that call DeviceManager::startAllDevices() and DeviceManager::stopAllDevices().
Connect DeviceManager::deviceStatusChanged signal to a slot in MainWindow to display basic device status updates (e.g., in a QStatusBar or QTextEdit). Implement basic status reporting in VirtualDevice.
Testing: Start and stop the acquisition using the UI buttons. Verify that data display and saving start/stop accordingly. Check status updates.
Runnable: Yes (GUI application with basic control, display, and saving for the virtual device).
# Phase 2: Adding Hardware Device Support (Iteratively)

 ## Step 8: Add Modbus Device Support

Goal: Integrate Modbus device reading into the existing framework.
Tasks:
Update ConfigManager to parse the modbus_devices section of config.json.
Implement ModbusDevice class inheriting AbstractDevice.
Use QtSerialBus module (QModbusTcpClient or QModbusRtuSerialMaster) or a third-party library.
Implement connection logic (connectDevice).
Implement data reading logic (startAcquisition likely sets up a timer or reads cyclically) based on config (slave ID, register address, command).
Emit rawDataPointReady for each register read. Handle potential Modbus errors and emit deviceStatusChanged or errorOccurred.
Update DeviceManager's factory logic to recognize DeviceType::MODBUS and create ModbusDevice instances.
Update Channel or DataSynchronizer's channel mapping logic to correctly associate Modbus hardware channels (device instance + slave + register) with software channels defined in the config. (This mapping needs careful definition, perhaps using unique channel names from the config).
Testing: Update config.json to include a Modbus device. Use a Modbus simulator (like Modbus Slave or a simple Python script) or real hardware. Run the application and verify:
Modbus device connects.
Data is read correctly and appears in the UI and CSV file alongside virtual device data (if still configured).
Status updates work.
Runnable: Yes (GUI application supporting Virtual and Modbus devices).
 ## Step 9: Add DAQ Device Support

Goal: Integrate DAQ device reading.
Tasks:
Update ConfigManager to parse daq_devices.
Implement DAQDevice class inheriting AbstractDevice.
This will likely involve using a vendor-specific SDK or a library like NI-DAQmx, interfacing via C APIs or potentially a C++ wrapper.
Implement device initialization, channel configuration (sample rate, ranges), and starting the acquisition task based on the SDK.
The SDK will likely provide callbacks or polling methods to get data buffers. Process these buffers, extract data for each configured channel, get timestamps (or assign based on sample rate/buffer time), and emit rawDataPointReady for each data point.
Update DeviceManager factory logic for DeviceType::DAQ.
Update channel mapping logic in DataSynchronizer for DAQ channels (device ID + hardware channel ID).
Testing: Update config.json with DAQ config. Use real DAQ hardware or a simulator if available. Verify:
DAQ device initializes and starts acquisition.
Data from DAQ channels appears correctly in UI/CSV.
Handles high data rates appropriately (ensure processing/UI doesn't lag).
Runnable: Yes (GUI application supporting Virtual, Modbus, and DAQ devices).
 ## Step 10: Add ECU Device Support

Goal: Integrate ECU data reading.
Tasks:
Update ConfigManager to parse ecu_devices.
Implement ECUDevice class inheriting AbstractDevice.
Likely involves serial communication (QSerialPort).
Implement the specific ECU communication protocol (e.g., request specific PIDs/data points, parse responses). This might be complex depending on the protocol.
Map received ECU parameters (like "speed", "throttle_position") to hardware channel identifiers.
Emit rawDataPointReady for each received parameter.
Update DeviceManager factory logic for DeviceType::ECU.
Update channel mapping logic in DataSynchronizer.
Testing: Update config.json. Use a real ECU, a simulator (like an OBD-II simulator connected via serial/USB adapter), or test data playback. Verify:
ECU connection and communication work.
Relevant ECU parameters are displayed and saved.
Runnable: Yes (GUI application supporting all specified device types).
# Phase 3: Enhancements and Refinements

 ## Step 11: Implement Secondary Instrument Calculation

Goal: Calculate derived values from primary measurements.
Tasks:
Define how secondary channels are specified (e.g., in config.json with formulas referencing primary channel names/IDs).
Update ConfigManager to parse this.
Extend DataSynchronizer or add a new processing stage after takeSnapshot. This stage takes the SynchronizedDataFrame (containing all primary values for a timestamp), calculates secondary values based on the formulas, and adds them to the frame (or creates a new frame type).
Update StorageManager and MainWindow to handle/display these secondary values.
Testing: Define test secondary channels in config. Verify calculations are correct in UI and CSV.
 ## Step 12: Implement Filtering

Goal: Add data filtering capabilities.
Tasks:
Decide where filtering occurs (in AbstractDevice::applyFilter before emitting raw data, or in DataSynchronizer after synchronization). applyFilter is simpler initially.
Add filter configuration options to config.json (e.g., filter type like 'moving_average', parameters like 'window_size').
Update ConfigManager to parse filter config.
Implement filter algorithms (e.g., simple moving average) and apply them in the chosen location based on config.
Testing: Configure filters for specific channels. Verify filtered data looks smoother/correct in UI/CSV compared to unfiltered data.
 ## Step 13: UI Enhancements

Goal: Improve the user interface.
Tasks:
Replace basic labels/text edits with tables (QTableView with a custom model) for better data organization.
Integrate plotting widgets (e.g., QCustomPlot, Qt Charts) to visualize channel data over time. Update plots in the MainWindow slot receiving synchronizedFrameReady.
Add more detailed status indicators per device.
(Optional) Add UI for editing configuration.
 ## Step 14: Robustness, Testing, and Documentation

Goal: Improve application stability and maintainability.
Tasks:
Add more comprehensive error handling (e.g., for file I/O, device communication errors, configuration errors). Report errors clearly in the UI.
Implement unit tests for critical modules (e.g., ConfigManager parsing, Channel processing logic, CsvStorageStrategy formatting).
Refine logging throughout the application.
Add code comments and update/finalize the development documentation.
This iterative plan allows for steady progress, frequent testing, and the ability to demonstrate working software at almost every step, starting with the core data flow using the virtual device.