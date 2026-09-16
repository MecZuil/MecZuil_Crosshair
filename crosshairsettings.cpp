#include "crosshairsettings.h"

#include <QCoreApplication>
#include <QSettings>

QString CrosshairSettings::iniPath()
{
    return QCoreApplication::applicationDirPath() + "/crosshair.ini";
}

static void loadLayer(QSettings &s, const QString &group, LayerSettings &l)
{
    s.beginGroup(group);
    l.style = s.value("style", l.style).toInt();
    l.size = s.value("size", l.size).toInt();
    l.rotation = s.value("rotation", l.rotation).toInt();
    l.width = s.value("width", l.width).toInt();
    l.opacity = s.value("opacity", l.opacity).toInt();
    l.color = QColor(s.value("color", l.color.name()).toString());
    l.imagePath = s.value("image", l.imagePath).toString();
    s.endGroup();
}

static void saveLayer(QSettings &s, const QString &group, const LayerSettings &l)
{
    s.beginGroup(group);
    s.setValue("style", l.style);
    s.setValue("size", l.size);
    s.setValue("rotation", l.rotation);
    s.setValue("width", l.width);
    s.setValue("opacity", l.opacity);
    s.setValue("color", l.color.name());
    s.setValue("image", l.imagePath);
    s.endGroup();
}

void CrosshairSettings::load()
{
    QSettings s(iniPath(), QSettings::IniFormat);
    loadLayer(s, "dot", dot);
    loadLayer(s, "focus", focus);
    focusDistance = s.value("focus/distance", focusDistance).toInt();
    focusOrbit = s.value("focus/orbit", focusOrbit).toInt();
    const QString vis = s.value("focus/visible", "1,1,1,1").toString();
    const QStringList parts = vis.split(',');
    for (int i = 0; i < 4 && i < parts.size(); ++i)
        focusVisible[i] = parts[i].toInt() != 0;
    windowPos = s.value("window/pos", windowPos).toPoint();
    hotkey = s.value("window/hotkey", hotkey).toUInt();
}

void CrosshairSettings::save() const
{
    QSettings s(iniPath(), QSettings::IniFormat);
    saveLayer(s, "dot", dot);
    saveLayer(s, "focus", focus);
    s.setValue("focus/distance", focusDistance);
    s.setValue("focus/orbit", focusOrbit);
    s.setValue("focus/visible", QString("%1,%2,%3,%4")
               .arg(focusVisible[0]).arg(focusVisible[1])
               .arg(focusVisible[2]).arg(focusVisible[3]));
    s.setValue("window/pos", windowPos);
    s.setValue("window/hotkey", hotkey);
}
