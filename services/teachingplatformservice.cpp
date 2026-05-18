#include "teachingplatformservice.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QNetworkCookieJar>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSet>
#include <QUrl>
#include <QUrlQuery>
#include <QVariant>

#include <functional>
#include <memory>

namespace {

const char *IAAA_OAUTH_LOGIN = "https://iaaa.pku.edu.cn/iaaa/oauthlogin.do";

const char *BLACKBOARD_APP_ID = "blackboard";
const char *BLACKBOARD_OAUTH_REDIR =
    "http://course.pku.edu.cn/webapps/bb-sso-BBLEARN/execute/authValidate/campusLogin";
const char *BLACKBOARD_SSO_LOGIN =
    "https://course.pku.edu.cn/webapps/bb-sso-BBLEARN/execute/authValidate/campusLogin";
const char *BLACKBOARD_HOME =
    "https://course.pku.edu.cn/webapps/portal/execute/tabs/tabAction";
const char *BLACKBOARD_COURSE_PAGE =
    "https://course.pku.edu.cn/webapps/blackboard/execute/announcement";
const char *BLACKBOARD_LIST_CONTENT =
    "https://course.pku.edu.cn/webapps/blackboard/content/listContent.jsp";
const char *BLACKBOARD_ASSIGNMENT_UPLOAD =
    "https://course.pku.edu.cn/webapps/assignment/uploadAssignment";

const char *PORTAL_APP_ID = "portalPublicQuery";
const char *PORTAL_REDIR = "https://portal.pku.edu.cn/publicQuery/ssoLogin.do";
const char *PORTAL_XNDXQ_LIST =
    "https://portal.pku.edu.cn/publicQuery/ctrl/topic/myCourseTable/getXndXqList.do";
const char *PORTAL_COURSE_INFO =
    "https://portal.pku.edu.cn/publicQuery/ctrl/topic/myCourseTable/getCourseInfo.do";

QString htmlDecode(QString s)
{
    s.replace("&amp;", "&");
    s.replace("&lt;", "<");
    s.replace("&gt;", ">");
    s.replace("&quot;", "\"");
    s.replace("&#39;", "'");
    s.replace("&apos;", "'");
    s.replace("&nbsp;", " ");
    return s;
}

QString stripHtmlTags(const QString &html)
{
    QString out;
    out.reserve(html.size());

    bool inTag = false;
    for (QChar ch : html) {
        if (ch == '<') {
            inTag = true;
            out.append(' ');
            continue;
        }
        if (ch == '>') {
            inTag = false;
            out.append(' ');
            continue;
        }
        if (!inTag) {
            out.append(ch);
        }
    }

    return htmlDecode(out).simplified();
}

QString normalizedCourseName(QString raw)
{
    raw = stripHtmlTags(raw).trimmed();

    int colon = raw.indexOf(':');
    if (colon >= 0 && colon + 1 < raw.size()) {
        raw = raw.mid(colon + 1).trimmed();
    }

    // Blackboard 课程名通常形如：
    //   程序设计实习（25-26学年第二学期）
    //   高级俄语（上）（25-26学年第二学期）
    // 这里只删除“最后一个表示学年/学期的括号”，保留课程名中有意义的括号。
    QRegularExpression semesterSuffixRe(
        QStringLiteral(R"(\s*[（(][^（）()]*((\d{2,4}\s*[-—–]\s*\d{2,4})|学年|学期|semester|term|spring|fall|autumn|summer|春|秋|夏)[^（）()]*[）)]\s*$)"),
        QRegularExpression::CaseInsensitiveOption
        );

    while (true) {
        QRegularExpressionMatch match = semesterSuffixRe.match(raw);
        if (!match.hasMatch()) {
            break;
        }
        raw = raw.left(match.capturedStart(0)).trimmed();
    }

    return raw.simplified();
}

QUrl blackboardUrlFromHref(QString href)
{
    href = htmlDecode(href).trimmed();

    if (href.startsWith("http://", Qt::CaseInsensitive) ||
        href.startsWith("https://", Qt::CaseInsensitive)) {
        return QUrl(href);
    }

    if (href.startsWith('/')) {
        return QUrl(QStringLiteral("https://course.pku.edu.cn") + href);
    }

    return QUrl(QStringLiteral("https://course.pku.edu.cn/") + href);
}

QString extractCourseIdFromHref(const QString &href)
{
    QRegularExpression keyRe(QStringLiteral(R"(key=([\d_]+))"));
    QRegularExpressionMatch keyMatch = keyRe.match(href);
    if (keyMatch.hasMatch()) {
        return keyMatch.captured(1);
    }

    QRegularExpression idRe(QStringLiteral(R"(course_id=([\d_]+))"));
    QRegularExpressionMatch idMatch = idRe.match(href);
    if (idMatch.hasMatch()) {
        return idMatch.captured(1);
    }

    return QString();
}

QString extractHtmlElementById(const QString &html, const QString &id)
{
    QRegularExpression startRe(
        QStringLiteral(R"(<([a-zA-Z0-9]+)\b[^>]*id=["']%1["'][^>]*>)")
            .arg(QRegularExpression::escape(id)),
        QRegularExpression::CaseInsensitiveOption
        );

    QRegularExpressionMatch startMatch = startRe.match(html);
    if (!startMatch.hasMatch()) {
        return QString();
    }

    QString tag = startMatch.captured(1).toLower();
    int start = startMatch.capturedStart(0);
    int pos = startMatch.capturedEnd(0);

    QRegularExpression tagRe(
        QStringLiteral(R"(<%1\b[^>]*>|</%1>)").arg(tag),
        QRegularExpression::CaseInsensitiveOption
        );

    int depth = 1;
    auto it = tagRe.globalMatch(html, pos);

    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        QString token = m.captured(0).toLower();

        if (token.startsWith(QStringLiteral("</"))) {
            --depth;
            if (depth == 0) {
                int end = m.capturedEnd(0);
                return html.mid(start, end - start);
            }
        } else {
            ++depth;
        }
    }

    return html.mid(start);
}

QString extractBalancedElementAt(const QString &html, int start)
{
    QRegularExpression openRe(
        QStringLiteral(R"(<([a-zA-Z0-9]+)\b[^>]*>)"),
        QRegularExpression::CaseInsensitiveOption
        );

    QRegularExpressionMatch openMatch = openRe.match(html, start);
    if (!openMatch.hasMatch()) {
        return QString();
    }

    QString tag = openMatch.captured(1).toLower();
    int elementStart = openMatch.capturedStart(0);
    int pos = openMatch.capturedEnd(0);

    QRegularExpression tagRe(
        QStringLiteral(R"(<%1\b[^>]*>|</%1>)").arg(tag),
        QRegularExpression::CaseInsensitiveOption
        );

    int depth = 1;
    auto it = tagRe.globalMatch(html, pos);

    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        QString token = m.captured(0).toLower();
        if (token.startsWith(QStringLiteral("</"))) {
            --depth;
            if (depth == 0) {
                int end = m.capturedEnd(0);
                return html.mid(elementStart, end - elementStart);
            }
        } else {
            ++depth;
        }
    }

    return html.mid(elementStart);
}

