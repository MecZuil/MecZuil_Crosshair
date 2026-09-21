#include "crosshairwindow.h"
#include "settingspanel.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDesktopWidget>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QTransform>
#include <QVBoxLayout>
#include <windows.h>

#ifndef MOD_NOREPEAT
#define MOD_NOREPEAT 0x4000
#endif

static const int HOTKEY_ID = 1;

CrosshairWindow::CrosshairWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setFixedSize(400, 320);
    m_settings.load();

    m_statusBar = new QWidget(this);
    m_statusBar->setObjectName("statusBar");
    m_statusBar->setFixedHeight(32);
    m_statusBar->setStyleSheet(
        "#statusBar{background:#fbfbfc;border:2px solid #ff69b4;"
        "border-bottom:1px solid #e4e7ec;}"
        "QPushButton{color:#3c4048;background:transparent;border:none;"
        "border-radius:6px;padding:2px 10px;font-size:12px;"
        "font-family:\"Segoe UI\",\"Microsoft YaHei\";}"
        "QPushButton:hover{background:#eceff5;color:#1a1c21;}"
        "QPushButton:pressed{background:#dde2ea;}"
        "QPushButton#closeBtn:hover{background:#f05145;color:#ffffff;}"
        "QPushButton#closeBtn:pressed{background:#d43d32;}");
    QHBoxLayout *barLay = new QHBoxLayout(m_statusBar);
    barLay->setContentsMargins(6, 3, 6, 3);
    barLay->setSpacing(4);

    auto mkBtn = [&](const QString &text) {
        return new QPushButton(text);
    };
    m_lockBtn = mkBtn(QStringLiteral("锁定"));
    QPushButton *centerBtn = mkBtn(QStringLiteral("居中"));
    QPushButton *settingsBtn = mkBtn(QStringLiteral("设置"));
    QPushButton *closeBtn = mkBtn(QStringLiteral("关闭"));
    closeBtn->setObjectName("closeBtn");
    barLay->addWidget(m_lockBtn);
    barLay->addWidget(centerBtn);
    barLay->addWidget(settingsBtn);
    barLay->addStretch();
    barLay->addWidget(closeBtn);

    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    lay->addWidget(m_statusBar);
    lay->addStretch();

    m_panel = new SettingsPanel(this);
    m_panel->hide();

    connect(m_lockBtn, &QPushButton::clicked, this, [this] { setLocked(!m_locked); });
    connect(centerBtn, &QPushButton::clicked, this, [this] {
        move(QApplication::desktop()->screenGeometry().center() - rect().center());
    });
    connect(settingsBtn, &QPushButton::clicked, this, [this] {
        if (m_panel->isVisible())
            m_panel->hide();
        else {
            m_panel->show();
            movePanel();
        }
    });
    connect(closeBtn, &QPushButton::clicked, qApp, &QApplication::quit);

    applySettings();
    if (m_settings.windowPos.x() >= 0)
        move(m_settings.windowPos);
    else
        move(QApplication::desktop()->screenGeometry().center() - rect().center());
    registerHotkey();
}

CrosshairWindow::~CrosshairWindow()
{
    UnregisterHotKey(reinterpret_cast<HWND>(winId()), HOTKEY_ID);
}

void CrosshairWindow::applySettings()
{
    m_dotPixmap = QPixmap(m_settings.dot.imagePath);
    m_focusPixmap = QPixmap(m_settings.focus.imagePath);
    m_settings.save();
    update();
}

void CrosshairWindow::setLocked(bool locked)
{
    m_locked = locked;
    m_statusBar->setVisible(!locked);
    setClickThrough(locked);
    if (locked)
        m_panel->hide();
    m_lockBtn->setText(locked ? QStringLiteral("解锁") : QStringLiteral("锁定"));
    update();
}

bool CrosshairWindow::registerHotkey()
{
    HWND hwnd = reinterpret_cast<HWND>(winId());
    UnregisterHotKey(hwnd, HOTKEY_ID);
    const quint32 mods = (m_settings.hotkey >> 16) | MOD_NOREPEAT;
    const quint32 vk = m_settings.hotkey & 0xffff;
    return RegisterHotKey(hwnd, HOTKEY_ID, mods, vk) != 0;
}

void CrosshairWindow::setClickThrough(bool on)
{
    HWND hwnd = reinterpret_cast<HWND>(winId());
    LONG_PTR ex = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    ex |= WS_EX_LAYERED;
    if (on)
        ex |= WS_EX_TRANSPARENT;
    else
        ex &= ~WS_EX_TRANSPARENT;
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, ex);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

void CrosshairWindow::movePanel()
{
    if (m_panel->isVisible())
        m_panel->move(geometry().topRight() + QPoint(2, 0));
}

bool CrosshairWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    if (eventType == "windows_generic_MSG") {
        MSG *msg = static_cast<MSG *>(message);
        if (msg->message == WM_HOTKEY && msg->wParam == HOTKEY_ID) {
            setLocked(!m_locked);
            return true;
        }
    }
    return QWidget::nativeEvent(eventType, message, result);
}

