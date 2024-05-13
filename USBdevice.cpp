#include "extractor_interface.h"
#include "usb_device_service.h"
//#include <stdbool.h>
//#include <dbus/dbus.h>

#include "player_logger.h"
#include <CommonAPI/CommonAPI.hpp>

namespace lge {
namespace mm {

#define MAX_DBUS_TIMEOUT 5 //seconds
#define AUTO_MOUNT_SERVICE  "com.lge.car.automount"
#define AUTO_MOUNT_OBJECT_PATH  "/com/lge/car/automount"
#define AUTO_MOUNT_INTERFACE "com.lge.car.automount"

std::function<void(MmError *)> *onUSBConnectedCb = NULL;

static void
onUsbMountedCb (GDBusConnection* connection,
               const gchar* sender_name,
               const gchar* object_path,
               const gchar* interface_name,
               const gchar* signal_name,
               GVariant* parameters,
               gpointer user_data) {

    MMLogInfo("");
    USBService *_this = ((USBService *) user_data);
    if (!g_strcmp0(signal_name, "UsbMount") || !g_strcmp0(signal_name, "UsbUnMount")) {
        MMLogInfo("%s is received \n", signal_name);

        // To do: GVariant parameters are depend on model USB-automount sturcture..
        const gchar *path = NULL, *label = NULL, *productName = NULL;
        guint32 port = 0;
        const gchar *temp1 = NULL; gint32 temp2 = 0; // For B-AVN USB-automount
        g_variant_get(parameters, "(sisssi)", // Need to change
            &path,
            &port,
            &label,
            &productName, &temp1, &temp2);

        MMLogInfo("Device info is [%s],[%u],[%s],[%s]\n", path, port, label, productName);
        if (!g_strcmp0(signal_name, "UsbMount"))
           _this->usbMountSig(std::string(path? path:""), port, std::string(label? label:""), std::string(productName? productName:""));
        else
            _this->usbUnMountSig(std::string(path), port, std::string(label), std::string(productName));
    }
}

static
void onNameAppeared (GDBusConnection *connection,
                     const gchar     *name,
                     const gchar     *name_owner,
                     void            *user_data) {
    MMLogInfo("Found automount on D-Bus\n");
    ((USBService *) user_data)->mConnection = connection;
    ((USBService *) user_data)->signalSubscribe();
    ((USBService *) user_data)->requestUsbList();
    (*onUSBConnectedCb) (NULL);
}

static
void onNameVanished (GDBusConnection *connection,
                     const gchar     *name,
                     void        *user_data) {
    MMLogError ("Failed to get name owner for %s\n", name);
    (*onUSBConnectedCb) (new MmError("Failed to connect to automount, is automount running?"));
}

USBService::USBService()
  : mWatcherId(0),
    mConnection(NULL),
    mUsbMountedHandlerId(0),
    mUsbUnmountedHandlerId(0) {

}

USBService::~USBService() {
    g_object_unref(mConnection);
    g_bus_unwatch_name (mWatcherId);
}

bool USBService::connectUSBService() {
    MMLogInfo("");
    onUSBConnectedCb = new std::function<void(MmError* e)>([&](MmError *e){
        if (!e) {
            return true;
        } else {
            MMLogError("Error connecting to : %s", e->message.c_str());
            return false;
        }
    });

    mWatcherId = g_bus_watch_name (G_BUS_TYPE_SYSTEM,
                                    AUTO_MOUNT_SERVICE,
                                    G_BUS_NAME_WATCHER_FLAGS_NONE,
                                    onNameAppeared,
                                    onNameVanished,
                                    this,
                                    NULL);

#if 0
    const std::string domain("local");
    const std::string instance("commonapi.examples.devicemanager");
    const std::string connection("Devicemanager");

    std::shared_ptr<CommonAPI::Runtime> runtime = CommonAPI::Runtime::get();
    mDevicemanagerProxy = runtime->buildProxy<v0_1::commonapi::examples::devicemanagerProxy>(domain, instance, connection);

    MMLogInfo("devicemanager_proxy->isAvailable() = %d", mDevicemanagerProxy->isAvailable());
    while (!mDevicemanagerProxy->isAvailable()) {
        usleep(10);
    }
    MMLogInfo("devicemanager_proxy->isAvailable() = %d", mDevicemanagerProxy->isAvailable());

    mDevicemanagerProxy->getOnDeviceAddedSelectiveEvent().subscribe([&](const int32_t& deviceId){
        MMLogInfo("Device Added : %d", deviceId);
    });

    mDevicemanagerProxy->getOnDeviceRemovedSelectiveEvent().subscribe([&](const int32_t& deviceId){
        MMLogInfo("Device Removed : %d", deviceId);
    });

    mDevicemanagerProxy->getOnChildDeviceAddedSelectiveEvent().subscribe([&](const int32_t& deviceId, const int32_t& parentId){
        MMLogInfo("Child Device Added : deviceId = %d, parentId = %d", deviceId, parentId);
    });

    mDevicemanagerProxy->getOnMsdMountedSelectiveEvent().subscribe([&](const int32_t& deviceId, const std::string& mountPath){
        std::string msdPath = "usb:"+mountPath;
        MMLogInfo("MsdDeviceMouted : deviceId = %d, msdPath = %s", deviceId, msdPath.c_str());
    });

    mDevicemanagerProxy->getOnMsdMountedWithDataSelectiveEvent().subscribe([&](const std::string& mountPath, const int32_t& port, const std::string& label, const std::string& productName){
        std::cout << "MsdDeviceMouted : port =  " << port << " msdPath = " << mountPath << " \n";
        std::cout << "MsdDeviceMouted : label =  " << label << " productName = " << productName << " \n";

        MMLogInfo("MsdDeviceMouted : port = %d, msdPath = %s", port, mountPath.c_str());
        MMLogInfo("MsdDeviceMouted : label = %s, productName = %s", label.c_str(), productName.c_str());

        usbMountSig(mountPath, port, label, productName);
    });

    mDevicemanagerProxy->getOnDeviceRemovedWithDataSelectiveEvent().subscribe([&](const std::string& mountPath, const int32_t& port, const std::string& label, const std::string& productName){
        std::cout << "MsdDeviceMouted : port =  " << port << " msdPath = " << mountPath << " \n";
        std::cout << "MsdDeviceMouted : label =  " << label << " productName = " << productName << " \n";

        MMLogInfo("MsdDeviceMouted : port = %d, msdPath = %s", port, mountPath.c_str());
        MMLogInfo("MsdDeviceMouted : label = %s, productName = %s", label.c_str(), productName.c_str());

        usbUnMountSig(mountPath, port, label, productName);
    });
#endif
    return true;
}

bool USBService::signalSubscribe() {
    MMLogInfo("");

    if (!mUsbMountedHandlerId)
        mUsbMountedHandlerId  = g_dbus_connection_signal_subscribe(mConnection,
                                NULL,
                                AUTO_MOUNT_INTERFACE,
                                "UsbMount",
                                AUTO_MOUNT_OBJECT_PATH,
                                NULL,
                                G_DBUS_SIGNAL_FLAGS_NONE,
                                onUsbMountedCb,
                                this,
                                NULL);

    if (!mUsbUnmountedHandlerId)
        mUsbUnmountedHandlerId = g_dbus_connection_signal_subscribe(mConnection,
                                NULL,
                                AUTO_MOUNT_INTERFACE,
                                "UsbUnMount",
                                AUTO_MOUNT_OBJECT_PATH,
                                NULL,
                                G_DBUS_SIGNAL_FLAGS_NONE,
                                onUsbMountedCb,
                                this,
                                NULL);

    if (mUsbMountedHandlerId == 0 || mUsbUnmountedHandlerId == 0 || mConnection == NULL ) {
        MMLogError("Fail to subscribe signal_prop :%d , signal_scan : %d \n" , mUsbMountedHandlerId, mUsbUnmountedHandlerId);
        return false;
    }

    return true;
}

bool USBService::requestUsbList() {
    MMLogInfo("");
    GError *error = NULL;

/*  GDBusMessage *msg = g_dbus_message_new_signal (DEVICE_MANAGER_OBJECT_PATH,
                                           "com.lge.car.automount.request",
                                           "ReqMountedList");

    g_dbus_connection_send_message(mConnection, msg, G_DBUS_SEND_MESSAGE_FLAGS_NONE, NULL, &error);
    g_object_unref (msg);
*/
    g_dbus_connection_emit_signal (mConnection,
                                            NULL,
                                            AUTO_MOUNT_OBJECT_PATH,
                                            "com.lge.car.automount.request",
                                            "ReqMountedList",
                                            NULL,
                                            &error);

    if (error) {
        MMLogError("Could not start com.lge.automount: %s\n", error->message);
        g_error_free(error);
        return false;
    }
    return true;
}

} // mm
} // lge