QStringList extractDirectLiElements(const QString &containerHtml)
{
    QStringList items;

    QRegularExpression liTokenRe(
        QStringLiteral(R"(<li\b[^>]*>|</li>)"),
        QRegularExpression::CaseInsensitiveOption
        );

    auto it = liTokenRe.globalMatch(containerHtml);

    int depth = 0;
    int itemStart = -1;

    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();
        QString token = m.captured(0).toLower();

        if (token.startsWith(QStringLiteral("<li"))) {
            if (depth == 0) {
                itemStart = m.capturedStart(0);
            }
            ++depth;
        } else {
            --depth;
            if (depth == 0 && itemStart >= 0) {
                int itemEnd = m.capturedEnd(0);
                items.append(containerHtml.mid(itemStart, itemEnd - itemStart));
                itemStart = -1;
            }
        }
    }

    return items;
}

QString extractFirstImgAltOrTitle(const QString &liHtml)
{
    QRegularExpression imgRe(
        QStringLiteral(R"(<img\b[^>]*>)"),
        QRegularExpression::CaseInsensitiveOption
        );

    QRegularExpressionMatch imgMatch = imgRe.match(liHtml);
    if (!imgMatch.hasMatch()) {
        return QString();
    }

    QString imgTag = imgMatch.captured(0);

    QRegularExpression attrRe(
        QStringLiteral(R"((alt|title)=["']([^"']+)["'])"),
        QRegularExpression::CaseInsensitiveOption
        );

    auto it = attrRe.globalMatch(imgTag);
    while (it.hasNext()) {
        QString value = htmlDecode(it.next().captured(2)).trimmed();
        if (!value.isEmpty()) {
            return value;
        }
    }

    return QString();
}

QString extractContentIdFromHtmlOrHref(const QString &html)
{
    QRegularExpression hrefRe(QStringLiteral(R"(content_id=([A-Za-z0-9_%\-]+))"));
    QRegularExpressionMatch hrefMatch = hrefRe.match(html);
    if (hrefMatch.hasMatch()) {
        return htmlDecode(hrefMatch.captured(1));
    }

    QRegularExpression idRe(
        QStringLiteral(R"(id=["']([^"']+)["'])"),
        QRegularExpression::CaseInsensitiveOption
        );
    auto it = idRe.globalMatch(html);
    while (it.hasNext()) {
        QString id = it.next().captured(1).trimmed();
        if (id.isEmpty()) {
            continue;
        }
        if (id == "content_listContainer" ||
            id.contains("container", Qt::CaseInsensitive) ||
            id.contains("menu", Qt::CaseInsensitive) ||
            id.contains("palette", Qt::CaseInsensitive)) {
            continue;
        }
        if (id.contains('_')) {
            return id;
        }
    }

    return QString();
}

QString extractItemContentIdFromLi(const QString &liHtml)
{
    // pku3b 取 title_div.attr("id")，通常是 li 内第一个有意义的 div id。
    QRegularExpression divIdRe(
        QStringLiteral(R"(<div\b[^>]*id=["']([^"']+)["'][^>]*>)"),
        QRegularExpression::CaseInsensitiveOption
        );

    auto it = divIdRe.globalMatch(liHtml);
    while (it.hasNext()) {
        QString id = it.next().captured(1).trimmed();
        if (id.isEmpty()) {
            continue;
        }
        if (id == "content_listContainer" ||
            id.contains("container", Qt::CaseInsensitive) ||
            id.contains("menu", Qt::CaseInsensitive) ||
            id.contains("palette", Qt::CaseInsensitive)) {
            continue;
        }
        return id;
    }

    return extractContentIdFromHtmlOrHref(liHtml);
}

