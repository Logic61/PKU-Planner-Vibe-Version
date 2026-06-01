# PKU Planner (Course Helper) 技术架构文档

> 生成日期：2026-05-11
> 技术栈：Qt6 + C++20
> 项目类型：桌面应用程序（Windows）

---

## 一、系统整体架构

### 1.1 架构分层图（文本描述）

```
┌─────────────────────────────────────────────────────────────┐
│                     Presentation Layer                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ MainWindow   │  │  Pages       │  │  Widgets     │      │
│  │ (Router)     │  │  Dashboard   │  │  SearchPopup │      │
│  │              │  │  Todo        │  │  CourseDrawer │      │
│  │              │  │  Stats       │  │  MascotWidget │      │
│  │              │  │  Settings    │  │  ToastWidget  │      │
│  └──────┬───────┘  └──────────────┘  └──────────────┘      │
│         │                    │                   │          │
│  ┌──────┴────────────────────┴───────────────────┴──────┐  │
│  │                    Components                          │  │
│  │  CourseCellWidget  TaskCardWidget  DDLPreviewWidget   │  │
│  │  BaseCardWidget   EmptyStateWidget                     │  │
│  └─────────────────────────┬─────────────────────────────┘  │
│                            │                                 │
│  ┌─────────────────────────┴─────────────────────────────┐  │
│  │                   UI Layer                             │  │
│  │  SidebarWidget  TopbarWidget  Theme (namespace)        │  │
│  └─────────────────────────┬─────────────────────────────┘  │
├────────────────────────────┼────────────────────────────────┤
│                     Service Layer                            │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌────────────┐  │
│  │DataService│ │Config    │  │Search    │  │Reminder    │  │
│  │(static)  │  │Service   │  │Service   │  │Service     │  │
│  │          │  │(singleton)│ │(static)  │  │(QObject)   │  │
│  └──────────┘  └──────────┘  └──────────┘  └────────────┘  │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────┐           │
│  │WeeklySum-│  │Mascot    │  │Export            │           │
│  │maryService│ │StateService│ │Service           │           │
│  │(static)  │  │(singleton)│  │(static)          │           │
│  └──────────┘  └──────────┘  └──────────────────┘           │
├────────────────────────────┼────────────────────────────────┤
│                     Model Layer                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │               DataManager (Singleton)                  │  │
│  │         ─ Signal: coursesChanged / tasksChanged       │  │
│  └──────────────────────────┬───────────────────────────┘  │
│         ┌──────────────────┼───────────────────┐          │
│  ┌──────┴──────┐    ┌──────┴──────┐                   │    │
│  │ DataStore   │    │DataRepository│                  │    │
│  │(in-memory)  │    │(file I/O)    │                  │    │
│  └─────────────┘    └──────────────┘                   │    │
│         │                   │                          │    │
│  ┌──────┴───────────────────┴──────┐                   │    │
│  │  POD Structs: Course / Task      │                   │    │
│  │  TaskModel (QAbstractTableModel) │                   │    │
│  └──────────────────────────────────┘                   │    │
├─────────────────────────────────────────────────────────────┤
│                     Platform / Qt6 Framework                  │
└─────────────────────────────────────────────────────────────┘
```

### 1.2 目录树逻辑分层

