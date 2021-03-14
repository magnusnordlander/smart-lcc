#include <Arduino.h>
#include "wiring_private.h"
#include "BiancaUart.h"
#include <U8g2lib.h>
#include <SPI.h>
#include <SD.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include "secrets.h"

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SSID;        // your network SSID (name)
char pass[] = PASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the WiFi radio's status

IPAddress server(192, 168, 11, 21);
WiFiClient wifiClient;
PubSubClient client(wifiClient);

void callback(char* topic, byte* payload, unsigned int length) {
    // handle message arrived
}

unsigned long lastReconnectAttempt = 0;
unsigned long lastMQTTUpdate = 0;

BiancaUart<3> lccSerial (&sercom0, 5, 6, SERCOM_RX_PAD_1, UART_TX_PAD_0);

void SERCOM0_Handler()
{
    lccSerial.IrqHandler();
}

BiancaUart<16> controlBoardSerial( &sercom5, PIN_SERIAL1_RX, PIN_SERIAL1_TX, PAD_SERIAL1_RX, PAD_SERIAL1_TX ) ;

void SERCOM5_Handler()
{
    controlBoardSerial.IrqHandler();
}

boolean reconnect() {
    if (client.connect("bianca", "bianca/online", 1, true, "false")) {
        client.publish("bianca/online","true");
    }
    return client.connected();
}

const int sdChipSelect = 14;
U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R2, 17, 16, 15);

uint16_t fileNum = 0;
String coliFileName;
String ciloFileName;
unsigned long st;

uint16_t transformHextripet(uint8_t byte0, uint8_t byte1, uint8_t byte2) {
    uint32_t triplet = ((uint32_t)byte0 << 16) | ((uint32_t)byte1 << 8) | (uint32_t)byte2;

    uint8_t lsb = (triplet & 0x00FE00) >> 9;

    uint8_t hig = (triplet & 0x060000) >> 16;
    uint8_t mid = (triplet & 0x000002) >> 1;

    uint8_t msb = (hig | mid) - 1;

    return (uint16_t)lsb | ((uint16_t)msb << 7);
}

float transformBrewTemp(int brewnumber) {
    // 754 = 24 deg C
    // 173 = 93 deg C

    float k = ((93. - 24.) / (754. - 173.))*-1.;
    float m = ((k * 754.) - 24.)*-1.;

    return k*((float)brewnumber)+m;
}

float transformServiceTemp(int servicenumber) {
    // 735 = 24 deg C
    // 63 = 125 deg C

    float k = ((125. - 24.) / (735. - 63.))*-1.;
    float m = ((k * 735.) - 24.)*-1;

    return k*((float)servicenumber)+m;
}

void setup() {
    u8g2.begin();

    pinPeripheral(5, PIO_SERCOM_ALT);
    pinPeripheral(6, PIO_SERCOM_ALT);

    Serial.begin(115200);
    controlBoardSerial.begin(9600);
    lccSerial.begin(9600);

    Serial.print("\nInitializing SD card...");

    if (!SD.begin(sdChipSelect)) {
        u8g2.clearBuffer();          // clear the internal memory
        u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
        u8g2.drawStr(0,10, "SD Card init failed");
        u8g2.sendBuffer();
        while (true);
    }

    while (1) {
        String fileNumString = String(fileNum);
        coliFileName = String("coli-" + fileNumString + ".txt");

        if (fileNum > 1) {
            u8g2.clearBuffer();          // clear the internal memory
            u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
            u8g2.drawStr(0,10, String("Trying file "+coliFileName).c_str());
            u8g2.sendBuffer();
            delay(50);
        }

        if (!SD.exists(coliFileName)) {
            ciloFileName = String("cilo-" + fileNumString + ".txt");
            break;
        }

        fileNum++;
    }

    st = millis();

    status = WiFi.begin(ssid, pass);

    client.setServer(server, 1883);
    client.setCallback(callback);

    lastReconnectAttempt = 0;

    Serial.println("Ready");
    Serial.println();
}

bool microswitchOn;
uint16_t brewboiler;
uint16_t serviceboiler;
uint16_t waterlevel;
bool serviceboilerhe;
bool brewboilerhe;
bool pumpOn;

bool mqttSentMicroswitchOn;
uint16_t mqttSentBrewboiler;
uint16_t mqttSentServiceboiler;
uint16_t mqttSentWaterlevel;
bool mqttSentServiceboilerhe;
bool mqttSentBrewboilerhe;
bool mqttSentPumpOn;

String fb(bool b) {
    return b ? "On" : "Off";
}

void updateStates() {
    uint8_t coliBuf[17];
    controlBoardSerial.readDatagram(coliBuf);
    uint8_t ciloBuf[4];
    lccSerial.readDatagram(ciloBuf);

    microswitchOn = (coliBuf[1] & 0x40) == 0x00;
    brewboiler = transformHextripet(coliBuf[7], coliBuf[8], coliBuf[9]);
    serviceboiler = transformHextripet(coliBuf[10], coliBuf[11], coliBuf[12]);
    waterlevel = transformHextripet(coliBuf[13], coliBuf[14], coliBuf[15]);

    serviceboilerhe = (ciloBuf[0] & 0x0E) == 0x0A;
    brewboilerhe = (ciloBuf[1] & 0xF0) == 0xE0;
    pumpOn = (ciloBuf[1] & 0x0E) == 0x0E;
}