QString extractListContentIdFromHref(const QString &html)
{
    QRegularExpression anchorRe(
        QStringLiteral(R"(<a\b[^>]*href=["']([^"']+)["'][^>]*>)"),
        QRegularExpression::CaseInsensitiveOption
        );

    auto it = anchorRe.globalMatch(html);
    while (it.hasNext()) {
        QString href = htmlDecode(it.next().captured(1));
        QUrl url = blackboardUrlFromHref(href);

        if (!url.path().endsWith(QStringLiteral("listContent.jsp"), Qt::CaseInsensitive)) {
            continue;
        }

        QUrlQuery query(url);
        QString contentId = query.queryItemValue(QStringLiteral("content_id"));
        if (!contentId.isEmpty()) {
            return contentId;
        }

        QRegularExpression fallbackRe(QStringLiteral(R"(content_id=([^&"'>]+))"));
        QRegularExpressionMatch fallback = fallbackRe.match(href);
        if (fallback.hasMatch()) {
            return htmlDecode(fallback.captured(1));
        }
    }

    return QString();
}

bool htmlHasAnyAnchor(const QString &html)
{
    QRegularExpression anchorRe(
        QStringLiteral(R"(<a\b[^>]*href=)"),
        QRegularExpression::CaseInsensitiveOption
        );
    return anchorRe.match(html).hasMatch();
}

QString extractItemTitleFromLi(const QString &liHtml)
{
    QRegularExpression headingRe(
        QStringLiteral(R"(<h[34][^>]*>([\s\S]*?)</h[34]>)"),
        QRegularExpression::CaseInsensitiveOption
        );
    QRegularExpressionMatch headingMatch = headingRe.match(liHtml);
    if (headingMatch.hasMatch()) {
        QString title = stripHtmlTags(headingMatch.captured(1));
        if (!title.isEmpty()) {
            return title;
        }
    }

    QRegularExpression anchorRe(
        QStringLiteral(R"(<a[^>]*>([\s\S]*?)</a>)"),
        QRegularExpression::CaseInsensitiveOption
        );
    QRegularExpressionMatch anchorMatch = anchorRe.match(liHtml);
    if (anchorMatch.hasMatch()) {
        QString title = stripHtmlTags(anchorMatch.captured(1));
        if (!title.isEmpty()) {
            return title;
        }
    }

    QString plain = stripHtmlTags(liHtml);
    plain.remove(QStringLiteral("作业"));
    plain.remove(QStringLiteral("Assignment"), Qt::CaseInsensitive);
    return plain.simplified();
}

QString formatDeadline(int year, int month, int day, QString ampm, int hour, int minute)
{
    ampm = ampm.trimmed();

    if (ampm == QStringLiteral("下午") ||
        ampm == QStringLiteral("晚上") ||
        ampm == QStringLiteral("中午")) {
        if (hour < 12) {
            hour += 12;
        }
    } else if (ampm == QStringLiteral("上午")) {
        if (hour == 12) {
            hour = 0;
        }
    }

    return QStringLiteral("%1-%2-%3 %4:%5")
        .arg(year, 4, 10, QLatin1Char('0'))
        .arg(month, 2, 10, QLatin1Char('0'))
        .arg(day, 2, 10, QLatin1Char('0'))
        .arg(hour, 2, 10, QLatin1Char('0'))
        .arg(minute, 2, 10, QLatin1Char('0'));
}

QString normalizeDeadlineText(QString text)
{
    text = htmlDecode(text);
    text = stripHtmlTags(text).simplified();

    QRegularExpression cnDateTimeRe(
        QStringLiteral(R"((\d{4})年(\d{1,2})月(\d{1,2})日(?:\s*星期.)?\s*(上午|下午|中午|晚上)?\s*(\d{1,2}):(\d{2}))")
        );
    QRegularExpressionMatch cnDateTimeMatch = cnDateTimeRe.match(text);
    if (cnDateTimeMatch.hasMatch()) {
        return formatDeadline(
            cnDateTimeMatch.captured(1).toInt(),
            cnDateTimeMatch.captured(2).toInt(),
            cnDateTimeMatch.captured(3).toInt(),
            cnDateTimeMatch.captured(4),
            cnDateTimeMatch.captured(5).toInt(),
            cnDateTimeMatch.captured(6).toInt()
            );
    }

    QRegularExpression cnDateOnlyRe(
        QStringLiteral(R"((\d{4})年(\d{1,2})月(\d{1,2})日(?:\s*星期.)?)")
        );
    QRegularExpressionMatch cnDateOnlyMatch = cnDateOnlyRe.match(text);
    if (cnDateOnlyMatch.hasMatch()) {
        return formatDeadline(
            cnDateOnlyMatch.captured(1).toInt(),
            cnDateOnlyMatch.captured(2).toInt(),
            cnDateOnlyMatch.captured(3).toInt(),
            QString(),
            23,
            59
            );
    }

    QRegularExpression simpleDateTimeRe(
        QStringLiteral(R"((\d{4})[-/](\d{1,2})[-/](\d{1,2})(?:\s+(\d{1,2}):(\d{2}))?)")
        );
    QRegularExpressionMatch simpleMatch = simpleDateTimeRe.match(text);
    if (simpleMatch.hasMatch()) {
        int hour = simpleMatch.captured(4).isEmpty() ? 23 : simpleMatch.captured(4).toInt();
        int minute = simpleMatch.captured(5).isEmpty() ? 59 : simpleMatch.captured(5).toInt();

        return formatDeadline(
            simpleMatch.captured(1).toInt(),
            simpleMatch.captured(2).toInt(),
            simpleMatch.captured(3).toInt(),
            QString(),
            hour,
            minute
            );
    }

    return QString();
}

QString extractDeadlineFromHtml(const QString &html)
{
    QRegularExpression dueFieldRe(
        QStringLiteral(R"(<div\b[^>]*id=["']assignMeta2["'][^>]*>[\s\S]*?</div>\s*<div\b[^>]*aria-describedby=["']assignMeta2["'][^>]*>([\s\S]*?)</div>)"),
        QRegularExpression::CaseInsensitiveOption
        );

    QRegularExpressionMatch dueFieldMatch = dueFieldRe.match(html);
    if (dueFieldMatch.hasMatch()) {
        QString deadline = normalizeDeadlineText(dueFieldMatch.captured(1));
        if (!deadline.isEmpty()) {
            return deadline;
        }
    }

    QRegularExpression assignMetaRe(
        QStringLiteral(R"(<div\b[^>]*id=["']assignMeta2["'][^>]*>[\s\S]*?</div>)"),
        QRegularExpression::CaseInsensitiveOption
        );
    QRegularExpressionMatch assignMeta = assignMetaRe.match(html);
    if (assignMeta.hasMatch()) {
        QString nearby = html.mid(assignMeta.capturedStart(0), 1800);
        QString deadline = normalizeDeadlineText(nearby);
        if (!deadline.isEmpty()) {
            return deadline;
        }
    }

    QRegularExpression metadataStartRe(
        QStringLiteral(R"(<li\b[^>]*id=["']metadata["'][^>]*>)"),
        QRegularExpression::CaseInsensitiveOption
        );
    QRegularExpressionMatch metadataStart = metadataStartRe.match(html);
    if (metadataStart.hasMatch()) {
        QString metadataHtml = extractBalancedElementAt(html, metadataStart.capturedStart(0));
        QString deadline = normalizeDeadlineText(metadataHtml);
        if (!deadline.isEmpty()) {
            return deadline;
        }
    }

    QString plain = stripHtmlTags(html);
    const QStringList labels = {
        QStringLiteral("到期日期"),
        QStringLiteral("到期时间"),
        QStringLiteral("到期"),
        QStringLiteral("截止日期"),
        QStringLiteral("截止时间"),
        QStringLiteral("截止"),
        QStringLiteral("Due Date"),
        QStringLiteral("Due")
    };

    for (const QString &label : labels) {
        int pos = plain.indexOf(label, 0, Qt::CaseInsensitive);
        while (pos >= 0) {
            QString window = plain.mid(pos, 600);
            QString deadline = normalizeDeadlineText(window);
            if (!deadline.isEmpty()) {
                return deadline;
            }
            pos = plain.indexOf(label, pos + label.size(), Qt::CaseInsensitive);
        }
    }

    return QString();
}


bool isSubmittedAssignmentDetailPage(const QString &html)
{
    QString plain = stripHtmlTags(html).simplified();

    QRegularExpression titleRe(
        QStringLiteral(R"(<title[^>]*>([\s\S]*?)</title>)"),
        QRegularExpression::CaseInsensitiveOption
        );
    QRegularExpressionMatch titleMatch = titleRe.match(html);
    if (titleMatch.hasMatch()) {
        QString title = stripHtmlTags(titleMatch.captured(1)).simplified();

        // 已提交作业详情页的标题通常是：
        // “复查提交历史记录: xxx – 课程名”
        if (title.contains(QStringLiteral("复查提交历史记录")) ||
            title.contains(QStringLiteral("提交历史记录")) ||
            title.contains(QStringLiteral("Review Submission History"), Qt::CaseInsensitive) ||
            title.contains(QStringLiteral("Submission History"), Qt::CaseInsensitive)) {
            return true;
        }
    }

    // Blackboard 已提交页面还常见这些标记。
    // 注意不要用单独的“提交”判断，未提交页面也会有“提交”按钮。
    if (plain.contains(QStringLiteral("复查提交历史记录")) ||
        plain.contains(QStringLiteral("提交历史记录")) ||
        plain.contains(QStringLiteral("Review Submission History"), Qt::CaseInsensitive) ||
        plain.contains(QStringLiteral("Submission History"), Qt::CaseInsensitive) ||
        plain.contains(QStringLiteral("当前尝试")) ||
        plain.contains(QStringLiteral("Current Attempt"), Qt::CaseInsensitive) ||
        plain.contains(QStringLiteral("attemptHistory"), Qt::CaseInsensitive) ||
        plain.contains(QStringLiteral("currentAttempt_label"), Qt::CaseInsensitive)) {
        return true;
    }

    // 关键兜底：
    // 未提交作业详情页必须包含 assignMeta2 / 到期日期。
    // 如果是 assignment 页面但完全没有到期日期元数据，通常就是已提交后的历史记录页。
    const bool isAssignmentPage =
        plain.contains(QStringLiteral("作业")) ||
        plain.contains(QStringLiteral("Assignment"), Qt::CaseInsensitive) ||
        html.contains(QStringLiteral("/webapps/assignment/"), Qt::CaseInsensitive);

    const bool hasDueMetadata =
        html.contains(QStringLiteral("assignMeta2"), Qt::CaseInsensitive) ||
        plain.contains(QStringLiteral("到期日期")) ||
        plain.contains(QStringLiteral("截止日期")) ||
        plain.contains(QStringLiteral("Due Date"), Qt::CaseInsensitive);

    if (isAssignmentPage && !hasDueMetadata) {
        return true;
    }

    return false;
}


QString normalizedTaskKey(const QString &course, const QString &title)
{
    QString c = stripHtmlTags(course).simplified().toLower();
    QString t = stripHtmlTags(title).simplified().toLower();

    c.remove(QRegularExpression(QStringLiteral(R"(\s+)")));
    t.remove(QRegularExpression(QStringLiteral(R"(\s+)")));

    return c + QStringLiteral("||") + t;
}

