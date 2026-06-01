# PKU Planner 项目审查报告

> 审查日期: 2026-05-26
> 技术栈: Qt6 + C++20
> 项目类型: Windows 桌面应用程序

---

## 一、项目概览

PKU Planner 是一款面向北京大学学生的课程与 DDL 管理工具。项目采用 Qt6 + C++20 技术栈，
使用 CMake 构建，包含约 50+ 个源文件，代码组织清晰。

### 核心功能
1. 可视化课程表（支持颜色编码的 DDL 紧急度）
2. 任务/DDL 管理（优先级、倒计时、完成状态）
3. 教学网课表自动导入（含 OAuth 登录、HTML 解析）
4. AI 课表图片识别（Gemini/Doubao 双引擎）
5. 吉祥物状态系统（根据任务紧迫度变化）
6. 周报摘要自动生成
7. 课程详情侧滑抽屉（含文件管理、统计）

---

## 二、架构评价

### 2.1 优点

**1. 分层架构清晰**
```
Presentation Layer (pages/, widgets/, components/, dialogs/)
         ↓
Service Layer (services/)
         ↓
Model Layer (models/)
         ↓
Data Persistence (DataRepository → JSON files)
```
各层职责明确，依赖方向正确（UI → Service → Model → Repository）。

**2. 信号-槽机制运用得当**
- DataManager 作为中央数据总线，通过 `coursesChanged()` / `tasksChanged()` 两个 signal 驱动全局更新
- 页面间通信统一经由 MainWindow 转发，避免了跨页面直接耦合
- ConfigService 的 `configChanged` signal 实现配置热更新

**3. 单例模式使用合理**
- DataManager、ConfigService、MascotStateService 采用 Meyers Singleton（静态局部变量），线程安全
- 避免了全局变量污染

**4. 数据持久化安全可靠**
- 使用 QSaveFile 实现原子写入，防止断电/崩溃导致的数据损坏
- JSON 解析失败时自动备份损坏文件（带时间戳的 .corrupted 后缀）
- 支持 .bak 文件回退

**5. 教学网爬虫实现健壮**
- TeachingPlatformService 完整实现了 OAuth → SSO → 课程列表 → 作业详情页的完整链路
- 支持 OTP 双重认证
- HTML 解析使用正则表达式，能处理 Blackboard 平台的复杂嵌套结构
- 并发获取作业详情时使用引用计数的完成计数器（std::shared_ptr<int>）

### 2.2 不足与改进建议

**1. DataManager 信号风暴**
```cpp
// models/datamanager.cpp
void DataManager::addCourse(const Course& c) {
    m_store.addCourse(c);     // DataStore 发射 coursesChanged
    emit coursesChanged();     // DataManager 又发射一次
    m_repository.saveCourses(m_store.courses());
}
```
DataStore 和 DataManager 各自都发射信号，导致每个操作触发两次信号。
**建议**: DataStore 的信号仅用于内部调试，DataManager 的操作不应触发 DataStore 的信号，或者 DataStore 不应继承 QObject 并发射信号。

**2. 全量重建性能问题**
```cpp
// pages/dashboardpage.cpp - renderCourses()
// 每次 coursesChanged 都销毁整个网格并重建
```
当课程数量较多时（如 30+ 门），每次数据变更都会重建整个课表网格。
**建议**: 实现增量更新机制，只更新变化的单元格。

**3. TaskModel 过滤全量遍历**
```cpp
// models/taskmodel.cpp - rebuildVisibleTasks()
// 每次过滤都遍历所有任务
```
任务量大时（如 100+）过滤会有明显延迟。
**建议**: 建立课程名索引（QHash<QString, QList<int>>），过滤时先按课程索引筛选。

**4. ReminderService 轮询频率固定**
```cpp
// services/reminderservice.cpp
m_timer->start(60000); // 固定每分钟
```
**建议**: 根据最近 DDL 动态调整轮询间隔（DDL 临近时增加频率）。

**5. 缺少数据版本号**
JSON 文件无版本号字段，未来升级时无法做自动迁移。
**建议**: 在 JSON 根对象添加 `"version": 1` 字段，fromJson 时检查版本。

---

## 三、代码质量评价

### 3.1 优点

**1. UI 主题统一**
Theme 命名空间集中管理所有颜色常量（北大红 #8B1E2D），所有控件样式引用 Theme::PRIMARY 等常量。

**2. 错误处理完善**
- JSON 解析失败有详细日志（qWarning）
- 网络请求失败有用户友好的错误提示
- 教学网登录失败能智能判断是否需要 OTP 并重试