```
Course-Helper/
├── main.cpp                          # 入口点，ensureDataFiles()
├── mainwindow.h/cpp                  # 根窗口（路由中枢）
├── CMakeLists.txt                    # 构建配置
├── models/                           # 数据模型层
│   ├── course.h                      # Course POD + JSON序列化
│   ├── task.h                        # Task POD + JSON序列化
│   ├── taskmodel.h/cpp               # QAbstractTableModel（任务过滤视图）
│   ├── datastore.h/cpp               # 内存缓存（QList<Course/Task>）
│   ├── datamanager.h/cpp             # 单例中央控制器（Store+Repository）
│   ├── datarepository.h/cpp          # 文件I/O层（JSON磁盘读写）
│   └── mascotstate.h                 # 吉祥物状态枚举
├── services/                         # 服务层（业务逻辑）
│   ├── dataservice.h/cpp             # 静态工具（JSON加载/保存/错误处理）
│   ├── configservice.h/cpp           # 单例持久化配置（QSettings后端）
│   ├── searchservice.h/cpp           # 静态搜索（模糊匹配Course/Task/File）
│   ├── reminderservice.h/cpp         # QTimer轮询截止日期提醒
│   ├── weeklysummaryservice.h/cpp     # 静态统计摘要生成
│   ├── mascotstateservice.h/cpp      # 单例紧迫度→吉祥物状态映射
│   └── exportservice.h/cpp           # 静态导出（CSV/TXT）
├── pages/                            # 主页面（四大Tab内容区）
│   ├── dashboardpage.h/cpp           # 首页：课表网格 + DDL + 今日课程
│   ├── todopage.h/cpp                # 任务页：看板 + 多维过滤
│   ├── statspage.h/cpp               # 统计页：热力图 + 趋势 + 建议
│   └── settingspage.h/cpp            # 设置页：提醒/导入导出/学期/关于
├── ui/                               # 布局组件
│   ├── sidebarwidget.h/cpp           # 左侧导航栏（吉祥物 + 4个导航按钮）
│   ├── topbarwidget.h/cpp            # 顶部搜索栏（含SearchPopup）
│   └── theme.h/cpp                   # 主题常量（颜色/CBS样式生成器）
├── components/                       # 可复用UI原子组件
│   ├── basecardwidget.h/cpp          # 可点击卡片基类
│   ├── coursecellwidget.h/cpp        # 课表网格单元格（支持hover/双击/右键）
│   ├── taskcardwidget.h/cpp          # 任务卡片（含复选框/优先级/倒计时）
│   ├── ddlpreviewwidget.h/cpp        # DDL悬浮预览组件
│   ├── emptystatewidget.h/cpp        # 空状态占位组件
│   └── toastwidget.h/cpp             # 通知弹出组件（渐变动画）
├── dialogs/                          # 模态对话框
│   ├── courseeditdialog.h/cpp        # 课程编辑/新建表单
│   ├── taskeditdialog.h/cpp          # 任务编辑/新建表单
│   ├── courseactiondialog.h/cpp      # 课程操作选择（编辑/删除/添加DDL）
│   └── confirmdialog.h/cpp           # 确认对话框工厂（静态方法）
├── widgets/                          # 复合页面级组件
│   ├── coursedetail/                 # 课程详情抽屉
│   │   ├── coursedetaildrawer.h/cpp  # 可拖拽宽度的侧滑抽屉（动画）
│   │   ├── courseheaderwidget.h/cpp  # 详情页顶栏（标题+操作按钮）
│   │   ├── coursetabbar.h/cpp        # 4标签切换（Info/Task/File/Stats）
│   │   ├── courseinfopage.h/cpp      # 基本信息Tab
│   │   ├── coursetaskpage.h/cpp      # 课程任务Tab
│   │   ├── coursefilepage.h/cpp      # 课程文件Tab（绑定文件夹/笔记）
│   │   └── coursestatswidget.h/cpp   # 课程统计Tab
│   ├── search/                       # 搜索组件
│   │   └── searchpopup.h/cpp         # 下拉搜索结果浮层
│   ├── mascot/                       # 吉祥物组件
│   │   └── mascotwidget.h/cpp        # 点击吉祥物弹出状态提示
│   ├── onboarding/                   # 首次引导
│   │   └── onboardingdialog.h/cpp     # 新手引导弹窗
│   └── dialogs/                      # 通用弹窗
│       └── weeklysummarydialog.h/cpp  # 周报摘要弹窗（可拖拽）
└── utils/                            # 工具函数
    └── pageanimator.h/cpp            # 页面滑动动画静态工具
```

---

## 二、核心数据流转路径

### 2.1 数据主路径

```
磁盘(JSON) ←→ DataRepository ←→ DataStore ←→ DataManager ←→ 全部UI层
                                        ↑
                              coursesChanged() signal
                              tasksChanged() signal
```

- **DataRepository**：负责从 `courses.json` / `tasks.json` 读写 JSON 文件
- **DataStore**：纯内存缓存，持有 `QList<Course>` 和 `QList<Task>`
- **DataManager**：单例门面，操作 DataStore 并通过 `DataRepository` 持久化。每次变更自动触发 signal

### 2.2 课程操作流程

```
用户点击格子 → CourseCellWidget emit信号 → DashboardPage::createCourse()
→ CourseEditDialog (exec) → 用户填表单 → accept
→ DataManager::addCourse() → DataStore add → DataRepository saveCourses()
→ emit coursesChanged() → DashboardPage 重新 renderCourses()
```

### 2.3 任务完成流程

```
用户勾选TaskCardWidget复选框 → emit completed()
→ TodoPage / CourseTaskPage 捕获 → DataManager::updateTask()
→ completedAt时间戳自动设置 → emit tasksChanged()
→ 所有监听页面刷新 → MascotStateService recalculateUrgency()
→ mascot状态更新
```

### 2.4 搜索流程

```
TopbarWidget 搜索文本变化 → debounce Timer → SearchService::search()
→ 静态方法遍历 courses / tasks / files → fuzzyMatch() 匹配
→ SearchPopup 显示结果 → 用户点击 → emit courseSelected/taskSelected
→ MainWindow 导航或高亮对应项
```

### 2.5 提醒流程

