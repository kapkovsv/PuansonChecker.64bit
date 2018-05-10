#include "photocamera.h"
#include "puansonchecker.h"

#include <QThread>

using namespace std;

LPMAIDEntryPointProc	g_pMAIDEntryPoint = NULL;
UCHAR	g_bFileRemoved = false;
ULONG	g_ulCameraType = 0;	// CameraType

HINSTANCE	g_hInstModule = NULL;

char g_szCapturedImageFileName[] = CAPTURED_CURRENT_IMAGE_FILENAME;

bool PhotoCamera::SelectSource( LPRefObj pRefObj, ULONG *pulSrcID )
{
    BOOL	bRet;
    NkMAIDEnum	stEnum;
    LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, kNkMAIDCapability_Children );
    if ( pCapInfo == NULL )
        return false;

    // check data type of the capability
    if ( pCapInfo->ulType != kNkMAIDCapType_Enum )
        return false;

    // check if this capability suports CapGet operation.
    if ( !CheckCapabilityOperation( pRefObj, kNkMAIDCapability_Children, kNkMAIDCapOperation_Get ) )
        return false;

    bRet = Command_CapGet( pRefObj->pObject, kNkMAIDCapability_Children, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL );
    if( bRet == false )
        return false;

    // check the data of the capability.
    if ( stEnum.wPhysicalBytes != 4 )
        return false;

    if ( stEnum.ulElements == 0 )
        return false;

    // allocate memory for array data
    stEnum.pData = malloc( stEnum.ulElements * stEnum.wPhysicalBytes );
    if ( stEnum.pData == NULL )
        return false;

    // get array data
    bRet = Command_CapGetArray( pRefObj->pObject, kNkMAIDCapability_Children, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL );
    if( bRet == false ) {
        free( stEnum.pData );
        return false;
    }

    *pulSrcID = ((ULONG*)stEnum.pData)[0];
    free( stEnum.pData );

    return true;
}

bool PhotoCamera::SelectItem(tagRefObj* pRefObj, ULONG *pulItemID)
{
    BOOL	bRet;
    NkMAIDEnum	stEnum;

    LPNkMAIDCapInfo pCapInfo = GetCapInfo( pRefObj, kNkMAIDCapability_Children );
    if ( pCapInfo == NULL )
        return false;

    // check data type of the capability
    if ( pCapInfo->ulType != kNkMAIDCapType_Enum )
        return false;
    // check if this capability suports CapGet operation.
    if ( !CheckCapabilityOperation( pRefObj, kNkMAIDCapability_Children, kNkMAIDCapOperation_Get ) )
        return false;

    bRet = Command_CapGet(pRefObj->pObject, kNkMAIDCapability_Children, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL );
    if( bRet == false )
        return false;

    // check the data of the capability.
    if ( stEnum.ulElements == 0 ) {
        qDebug(qPrintable("No images returned"));
        return false;
    }

    // check the data of the capability.
    if ( stEnum.wPhysicalBytes != 4 )
        return false;

    // allocate memory for array data
    stEnum.pData = malloc( stEnum.ulElements * stEnum.wPhysicalBytes );
    if ( stEnum.pData == NULL )
        return false;
    // get array data
    bRet = Command_CapGetArray(pRefObj->pObject, kNkMAIDCapability_Children, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL );
    if( bRet == false ) {
        free( stEnum.pData );
        return false;
    }

    *pulItemID = ((ULONG*)stEnum.pData)[0];
    free( stEnum.pData );
    return true;
}

