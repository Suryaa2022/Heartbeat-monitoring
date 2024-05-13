#include "serviceprovider.h"

#include <iostream>

#include "player_logger.h"

namespace lge {
namespace mm {

ServiceProvider::ServiceProvider(std::string iface)
  : m_watcherId(0),
    connection(NULL),
    ifacePath(iface),
    onConnectedCallback(NULL) {
}

static void onNameAppeared(GDBusConnection *connection,
                           const gchar     *name,
                           const gchar     *name_owner,
                           void            *user_data) {
  MMLogInfo("Found %s on D-Bus", ((ServiceProvider*)user_data)->ifacePath.c_str());

  ServiceProvider* this_ = ((ServiceProvider*) user_data);
  ((ServiceProvider*) user_data)->connection = connection;
  (*(this_->onConnectedCallback))(NULL);
}

static void onNameVanished(GDBusConnection *connection,
                           const gchar     *name,
                           void        *user_data) {
  MMLogError("Failed to get name owner for %s", name);

  ServiceProvider* this_ = ((ServiceProvider*) user_data);
  (*(this_->onConnectedCallback))(new MmError("Failed to connect, is " + this_->ifacePath + " running?"));
}

bool ServiceProvider::connect(std::function<void(MmError* e)> cb) {
  onConnectedCallback = new std::function<void(MmError* e)>(cb);

  GError *error = NULL;

  /* Create a dummy proxy, so that we can poke the service to life if it
     is activatable, but not yet started. Then use the g_bus_watcher for
     further interaction. DBus activation is not supported using the
     watcher. */
  GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION,
                                         NULL,
                                         &error);

  if (error) {
    MMLogError("Failed to get bus: %s", error->message);
    return false;
  }

  GDBusProxy *proxy = g_dbus_proxy_new_sync(conn,
                                            G_DBUS_PROXY_FLAGS_NONE,
                                            NULL,
                                            ifacePath.c_str(),
                                            "/",
                                            "org.freedesktop.DBus.Peer",
                                            NULL,
                                            &error);
  g_object_unref(proxy);
  g_object_unref(conn);

  m_watcherId = g_bus_watch_name(G_BUS_TYPE_SESSION,
                   ifacePath.c_str(),
                   G_BUS_NAME_WATCHER_FLAGS_NONE,
                   onNameAppeared,
                   onNameVanished,
                   this,
                   NULL);

  return true;
}

bool ServiceProvider::isConnected() {
  return (connection && !g_dbus_connection_is_closed(connection));
}

void ServiceProvider::disconnect() {
  g_bus_unwatch_name(m_watcherId);
}

ServiceProvider::~ServiceProvider() {
  disconnect();
}

bool ServiceProvider::checkError(GError *error, MmError **e) {
  if (error) {
    MMLogError("D-Bus call: %s", error->message);
    if (e) {
      (*e) = new MmError(error->message ? error->message : "");
    }
    return false;
  }
  return true;
}

} // namespace mm
} // namespace lge