```
ReminderService::start() → m_timer 每分钟触发 checkUpcomingTasks()
→ 读取 DataManager::tasks() → 比较 deadline 与当前时间
→ 超配置阈值 → emit reminderTriggered() → ToastWidget 弹出
```

---

## 三、关键模块协作关系

### 3.1 MainWindow（路由中枢）

- 持有 `QStackedWidget`（4个页面槽）
- 持有 `CourseDetailDrawer`（课程详情侧滑面板）
- 持有 `MascotWidget`（吉祥物弹窗）
- 接收所有跨页面信号，统一调度导航

### 3.2 DataManager（中央数据总线）

- **唯一数据源**，所有UI均通过它读写数据
- 所有变更自动持久化到磁盘
- 提供 `coursesChanged` / `tasksChanged` 两个 signal 分发状态更新

### 3.3 CourseDetailDrawer（复合页面容器）

- 包含 4 个子页面（Info / Task / File / Stats）
- 可拖拽调整宽度（300-700px）
- 内部动画控制侧滑展开/收起
- 与 MainWindow 双向通信（编辑课程/添加任务/关闭）

### 3.4 TaskModel（过滤视图）

- 持有 `m_allTasks` 和 `m_visibleTasks` 两层列表
- `setFilter()` 支持 4 维过滤：课程名/时间范围/状态/关键词
- 提供 `sourceIndexAt()` 将过滤后的行号映射回原始 Task 索引

### 3.5 ConfigService（配置单例）

- 基于 `QSettings` 持久化到系统注册表（Windows）
- 管理学期起止日期 → 计算当前周数
- 管理提醒开关和提醒提前小时数
- 监听 `configChanged` signal 实时生效

### 3.6 MascotStateService（吉祥物状态引擎）

- 监听 `tasksChanged`，重新计算紧迫度
- 紧迫度映射：`0~0.2 → Happy / 0.2~0.5 → Worried / 0.5~0.8 → Sweating / 0.8+ → Dead`
- SidebarWidget 显示对应吉祥物图标

---

## 四、关键文件索引

| 文件 | 职责摘要 |
|------|----------|
| `main.cpp:11` | `ensureDataFiles()` 确保运行时数据文件存在 |
| `mainwindow.cpp:32` | MainWindow 构造函数，布局组装 |
| `mainwindow.cpp:87` | `initPages()` 延迟初始化所有页面并建立信号连接 |
| `mainwindow.cpp:259` | `eventFilter()` 点击抽屉外区域自动关闭抽屉 |
| `models/datamanager.cpp:74` | `updateTask()` 自动设置 completedAt 时间戳 |
| `models/taskmodel.cpp` | `rebuildVisibleTasks()` 多维过滤逻辑核心 |
| `pages/dashboardpage.cpp` | 课表网格渲染 / DDL聚合 / 学期进度 |
| `pages/todopage.cpp` | 任务看板 + 多维过滤筛选器 |
| `widgets/coursedetail/coursedetaildrawer.cpp` | 抽屉动画 + 可拖拽宽度 |
| `services/reminderservice.cpp` | QTimer 轮询检查即将到期任务 |
| `services/weeklysummaryservice.cpp` | 周报统计生成（含建议文案） |
| `services/searchservice.cpp` | `fuzzyMatch()` 模糊搜索实现 |
| `services/mascotstateservice.cpp` | `calculateUrgency()` 紧迫度算法 |
| `ui/theme.cpp` | 主色调 `#8B1E2D`（北大红）常量定义 |

---

## 五、给新手的维护建议

### 5.1 添加新课程字段

1. 在 `models/course.h` 的 `Course` struct 中添加字段
2. 更新 `toJson()` 和 `fromJson()` 的序列化逻辑
3. 在 `dialogs/courseeditdialog.cpp` 的表单中增加对应 UI 控件
4. 在 `widgets/coursedetail/courseinfopage.cpp` 中增加展示逻辑

### 5.2 添加新页面

1. 在 `pages/` 下创建 `newpage.h/cpp`
2. 在 `CMakeLists.txt` 中添加源文件
3. 在 `mainwindow.cpp:initPages()` 中 `new` 并 `stack->insertWidget()`
4. 在 `SidebarWidget` 中添加导航按钮并连接 `pageChanged` 信号

### 5.3 数据迁移注意事项

- JSON 持久化无版本号，若字段结构变更，需在 `fromJson()` 中处理向后兼容（使用 `obj["field"].toString(defaultValue)`）
- 优先使用 `DataManager` 作为唯一写入入口，不要绕过它直接操作 `DataStore`

### 5.4 性能注意点

- `TaskModel::rebuildVisibleTasks()` 在过滤条件变更时会全量遍历，任务量大时可考虑索引优化
- `ReminderService` 使用 `QTimer` 每分钟轮询，频繁 `DataManager::tasks()` 调用，注意信号去重
- `DashboardPage::renderCourses()` 每次 `coursesChanged` 都重建网格，可考虑增量更新

