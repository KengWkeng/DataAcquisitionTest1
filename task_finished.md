# 已完成的任务

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
  - 创建了DataSynchronizer类，用于同步和处理来自不同设备的数据
  - 创建了DataProcessor类，统一管理数据同步和通道计算
- 实现了数据处理功能：
  - 增益和偏移应用
  - 校准多项式应用（立方校准）
- 实现了数据同步功能：
  - 定时创建同步数据帧
  - 将所有通道的最新数据组合到一个帧中
- 实现了线程管理：
  - 将DataProcessor移至独立线程
  - 使用Qt::QueuedConnection确保线程安全
  - 添加线程ID调试输出
- 更新了mainwindow.cpp以使用DataProcessor
- 实现了完整的数据流：从设备采集 -> 通道处理 -> 数据同步
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
- 实现了完整的数据流：从设备采集 -> 通道处理 -> 数据同步 -> 实时显示