QList<QJsonObject> filterNewTasksByCourseAndTitle(const QList<QJsonObject> &tasks)
{
    static QSet<QString> importedTaskKeys;

    QList<QJsonObject> result;
    for (const QJsonObject &task : tasks) {
        QString key = normalizedTaskKey(
            task.value(QStringLiteral("course")).toString(),
            task.value(QStringLiteral("title")).toString()
            );

        if (key == QStringLiteral("||")) {
            continue;
        }

        if (importedTaskKeys.contains(key)) {
            continue;
        }

        importedTaskKeys.insert(key);
        result.append(task);
    }

    return result;
}

bool looksLikeLoginPage(const QString &html)
{
    QString lower = html.toLower();
    bool hasPassword =
        lower.contains("type=\"password\"") ||
        lower.contains("type='password'") ||
        lower.contains("name=\"password\"") ||
        lower.contains("name='password'");
    bool hasLogin =
        lower.contains("webapps/login") ||
        lower.contains("oauthlogin.do") ||
        lower.contains("iaaa.pku.edu.cn") ||
        lower.contains("login");
    return hasPassword && hasLogin;
}

bool isOtpErrorMessage(const QString &err)
{
    QString lower = err.toLower();
    return lower.contains("otp") ||
           lower.contains("e05") ||
           lower.contains(QStringLiteral("令牌")) ||
           lower.contains(QStringLiteral("动态码")) ||
           lower.contains(QStringLiteral("二次认证"));
}

void getWithRedirects(QNetworkAccessManager *manager,
                      QObject *context,
                      const QUrl &url,
                      const std::function<void(QNetworkReply::NetworkError, const QString &, const QByteArray &)> &done,
                      int remaining = 8)
{
    if (remaining <= 0) {
        done(QNetworkReply::ProtocolInvalidOperationError,
             QStringLiteral("Too many redirects"),
             QByteArray());
        return;
    }

    QNetworkRequest request(url);
    request.setRawHeader("User-Agent", "Mozilla/5.0");
    request.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");

    QNetworkReply *reply = manager->get(request);
    QObject::connect(reply, &QNetworkReply::finished, context, [manager, context, reply, done, remaining]() {
        QNetworkReply::NetworkError error = reply->error();
        QString errorString = reply->errorString();
        QByteArray body = reply->readAll();

        QVariant locationHeader = reply->header(QNetworkRequest::LocationHeader);
        QUrl baseUrl = reply->url();

        reply->deleteLater();

        if (error != QNetworkReply::NoError) {
            done(error, errorString, body);
            return;
        }

        if (locationHeader.isValid()) {
            QUrl next = locationHeader.toUrl();
            if (next.isRelative()) {
                next = baseUrl.resolved(next);
            }
            getWithRedirects(manager, context, next, done, remaining - 1);
            return;
        }

        QString sbody = QString::fromUtf8(body);
        QRegularExpression locationRe(
            QStringLiteral(R"(window\.location(?:\.href)?\s*=\s*['"]([^'"]+)['"])"),
            QRegularExpression::CaseInsensitiveOption
            );
        QRegularExpressionMatch locationMatch = locationRe.match(sbody);
        if (locationMatch.hasMatch()) {
            QUrl next = QUrl::fromUserInput(htmlDecode(locationMatch.captured(1)));
            if (next.isRelative()) {
                next = baseUrl.resolved(next);
            }
            getWithRedirects(manager, context, next, done, remaining - 1);
            return;
        }

        done(QNetworkReply::NoError, QString(), body);
    });
}

QList<QJsonObject> extractCurrentSemesterCourses(const QString &html)
{
    QList<QJsonObject> courses;
    QSet<QString> seenIds;

    QRegularExpression portletRe(
        QStringLiteral(R"(<div[^>]*class=["'][^"']*portlet[^"']*["'][^>]*>([\s\S]*?)(?=<div[^>]*class=["'][^"']*portlet[^"']*["']|$))"),
        QRegularExpression::CaseInsensitiveOption
        );

    QRegularExpression anchorRe(
        QStringLiteral(R"(<a[^>]+href=["']([^"']+)["'][^>]*>([\s\S]*?)</a>)"),
        QRegularExpression::CaseInsensitiveOption
        );

    auto portletIt = portletRe.globalMatch(html);
    while (portletIt.hasNext()) {
        QString portlet = portletIt.next().captured(1);
        QString plainPortlet = stripHtmlTags(portlet);

        bool isCoursePortlet =
            plainPortlet.contains(QStringLiteral("课程")) ||
            plainPortlet.contains(QStringLiteral("Courses"), Qt::CaseInsensitive);

        bool isCurrentSemester =
            plainPortlet.contains(QStringLiteral("当前")) ||
            plainPortlet.contains(QStringLiteral("Current Semester Courses"), Qt::CaseInsensitive);

        if (!isCoursePortlet || !isCurrentSemester) {
            continue;
        }

        auto anchorIt = anchorRe.globalMatch(portlet);
        while (anchorIt.hasNext()) {
            QRegularExpressionMatch m = anchorIt.next();
            QString href = htmlDecode(m.captured(1));
            QString text = m.captured(2);

            QString courseId = extractCourseIdFromHref(href);
            if (courseId.isEmpty() || seenIds.contains(courseId)) {
                continue;
            }

            QString courseName = normalizedCourseName(text);
            if (courseName.isEmpty()) {
                continue;
            }

            QJsonObject course;
            course.insert(QStringLiteral("id"), courseId);
            course.insert(QStringLiteral("name"), courseName);
            course.insert(QStringLiteral("href"), href);
            courses.append(course);
            seenIds.insert(courseId);
        }
    }

    // 兼容性兜底：如果 Blackboard 首页结构与预期不同，至少尝试从所有课程链接中解析课程。
    if (courses.isEmpty()) {
        auto anchorIt = anchorRe.globalMatch(html);
        while (anchorIt.hasNext()) {
            QRegularExpressionMatch m = anchorIt.next();
            QString href = htmlDecode(m.captured(1));
            QString courseId = extractCourseIdFromHref(href);
            if (courseId.isEmpty() || seenIds.contains(courseId)) {
                continue;
            }

            QString courseName = normalizedCourseName(m.captured(2));
            if (courseName.isEmpty()) {
                courseName = courseId;
            }

            QJsonObject course;
            course.insert(QStringLiteral("id"), courseId);
            course.insert(QStringLiteral("name"), courseName);
            course.insert(QStringLiteral("href"), href);
            courses.append(course);
            seenIds.insert(courseId);
        }
    }

    return courses;
}

