#ifndef CROSSHAIRWINDOW_H
#define CROSSHAIRWINDOW_H

#include <QPixmap>
#include <QWidget>
#include "crosshairsettings.h"

class QPainter;
class QPushButton;
class SettingsPanel;

class CrosshairWindow : public QWidget
{
    Q_OBJECT
public:
    explicit CrosshairWindow(QWidget *parent = nullptr);
    ~CrosshairWindow();

    CrosshairSettings &settings() { return m_settings; }
    void applySettings();              // 设置变更后调用：刷新缓存、重绘并持久化
    void setLocked(bool locked);
    bool isLocked() const { return m_locked; }
    bool registerHotkey();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;
    void moveEvent(QMoveEvent *) override;
    void closeEvent(QCloseEvent *) override;
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;

private:
    void drawDotLayer(QPainter &p, const QPoint &center);
    void drawFocusLayer(QPainter &p, const QPoint &center);
    void drawFocusGlyph(QPainter &p, int style, int size, int width,
                        const QColor &color);
    void setClickThrough(bool on);
    void movePanel();

    CrosshairSettings m_settings;
    QWidget *m_statusBar;
    QPushButton *m_lockBtn;
    SettingsPanel *m_panel;
    bool m_locked = false;
    bool m_dragging = false;
    QPoint m_dragOffset;
    QPixmap m_dotPixmap;
    QPixmap m_focusPixmap;
};

#endif // CROSSHAIRWINDOW_H