void CrosshairWindow::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    if (!m_locked) {
        p.setPen(QPen(Qt::white, 2));
        p.drawRect(rect().adjusted(1, 1, -2, -2));
    }
    // 准星按物理像素绘制：将 painter 缩放到设备像素坐标系，
    // 大小/粗细/距离参数即为真实像素，不随系统 DPI 缩放变化
    const qreal dpr = devicePixelRatioF();
    p.save();
    p.scale(1.0 / dpr, 1.0 / dpr);
    const QPoint c = (QPointF(rect().center()) * dpr).toPoint();
    drawDotLayer(p, c);
    drawFocusLayer(p, c);
    p.restore();
}

void CrosshairWindow::drawDotLayer(QPainter &p, const QPoint &center)
{
    const LayerSettings &l = m_settings.dot;
    const int h = l.size / 2;
    p.save();
    p.translate(center);
    p.rotate(l.rotation);
    p.setOpacity(l.opacity / 100.0);
    switch (l.style) {
    case 0: // 圆
        p.setPen(Qt::NoPen);
        p.setBrush(l.color);
        p.drawEllipse(-h, -h, l.size, l.size);
        break;
    case 1: // 方
        p.setPen(Qt::NoPen);
        p.setBrush(l.color);
        p.drawRect(-h, -h, l.size, l.size);
        break;
    case 2: { // 三角
        QPolygonF poly;
        poly << QPointF(0, -h) << QPointF(h, h) << QPointF(-h, h);
        p.setPen(Qt::NoPen);
        p.setBrush(l.color);
        p.drawPolygon(poly);
        break; }
    default: // 自定义图片
        if (!m_dotPixmap.isNull()) {
            QPixmap pm = m_dotPixmap.scaled(l.size, l.size, Qt::KeepAspectRatio,
                                            Qt::SmoothTransformation);
            p.drawPixmap(-pm.width() / 2, -pm.height() / 2, pm);
        }
        break;
    }
    p.restore();
}

void CrosshairWindow::drawFocusLayer(QPainter &p, const QPoint &center)
{
    const LayerSettings &l = m_settings.focus;
    static const QPoint dirs[4] = {QPoint(0, -1), QPoint(1, 0),
                                   QPoint(0, 1), QPoint(-1, 0)};
    const int offset = m_settings.dot.size / 2 + m_settings.focusDistance + l.size / 2;
    const QTransform orbit = QTransform().rotate(m_settings.focusOrbit);
    for (int i = 0; i < 4; ++i) {
        if (!m_settings.focusVisible[i])
            continue;
        const QPointF d = orbit.map(QPointF(dirs[i]));
        p.save();
        p.translate(center + QPointF(d.x() * offset, d.y() * offset));
        // 图形按尖端/开口朝向点心绘制：上 90°、右 180°、下 270°、左 0°，再叠加公转角
        p.rotate((i + 1) * 90 + l.rotation + m_settings.focusOrbit);
        p.setOpacity(l.opacity / 100.0);
        drawFocusGlyph(p, l.style, l.size, l.width, l.color);
        p.restore();
    }
}

void CrosshairWindow::drawFocusGlyph(QPainter &p, int style, int size,
                                     int width, const QColor &color)
{
    const double h = size / 2.0;
    QPen pen(color, qMax(1, width));
    pen.setCapStyle(Qt::RoundCap);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    switch (style) {
    case 0: // > 尖端朝中心
        p.drawLine(QPointF(-h * 0.6, -h), QPointF(h, 0));
        p.drawLine(QPointF(h, 0), QPointF(-h * 0.6, h));
        break;
    case 1: // < 尖端朝外
        p.drawLine(QPointF(h * 0.6, -h), QPointF(-h, 0));
        p.drawLine(QPointF(-h, 0), QPointF(h * 0.6, h));
        break;
    case 2: // ) 开口朝中心
        p.drawArc(QRectF(-h, -h, size, size), 120 * 16, 120 * 16);
        break;
    case 3: { // V 实心三角，尖端朝中心
        QPolygonF poly;
        poly << QPointF(h, 0) << QPointF(-h * 0.5, -h) << QPointF(-h * 0.5, h);
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPolygon(poly);
        break; }
    case 4: // - 沿方向轴的短线
        p.drawLine(QPointF(-h, 0), QPointF(h, 0));
        break;
    default: // 自定义图片
        if (!m_focusPixmap.isNull()) {
            QPixmap pm = m_focusPixmap.scaled(size, size, Qt::KeepAspectRatio,
                                              Qt::SmoothTransformation);
            p.drawPixmap(-pm.width() / 2, -pm.height() / 2, pm);
        }
        break;
    }
}

void CrosshairWindow::mousePressEvent(QMouseEvent *e)
{
    if (!m_locked && e->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragOffset = e->globalPos() - frameGeometry().topLeft();
    }
    QWidget::mousePressEvent(e);
}

void CrosshairWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging)
        move(e->globalPos() - m_dragOffset);
    QWidget::mouseMoveEvent(e);
}

void CrosshairWindow::mouseReleaseEvent(QMouseEvent *e)
{
    m_dragging = false;
    QWidget::mouseReleaseEvent(e);
}

void CrosshairWindow::moveEvent(QMoveEvent *e)
{
    m_settings.windowPos = pos();
    movePanel();
    QWidget::moveEvent(e);
}

void CrosshairWindow::closeEvent(QCloseEvent *e)
{
    m_settings.save();
    QWidget::closeEvent(e);
}