QStringList extractContentIdsFromCourseMenu(const QString &courseHtml)
{
    QString menuHtml = extractHtmlElementById(courseHtml, QStringLiteral("courseMenuPalette_contents"));
    if (menuHtml.isEmpty()) {
        menuHtml = courseHtml;
    }

    QStringList contentIds;
    QSet<QString> seen;

    QRegularExpression anchorRe(
        QStringLiteral(R"(<a\b[^>]*href=["']([^"']+)["'][^>]*>)"),
        QRegularExpression::CaseInsensitiveOption
        );

    auto it = anchorRe.globalMatch(menuHtml);
    while (it.hasNext()) {
        QString href = htmlDecode(it.next().captured(1));
        QUrl url = blackboardUrlFromHref(href);

        if (!url.path().endsWith(QStringLiteral("listContent.jsp"), Qt::CaseInsensitive)) {
            continue;
        }

        QUrlQuery query(url);
        QString contentId = query.queryItemValue(QStringLiteral("content_id"));
        if (contentId.isEmpty()) {
            QRegularExpression re(QStringLiteral(R"(content_id=([^&"']+))"));
            QRegularExpressionMatch m = re.match(href);
            if (m.hasMatch()) {
                contentId = htmlDecode(m.captured(1));
            }
        }

        if (!contentId.isEmpty() && !seen.contains(contentId)) {
            seen.insert(contentId);
            contentIds.append(contentId);
        }
    }

    return contentIds;
}

struct ParsedContentPage {
    QList<QJsonObject> assignments;
    QStringList childContentIds;
};

ParsedContentPage parseContentPage(const QString &html, const QString &courseId, const QString &courseName)
{
    ParsedContentPage result;

    QString containerHtml = extractHtmlElementById(html, QStringLiteral("content_listContainer"));
    if (containerHtml.isEmpty()) {
        return result;
    }

    QStringList items = extractDirectLiElements(containerHtml);
    QSet<QString> seenChildIds;
    QSet<QString> seenAssignmentIds;

    for (const QString &liHtml : items) {
        QString kind = extractFirstImgAltOrTitle(liHtml);
        QString contentId = extractItemContentIdFromLi(liHtml);
        QString linkedContentId = extractListContentIdFromHref(liHtml);
        QString title = extractItemTitleFromLi(liHtml);
        bool hasLink = htmlHasAnyAnchor(liHtml);

        bool isAssignment =
            kind == QStringLiteral("作业") ||
            kind.compare(QStringLiteral("Assignment"), Qt::CaseInsensitive) == 0;

        if (isAssignment) {
            if (contentId.isEmpty()) {
                contentId = linkedContentId;
            }
            if (contentId.isEmpty() || seenAssignmentIds.contains(contentId)) {
                continue;
            }
            if (title.isEmpty()) {
                title = contentId;
            }

            QJsonObject task;
            task.insert(QStringLiteral("title"), title);
            task.insert(QStringLiteral("course"), normalizedCourseName(courseName));
            task.insert(QStringLiteral("courseId"), courseId);
            task.insert(QStringLiteral("contentId"), contentId);
            task.insert(QStringLiteral("deadline"), extractDeadlineFromHtml(liHtml));
            task.insert(QStringLiteral("completed"), false);
            task.insert(QStringLiteral("source"), QStringLiteral("blackboard_assignment"));

            result.assignments.append(task);
            seenAssignmentIds.insert(contentId);
            continue;
        }

        // pku3b 的 CourseContentStream 会对 has_link 的内容继续探测。
        QString nextId = !linkedContentId.isEmpty() ? linkedContentId : contentId;
        if (hasLink && !nextId.isEmpty() && !seenChildIds.contains(nextId)) {
            seenChildIds.insert(nextId);
            result.childContentIds.append(nextId);
        }
    }

    return result;
}

QUrl makeBlackboardHomeUrl()
{
    QUrl url(BLACKBOARD_HOME);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("tab_tab_group_id"), QStringLiteral("_1_1"));
    url.setQuery(query);
    return url;
}

QUrl makeBlackboardCoursePageUrl(const QString &courseId)
{
    QUrl url(BLACKBOARD_COURSE_PAGE);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("method"), QStringLiteral("search"));
    query.addQueryItem(QStringLiteral("context"), QStringLiteral("course_entry"));
    query.addQueryItem(QStringLiteral("course_id"), courseId);
    query.addQueryItem(QStringLiteral("handle"), QStringLiteral("announcements_entry"));
    query.addQueryItem(QStringLiteral("mode"), QStringLiteral("view"));
    url.setQuery(query);
    return url;
}

QUrl makeBlackboardContentPageUrl(const QString &courseId, const QString &contentId)
{
    QUrl url(BLACKBOARD_LIST_CONTENT);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("content_id"), contentId);
    query.addQueryItem(QStringLiteral("course_id"), courseId);
    url.setQuery(query);
    return url;
}

QUrl makeBlackboardAssignmentUploadPageUrl(const QString &courseId, const QString &contentId)
{
    QUrl url(BLACKBOARD_ASSIGNMENT_UPLOAD);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("action"), QStringLiteral("newAttempt"));
    query.addQueryItem(QStringLiteral("content_id"), contentId);
    query.addQueryItem(QStringLiteral("course_id"), courseId);
    url.setQuery(query);
    return url;
}

QUrl makeBlackboardAssignmentViewPageUrl(const QString &courseId, const QString &contentId)
{
    QUrl url(BLACKBOARD_ASSIGNMENT_UPLOAD);
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("mode"), QStringLiteral("view"));
    query.addQueryItem(QStringLiteral("content_id"), contentId);
    query.addQueryItem(QStringLiteral("course_id"), courseId);
    url.setQuery(query);
    return url;
}

} // namespace

TeachingPlatformService::TeachingPlatformService(QObject *parent)
    : QObject(parent),
    networkManager(new QNetworkAccessManager(this))
{
    networkManager->setCookieJar(new QNetworkCookieJar(networkManager));
}

void TeachingPlatformService::login(const QString &username, const QString &password, const QString &otp)
{
    lastUsername = username;
    lastPassword = password;
    lastOtp = otp;

    QUrl iaaaUrl(IAAA_OAUTH_LOGIN);
    QNetworkRequest request(iaaaUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("User-Agent", "Mozilla/5.0");
    request.setRawHeader("Accept", "application/json, text/plain, */*");

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("appid"), BLACKBOARD_APP_ID);
    params.addQueryItem(QStringLiteral("userName"), username);
    params.addQueryItem(QStringLiteral("password"), password);
    params.addQueryItem(QStringLiteral("randCode"), QString());
    params.addQueryItem(QStringLiteral("smsCode"), QString());
    params.addQueryItem(QStringLiteral("otpCode"), otp);
    params.addQueryItem(QStringLiteral("redirUrl"), BLACKBOARD_OAUTH_REDIR);

    QNetworkReply *reply = networkManager->post(request, params.toString(QUrl::FullyEncoded).toUtf8());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            QString err = reply->errorString();
            reply->deleteLater();
            emit loginFailed(err);
            return;
        }

        QByteArray data = reply->readAll();
        reply->deleteLater();

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit loginFailed(QStringLiteral("Invalid JSON response from IAAA: %1").arg(parseError.errorString()));
            return;
        }

        QJsonObject obj = doc.object();
        if (!obj.value(QStringLiteral("success")).toBool(false)) {
            QString emsg;
            QJsonValue errors = obj.value(QStringLiteral("errors"));
            if (errors.isObject()) {
                QJsonObject eobj = errors.toObject();
                emsg = QStringLiteral("[%1] %2")
                           .arg(eobj.value(QStringLiteral("code")).toString(),
                                eobj.value(QStringLiteral("msg")).toString());
            } else if (!errors.isUndefined() && !errors.isNull()) {
                emsg = errors.toVariant().toString();
            } else {
                emsg = obj.value(QStringLiteral("msg")).toString(QStringLiteral("iaaa login failed"));
            }
            emit loginFailed(emsg);
            return;
        }

        QString token = obj.value(QStringLiteral("token")).toString();
        if (token.isEmpty()) {
            emit loginFailed(QStringLiteral("IAAA returned success but no token"));
            return;
        }

        QUrl ssoUrl(BLACKBOARD_SSO_LOGIN);
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("token"), token);
        ssoUrl.setQuery(query);

        getWithRedirects(networkManager, this, ssoUrl,
                         [this](QNetworkReply::NetworkError error, const QString &err, const QByteArray &) {
                             if (error != QNetworkReply::NoError) {
                                 emit loginFailed(err);
                                 return;
                             }
                             emit loginSuccess();
                         });
    });
}

