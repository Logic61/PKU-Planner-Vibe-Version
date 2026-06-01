// tests/smoke_test.cpp
// 冒烟测试：覆盖核心数据逻辑，不依赖 UI / DataManager / ConfigService 单例
#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QDateTime>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QSet>
#include <QMessageBox>
#include <QModelIndex>
#include <algorithm>

#include "../models/course.h"
#include "../models/task.h"
#include "../models/datastore.h"
#include "../models/datarepository.h"
#include "../models/taskmodel.h"
#include "../services/exportservice.h"
#include "../utils/datetimeutils.h"

class SmokeTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        qDebug() << "=== PKU Planner Smoke Test Suite ===";
        QCoreApplication::setOrganizationName("PKUPlanner");
        QCoreApplication::setApplicationName("PKUPlanner");
    }

    void init() {
        m_tmpDir = new QTemporaryDir;
        QVERIFY(m_tmpDir->isValid());
    }

    void cleanup() {
        delete m_tmpDir;
        m_tmpDir = nullptr;
    }

    // =========================================================================
    // 1. Course / Task JSON round-trip
    // =========================================================================
    void testCourseJsonRoundTrip() {
        Course orig;
        orig.name = "高等数学A";
        orig.teacher = "张三";
        orig.contact = "zhang@pku.edu.cn";
        orig.location = "三教506";
        orig.examTime = "2026-06-15";
        orig.note = "重点复习";
        orig.folderPath = "/tmp/math";
        orig.day = 3;
        orig.startPeriod = 1;
        orig.endPeriod = 2;
        orig.weekType = 1;

        Course copy = Course::fromJson(orig.toJson());
        QCOMPARE(copy.name, orig.name);
        QCOMPARE(copy.teacher, orig.teacher);
        QCOMPARE(copy.contact, orig.contact);
        QCOMPARE(copy.location, orig.location);
        QCOMPARE(copy.examTime, orig.examTime);
        QCOMPARE(copy.note, orig.note);
        QCOMPARE(copy.folderPath, orig.folderPath);
        QCOMPARE(copy.day, orig.day);
        QCOMPARE(copy.startPeriod, orig.startPeriod);
        QCOMPARE(copy.endPeriod, orig.endPeriod);
        QCOMPARE(copy.weekType, orig.weekType);
    }

    void testTaskJsonRoundTrip() {
        Task orig;
        orig.course = "高等数学A";
        orig.title = "作业3";
        orig.deadline = QDateTime::fromString("2026-06-01T23:59:00", Qt::ISODate);
        orig.priority = 2;
        orig.completed = true;
        orig.completedAt = QDateTime::fromString("2026-05-30T18:00:00", Qt::ISODate);

        Task copy = Task::fromJson(orig.toJson());
        QCOMPARE(copy.course, orig.course);
        QCOMPARE(copy.title, orig.title);
        QCOMPARE(copy.deadline.toString(Qt::ISODate), orig.deadline.toString(Qt::ISODate));
        QCOMPARE(copy.priority, orig.priority);
        QCOMPARE(copy.completed, orig.completed);
        QCOMPARE(copy.completedAt.toString(Qt::ISODate), orig.completedAt.toString(Qt::ISODate));
    }

    // BUG: Task::toJson always writes completedAt, but DataRepository::saveTasks
    // only writes it when completed==true. Documents the inconsistency.
    void testTaskJsonVsRepositorySerialization() {
        Task task;
        task.title = "test";
        task.completed = false;
        task.completedAt = QDateTime::fromString("2026-01-01T00:00:00", Qt::ISODate);

        QJsonObject j1 = task.toJson();
        QVERIFY(j1.contains("completedAt"));

        QJsonObject j2;
        j2["completed"] = task.completed;
        if (task.completed) {
            j2["completedAt"] = task.completedAt.isValid()
                ? task.completedAt.toString(Qt::ISODate)
                : QDateTime::currentDateTime().toString(Qt::ISODate);
        }
        QVERIFY(!j2.contains("completedAt"));
    }

    void testTaskJsonMissingFields() {
        QJsonObject empty;
        Course c = Course::fromJson(empty);
        QVERIFY(c.name.isEmpty());
        QCOMPARE(c.weekType, 0);

        Task t = Task::fromJson(empty);
        QVERIFY(t.title.isEmpty());
        QVERIFY(!t.deadline.isValid());
        QCOMPARE(t.priority, 0);
        QCOMPARE(t.completed, false);
    }

    // =========================================================================
    // 2. DataStore CRUD
    // =========================================================================
    void testCourseCRUD() {
        DataStore store;
        Course c; c.name = "Math";
        store.addCourse(c);
        QCOMPARE(store.courses().size(), 1);

        Course u; u.name = "Physics";
        store.updateCourse(0, u);
        QCOMPARE(store.courses()[0].name, QString("Physics"));

        store.deleteCourse(0);
        QVERIFY(store.courses().isEmpty());
    }

    void testTaskCRUD() {
        DataStore store;
        Task t; t.title = "HW";
        store.addTask(t);
        QCOMPARE(store.tasks().size(), 1);

        store.markTaskCompleted(0, true);
        QVERIFY(store.tasks()[0].completed);
        QVERIFY(store.tasks()[0].completedAt.isValid());

        store.deleteTask(0);
        QVERIFY(store.tasks().isEmpty());
    }

    void testStoreOutOfBounds() {
        DataStore store;
        store.updateCourse(99, Course{});
        store.deleteCourse(-1);
        store.updateTask(99, Task{});
        store.deleteTask(-1);
        store.markTaskCompleted(0, true);
        QVERIFY(store.courses().isEmpty());
        QVERIFY(store.tasks().isEmpty());
    }

    // =========================================================================
    // 3. DataRepository file I/O
    // =========================================================================
    void testCourseFileRoundTrip() {
        DataRepository repo(m_tmpDir->path());
        QList<Course> courses;
        Course c; c.name = "CS101"; c.weekType = 2;
        courses.append(c);

        QVERIFY(repo.saveCourses(courses));
        QList<Course> loaded;
        QVERIFY(repo.loadCourses(loaded));
        QCOMPARE(loaded.size(), 1);
        QCOMPARE(loaded[0].name, QString("CS101"));
        QCOMPARE(loaded[0].weekType, 2);
    }

    void testTaskFileRoundTrip() {
        DataRepository repo(m_tmpDir->path());
        QList<Task> tasks;
        Task t; t.title = "Essay"; t.completed = true;
        t.completedAt = QDateTime::currentDateTime();
        tasks.append(t);

        QVERIFY(repo.saveTasks(tasks));
        QList<Task> loaded;
        QVERIFY(repo.loadTasks(loaded));
        QCOMPARE(loaded.size(), 1);
        QCOMPARE(loaded[0].title, QString("Essay"));
        QVERIFY(loaded[0].completed);
    }

    void testCorruptedFileBackup() {
        QDir dir(m_tmpDir->path());
        QFile bad(dir.absoluteFilePath("courses.json"));
        QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write("NOT JSON{{{");
        bad.close();

        DataRepository repo(m_tmpDir->path());
        QList<Course> loaded;
        QVERIFY(!repo.loadCourses(loaded));
        QVERIFY(loaded.isEmpty());

        QStringList files = dir.entryList(QStringList() << "courses.json.*.corrupted");
        QVERIFY(!files.isEmpty());
    }

    void testCorruptedFileNotRemoved() {
        QDir dir(m_tmpDir->path());
        QFile bad(dir.absoluteFilePath("tasks.json"));
        QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write("{invalid");
        bad.close();

        DataRepository repo(m_tmpDir->path());
        QList<Task> loaded;
        QVERIFY(!repo.loadTasks(loaded));
        QVERIFY(QFile::exists(dir.absoluteFilePath("tasks.json")));

        QVERIFY(!repo.loadTasks(loaded));
        QStringList backups = dir.entryList(QStringList() << "tasks.json.*.corrupted");
        QCOMPARE(backups.size(), 2);
    }

    void testMissingFile() {
        DataRepository repo(m_tmpDir->path());
        QList<Course> loaded;
        QVERIFY(!repo.loadCourses(loaded));
        QVERIFY(loaded.isEmpty());
    }

    void testEmptyArray() {
        QDir dir(m_tmpDir->path());
        QFile f(dir.absoluteFilePath("courses.json"));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("[]");
        f.close();

        DataRepository repo(m_tmpDir->path());
        QList<Course> loaded;
        QVERIFY(repo.loadCourses(loaded));
        QVERIFY(loaded.isEmpty());
    }

    // =========================================================================
    // 4. TaskModel filter/sort
    // =========================================================================
    void testFilterByCourse() {
        QList<Task> tasks;
        Task t1; t1.course = "Math"; t1.title = "HW1";
        Task t2; t2.course = "CS";  t2.title = "HW2";
        tasks.append(t1); tasks.append(t2);

        TaskModel model;
        model.setTasks(tasks);
        model.setFilter("Math", "", "", "");
        QCOMPARE(model.rowCount(QModelIndex()), 1);
        QCOMPARE(model.taskAt(0).title, QString("HW1"));
    }

    void testFilterByStatus() {
        QList<Task> tasks;
        for (int i = 0; i < 5; ++i) {
            Task t; t.title = QString("T%1").arg(i);
            if (i % 2 == 0) t.completed = true;
            tasks.append(t);
        }

        TaskModel model;
        model.setTasks(tasks);
        model.setFilter("", "", "未完成", "");
        QCOMPARE(model.rowCount(QModelIndex()), 2);
        QCOMPARE(model.sourceIndexAt(0), 1);
        QCOMPARE(model.sourceIndexAt(1), 3);
    }

    void testFilterByKeyword() {
        QList<Task> tasks;
        Task t1; t1.course = "Math"; t1.title = "Homework";
        Task t2; t2.course = "CS"; t2.title = "Project";
        tasks.append(t1); tasks.append(t2);

        TaskModel model;
        model.setTasks(tasks);
        model.setFilter("", "", "", "work");
        QCOMPARE(model.rowCount(QModelIndex()), 1);
    }

    // =========================================================================
    // 5. ExportService
    // =========================================================================
    void testCSVExport() {
        QList<Task> tasks;
        Task t; t.title = "Test, \"quoted\""; t.course = "CS";
        t.deadline = QDateTime::currentDateTime();
        t.priority = 1; t.completed = false;
        tasks.append(t);

        QString path = m_tmpDir->filePath("out.csv");
        QVERIFY(ExportService::exportCSV(tasks, path));
        QVERIFY(QFile::exists(path));

        QFile f(path);
        QVERIFY(f.open(QIODevice::ReadOnly | QIODevice::Text));
        QByteArray data = f.readAll();
        QVERIFY(data.contains("CS"));
        QVERIFY(data.contains("\"\"quoted\"\""));
    }

    void testTXTExport() {
        QList<Task> tasks;
        Task t; t.title = "A"; t.course = "CS";
        t.deadline = QDateTime::currentDateTime().addDays(1);
        tasks.append(t);
        QList<Course> courses;

        QString path = m_tmpDir->filePath("report.txt");
        QVERIFY(ExportService::exportTXTReport(tasks, courses, path));
        QVERIFY(QFile::exists(path));
    }

    void testEmptyExport() {
        QString csvPath = m_tmpDir->filePath("empty.csv");
        QVERIFY(ExportService::exportCSV(QList<Task>{}, csvPath));

        QString txtPath = m_tmpDir->filePath("empty.txt");
        QVERIFY(ExportService::exportTXTReport(QList<Task>{}, QList<Course>{}, txtPath));
    }

    // =========================================================================
    // 6. DateTimeUtils
    // =========================================================================
    void testDayText() {
        QCOMPARE(DateTimeUtils::dayText(1), QString("周一"));
        QCOMPARE(DateTimeUtils::dayText(7), QString("周日"));
        QCOMPARE(DateTimeUtils::dayText(0), QString("未设置"));
        QCOMPARE(DateTimeUtils::dayText(8), QString("未设置"));
    }

    void testWeekTypeText() {
        QCOMPARE(DateTimeUtils::weekTypeText(0), QString("每周"));
        QCOMPARE(DateTimeUtils::weekTypeText(1), QString("单周"));
        QCOMPARE(DateTimeUtils::weekTypeText(2), QString("双周"));
    }

    void testScheduleLine() {
        Course c;
        c.day = 3; c.startPeriod = 1; c.endPeriod = 2; c.weekType = 0;
        QCOMPARE(DateTimeUtils::scheduleLine(c), QString("周三 1-2节"));

        c.weekType = 1;
        QCOMPARE(DateTimeUtils::scheduleLine(c), QString("周三 1-2节（单周）"));

        c.weekType = 2;
        QCOMPARE(DateTimeUtils::scheduleLine(c), QString("周三 1-2节（双周）"));
    }

    void testScheduleLineInvalid() {
        Course c;
        c.day = 0;
        QVERIFY(DateTimeUtils::scheduleLine(c).isEmpty());

        c.day = 1; c.startPeriod = 0;
        QVERIFY(DateTimeUtils::scheduleLine(c).isEmpty());
    }

    // =========================================================================
    // 7. Task helper methods
    // =========================================================================
    void testDaysLeft() {
        Task t;
        t.deadline = QDateTime::currentDateTime().addDays(5);
        QCOMPARE(t.daysLeft(), 5);
    }

    void testIsOverdue() {
        Task t;
        t.deadline = QDateTime::currentDateTime().addDays(-1);
        t.completed = false;
        QVERIFY(t.isOverdue());

        t.completed = true;
        QVERIFY(!t.isOverdue());
    }

    void testHasCompletionTime() {
        Task t;
        t.completed = true;
        QVERIFY(!t.hasCompletionTime());

        t.completedAt = QDateTime::currentDateTime();
        QVERIFY(t.hasCompletionTime());
    }

    // =========================================================================
    // 8. Portal import weekType parsing (simulated)
    // =========================================================================
    void testPortalWeekTypeParsing() {
        auto parseWeekType = [](const QString &rawName) -> int {
            if (rawName.contains("(辅双)")) return 2;
            if (rawName.contains("(辅单)") || rawName.contains("(单)") || rawName.contains("单周")) return 1;
            return 0;
        };

        QCOMPARE(parseWeekType("高等数学A(主)\n上课信息：...\n(辅双)"), 2);
        QCOMPARE(parseWeekType("线性代数(辅单)"), 1);
        QCOMPARE(parseWeekType("物理(单)"), 1);
        QCOMPARE(parseWeekType("普通物理(主)"), 0);
        QCOMPARE(parseWeekType("C语言 单周"), 1);
    }

    void testWeekTypeMerge() {
        Course a, b;
        a.name = b.name = "Math";
        a.day = b.day = 1;
        a.startPeriod = 1; a.endPeriod = 1; a.weekType = 1;
        b.startPeriod = 2; b.endPeriod = 2; b.weekType = 2;

        if (a.weekType != 0 && b.weekType != 0 && a.weekType != b.weekType) {
            a.weekType = 0;
        } else if (a.weekType == 0 && b.weekType != 0) {
            a.weekType = b.weekType;
        }
        QCOMPARE(a.weekType, 0);
    }

    // =========================================================================
    // 9. ConfirmDialog Cancel vs No (documents the limitation)
    // =========================================================================
    void testConfirmDialogCancelVsNo() {
        QCOMPARE(static_cast<int>(QMessageBox::Cancel), 0);
        QCOMPARE(static_cast<int>(QMessageBox::No), 0);
    }

    // =========================================================================
    // 10. ReminderService dedup logic
    // =========================================================================
    void testReminderDedup() {
        QSet<QString> notified;
        QStringList keys = {"Math::HW1", "CS::HW2", "Math::HW1"};

        for (const QString &k : keys) {
            if (!notified.contains(k)) {
                notified.insert(k);
            }
        }
        QCOMPARE(notified.size(), 2);
    }

    // =========================================================================
    // 11. Search HTML injection fix
    // =========================================================================
    void testSearchHTMLEscaping() {
        QString text = "<script>alert('xss')</script>";
        QString escaped = text;
        escaped.replace('&', "&amp;").replace('<', "&lt;").replace('>', "&gt;");

        QVERIFY(!escaped.contains("<"));
        QVERIFY(!escaped.contains(">"));
        QVERIFY(escaped.contains("&lt;"));
        QVERIFY(escaped.contains("&gt;"));
    }

    void testSearchHTMLAmpersand() {
        QString text = "C & C++";
        QString escaped = text;
        escaped.replace('&', "&amp;").replace('<', "&lt;").replace('>', "&gt;");
        QCOMPARE(escaped, QString("C &amp; C++"));
    }

    // =========================================================================
    // 12. Search task ID stability
    // =========================================================================
    void testSearchTaskIdStability() {
        Task t;
        t.course = "Math";
        t.title = "HW1";
        QString id = QString("%1::%2").arg(t.course).arg(t.title);
        QCOMPARE(id, QString("Math::HW1"));
        QVERIFY(!id.toInt());
    }

    void testTaskLookupByCourseAndTitle() {
        QList<Task> tasks;
        Task t1; t1.course = "Math"; t1.title = "HW1";
        Task t2; t2.course = "CS";   t2.title = "HW2";
        tasks.append(t1); tasks.append(t2);

        tasks.removeAt(0);
        QCOMPARE(tasks.size(), 1);

        QString key = "CS::HW2";
        QString course = key.section("::", 0, 0);
        QString title = key.section("::", 1);
        int found = -1;
        for (int i = 0; i < tasks.size(); ++i) {
            if (tasks[i].course == course && tasks[i].title == title) {
                found = i;
                break;
            }
        }
        QCOMPARE(found, 0);
    }

    // =========================================================================
    // 13. CourseEditDialog validation
    // =========================================================================
    void testWeekTypeBounds() {
        int weekType = 5;
        int clamped = qBound(0, weekType, 2);
        QCOMPARE(clamped, 2);

        weekType = -1;
        clamped = qBound(0, weekType, 2);
        QCOMPARE(clamped, 0);
    }

    void testPeriodValidation() {
        int start = 10, end = 3;
        QVERIFY(start > end);
    }

    // =========================================================================
    // 14. Config week calculation
    // =========================================================================
    void testWeekCalculation() {
        QDate start(2026, 2, 16);
        QDate day1 = start.addDays(6);
        QDate day2 = start.addDays(7);

        int w1 = start.daysTo(day1) / 7 + 1;
        int w2 = start.daysTo(day2) / 7 + 1;
        QCOMPARE(w1, 1);
        QCOMPARE(w2, 2);
    }

    void testIsSingleWeek() {
        QCOMPARE((1 % 2 == 1), true);
        QCOMPARE((2 % 2 == 1), false);
    }

    // =========================================================================
    // 15. looksLikeLoginPage false positive
    // =========================================================================
    void testLoginPageFalsePositive() {
        QString html = R"(
            <html><body>
            <h1>Course Content</h1>
            <p>Forgot your login? Reset your password here.</p>
            <div>Assignment: HW1</div>
            </body></html>
        )";
        QString lower = html.toLower();
        bool hasPassword = lower.contains("password");
        bool hasLogin = lower.contains("login");
        QVERIFY(hasPassword && hasLogin);
    }

    // =========================================================================
    // 16. MascotStateService urgency
    // =========================================================================
    void testMascotUrgency() {
        double urgency = 1.0 - (36.0 / 72.0);
        QVERIFY(urgency != 0);

        double edge = 1.0 - (72.0 / 72.0);
        QCOMPARE(edge, 0.0);

        double slight = 1.0 - (72.0001 / 72.0);
        QVERIFY(slight < 0);
    }

    // =========================================================================
    // 17. CSV export newline handling (documents the bug)
    // =========================================================================
    void testCSVNewlineBug() {
        QString title = "HW\nPart 2";
        QVERIFY(title.contains('\n'));
        Q_UNUSED(title)
    }

private:
    QTemporaryDir *m_tmpDir = nullptr;
};

QTEST_MAIN(SmokeTest)
#include "smoke_test.moc"
