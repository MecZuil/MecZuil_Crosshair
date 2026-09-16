#ifndef SETTINGSPANEL_H
#define SETTINGSPANEL_H

#include <QWidget>
#include <functional>

class QComboBox;
class QGroupBox;
class CrosshairWindow;
struct LayerSettings;

class SettingsPanel : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPanel(CrosshairWindow *crosshair);

private:
    QWidget *makeSliderRow(const QString &name, int min, int max, int value,
                           const std::function<void(int)> &onChange);
    QWidget *makeColorRow(LayerSettings &layer);
    QWidget *makeStyleRow(LayerSettings &layer, const QStringList &styles,
                          QComboBox **outCombo = nullptr);
    QGroupBox *makeDotCard();
    QGroupBox *makeFocusCard();
    QGroupBox *makeHotkeyCard();

    CrosshairWindow *m_ch;
};

#endif // SETTINGSPANEL_H