void TeachingPlatformService::followRedirectAndEstablishSession(const QUrl &url, int remaining)
{
    getWithRedirects(networkManager, this, url,
                     [this](QNetworkReply::NetworkError error, const QString &err, const QByteArray &) {
                         if (error != QNetworkReply::NoError) {
                             emit loginFailed(err);
                             return;
                         }
                         emit loginSuccess();
                     },
                     remaining);
}

void TeachingPlatformService::handleLoginResponse(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        sessionCookie = reply->rawHeader("Set-Cookie");
        emit loginSuccess();
    } else {
        emit loginFailed(reply->errorString());
    }
    reply->deleteLater();
}

void TeachingPlatformService::fetchTodoTasks()
{
    if (!hasCredentials()) {
        emit todoAuthRequired(QStringLiteral("请先登录教学网后再同步课程作业"));
        return;
    }

    loginBlackboardAndFetchTodoTasks();
}

void TeachingPlatformService::fetchTodoTasksWithCredentials(const QString &username,
                                                            const QString &password,
                                                            const QString &otp)
{
    lastUsername = username;
    lastPassword = password;
    lastOtp = otp;
    fetchTodoTasks();
}

void TeachingPlatformService::loginBlackboardAndFetchTodoTasks()
{
    QUrl iaaaUrl(IAAA_OAUTH_LOGIN);
    QNetworkRequest request(iaaaUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("User-Agent", "Mozilla/5.0");
    request.setRawHeader("Accept", "application/json, text/plain, */*");

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("appid"), BLACKBOARD_APP_ID);
    params.addQueryItem(QStringLiteral("userName"), lastUsername);
    params.addQueryItem(QStringLiteral("password"), lastPassword);
    params.addQueryItem(QStringLiteral("randCode"), QString());
    params.addQueryItem(QStringLiteral("smsCode"), QString());
    params.addQueryItem(QStringLiteral("otpCode"), lastOtp);
    params.addQueryItem(QStringLiteral("redirUrl"), BLACKBOARD_OAUTH_REDIR);

    QNetworkReply *reply = networkManager->post(request, params.toString(QUrl::FullyEncoded).toUtf8());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            QString err = reply->errorString();
            reply->deleteLater();
            emit fetchFailed(QStringLiteral("Blackboard 登录失败: %1").arg(err));
            return;
        }

        QByteArray data = reply->readAll();
        reply->deleteLater();

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit fetchFailed(QStringLiteral("Blackboard 登录响应不是有效 JSON: %1").arg(parseError.errorString()));
            return;
        }

        QJsonObject obj = doc.object();
        if (!obj.value(QStringLiteral("success")).toBool(false)) {
            QString emsg;
            QJsonValue errors = obj.value(QStringLiteral("errors"));
            if (errors.isObject()) {
                QJsonObject eobj = errors.toObject();
                emsg = QStringLiteral("[%1] %2")
                           .arg(eobj.value(QStringLiteral("code")).toString(),
                                eobj.value(QStringLiteral("msg")).toString());
            } else if (!errors.isUndefined() && !errors.isNull()) {
                emsg = errors.toVariant().toString();
            } else {
                emsg = obj.value(QStringLiteral("msg")).toString(QStringLiteral("blackboard login failed"));
            }

            if (isOtpErrorMessage(emsg)) {
                emit todoAuthRequired(emsg);
            } else {
                emit fetchFailed(QStringLiteral("Blackboard 登录失败: %1").arg(emsg));
            }
            return;
        }

        QString token = obj.value(QStringLiteral("token")).toString();
        if (token.isEmpty()) {
            emit fetchFailed(QStringLiteral("Blackboard 登录成功但没有返回 token"));
            return;
        }

        QUrl ssoUrl(BLACKBOARD_SSO_LOGIN);
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("token"), token);
        query.addQueryItem(QStringLiteral("_rand"), QString::number(QRandomGenerator::global()->generateDouble(), 'f', 20));
        ssoUrl.setQuery(query);

        getWithRedirects(networkManager, this, ssoUrl,
                         [this](QNetworkReply::NetworkError error, const QString &err, const QByteArray &) {
                             if (error != QNetworkReply::NoError) {
                                 emit fetchFailed(QStringLiteral("Blackboard SSO 失败: %1").arg(err));
                                 return;
                             }
                             fetchTodoTasksAfterBlackboardLogin();
                         });
    });
}

