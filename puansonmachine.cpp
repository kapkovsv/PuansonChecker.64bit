#include "puansonchecker.h"
#include "puansonmachine.h"

#include <QDebug>

#if defined(Q_OS_WIN)
template<class T, class D> auto make_unique(T* p, D&& d) {
    return std::unique_ptr<T, typename std::remove_reference<D>::type>(p, std::forward<D>(d));
}
template<class T> auto make_unique(T* p) {
    return std::unique_ptr<T>(p);
}
#endif // defined(Q_OS_WIN)

PuansonMachine::PuansonMachine(QObject *parent) : QObject(parent)
{
#if defined(Q_OS_WIN)
    try
    {
        auto en = make_unique(CreateDeviceEnumerator(guidSTLink));
        auto deleter = [](LPSTR p) { free(p); };
        if ( auto path = make_unique(en->GetNext(), deleter) )
        {
            qDebug() << "device path: " << path.get();

            try
            {
                controller = make_unique(CreateMachineController(path.get()));
            }
            catch(const Win32Error &ex)
            {
                qDebug() << "create MachineController object error. Error code: " << ex.ErrorCode;
                return;
            }

            try
            {
                try
                {
                    controller->LoadElf(IMAGE_PATH);
                }
                catch(const Win32Error &ex)
                {
                    qDebug() << "ELF loading error. Error code: " << ex.ErrorCode;
                    throw;
                }
                catch(const ElfFormatException &)
                {
                    qDebug() << "Wrong ELF format";
                    throw;
                }
            }
            catch(const MachineControllerSpace::Exception &)
            {
                return;
            }

            try
            {
                try
                {
                    controller->WaitCompletion();
                    controller->FindReferencePos(1);
                }
                catch(const IncompleteTransfer &ex)
                {
                    qDebug() << "USB transfer error transfered bytes: " << ex.cbActual << " expected bytes: " << ex.cbExpected;
                    throw;
                }
                catch(const Win32Error &ex)
                {
                    qDebug() << "Error code: " << ex.ErrorCode;
                    throw;
                }
            }
            catch(const MachineControllerSpace::Exception &)
            {
                return;
            }
        }
    }
    catch(const EnumerationException &ex)
    {
        qDebug() << "Device enumeration error. Error code: " << ex.ErrorCode;
        return;
    }

    machine_ready = true;
#endif // defined(Q_OS_WIN)

    for(quint8 angle = 1; angle <= NUMBER_OF_ETALON_ANGLES; angle++)
        angles_position_map.insert(angle, QPoint());
}

bool PuansonMachine::moveToAnglePosition(const quint8 new_angle)
{
    if(new_angle >= 1 && new_angle <= NUMBER_OF_ETALON_ANGLES)
    {
#if defined(Q_OS_WIN)
        if(new_angle == current_angle)
        {
            return true;
        }
        else if(machine_ready)
        {
            try
            {
                try
                {
                    controller->MoveTo(angles_position_map[new_angle].x(), angles_position_map[new_angle].y());
                    controller->WaitCompletion();
                }
                catch(const IncompleteTransfer &ex)
                {
                    qDebug() << "USB transfer error transfered bytes: " << ex.cbActual << " expected bytes: " << ex.cbExpected;
                    throw;
                }
                catch(const Win32Error &ex)
                {
                    qDebug() << "Error code: " << ex.ErrorCode;
                    throw;
                }
            }
            catch(const MachineControllerSpace::Exception &)
            {
                return false;
            }

            current_angle = new_angle;

            return true;
        }
#endif // defined(Q_OS_WIN)
    }

    return false;
}

bool PuansonMachine::setAnglePositions(const QMap<quint8, QPoint> &new_positions)
{
    if(new_positions.size() == NUMBER_OF_ETALON_ANGLES)
    {
        bool all_angles_finded = true;

        for(quint8 angle = 1; angle <= NUMBER_OF_ETALON_ANGLES; angle++)
        {
            if(new_positions.find(angle) == new_positions.end())
            {
                all_angles_finded = false;
                break;
            }
        }

        if(all_angles_finded)
        {
            angles_position_map = new_positions;
            return true;
        }
    }
    return false;
}

bool PuansonMachine::setAnglePosition(const quint8 angle, const QPoint &new_pos)
{
    if(angle >= 1 && angle <= NUMBER_OF_ETALON_ANGLES)
    {
        angles_position_map[angle] = new_pos;

        return true;
    }

    return false;
}
