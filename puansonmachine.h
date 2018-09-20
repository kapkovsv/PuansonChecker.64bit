#ifndef PUANSONMACHINE_H
#define PUANSONMACHINE_H

#include <QObject>
#include <QMap>
#include <QPoint>

#include <memory>
#include <type_traits>

#if defined(Q_OS_WIN)
#include <windows.h>
#include "MachineController/MachineController.h"

#define IMAGE_PATH "mach_controller.elf"
#endif // Q_OS_WIN

class PuansonMachine : public QObject
{
    Q_OBJECT

public:
    explicit PuansonMachine(QObject *parent = 0);

    bool moveToAnglePosition(const quint8 etalon_research_active_angle);

    inline bool isReady() const
    {
        return machine_ready;
    }

    inline quint8 getCurrentAngle() const
    {
        return current_angle;
    }

    inline QPoint getAnglePosition(const quint8 angle) const
    {
        return angles_position_map[angle];
    }

    inline QMap<quint8, QPoint> getAnglePositions() const
    {
        return angles_position_map;
    }

    bool setAnglePositions(const QMap<quint8, QPoint> &new_positions);
    bool setAnglePosition(const quint8 angle, const QPoint &new_pos);

signals:

public slots:

private:
#if defined(Q_OS_WIN)
    std::unique_ptr<MachineController::MachineController> controller;
#endif // Q_OS_WIN
    QMap<quint8, QPoint> angles_position_map;
    quint8 current_angle = 1;
    bool machine_ready = false;
};

#endif // PUANSONMACHINE_H
