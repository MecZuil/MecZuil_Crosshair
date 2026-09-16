#ifndef CROSSHAIRSETTINGS_H
#define CROSSHAIRSETTINGS_H

#include <QColor>
#include <QPoint>
#include <QString>

struct LayerSettings
{
    int style = 0;                       // 内置样式索引，最后一项为自定义图片
    int size = 20;                       // 像素
    int rotation = 0;                    // 静态旋转角度（度）
    int width = 3;                       // 描边粗细（像素），仅聚焦层描边样式使用
    int opacity = 100;                   // 0-100
    QColor color = QColor(255, 0, 0);
    QString imagePath;                   // 自定义图片路径
};

struct CrosshairSettings
{
    LayerSettings dot;
    LayerSettings focus;
    int focusDistance = 10;              // 聚焦块近端到点心边缘的距离，可为负
    int focusOrbit = 0;                  // 聚焦块绕点心的公转角度（度）
    bool focusVisible[4] = {true, true, true, true}; // 顺序：上、右、下、左
    QPoint windowPos = QPoint(-1, -1);
    quint32 hotkey = 0x0078;             // 高 16 位 MOD_*，低 16 位 VK；默认 F9

    static QString iniPath();
    void load();
    void save() const;
};

#endif // CROSSHAIRSETTINGS_H
