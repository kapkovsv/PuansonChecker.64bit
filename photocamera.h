#ifndef PHOTOCAMERA_H
#define PHOTOCAMERA_H

#include <QObject>
#include <QtGlobal>
#include <QDebug>

#if defined(Q_OS_WIN)
#include "nikon/Maid3.h"
#include "nikon/Maid3d1.h"
#include "nikon/CtrlSample.h"
#endif // defined(Q_OS_WIN)

class PhotoCamera : public QObject
{
    Q_OBJECT

public:
    PhotoCamera();
    ~PhotoCamera() Q_DECL_OVERRIDE;

    bool Connect();
    bool Disconnect();

    bool CaptureAndAcquireImage(const char *captured_file_name);

    inline QString getCameraModel() const
    {
        return camera_model;
    }

    inline bool isCameraConnected() const
    {
        return connection_status;
    }

    QString getCameraStatus() const;

signals:
    void cameraConnectionStatusChanged(bool connected);

private:
#if defined(Q_OS_WIN)
    LPRefObj pRefMod = Q_NULLPTR, pRefSrc = Q_NULLPTR, pRefItm = Q_NULLPTR, pRefDat = Q_NULLPTR;
    ULONG ulModID, ulSrcID;
#endif // defined(Q_OS_WIN)
    QString camera_model;
    bool connection_status;
    bool nikon_sdk_module_loaded;
    QString error_message;

#if defined(Q_OS_WIN)
    static bool SelectSource( LPRefObj pRefObj, ULONG *pulSrcID );
    static bool SelectItem(tagRefObj* pRefObj, unsigned long *pulItemID);
#endif // defined(Q_OS_WIN)

    bool NikonSDKModuleUnload();
    bool NikonSDKModuleLoad();

    bool CaptureImage();
    bool AcquireImage();
};

#endif // PHOTOCAMERA_H