bool PhotoCamera::Connect()
{
    BOOL bRet = true;

    if(connection_status == false)
    {
        // Select Device
        bRet = SelectSource( pRefMod, &ulSrcID );
        if ( bRet == false )
        {
            qDebug(qPrintable( "SelectSource error" ));
            error_message = "Физических устройств не обнаружено";
            return false;
        }

        pRefSrc = GetRefChildPtr_ID( pRefMod, ulSrcID );
        if ( pRefSrc == NULL ) {
            // Create Source object and RefSrc structure.
            if ( AddChild( pRefMod, ulSrcID ) == TRUE ) {
                qDebug(qPrintable("Source object is opened.\n"));
            } else {
                qDebug(qPrintable("Source object can't be opened.\n"));
                error_message = "Физическое устройство не может быть открыто";
                return false;
            }

            pRefSrc = GetRefChildPtr_ID( pRefMod, ulSrcID );
        }

        //qDebug() << endl << endl;

        // Get CameraType
        Command_CapGet( pRefSrc->pObject, kNkMAIDCapability_CameraType, kNkMAIDDataType_UnsignedPtr, (NKPARAM)&g_ulCameraType, NULL, NULL );

        // Get Name
        NkMAIDString name;
        Command_CapGet(pRefSrc->pObject, kNkMAIDCapability_Name, kNkMAIDDataType_StringPtr, (NKPARAM)&name, NULL, NULL);
        camera_model = QString((const char *) name.str);

        // LockCamera flag
        BYTE bLockCameraFlag;

        // LockCamera Capability
        bRet = Command_CapGet( pRefSrc->pObject, kNkMAIDCapability_LockCamera, kNkMAIDDataType_BooleanPtr, (NKPARAM)&bLockCameraFlag, NULL, NULL );

        //qDebug() << "bRet " << bRet << " kNkMAIDCapability_LockCamera value " << (int)bLockCameraFlag << " !!!" << endl;

        if(bLockCameraFlag == false)
        {
            bRet = Command_CapSet( pRefSrc->pObject, kNkMAIDCapability_LockCamera, kNkMAIDCapType_Boolean, (NKPARAM)true, NULL, NULL );
            if ( bRet == false )
            {
                qDebug(qPrintable( "LockCamera Set. An Error occured.\n" ));
                error_message = "Невозможно заблокировать камеру";
                return false;
            }
        }

        //qDebug() << "kNkMAIDCapability_LockCamera Command_CapSet bRet " << bRet << endl;
        //qDebug() << endl << endl;
        // ---------------------

        // ShootingMode
        NkMAIDEnum	stEnum;

        //bRet = Command_CapGet( pRefSrc->pObject, kNkMAIDCapability_ShootingMode, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL );
        //qDebug(qPrintable( "bRet " << bRet << " kNkMAIDCapability_ShootingMode value " << (int)stEnum.ulValue << " !!!" ));;

        //stEnum.ulType = kNkMAIDArrayType_Unsigned;
        stEnum.ulValue = eNkMAIDShootingMode_S;

        // send the selected number
        bRet = Command_CapSet( pRefSrc->pObject, kNkMAIDCapability_ShootingMode, kNkMAIDDataType_EnumPtr, (NKPARAM)&stEnum, NULL, NULL );
        if ( bRet == false )
        {
            qDebug(qPrintable( "ShootingMode Set. An Error occured.\n" ));
            error_message = "Невозможно установить режим съёмки";
            return false;
        }
        // ------------

        // SaveMedia
        // Настроено сохранение фотоснимков в SDRAM, поэтому после совершения снимков необходимо загружать их на ПК,
        //                                      в противном случае SDRAM будет занята и сделать снимок будет нельзя
        bRet = Command_CapSet( pRefSrc->pObject, kNkMAIDCapability_SaveMedia, kNkMAIDDataType_Unsigned, (NKPARAM)kNkMAIDSaveMedia_SDRAM, NULL, NULL );
        if ( bRet == false )
        {
            qDebug(qPrintable( "SaveMedia Set. An Error occured.\n" ));
            error_message = "Невозможно установить место сохранения изображений";
            return false;
        }
        // ---------

        error_message = "";
        connection_status = true;

        emit cameraConnectionStatusChanged(connection_status);
    }

    return true;
}

bool PhotoCamera::Disconnect()
{
    BOOL	bRet;

    // Close Source_Object
    bRet = RemoveChild( pRefMod, ulSrcID );
    if( bRet == false ) {
        error_message = "Невозможно завершить работу с камерой";
        return false;
    }
    pRefSrc = NULL;

    if(connection_status == true)
    {
        connection_status = false;
        camera_model = "";

        emit cameraConnectionStatusChanged(connection_status);
    }

    return true;
}

