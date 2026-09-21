#ifndef SETTINGSPANEL_H
#define SETTINGSPANEL_H

#include <QWidget>
#include <functional>

class QComboBox;
class QGroupBox;
class QVBoxLayout;
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
    QGroupBox *makePresetCard();
    QGroupBox *makeHotkeyCard();
    QGroupBox *makeDotCard();
    QGroupBox *makeFocusCard();

    void onAppearanceChanged();      // 外观参数变更：刷新绘制、持久化并标记预设偏离
    void rebuildLayerCards();        // 应用预设后重建点心/聚焦卡片以刷新控件值
    void refreshPresetList();
    void updatePresetComboTexts();
    void savePreset();
    void deletePreset();
    void applyPreset(int comboIndex);

    CrosshairWindow *m_ch;
    QVBoxLayout *m_lay;
    QGroupBox *m_dotCardBox;
    QGroupBox *m_focusCardBox;
    QComboBox *m_presetCombo;
    QString m_currentPreset;         // 当前应用的预设名，空为未应用
    bool m_presetDirty = false;      // 应用预设后参数被修改
};

#endif // SETTINGSPANEL_H
