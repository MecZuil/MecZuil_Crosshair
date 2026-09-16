#include "settingspanel.h"
#include "crosshairwindow.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <windows.h>

namespace {

// QKeySequence -> RegisterHotKey 参数；不支持的键返回 false
bool keySequenceToHotkey(const QKeySequence &seq, quint32 &mods, quint32 &vk)
{
    if (seq.isEmpty())
        return false;
    const int k = seq[0];
    const int key = k & ~Qt::KeyboardModifierMask;
    mods = 0;
    if (k & Qt::SHIFT)   mods |= MOD_SHIFT;
    if (k & Qt::CTRL)    mods |= MOD_CONTROL;
    if (k & Qt::ALT)     mods |= MOD_ALT;
    if (k & Qt::META)    mods |= MOD_WIN;

    if (key >= Qt::Key_A && key <= Qt::Key_Z)
        vk = 'A' + (key - Qt::Key_A);
    else if (key >= Qt::Key_0 && key <= Qt::Key_9)
        vk = '0' + (key - Qt::Key_0);
    else if (key >= Qt::Key_F1 && key <= Qt::Key_F24)
        vk = VK_F1 + (key - Qt::Key_F1);
    else {
        switch (key) {
        case Qt::Key_Space:    vk = VK_SPACE;   break;
        case Qt::Key_Tab:      vk = VK_TAB;     break;
        case Qt::Key_Escape:   vk = VK_ESCAPE;  break;
        case Qt::Key_Insert:   vk = VK_INSERT;  break;
        case Qt::Key_Delete:   vk = VK_DELETE;  break;
        case Qt::Key_Home:     vk = VK_HOME;    break;
        case Qt::Key_End:      vk = VK_END;     break;
        case Qt::Key_PageUp:   vk = VK_PRIOR;   break;
        case Qt::Key_PageDown: vk = VK_NEXT;    break;
        case Qt::Key_Left:     vk = VK_LEFT;    break;
        case Qt::Key_Right:    vk = VK_RIGHT;   break;
        case Qt::Key_Up:       vk = VK_UP;      break;
        case Qt::Key_Down:     vk = VK_DOWN;    break;
        default: return false;
        }
    }
    return true;
}

QKeySequence hotkeyToKeySequence(quint32 hotkey)
{
    const quint32 vk = hotkey & 0xffff;
    const quint32 mods = hotkey >> 16;
    int key = 0;
    if (vk >= 'A' && vk <= 'Z')
        key = Qt::Key_A + (vk - 'A');
    else if (vk >= '0' && vk <= '9')
        key = Qt::Key_0 + (vk - '0');
    else if (vk >= VK_F1 && vk <= VK_F24)
        key = Qt::Key_F1 + (vk - VK_F1);
    else {
        switch (vk) {
        case VK_SPACE:   key = Qt::Key_Space;    break;
        case VK_TAB:     key = Qt::Key_Tab;      break;
        case VK_ESCAPE:  key = Qt::Key_Escape;   break;
        case VK_INSERT:  key = Qt::Key_Insert;   break;
        case VK_DELETE:  key = Qt::Key_Delete;   break;
        case VK_HOME:    key = Qt::Key_Home;     break;
        case VK_END:     key = Qt::Key_End;      break;
        case VK_PRIOR:   key = Qt::Key_PageUp;   break;
        case VK_NEXT:    key = Qt::Key_PageDown; break;
        case VK_LEFT:    key = Qt::Key_Left;     break;
        case VK_RIGHT:   key = Qt::Key_Right;    break;
        case VK_UP:      key = Qt::Key_Up;       break;
        case VK_DOWN:    key = Qt::Key_Down;     break;
        }
    }
    if (!key)
        return QKeySequence();
    if (mods & MOD_CONTROL) key |= Qt::CTRL;
    if (mods & MOD_SHIFT)   key |= Qt::SHIFT;
    if (mods & MOD_ALT)     key |= Qt::ALT;
    if (mods & MOD_WIN)     key |= Qt::META;
    return QKeySequence(key);
}

} // namespace