**3. 动画效果流畅**
- PageAnimator 使用 QPropertyAnimation + QEasingCurve 实现页面切换动画
- CourseDetailDrawer 侧滑抽屉支持拖拽调整宽度
- ToastWidget 渐隐渐现动画

**4. 搜索功能实用**
- TopbarWidget 的搜索支持 debounce（150ms）
- 搜索结果分类显示（课程/任务/文件）
- 关键词高亮显示

### 3.2 代码异味（Code Smells）

**1. 重复代码**
```cpp
// services/configservice.cpp 和 main.cpp 都有 ensureDataFiles 逻辑
// services/dataservice.cpp 也有类似的 ensureDataFiles
```
**建议**: 统一到 DataRepository::ensureDataFiles()。

**2. 魔法数字**
```cpp
// pages/dashboardpage.cpp
m_showTimer->start(220);  // 220ms hover 延迟
m_hideTimer->start(120);  // 120ms hide 延迟
```
**建议**: 提取为常量或配置项。

**3. 硬编码字符串**
```cpp
// pages/dashboardpage.cpp
"周一", "周二", ... // 多处硬编码
```
虽然 utils/datetimeutils.h 中有 dayText() 函数，但部分地方仍使用硬编码。

**4. 样式字符串分散**
大量 QSS 样式字符串散落在各个 .cpp 文件中，虽然使用了 Theme 常量，但样式模板本身难以维护。
**建议**: 考虑使用外部 .qss 文件或样式模板函数。

**5. 内存管理**
```cpp
// widgets/coursedetail/coursedetaildrawer.cpp
anim = new QPropertyAnimation(this, "geometry", this);
```
大部分对象使用 Qt 父子对象机制管理，但部分地方（如 ToastWidget）使用 WA_DeleteOnClose，风格不统一。

---

## 四、安全性评价

### 4.1 优点
- 教学网密码使用 Base64 编码存储（虽然 Base64 不是加密，但比明文好）
- 密码字段有 `remember` 选项，用户可选择不保存

### 4.2 风险
1. **密码存储不安全**: Base64 不是加密，任何能访问 config.json 的人都能解码。
   **建议**: 使用 Qt 的 QCryptographicHash 或系统密钥链（Windows Credential Manager）。

2. **API Key 明文存储**: Gemini/Doubao API Key 如果存储在配置中，同样面临泄露风险。
   **建议**: 使用系统密钥链存储敏感信息。

3. **XSS 风险**: 搜索结果中使用 `highlightText()` 生成 HTML，如果课程名/任务名包含 `<script>` 等标签，可能导致 XSS。
   **建议**: 对文本进行 HTML 转义后再插入 highlight 标签。

---

## 五、可维护性评价

### 5.1 优点
1. **文档完善**: Technical_Architecture.md 非常详细，包含架构图、数据流、维护指南
2. **目录结构规范**: 按职责分层，新开发者容易定位代码
3. **CMake 配置良好**: 支持 MSVC 和 MinGW，自动发现 Qt6 路径

### 5.2 改进空间
1. **缺少单元测试**: tests/ 目录为空
2. **缺少头文件保护宏一致性**: 部分使用 `#pragma once`，部分使用传统 guard
3. **部分函数过长**: DashboardPage 构造函数约 200 行，建议拆分

---

## 六、性能评价

| 操作 | 当前实现 | 性能评级 | 建议 |
|------|---------|---------|------|
| 课程表渲染 | 全量重建网格 | ⚠️ 中 | 增量更新 |
| 任务过滤 | 全量遍历 + 排序 | ⚠️ 中 | 索引优化 |
| JSON 读写 | 完整序列化 | ✅ 好 | QSaveFile 原子写入 |
| 搜索 | debounce + 全量匹配 | ✅ 好 | 任务量大时考虑索引 |
| 提醒检查 | 每分钟轮询 | ✅ 好 | 可动态调整 |
| 页面切换 | 动画滑动 | ✅ 好 | - |
| 教学网爬取 | 串行课程 + 并发作业 | ⚠️ 中 | 可并行课程 |

---

## 七、总结

### 整体评分: 7.5/10

**项目亮点:**
- 架构设计合理，分层清晰
- 教学网爬虫功能强大且健壮
- UI 设计精美，交互流畅
- 数据持久化安全可靠
- 文档完善

**主要改进方向:**
1. 消除信号重复发射（DataStore vs DataManager）
2. 实现课程表增量更新
3. 加强密码存储安全性
4. 补充单元测试
5. 添加数据版本迁移机制

这是一个完成度较高、架构清晰的 Qt 桌面应用项目，适合北京大学学生日常使用。
代码质量整体良好，主要需要在性能和安全性方面做进一步优化。