bool PhotoCamera::NikonSDKModuleLoad()
{
    char	ModulePath[MAX_PATH];
    BOOL	bRet;

    // Search for a Module-file like "Type0006.md3".
    bRet = Search_Module( ModulePath );

    if ( bRet == false ) {
        qDebug(qPrintable( "\"Type0006 Module\" is not found.\n" ));
        error_message = "Модуль Type0006 не найден";
        return false;
    }

    // Load the Module-file.
    bRet = Load_Module( ModulePath );
    if ( bRet == false ) {
        qDebug(qPrintable( "Failed in loading \"Type0006 Module\".\n" ));
        error_message = "Проблемы с загрузкой модуля Type0006";
        return false;
    }

    // Allocate memory for reference to Module object.
    pRefMod = (LPRefObj)malloc(sizeof(RefObj));
    if ( pRefMod == NULL ) {
        qDebug(qPrintable( "There is not enough memory." ));
        error_message = "Недостаточно памяти для загрузки модуля Type0006";
        return false;
    }
    InitRefObj( pRefMod );

    // Allocate memory for Module object.
    pRefMod->pObject = (LPNkMAIDObject)malloc(sizeof(NkMAIDObject));
    if ( pRefMod->pObject == NULL ) {
        qDebug(qPrintable( "There is not enough memory." ));
        error_message = "Недостаточно памяти для загрузки модуля Type0006";
        if ( pRefMod != NULL )
            free( pRefMod );
        return false;
    }

    //	Open Module object
    pRefMod->pObject->refClient = (NKREF)pRefMod;
    bRet = Command_Open(	NULL,					// When Module_Object will be opend, "pParentObj" is "NULL".
                                pRefMod->pObject,	// Pointer to Module_Object
                                ulModID );			// Module object ID set by Client
    if ( bRet == false ) {
        qDebug(qPrintable( "Module object can't be opened.\n" ));
        error_message = "Модуль Type0006 не может быть открыт";
        if ( pRefMod->pObject != NULL )
            free( pRefMod->pObject );
        if ( pRefMod != NULL )
            free( pRefMod );
        return false;
    }

    //	Enumerate Capabilities that the Module has.
    bRet = EnumCapabilities( pRefMod->pObject, &(pRefMod->ulCapCount), &(pRefMod->pCapArray), NULL, NULL );
    if ( bRet == false ) {
        qDebug(qPrintable( "Failed in enumeration of capabilities." ));
        error_message = "Ошибка при инициализации камеры";
        if ( pRefMod->pObject != NULL )
            free( pRefMod->pObject );
        if ( pRefMod != NULL )
            free( pRefMod );
        return false;
    }

    //	Set the callback functions(ProgressProc, EventProc and UIRequestProc).
    bRet = SetProc( pRefMod );
    if ( bRet == false ) {
        qDebug(qPrintable( "Failed in setting a call back function." ));
        error_message = "Ошибка при инициализации камеры";
        if ( pRefMod->pObject != NULL )
            free( pRefMod->pObject );
        if ( pRefMod != NULL )
            free( pRefMod );
        return false;
    }

    //	Set the kNkMAIDCapability_ModuleMode.
    if( CheckCapabilityOperation( pRefMod, kNkMAIDCapability_ModuleMode, kNkMAIDCapOperation_Set )  ){
        bRet = Command_CapSet( pRefMod->pObject, kNkMAIDCapability_ModuleMode, kNkMAIDDataType_Unsigned,
                                        (NKPARAM)kNkMAIDModuleMode_Controller, NULL, NULL);
        if ( bRet == false ) {
            qDebug(qPrintable( "Failed in setting kNkMAIDCapability_ModuleMode." ));
            error_message = "Ошибка при инициализации камеры";
            return false;
        }
    }

    nikon_sdk_module_loaded = true;

    return true;
}

bool PhotoCamera::NikonSDKModuleUnload()
{
    BOOL	bRet;

    // Close Module_Object
    bRet = Close_Module( pRefMod );
    if ( bRet == false )
    {
        qDebug() << "Module object can not be closed.\n";
        error_message = "Модуль Type0006 не может быть закрыт";
        return false;
    }

    // Unload Module
    if(FreeLibrary( g_hInstModule ) == 0)
    {
        error_message = "Работа с библиотекой модуля Type0006 не может завершена";
        return false;
    }

    g_hInstModule = NULL;

    // Free memory blocks allocated in this function.
    if ( pRefMod->pObject != NULL )
        free( pRefMod->pObject );

    if ( pRefMod != NULL )
        free( pRefMod );

    nikon_sdk_module_loaded = false;

    return true;
}