SettingsPanel::SettingsPanel(CrosshairWindow *crosshair)
    : QWidget(crosshair, Qt::Tool | Qt::FramelessWindowHint),
      m_ch(crosshair)
{
    setAttribute(Qt::WA_DeleteOnClose, false);
    setAttribute(Qt::WA_TranslucentBackground);
    setStyleSheet(R"(
        SettingsPanel{background:#f4f5f7;border:1px solid #dfe3ea;border-radius:10px;}
        QWidget{font-family:"Segoe UI","Microsoft YaHei";font-size:12px;color:#333;}
        QGroupBox{background:#ffffff;border:1px solid #e4e7ec;border-radius:10px;
                  margin-top:16px;padding:12px 8px 8px 8px;font-weight:bold;color:#222;}
        QGroupBox::title{subcontrol-origin:margin;subcontrol-position:top left;
                         left:12px;top:4px;padding:0 4px;background:#ffffff;}
        QLabel{background:transparent;border:none;font-weight:normal;}
        QSpinBox,QLineEdit,QKeySequenceEdit,QComboBox{
            background:#ffffff;border:1px solid #d5d9e0;border-radius:6px;
            padding:2px 6px;min-height:22px;selection-background-color:#4f7cff;}
        QSpinBox:focus,QLineEdit:focus,QKeySequenceEdit:focus,QComboBox:focus{
            border:1px solid #4f7cff;}
        QSpinBox::up-button,QSpinBox::down-button{width:14px;border:none;}
        QComboBox::drop-down{border:none;width:20px;}
        QComboBox QAbstractItemView{background:#ffffff;border:1px solid #d5d9e0;
            selection-background-color:#4f7cff;selection-color:#ffffff;outline:none;}
        QPushButton{background:#4f7cff;color:#ffffff;border:none;border-radius:6px;
                    padding:4px 12px;min-height:22px;}
        QPushButton:hover{background:#3f6af0;}
        QPushButton:pressed{background:#3458c9;}
        QSlider::groove:horizontal{height:4px;background:#dcdfe6;border-radius:2px;}
        QSlider::sub-page:horizontal{background:#4f7cff;border-radius:2px;}
        QSlider::handle:horizontal{width:14px;height:14px;margin:-5px 0;
            border-radius:7px;background:#ffffff;border:2px solid #4f7cff;}
        QCheckBox{spacing:4px;background:transparent;border:none;font-weight:normal;}
        QCheckBox::indicator{width:14px;height:14px;border:1px solid #c9cdd6;
            border-radius:4px;background:#ffffff;}
        QCheckBox::indicator:checked{background:#4f7cff;border-color:#4f7cff;
            image:url(:/icons/check.svg);}
        QLabel:disabled{color:#a0a4ac;}
        QSpinBox:disabled{background:#f0f1f4;color:#a0a4ac;}
        QSlider::groove:horizontal:disabled{background:#eceef2;}
        QSlider::handle:horizontal:disabled{border-color:#c9cdd6;}
    )");
    QVBoxLayout *lay = new QVBoxLayout(this);
    lay->setContentsMargins(6, 6, 6, 6);
    lay->setSpacing(6);
    lay->addWidget(makeHotkeyCard());
    lay->addWidget(makeDotCard());
    lay->addWidget(makeFocusCard());
    setFixedWidth(300);
    adjustSize();
}

QWidget *SettingsPanel::makeSliderRow(const QString &name, int min, int max,
                                      int value,
                                      const std::function<void(int)> &onChange)
{
    QWidget *w = new QWidget;
    QHBoxLayout *lay = new QHBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    QLabel *lab = new QLabel(name);
    lab->setFixedWidth(42);
    QSpinBox *spin = new QSpinBox;
    spin->setRange(min, max);
    spin->setValue(value);
    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setRange(min, max);
    slider->setValue(value);
    connect(spin, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            slider, &QSlider::setValue);
    connect(slider, &QSlider::valueChanged, spin, &QSpinBox::setValue);
    connect(slider, &QSlider::valueChanged, this,
            [onChange](int v) { onChange(v); });
    lay->addWidget(lab);
    lay->addWidget(spin);
    lay->addWidget(slider);
    return w;
}

QWidget *SettingsPanel::makeStyleRow(LayerSettings &layer,
                                     const QStringList &styles,
                                     QComboBox **outCombo)
{
    QWidget *w = new QWidget;
    QHBoxLayout *lay = new QHBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    QLabel *lab = new QLabel(QStringLiteral("样式"));
    lab->setFixedWidth(42);
    QComboBox *combo = new QComboBox;
    if (outCombo)
        *outCombo = combo;
    combo->addItems(styles);
    combo->setCurrentIndex(layer.style);
    QPushButton *imgBtn = new QPushButton(QStringLiteral("图片…"));
    connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, [this, &layer](int i) {
        layer.style = i;
        m_ch->applySettings();
    });
    connect(imgBtn, &QPushButton::clicked, this, [this, &layer, combo]() {
        const QString path = QFileDialog::getOpenFileName(
            this, QStringLiteral("选择图片"), QString(),
            QStringLiteral("图片文件 (*.png *.jpg *.jpeg *.bmp *.svg)"));
        if (path.isEmpty())
            return;
        layer.imagePath = path;
        layer.style = combo->count() - 1;
        combo->setCurrentIndex(layer.style);
        m_ch->applySettings();
    });
    lay->addWidget(lab);
    lay->addWidget(combo);
    lay->addWidget(imgBtn);
    return w;
}

QWidget *SettingsPanel::makeColorRow(LayerSettings &layer)
{
    QWidget *w = new QWidget;
    QHBoxLayout *lay = new QHBoxLayout(w);
    lay->setContentsMargins(0, 0, 0, 0);
    QLabel *lab = new QLabel(QStringLiteral("颜色"));
    lab->setFixedWidth(42);
    QPushButton *colorBtn = new QPushButton;
    colorBtn->setFixedWidth(48);
    QLineEdit *rgbEdit = new QLineEdit;
    rgbEdit->setPlaceholderText(QStringLiteral("R,G,B 或 #RRGGBB"));

    auto updateBtn = [colorBtn, rgbEdit](const QColor &c) {
        colorBtn->setStyleSheet(
            QString("background:%1;border:1px solid #c9cdd6;border-radius:6px;")
                .arg(c.name()));
        rgbEdit->setText(QString("%1,%2,%3").arg(c.red()).arg(c.green()).arg(c.blue()));
    };
    updateBtn(layer.color);

    connect(colorBtn, &QPushButton::clicked, this,
            [this, &layer, updateBtn]() {
        const QColor c = QColorDialog::getColor(layer.color, this,
                                                QStringLiteral("选择颜色"));
        if (!c.isValid())
            return;
        layer.color = c;
        updateBtn(c);
        m_ch->applySettings();
    });
    connect(rgbEdit, &QLineEdit::editingFinished, this,
            [this, &layer, rgbEdit, updateBtn]() {
        const QString t = rgbEdit->text().trimmed();
        QColor c;
        if (t.startsWith('#')) {
            c = QColor(t);
        } else {
            const QStringList parts = t.split(',');
            if (parts.size() == 3)
                c = QColor(parts[0].toInt(), parts[1].toInt(), parts[2].toInt());
        }
        if (c.isValid()) {
            layer.color = c;
            updateBtn(c);
            m_ch->applySettings();
        }
    });
    lay->addWidget(lab);
    lay->addWidget(colorBtn);
    lay->addWidget(rgbEdit);
    return w;
}

QGroupBox *SettingsPanel::makeDotCard()
{
    LayerSettings &l = m_ch->settings().dot;
    QGroupBox *box = new QGroupBox(QStringLiteral("点心层"));
    QVBoxLayout *lay = new QVBoxLayout(box);
    lay->addWidget(makeStyleRow(l, {QStringLiteral("圆"), QStringLiteral("方"),
                                    QStringLiteral("三角"), QStringLiteral("自定义图片")}));
    lay->addWidget(makeSliderRow(QStringLiteral("大小"), 1, 200, l.size,
        [this](int v) { m_ch->settings().dot.size = v; m_ch->applySettings(); }));
    lay->addWidget(makeSliderRow(QStringLiteral("自转"), -360, 360, l.rotation,
        [this](int v) { m_ch->settings().dot.rotation = v; m_ch->applySettings(); }));
    lay->addWidget(makeSliderRow(QStringLiteral("透明度"), 0, 100, l.opacity,
        [this](int v) { m_ch->settings().dot.opacity = v; m_ch->applySettings(); }));
    lay->addWidget(makeColorRow(l));
    return box;
}

QGroupBox *SettingsPanel::makeFocusCard()
{
    LayerSettings &l = m_ch->settings().focus;
    CrosshairSettings &s = m_ch->settings();
    QGroupBox *box = new QGroupBox(QStringLiteral("聚焦层"));
    QVBoxLayout *lay = new QVBoxLayout(box);
    QComboBox *styleCombo = nullptr;
    lay->addWidget(makeStyleRow(l, {">", "<", ")", "V", "-",
                                    QStringLiteral("自定义图片")}, &styleCombo));
    lay->addWidget(makeSliderRow(QStringLiteral("大小"), 1, 200, l.size,
        [this](int v) { m_ch->settings().focus.size = v; m_ch->applySettings(); }));
    QWidget *widthRow = makeSliderRow(QStringLiteral("粗细"), 1, 30, l.width,
        [this](int v) { m_ch->settings().focus.width = v; m_ch->applySettings(); });
    lay->addWidget(widthRow);
    // 自定义图片没有描边粗细可调
    widthRow->setEnabled(l.style != styleCombo->count() - 1);
    connect(styleCombo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            widthRow, [widthRow, styleCombo](int i) {
        widthRow->setEnabled(i != styleCombo->count() - 1);
    });
    lay->addWidget(makeSliderRow(QStringLiteral("自转"), -360, 360, l.rotation,
        [this](int v) { m_ch->settings().focus.rotation = v; m_ch->applySettings(); }));
    lay->addWidget(makeSliderRow(QStringLiteral("透明度"), 0, 100, l.opacity,
        [this](int v) { m_ch->settings().focus.opacity = v; m_ch->applySettings(); }));
    lay->addWidget(makeSliderRow(QStringLiteral("距离"), -300, 300, s.focusDistance,
        [this](int v) { m_ch->settings().focusDistance = v; m_ch->applySettings(); }));
    lay->addWidget(makeSliderRow(QStringLiteral("公转"), -360, 360, s.focusOrbit,
        [this](int v) { m_ch->settings().focusOrbit = v; m_ch->applySettings(); }));
    lay->addWidget(makeColorRow(l));

    QWidget *visRow = new QWidget;
    QHBoxLayout *vlay = new QHBoxLayout(visRow);
    vlay->setContentsMargins(0, 0, 0, 0);
    QLabel *lab = new QLabel(QStringLiteral("显隐"));
    lab->setFixedWidth(42);
    vlay->addWidget(lab);
    const QString names[4] = {QStringLiteral("上"), QStringLiteral("下"),
                              QStringLiteral("左"), QStringLiteral("右")};
    const int idx[4] = {0, 2, 3, 1}; // 界面顺序 上下左右 -> 内部顺序 上右下左
    for (int i = 0; i < 4; ++i) {
        QCheckBox *cb = new QCheckBox(names[i]);
        cb->setChecked(s.focusVisible[idx[i]]);
        connect(cb, &QCheckBox::toggled, this, [this, i, idx](bool on) {
            m_ch->settings().focusVisible[idx[i]] = on;
            m_ch->applySettings();
        });
        vlay->addWidget(cb);
    }
    lay->addWidget(visRow);
    return box;
}

QGroupBox *SettingsPanel::makeHotkeyCard()
{
    QGroupBox *box = new QGroupBox(QStringLiteral("快捷键"));
    QHBoxLayout *lay = new QHBoxLayout(box);
    lay->addWidget(new QLabel(QStringLiteral("打开/关闭状态栏")));
    QKeySequenceEdit *edit = new QKeySequenceEdit(
        hotkeyToKeySequence(m_ch->settings().hotkey));
    connect(edit, &QKeySequenceEdit::editingFinished, this, [this, edit]() {
        quint32 mods = 0, vk = 0;
        if (!keySequenceToHotkey(edit->keySequence(), mods, vk)) {
            QMessageBox::warning(this, QStringLiteral("快捷键"),
                                 QStringLiteral("不支持的按键，请使用字母、数字、F1-F24 或方向键。"));
            edit->setKeySequence(hotkeyToKeySequence(m_ch->settings().hotkey));
            return;
        }
        m_ch->settings().hotkey = (mods << 16) | vk;
        if (!m_ch->registerHotkey())
            QMessageBox::warning(this, QStringLiteral("快捷键"),
                                 QStringLiteral("快捷键注册失败，可能已被其他程序占用。"));
        m_ch->applySettings();
    });
    lay->addWidget(edit);
    return box;
}
