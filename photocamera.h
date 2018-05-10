#ifndef PHOTOCAMERA_H
#define PHOTOCAMERA_H

#include <QObject>
#include <QtGlobal>
#include <QDebug>

#include "nikon/Maid3.h"
#include "nikon/Maid3d1.h"
#include "nikon/CtrlSample.h"

class PhotoCamera : public QObject
{
    Q_OBJECT

public:
    PhotoCamera();
    ~PhotoCamera();

    bool Connect();
    bool Disconnect();

    bool CaptureAndAcquireImage(const char *captured_file_name);

    inline QString getCameraModel()
    {
        return camera_model;
    }

    inline bool isCameraConnected()
    {
        return connection_status;
    }

    QString getCameraStatus();

signals:
    void cameraConnectionStatusChanged(bool connected);

private:
    LPRefObj pRefMod = NULL, pRefSrc = NULL, pRefItm = NULL, pRefDat = NULL;
    ULONG ulModID, ulSrcID;
    QString camera_model;
    bool connection_status;
    bool nikon_sdk_module_loaded;
    QString error_message;

    static bool SelectSource( LPRefObj pRefObj, ULONG *pulSrcID );
    static bool SelectItem(tagRefObj* pRefObj, unsigned long *pulItemID);

    bool NikonSDKModuleUnload();
    bool NikonSDKModuleLoad();

    bool CaptureImage();
    bool AcquireImage();
};

#endif // PHOTOCAMERA_H
