# 已完成的任务

## 九、添加DAQ设备支持
- 添加了DAQDevice类，实现了AbstractDevice接口
- 更新了CMakeLists.txt，添加了Art_DAQ库的链接和DLL复制
- 更新了ConfigManager，实现了DAQ设备配置的解析
- 更新了DeviceManager，添加了创建和管理DAQ设备的功能
- 实现了DAQ设备的初始化、连接、数据采集和处理功能
- 实现了滤波器功能，支持FIR低通滤波
- 更新了config.json，添加了DAQ设备配置
- 优化了DAQ设备实现：
  - 改进了资源管理，确保任务正确停止和清理
  - 增强了线程安全性，避免主线程阻塞
  - 优化了缓冲区管理，防止数据积压
  - 增强了错误处理和异常捕获
  - 改进了回调函数实现，确保内存正确释放

## 八、添加硬件测试样例
- 添加了ModbusDevice类，实现了AbstractDevice接口
- 更新了CMakeLists.txt，添加了Qt SerialPort和SerialBus模块
- 更新了ConfigManager，实现了Modbus设备配置的解析
- 更新了DeviceManager，添加了创建和管理Modbus设备的功能
- 更新了config.json，将串口名称从Linux风格改为Windows风格

## 一、项目设置和核心数据结构
- 创建了项目文件夹结构（Core, Config, Device, Processing, Storage）
- 实现了Core模块：
  - 在Constants.h中定义了枚举（DeviceType, StatusCode）
  - 在DataTypes.h中定义了所有结构体（DeviceConfig, ChannelConfig, RawDataPoint, ProcessedDataPoint, SynchronizedDataFrame等）
- 更新了CMakeLists.txt以包含新文件

## 二、配置加载
- 实现了Config模块：
  - 创建了ConfigManager类，用于读取和解析config.json文件
  - 实现了loadConfig()方法解析配置文件（目前主要关注虚拟设备部分）
  - 实现了getter方法获取设备配置和同步间隔
- 更新了main.cpp以测试配置加载功能
- 更新了CMakeLists.txt以包含新文件

## 三、基本设备框架和虚拟设备
- 实现了Device模块：
  - 创建了AbstractDevice接口，定义了所有设备的通用接口
  - 实现了VirtualDevice类，用于生成测试数据
  - 创建了DeviceManager类，用于管理所有设备实例
- 实现了设备线程管理：
  - 每个设备在独立的线程中运行
  - 使用信号和槽机制在线程间通信
- 更新了mainwindow.cpp以使用DeviceManager
- 实现了原始数据生成和处理
- 更新了CMakeLists.txt以包含新文件

## 四、数据同步和处理（基础）
- 实现了Processing模块：
  - 创建了Channel类，用于处理和存储单个通道的数据
  - 创建了DataProcessor类，用于统一管理数据同步和通道处理
- 实现了数据处理功能：
  - 增益和偏移应用
  - 校准多项式应用（立方校准）
- 实现了数据同步功能：
  - 定时创建同步数据帧
  - 将所有通道的最新数据组合到一个帧中
- 实现了线程管理：
  - 将DataProcessor移动到独立线程中运行
  - 使用Qt::QueuedConnection确保线程安全
  - 添加线程ID输出以便调试
- 更新了mainwindow.cpp以使用DataProcessor
- 实现了完整的数据流：从设备采集 -> 数据同步 -> 通道处理 -> 数据显示
- 更新了CMakeLists.txt以包含新文件

## 五、实时数据可视化
- 添加了QCustomPlot库到项目中：
  - 更新了CMakeLists.txt以包含QCustomPlot源文件
  - 添加了Qt PrintSupport模块以支持QCustomPlot的PDF导出功能
- 实现了UI控制功能：
  - 添加了开始/停止采集按钮
  - 实现了采集状态控制逻辑
- 实现了实时数据绘图功能：
  - 为每个通道创建一条曲线
  - 实现了数据点添加和图表更新逻辑
  - 添加了自动缩放和时间轴滚动功能
- 优化了UI布局：
  - 使用QVBoxLayout和QHBoxLayout组织控件
  - 设置了合适的窗口大小和标题
- 优化了数据处理和显示性能：
  - 在DataProcessor中使用队列存储有限长度的数据（最多1000点）
  - 使用QCustomPlot的addData方法而不是setData
  - 使用removeBefore方法限制显示的数据点数量
  - 启用了QCustomPlot的性能优化选项（自适应采样、禁用抗锯齿等）
  - 使用QCustomPlot::rpQueuedReplot提高重绘性能
  - 优化了UI更新频率（50ms）
- 实现了完整的数据流：从设备采集 -> 通道处理 -> 数据同步 -> 实时显示