PhotoCamera::PhotoCamera()
{
    bool res;

    nikon_sdk_module_loaded = false;

    camera_model = "";
    connection_status = false;

    pRefMod = NULL, pRefSrc = NULL, pRefItm = NULL, pRefDat = NULL;
    ulModID = 0, ulSrcID = 0;

    error_message = "";

    res = NikonSDKModuleLoad();
    if(res == true)
    {
        res = Connect();
        if(res == false)
            qDebug() << "Connect error";
    }
    else
        qDebug() << "Module load error";
}

PhotoCamera::~PhotoCamera()
{
    bool res;

    res = Disconnect();
    if(res == false)
        qDebug() << "Disconnect error";

    res = NikonSDKModuleUnload();
    if(res == false)
        qDebug() << "Module unload error";
}

bool PhotoCamera::CaptureImage()
{
    BOOL	bRet;

    // Capture
    bRet = IssueProcess( pRefSrc, kNkMAIDCapability_Capture );
    if ( bRet == false ) {
        qDebug(qPrintable( "Capture IssueProcess. An Error occured.\n" ));
        error_message = "Ошибка при фотосъёмке";
        return false;
    }

    bRet = Command_Async( pRefSrc->pObject );
    if ( bRet == false ) {
        qDebug(qPrintable( "Capture Command_Async. An Error occured.\n" ));
        error_message = "Ошибка при фотосъёмке";
        return false;
    }

    // Нужна задержка 1 сек. после посылки kNkMAIDCapability_Capture
    QThread::sleep(1);
    // -------

    return true;
}

bool PhotoCamera::AcquireImage()
{
    bool success = true;
    ULONG ulItemID;

    BOOL	bRet;

    success = PhotoCamera::SelectItem(pRefSrc, &ulItemID);
    if(!success) {
        qDebug(qPrintable("Image item not found."));
        error_message = "Изображение не найдено в SDRAM камеры";
        return false;
    }

    pRefItm = GetRefChildPtr_ID(pRefSrc, ulItemID);
    if (pRefItm == NULL)
    {
        if ((success = AddChild(pRefSrc, ulItemID)) != true) {
            qDebug(qPrintable("Item object can't be opened."));
        }
        else {
            pRefItm = GetRefChildPtr_ID(pRefSrc, ulItemID);
        }
    }

    if(success)
    {
        pRefDat = GetRefChildPtr_ID( pRefItm, kNkMAIDDataObjType_Image );
        if (pRefDat == NULL)
        {
            if ((success = AddChild(pRefItm, kNkMAIDDataObjType_Image)) != true) {
                qDebug(qPrintable("Image object can't be opened."));
            }
            else {
                pRefDat = GetRefChildPtr_ID(pRefItm, kNkMAIDDataObjType_Image);
            }
        }
    }

    if(success)
    {
        qDebug(qPrintable("Issue image acquire"));

        g_bFileRemoved = false;

        success = IssueAcquire(pRefDat);
        if (!success) {
            qDebug(qPrintable("Unable to acquire image."));
            error_message = "Изображение не может быть загружено из SDRAM камеры";
        } else {
            qDebug(qPrintable("Image acquired"));
        }
    }
    else
    {
        error_message = "Изображение не найдено в SDRAM камеры";
    }

    // Для повторной загрузки фотографий нужно обязательно удалять pRefDat(изображение) и pRefItm(Item)
    if ( g_bFileRemoved || !success ) {
        // Close Image Object
        bRet = RemoveChild( pRefItm, kNkMAIDDataObjType_Image );
        if(bRet == false)
        {}
        pRefDat = NULL;

        // Close Item
        bRet = RemoveChild( pRefSrc, ulItemID );
        if(bRet == false)
        {}
        pRefItm = NULL;
    }

    QThread::sleep(1);

    return true;
}

bool PhotoCamera::CaptureAndAcquireImage()
{
    if(CaptureImage() != true)
        return false;

    // Настроено сохранение фотоснимков в SDRAM, поэтому после совершения снимков необходимо загружать их на ПК,
    //                                      в противном случае SDRAM будет занята и сделать снимок будет нельзя
    if(AcquireImage() != true)
        return false;

    return true;
}

QString PhotoCamera::getCameraStatus()
{
    QString status;

    if(!nikon_sdk_module_loaded)
    {
        status = "Модуль для роботы с камерой не загружен";
    }
    else
    {
        if(!isCameraConnected())
        {
            if(!error_message.isEmpty())
                status = error_message;
            else
                status = "Камера не подключена";
        }
        else
        {
            status = "Камера " + getCameraModel() + " готова к использованию";
        }
    }

    return status;
}