### 5.5 信号-槽连接约定

- 页面间通信统一经由 `MainWindow` 转发，不要跨页面直接 connect
- `DataManager` 的两个 signal 是全局状态广播，页面需在 `connect` 后手动刷新一次（因为 signal 不携带数据）
- `ConfigService` 配置变更后通过 `configChanged` signal 分发，不要在 UI 中直接轮询

### 5.6 构建与部署

- **构建流程**：使用 CMake 进行跨平台配置，`CMakeLists.txt` 声明了 `Qt6::Widgets`、`Qt6::Core`，并通过 `qt_wrap_ui` 处理 `.ui` 文件。典型命令序列：
  ```
  mkdir build && cd build
  cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.5.2/msvc2019_64"
  cmake --build . --config Release
  ```
- **依赖管理**：项目仅依赖 Qt6，所有第三方库均通过系统安装的 Qt 包提供。若需额外库，请在 `CMakeLists.txt` 中使用 `find_package` 并将对应模块链接到目标。
- **发布**：在 Windows 上运行 `windeployqt.exe <executable>` 将必要的 Qt runtime DLL、平台插件 (`platforms/qwindows.dll`) 以及图像插件复制到发布目录。确保 `plugins/` 文件夹结构保持原样。
- **跨平台**：虽然当前 CI 只在 Windows 上，但 CMake 配置已具备跨平台兼容性（仅需提供对应的 Qt6 编译器和 `CMAKE_PREFIX_PATH`）。在 macOS/Linux 上执行同样的 CMake 步骤即可生成可执行文件。

### 6. 错误处理与日志

- **全局异常捕获**：`main.cpp` 中使用 `QCoreApplication::setAttribute(Qt::AA_UseSoftwareOpenGL)` 并在 `main` 顶层捕获 `std::exception`，打印到 `stderr`。
- **JSON 读写错误**：`DataRepository::loadCourses()` 与 `loadTasks()` 在读取失败时返回空列表并使用 `qWarning()` 记录错误路径与错误码。调用方（`DataManager`）会弹出轻量级 `QMessageBox` 提示用户并尝试创建默认文件。
- **业务级错误**：诸如 “课程冲突” 或 “任务截止日期已过” 在 UI 层通过 `QMessageBox::warning` 通知用户；相应的检查逻辑驻守在 `DataManager`（例如 `isCourseTimeSlotTaken`）。
- **日志策略**：项目目前不使用外部日志库，依赖 Qt 自带的 `qDebug()/qInfo()/qWarning()/qCritical()`。若以后需要持久化日志，可在 `utils/logger.h` 中封装并在 `CMakeLists.txt` 中加入 `spdlog`。

### 7. 测试概览（手动与单元）

- **手动 UI 测试**：主要通过 `Qt Test` 框架的交互式测试脚本（`.qml`）进行基本页面切换与信号触发验证。
- **单元测试**：`tests/` 目录（目前空）预留给关键业务逻辑的 GoogleTest/QtTest 单元测试。建议覆盖：
  1. `DataRepository` 的 JSON 序列化/反序列化。 
  2. `DataManager` 的增删改查逻辑以及信号触发。 
  3. `SearchService` 的模糊匹配准确性。 
  4. `MascotStateService` 的紧迫度计算边界条件。
- **CI 集成**：使用 GitHub Actions（或本地 PowerShell 脚本）执行 `cmake --build` 并运行 `ctest`，确保每次提交保持构建与测试通过。

### 8. 未来可扩展方向

| 方向 | 说明 | 影响层 | 关键实现点 |
|------|------|--------|-----------|
| 多语言/本地化 | 引入 Qt `QTranslator`，使用 `.ts` 文件提供中英文切换 | UI/Service | `MainWindow` 加载 `QTranslator`，所有 UI 文本使用 `tr()`。
| 云同步 | 将 `DataRepository` 替换为远程 REST/GraphQL 存储 | Model | 抽象 `IDataRepository` 接口，实现 `RemoteDataRepository`。
| 插件系统 | 支持第三方插件添加自定义页面或任务类型 | Architecture | 在 `MainWindow` 中加入插件加载器 (`QLibrary`) 并通过信号/槽扩展 UI。
| 单元测试覆盖率提升 | 引入 `gcov`/`lcov` 生成覆盖率报告 | Build/Test | 在 CMake 中添加 `--coverage` 编译标志并收集报告。

---

**本文档生成于 2026-05-11**。


- Qt6 路径硬编码在 `CMakeLists.txt:6`，迁移环境需修改 `CMAKE_PREFIX_PATH`
- `windeployqt` 自动复制 Qt 运行时 DLL 到 build 目录，发布时需确认所有 plugin DLL 存在（`platforms/`, `imageformats/` 等）
