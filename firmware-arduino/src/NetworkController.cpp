//
// Created by Magnus Nordlander on 2022-01-30.
//

#include "NetworkController.h"

NetworkController::NetworkController(NetworkControllerMode mode) : mode(mode) {
    wifiManager = new WiFiManager_NINA_Lite();
    wifiManager->setConfigPortalChannel(0);
    wifiManager->begin();
}

void NetworkController::loop() {
    wifiManager->run();

    if (WiFi.status() == WL_CONNECTED && mode == NETWORK_CONTROLLER_MODE_OTA)
    {
        initOTA();
        ArduinoOTA.poll();
    } else {
        otaInited = false;
    }

    static unsigned long checkstatus_timeout = 0;

    if ((millis() > checkstatus_timeout) || (checkstatus_timeout == 0))
    {
        if (WiFi.status() == WL_CONNECTED)
        {
            // Publish and subscribe MQTT stuff
        }

        checkstatus_timeout = millis() + 500L;
    }
}

inline void NetworkController::initOTA() {
    if (!otaInited) {
        ArduinoOTA.begin(WiFi.localIP(), "Arduino", "password", InternalStorage);
        otaInited = true;
    }
}



/*
void createNewInstances(void)
{
    if (!client)
    {
        client = new WiFiClient;
        // client can somehow be null here?
    }

    // Create new instances from new data
    if (!mqtt)
    {
        // Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
        mqtt = new Adafruit_MQTT_Client(client, AIO_SERVER, atoi(AIO_SERVERPORT), AIO_USERNAME, AIO_KEY);
        // mqtt can be null here, if creation fails
    }
}

void MQTT_connect()
{
    int8_t ret;

    createNewInstances();

    // Return if already connected
    if (mqtt->connected())
    {
        return;
    }

    //Connecting to WiFi MQTT (3 attempts)...
    uint8_t attempt = 3;

    while ( (ret = mqtt->connect()) )
    {
        // connect will return 0 for connected
        Serial.println(mqtt->connectErrorString(ret));

        // Another attemtpt to connect to MQTT in 5 seconds...
        mqtt->disconnect();
        delay(5000);  // wait 5 seconds
        attempt--;

        if (attempt == 0)
        {
            // WiFi MQTT connection failed. Continuing with program...
            return;
        }
    }
    // Connection successful
} */
