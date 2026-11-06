#include "ProvisioningManager.h"

#include <WiFi.h>

#include "StorageManager.h"
#include "WiFiManager.h"
#include "SystemLogger.h"
#include "Config.h"

ProvisioningManager Provisioning;

bool ProvisioningManager::begin()
{
    return true;
}

bool ProvisioningManager::active() const
{
    return _running;
}

void ProvisioningManager::start()
{
    if (_running)
        return;

    startAP();

    setupRoutes();

    _server.begin();

    _running = true;

    _startTime = millis();

    SysLog.log(
        LogLevel::INFO,
        LogCategory::NETWORK,
        "Provisioning started",
        "Connect to SafeAir AP");
}

void ProvisioningManager::stop()
{
    if (!_running)
        return;

    _server.stop();

    stopAP();

    _running = false;

    SysLog.log(
        LogLevel::INFO,
        LogCategory::NETWORK,
        "Provisioning stopped",
        "");
}

void ProvisioningManager::update()
{
    if (!_running)
        return;

    _server.handleClient();

    if (millis() - _startTime > SOFTAP_TIMEOUT_MS)
    {
        SysLog.log(
            LogLevel::WARNING,
            LogCategory::NETWORK,
            "Provisioning timeout",
            "");

        stop();

        Net.connectStored();
    }
}

void ProvisioningManager::startAP()
{
    WiFi.mode(WIFI_AP);

    String ssid =
        String(SOFTAP_SSID_PREFIX) +
        String((uint32_t)ESP.getEfuseMac(), HEX);

    WiFi.softAP(ssid.c_str(), SOFTAP_PASS);

    SysLog.log(
        LogLevel::INFO,
        LogCategory::NETWORK,
        "SoftAP",
        "SSID=%s IP=%s",
        ssid.c_str(),
        WiFi.softAPIP().toString().c_str());
}

void ProvisioningManager::stopAP()
{
    WiFi.softAPdisconnect(true);
}

void ProvisioningManager::setupRoutes()
{
    _server.on("/", HTTP_GET,
               std::bind(
                   &ProvisioningManager::handleRoot,
                   this));

    _server.on("/scan", HTTP_GET,
               std::bind(
                   &ProvisioningManager::handleScan,
                   this));

    _server.on("/save", HTTP_POST,
               std::bind(
                   &ProvisioningManager::handleSave,
                   this));

    _server.onNotFound(
        std::bind(
            &ProvisioningManager::handleNotFound,
            this));
}

void ProvisioningManager::handleRoot()
{
    _server.send(
        200,
        "text/html",
        "<h2>SafeAir Provisioning</h2>");
}

void ProvisioningManager::handleScan()
{
    int n = WiFi.scanNetworks();

    String json = "[";

    for (int i = 0; i < n; i++)
    {
        json += "\"";
        json += WiFi.SSID(i);
        json += "\"";

        if (i != n - 1)
            json += ",";
    }

    json += "]";

    _server.send(
        200,
        "application/json",
        json);
}

void ProvisioningManager::handleSave()
{
    if (!_server.hasArg("ssid") ||
        !_server.hasArg("password"))
    {
        _server.send(
            400,
            "text/plain",
            "Missing parameters");

        return;
    }

    WiFiCredentials cred;

    memset(&cred, 0, sizeof(cred));

    strcpy(cred.ssid,
           _server.arg("ssid").c_str());

    strcpy(cred.password,
           _server.arg("password").c_str());

    cred.valid = true;

    Storage.saveWiFi(cred);

    _server.send(
        200,
        "text/plain",
        "Saved");

    SysLog.log(
        LogLevel::INFO,
        LogCategory::NETWORK,
        "WiFi credentials saved",
        "%s",
        cred.ssid);

    stop();

    Net.connectStored();
}

void ProvisioningManager::handleNotFound()
{
    _server.send(
        404,
        "text/plain",
        "Not Found");
}