void TeachingPlatformService::fetchTodoTasksAfterBlackboardLogin()
{
    getWithRedirects(networkManager, this, makeBlackboardHomeUrl(),
                     [this](QNetworkReply::NetworkError error, const QString &err, const QByteArray &body) {
                         if (error != QNetworkReply::NoError) {
                             emit fetchFailed(QStringLiteral("获取教学网首页失败: %1").arg(err));
                             return;
                         }

                         QString homeHtml = QString::fromUtf8(body);
                         if (looksLikeLoginPage(homeHtml)) {
                             emit todoAuthRequired(QStringLiteral("Blackboard 登录态无效，请重新登录教学网"));
                             return;
                         }

                         QList<QJsonObject> courses = extractCurrentSemesterCourses(homeHtml);
                         if (courses.isEmpty()) {
                             emit fetchFailed(QStringLiteral("未能解析当前学期课程列表"));
                             return;
                         }

                         auto allTasks = std::make_shared<QList<QJsonObject>>();
                         auto remainingCourses = std::make_shared<int>(courses.size());
                         auto completed = std::make_shared<bool>(false);

                         auto emitTasksAfterDeadlineFetch = [this, allTasks]() {
                             if (allTasks->isEmpty()) {
                                 emit fetchFailed(QStringLiteral("当前课程没有课程作业"));
                                 return;
                             }

                             auto pendingDetails = std::make_shared<int>(0);
                             auto emitted = std::make_shared<bool>(false);
                             auto finish = std::make_shared<std::function<void()>>();

                             *finish = [this, allTasks, pendingDetails, emitted]() {
                                 if (*emitted || *pendingDetails > 0) {
                                     return;
                                 }

                                 QList<QJsonObject> unfinishedTasks;
                                 for (const QJsonObject &task : *allTasks) {
                                     if (!task.value(QStringLiteral("__skip")).toBool(false)) {
                                         unfinishedTasks.append(task);
                                     }
                                 }

                                 *emitted = true;

                                 if (unfinishedTasks.isEmpty()) {
                                     emit fetchFailed(QStringLiteral("当前课程没有未提交课程作业"));
                                     return;
                                 }

                                 // 避免重复导入：同一课程名 + 同一作业名只导入一次。
                                 // 第二次点击同步时，如果没有新增作业，会返回空列表，让 UI 显示同步 0 个，而不是重复创建。
                                 emit tasksFetched(filterNewTasksByCourseAndTitle(unfinishedTasks));
                             };

                             for (int i = 0; i < allTasks->size(); ++i) {
                                 QJsonObject task = allTasks->at(i);
                                 const QString courseId = task.value(QStringLiteral("courseId")).toString();
                                 const QString contentId = task.value(QStringLiteral("contentId")).toString();

                                 if (courseId.isEmpty() || contentId.isEmpty()) {
                                     continue;
                                 }

                                 ++(*pendingDetails);

                                 // 先访问 mode=view 页面判断是否已经提交。
                                 // 已提交作业在该页面通常会出现“复查提交历史记录 / Current Attempt”等标记。
                                 getWithRedirects(networkManager, this, makeBlackboardAssignmentViewPageUrl(courseId, contentId),
                                                  [this, allTasks, i, courseId, contentId, pendingDetails, finish](
                                                      QNetworkReply::NetworkError viewError,
                                                      const QString &,
                                                      const QByteArray &viewBody) {
                                                      QJsonObject updated = allTasks->at(i);

                                                      if (viewError == QNetworkReply::NoError) {
                                                          const QString viewHtml = QString::fromUtf8(viewBody);
                                                          if (isSubmittedAssignmentDetailPage(viewHtml)) {
                                                              updated.insert(QStringLiteral("__skip"), true);
                                                              updated.insert(QStringLiteral("completed"), true);
                                                              (*allTasks)[i] = updated;

                                                              --(*pendingDetails);
                                                              (*finish)();
                                                              return;
                                                          }
                                                      }

                                                      // 未提交时，再访问 action=newAttempt 页面解析到期日期。
                                                      getWithRedirects(networkManager, this, makeBlackboardAssignmentUploadPageUrl(courseId, contentId),
                                                                       [allTasks, i, pendingDetails, finish](
                                                                           QNetworkReply::NetworkError detailError,
                                                                           const QString &,
                                                                           const QByteArray &detailBody) {
                                                                           QJsonObject updated = allTasks->at(i);

                                                                           if (detailError == QNetworkReply::NoError) {
                                                                               const QString detailHtml = QString::fromUtf8(detailBody);

                                                                               if (isSubmittedAssignmentDetailPage(detailHtml)) {
                                                                                   updated.insert(QStringLiteral("__skip"), true);
                                                                                   updated.insert(QStringLiteral("completed"), true);
                                                                                   (*allTasks)[i] = updated;
                                                                               } else {
                                                                                   const QString detailDeadline = extractDeadlineFromHtml(detailHtml);

                                                                                   // 未提交作业必须能解析到有效截止时间才导入；
                                                                                   // 否则 UI 可能把无效时间显示成“已逾期”。
                                                                                   if (!detailDeadline.isEmpty()) {
                                                                                       updated.insert(QStringLiteral("deadline"), detailDeadline);
                                                                                       updated.insert(QStringLiteral("completed"), false);
                                                                                       (*allTasks)[i] = updated;
                                                                                   } else {
                                                                                       updated.insert(QStringLiteral("__skip"), true);
                                                                                       (*allTasks)[i] = updated;
                                                                                   }
                                                                               }
                                                                           } else {
                                                                               // 详情页无法访问时不要导入，避免把未知状态/未知 DDL 的作业误导入。
                                                                               updated.insert(QStringLiteral("__skip"), true);
                                                                               (*allTasks)[i] = updated;
                                                                           }

                                                                           --(*pendingDetails);
                                                                           (*finish)();
                                                                       });
                                                  });
                             }

                             (*finish)();
                         };

                         auto finishOneCourse = [this, allTasks, remainingCourses, completed, emitTasksAfterDeadlineFetch](const QList<QJsonObject> &courseTasks) {
                             if (*completed) {
                                 return;
                             }

                             for (const QJsonObject &task : courseTasks) {
                                 allTasks->append(task);
                             }

                             --(*remainingCourses);
                             if (*remainingCourses > 0) {
                                 return;
                             }

                             *completed = true;
                             emitTasksAfterDeadlineFetch();
                         };

                         for (const QJsonObject &course : courses) {
                             const QString courseId = course.value(QStringLiteral("id")).toString();
                             const QString courseName = course.value(QStringLiteral("name")).toString(courseId);

                             getWithRedirects(networkManager, this, makeBlackboardCoursePageUrl(courseId),
                                              [this, courseId, courseName, finishOneCourse](QNetworkReply::NetworkError error,
                                                                                            const QString &,
                                                                                            const QByteArray &courseBody) {
                                                  if (error != QNetworkReply::NoError) {
                                                      finishOneCourse({});
                                                      return;
                                                  }

                                                  QString courseHtml = QString::fromUtf8(courseBody);
                                                  QStringList rootContentIds = extractContentIdsFromCourseMenu(courseHtml);

                                                  QList<QJsonObject> directTasks = parseContentPage(courseHtml, courseId, courseName).assignments;

                                                  if (rootContentIds.isEmpty()) {
                                                      finishOneCourse(directTasks);
                                                      return;
                                                  }

                                                  auto courseTasks = std::make_shared<QList<QJsonObject>>(directTasks);
                                                  auto visitedContentIds = std::make_shared<QSet<QString>>();
                                                  auto pendingPages = std::make_shared<int>(0);
                                                  auto courseDone = std::make_shared<bool>(false);
                                                  auto fetchContentPage = std::make_shared<std::function<void(const QString &)>>();

                                                  auto tryFinishCourse = [finishOneCourse, courseTasks, pendingPages, courseDone]() {
                                                      if (!*courseDone && *pendingPages == 0) {
                                                          *courseDone = true;
                                                          finishOneCourse(*courseTasks);
                                                      }
                                                  };

                                                  *fetchContentPage = [this,
                                                                       courseId,
                                                                       courseName,
                                                                       courseTasks,
                                                                       visitedContentIds,
                                                                       pendingPages,
                                                                       courseDone,
                                                                       fetchContentPage,
                                                                       tryFinishCourse](const QString &contentId) {
                                                      if (contentId.isEmpty() || visitedContentIds->contains(contentId)) {
                                                          return;
                                                      }

                                                      visitedContentIds->insert(contentId);
                                                      ++(*pendingPages);

                                                      getWithRedirects(networkManager, this, makeBlackboardContentPageUrl(courseId, contentId),
                                                                       [this,
                                                                        courseId,
                                                                        courseName,
                                                                        courseTasks,
                                                                        visitedContentIds,
                                                                        pendingPages,
                                                                        courseDone,
                                                                        fetchContentPage,
                                                                        tryFinishCourse](QNetworkReply::NetworkError error,
                                                                                         const QString &,
                                                                                         const QByteArray &pageBody) {
                                                                           if (error == QNetworkReply::NoError) {
                                                                               ParsedContentPage parsed = parseContentPage(QString::fromUtf8(pageBody), courseId, courseName);

                                                                               for (const QJsonObject &task : parsed.assignments) {
                                                                                   const QString contentId = task.value(QStringLiteral("contentId")).toString();
                                                                                   bool alreadyExists = false;
                                                                                   for (const QJsonObject &existing : *courseTasks) {
                                                                                       if (!contentId.isEmpty() &&
                                                                                           existing.value(QStringLiteral("contentId")).toString() == contentId) {
                                                                                           alreadyExists = true;
                                                                                           break;
                                                                                       }
                                                                                   }
                                                                                   if (!alreadyExists) {
                                                                                       courseTasks->append(task);
                                                                                   }
                                                                               }

                                                                               for (const QString &childId : parsed.childContentIds) {
                                                                                   if (!visitedContentIds->contains(childId)) {
                                                                                       (*fetchContentPage)(childId);
                                                                                   }
                                                                               }
                                                                           }

                                                                           --(*pendingPages);
                                                                           tryFinishCourse();
                                                                       });
                                                  };

                                                  for (const QString &contentId : rootContentIds) {
                                                      (*fetchContentPage)(contentId);
                                                  }

                                                  tryFinishCourse();
                                              });
                         }
                     });
}