void updateDisplay() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);

    u8g2.drawStr(0,10, "SW/P");
    u8g2.drawStr(0,20, "Brew");
    u8g2.drawStr(0,30, "Serv");
    u8g2.drawStr(0,40, "B HE");
    u8g2.drawStr(64,10, "Water");
    u8g2.drawStr(64,20, "T");
    u8g2.drawStr(64,30, "T");
    u8g2.drawStr(64,40, "S HE");

    u8g2.drawStr(40, 10, String(String((int)microswitchOn) + "/" + String((int)pumpOn)).c_str());
    u8g2.drawStr(40, 20, String(brewboiler).c_str());
    u8g2.drawStr(40, 30, String(serviceboiler).c_str());
    u8g2.drawStr(40, 40, fb(brewboilerhe).c_str());

    u8g2.drawStr(104, 10, String(waterlevel).c_str());
    u8g2.drawStr(80, 20, String(String(transformBrewTemp(brewboiler), 1) + "C").c_str());
    u8g2.drawStr(80, 30, String(String(transformServiceTemp(serviceboiler), 1) + "C").c_str());
    u8g2.drawStr(104, 40, fb(serviceboilerhe).c_str());

    u8g2.sendBuffer();
}

void writeCiloToSD() {
    unsigned long dt = millis() - st;

    byte ciloBuf[64];

    int n = min(64, lccSerial.available());

    if (n > 0) {
        lccSerial.readBytes(ciloBuf, n);

        File dataFile = SD.open(ciloFileName, FILE_WRITE);

        if (dataFile) {
            dataFile.print(String(String(dt) + ": "));
            for (int j = 0; j < n; j++) {
                dataFile.print(" ");
                dataFile.print(ciloBuf[j], HEX);
            }

            dataFile.println();
            dataFile.close();
        } else {
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_ncenB08_tr);
            u8g2.drawStr(0, 10, "Error opening file.");
            u8g2.drawStr(0, 20, String("File: " + coliFileName).c_str());
            u8g2.sendBuffer();
            while (true);
        }
    }
}

void writeColiToSD() {
    unsigned long dt = millis() - st;

    byte coliBuf[64];

    int n = min(64, controlBoardSerial.available());

    if (n > 0) {
        controlBoardSerial.readBytes(coliBuf, n);

        File dataFile = SD.open(coliFileName, FILE_WRITE);

        if (dataFile) {
            dataFile.print(String(String(dt) + ": "));
            for (int j = 0; j < n; j++) {
                dataFile.print(" ");
                dataFile.print(coliBuf[j], HEX);
            }

            dataFile.println();
            dataFile.close();
        } else {
            u8g2.clearBuffer();
            u8g2.setFont(u8g2_font_ncenB08_tr);
            u8g2.drawStr(0, 10, "Error opening file.");
            u8g2.drawStr(0, 20, String("File: " + coliFileName).c_str());
            u8g2.sendBuffer();
            while (true);
        }
    }
}

void sendMqttMessages(bool ignoreLastSent = false) {
    if (!ignoreLastSent && (!controlBoardSerial.goodTimeForWifi() && !lccSerial.goodTimeForWifi())) {
        return;
    }

    char stringBuf[20];

    if (ignoreLastSent || brewboiler != mqttSentBrewboiler) {
        snprintf (stringBuf, sizeof(stringBuf), "%d", brewboiler);
        client.publish("bianca/brewboiler",stringBuf);

        snprintf (stringBuf, sizeof(stringBuf), "%.2f", transformBrewTemp(brewboiler));
        client.publish("bianca/brewTemp",stringBuf);

        mqttSentServiceboiler = brewboiler;
    }

    if (ignoreLastSent || serviceboiler != mqttSentServiceboiler) {
        snprintf(stringBuf, sizeof(stringBuf), "%d", serviceboiler);
        client.publish("bianca/serviceboiler", stringBuf);

        snprintf (stringBuf, sizeof(stringBuf), "%.2f", transformServiceTemp(serviceboiler));
        client.publish("bianca/serviceTemp",stringBuf);

        mqttSentServiceboiler = serviceboiler;
    }

    if (ignoreLastSent || waterlevel != mqttSentWaterlevel) {
        snprintf(stringBuf, sizeof(stringBuf), "%d", waterlevel);
        client.publish("bianca/waterlevel", stringBuf);

        mqttSentWaterlevel = waterlevel;
    }

    if (ignoreLastSent || microswitchOn != mqttSentMicroswitchOn) {
        client.publish("bianca/microswitch", microswitchOn ? "true" : "false");

        mqttSentMicroswitchOn = microswitchOn;
    }

    if (ignoreLastSent || pumpOn != mqttSentPumpOn) {
        client.publish("bianca/pump", pumpOn ? "true" : "false");

        mqttSentPumpOn = pumpOn;
    }

    if (ignoreLastSent || brewboilerhe != mqttSentBrewboilerhe) {
        client.publish("bianca/brewboilerhe", brewboilerhe ? "true" : "false");

        mqttSentBrewboilerhe = brewboilerhe;

        // This is a good time to let everyone know we're still around
        client.publish("bianca/online", "true");
    }

    if (ignoreLastSent || serviceboilerhe != mqttSentServiceboilerhe) {
        client.publish("bianca/serviceboilerhe", serviceboilerhe ? "true" : "false");

        mqttSentServiceboilerhe = serviceboilerhe;
    }
}

void handleMqtt() {
    if (!client.connected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            lastMQTTUpdate = 0;
            // Attempt to reconnect
            Serial.println("Reconnecting");
            if (reconnect()) {
                lastReconnectAttempt = 0;
            }
        }
    } else {
        client.loop();

        unsigned long now = millis();

        if (lastMQTTUpdate == 0) {
            lastMQTTUpdate = now;

            sendMqttMessages(true);
        } else if ((now - lastMQTTUpdate) > 5000) {
            lastMQTTUpdate = now;

            sendMqttMessages();
        }
    }
}

void loop() {
    updateStates();
    handleMqtt();
    updateDisplay();
    writeColiToSD();
    writeCiloToSD();
}