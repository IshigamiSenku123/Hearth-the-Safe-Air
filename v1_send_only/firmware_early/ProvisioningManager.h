#ifndef PROVISIONING_MANAGER_H
#define PROVISIONING_MANAGER_H

#include <Arduino.h>
#include <WebServer.h>

class ProvisioningManager
{
public:

    bool begin();

    void update();

    void start();

    void stop();

    bool active() const;

private:

    void startAP();

    void stopAP();

    void setupRoutes();

    void handleRoot();

    void handleScan();

    void handleSave();

    void handleNotFound();

    bool _running = false;

    unsigned long _startTime = 0;

    WebServer _server = WebServer(80);
};

extern ProvisioningManager Provisioning;

#endif