void TeachingPlatformService::handleFetchTasksResponse(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            emit fetchFailed(QStringLiteral("Invalid JSON response"));
            reply->deleteLater();
            return;
        }

        QList<QJsonObject> list;
        if (doc.isArray()) {
            for (const QJsonValue &v : doc.array()) {
                if (v.isObject()) {
                    list.append(v.toObject());
                }
            }
        } else if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.value(QStringLiteral("tasks")).isArray()) {
                for (const QJsonValue &v : obj.value(QStringLiteral("tasks")).toArray()) {
                    if (v.isObject()) {
                        list.append(v.toObject());
                    }
                }
            }
        }

        if (list.isEmpty()) {
            emit fetchFailed(QStringLiteral("Invalid response format"));
        } else {
            emit tasksFetched(list);
        }
    } else {
        emit fetchFailed(reply->errorString());
    }
    reply->deleteLater();
}

QStringList TeachingPlatformService::extractCourseKeysFromHomepage(const QString &html)
{
    QStringList keys;
    QList<QJsonObject> courses = extractCurrentSemesterCourses(html);
    for (const QJsonObject &course : courses) {
        QString id = course.value(QStringLiteral("id")).toString();
        if (!id.isEmpty() && !keys.contains(id)) {
            keys.append(id);
        }
    }
    return keys;
}

QList<QJsonObject> TeachingPlatformService::extractAssignmentsFromCoursePage(const QString &html, const QString &courseKey)
{
    ParsedContentPage parsed = parseContentPage(html, courseKey, courseKey);
    return parsed.assignments;
}

QList<QJsonObject> TeachingPlatformService::extractAssignmentsFromCoursePageTokenizer(const QString &html, const QString &courseKey)
{
    return extractAssignmentsFromCoursePage(html, courseKey);
}

void TeachingPlatformService::fetchCourseTable()
{
    if (!hasCredentials()) {
        emit courseTableFetchFailed(QStringLiteral("请先登录教学网后再导入课表"));
        return;
    }

    loginPortalAndFetchCourseTable();
}

void TeachingPlatformService::loginPortalAndFetchCourseTable()
{
    QUrl iaaaUrl(IAAA_OAUTH_LOGIN);
    QNetworkRequest request(iaaaUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    request.setRawHeader("User-Agent", "Mozilla/5.0");
    request.setRawHeader("Accept", "application/json, text/plain, */*");

    QUrlQuery params;
    params.addQueryItem(QStringLiteral("appid"), PORTAL_APP_ID);
    params.addQueryItem(QStringLiteral("userName"), lastUsername);
    params.addQueryItem(QStringLiteral("password"), lastPassword);
    params.addQueryItem(QStringLiteral("randCode"), QString());
    params.addQueryItem(QStringLiteral("smsCode"), QString());
    params.addQueryItem(QStringLiteral("otpCode"), lastOtp);
    params.addQueryItem(QStringLiteral("redirUrl"), PORTAL_REDIR);

    QNetworkReply *reply = networkManager->post(request, params.toString(QUrl::FullyEncoded).toUtf8());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            QString err = reply->errorString();
            reply->deleteLater();
            emit courseTableFetchFailed(QStringLiteral("Portal 登录失败: %1").arg(err));
            return;
        }

        QByteArray data = reply->readAll();
        reply->deleteLater();

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit courseTableFetchFailed(QStringLiteral("Portal 登录响应不是有效 JSON: %1").arg(parseError.errorString()));
            return;
        }

        QJsonObject obj = doc.object();
        if (!obj.value(QStringLiteral("success")).toBool(false)) {
            QString emsg;
            QJsonValue errors = obj.value(QStringLiteral("errors"));
            if (errors.isObject()) {
                QJsonObject eobj = errors.toObject();
                emsg = QStringLiteral("[%1] %2")
                           .arg(eobj.value(QStringLiteral("code")).toString(),
                                eobj.value(QStringLiteral("msg")).toString());
            } else if (!errors.isUndefined() && !errors.isNull()) {
                emsg = errors.toVariant().toString();
            } else {
                emsg = obj.value(QStringLiteral("msg")).toString(QStringLiteral("portal login failed"));
            }
            emit courseTableFetchFailed(QStringLiteral("Portal 登录失败: %1").arg(emsg));
            return;
        }

        QString token = obj.value(QStringLiteral("token")).toString();
        if (token.isEmpty()) {
            emit courseTableFetchFailed(QStringLiteral("Portal 登录成功但没有返回 token"));
            return;
        }

        QUrl ssoUrl(PORTAL_REDIR);
        QUrlQuery query;
        query.addQueryItem(QStringLiteral("token"), token);
        ssoUrl.setQuery(query);

        getWithRedirects(networkManager, this, ssoUrl,
                         [this](QNetworkReply::NetworkError error, const QString &err, const QByteArray &) {
                             if (error != QNetworkReply::NoError) {
                                 emit courseTableFetchFailed(QStringLiteral("Portal SSO 失败: %1").arg(err));
                                 return;
                             }
                             fetchCourseTableAfterPortalLogin();
                         });
    });
}

void TeachingPlatformService::fetchCourseTableAfterPortalLogin()
{
    QUrl xndUrl(PORTAL_XNDXQ_LIST);
    getWithRedirects(networkManager, this, xndUrl,
                     [this](QNetworkReply::NetworkError error, const QString &err, const QByteArray &body) {
                         if (error != QNetworkReply::NoError) {
                             emit courseTableFetchFailed(err);
                             return;
                         }

                         QJsonParseError parseError;
                         QJsonDocument doc = QJsonDocument::fromJson(body, &parseError);
                         if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
                             emit courseTableFetchFailed(QStringLiteral("无法解析学年学期列表: %1").arg(parseError.errorString()));
                             return;
                         }

                         QJsonObject obj = doc.object();
                         QString xndxq;
                         if (obj.value(QStringLiteral("nowXnxq")).isObject()) {
                             xndxq = obj.value(QStringLiteral("nowXnxq")).toObject().value(QStringLiteral("xndxq")).toString();
                         }
                         if (xndxq.isEmpty() && obj.value(QStringLiteral("list")).isArray() && !obj.value(QStringLiteral("list")).toArray().isEmpty()) {
                             xndxq = obj.value(QStringLiteral("list")).toArray().first().toObject().value(QStringLiteral("xndxq")).toString();
                         }
                         if (xndxq.isEmpty()) {
                             emit courseTableFetchFailed(QStringLiteral("无法确定学年学期 (xndxq)"));
                             return;
                         }

                         QUrl infoUrl(PORTAL_COURSE_INFO);
                         QUrlQuery query;
                         query.addQueryItem(QStringLiteral("xndxq"), xndxq);
                         infoUrl.setQuery(query);

                         getWithRedirects(networkManager, this, infoUrl,
                                          [this](QNetworkReply::NetworkError error, const QString &err, const QByteArray &infoBody) {
                                              if (error != QNetworkReply::NoError) {
                                                  emit courseTableFetchFailed(err);
                                                  return;
                                              }

                                              QJsonParseError parseError2;
                                              QJsonDocument doc2 = QJsonDocument::fromJson(infoBody, &parseError2);
                                              if (parseError2.error != QJsonParseError::NoError || !doc2.isObject()) {
                                                  emit courseTableFetchFailed(QStringLiteral("无法解析课表数据: %1").arg(parseError2.errorString()));
                                                  return;
                                              }

                                              emit courseTableFetched(doc2.object());
                                          });
                     });
}
