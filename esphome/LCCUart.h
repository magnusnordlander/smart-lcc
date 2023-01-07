//
// Created by Magnus Nordlander on 2023-01-01.
//

#include "esphome.h"
#include "esphome/core/component.h"
#include "esphome/components/uart/uart_component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/switch/switch.h"

#include <cstdint>
#include <cmath>
#include "esp-protocol.h"
#include "LambdaSwitch.h"
#include "LambdaNumber.h"

uint32_t esp_random();

class LCCUart : public esphome::Component, public esphome::uart::UARTDevice {
public:
    esphome::sensor::Sensor *brewTemperatureSensor{nullptr};
    esphome::sensor::Sensor *serviceTemperatureSensor{nullptr};
    LambdaSwitch *ecoModeSwitch{nullptr};
    LambdaSwitch *sleepSwitch{nullptr};
    LambdaNumber *brewTemperatureSetPoint{nullptr};
    LambdaNumber *serviceTemperatureSetPoint{nullptr};
    LambdaNumber *brewTemperatureOffset{nullptr};
    esphome::binary_sensor::BinarySensor *waterTankLowSensor{nullptr};
    esphome::binary_sensor::BinarySensor *currentlyBrewingSensor{nullptr};
    esphome::binary_sensor::BinarySensor *currentlyFillingServiceBoilerSensor{nullptr};

    LCCUart(esphome::uart::UARTComponent *parent) : UARTDevice(parent) {
        brewTemperatureSensor = new esphome::sensor::Sensor();
        brewTemperatureSensor->set_name("Brew temperature");
        brewTemperatureSensor->set_disabled_by_default(false);
        brewTemperatureSensor->set_unit_of_measurement("\302\260C");
        brewTemperatureSensor->set_accuracy_decimals(1);
        brewTemperatureSensor->set_force_update(false);

        serviceTemperatureSensor = new esphome::sensor::Sensor();
        serviceTemperatureSensor->set_name("Service temperature");
        serviceTemperatureSensor->set_disabled_by_default(false);
        serviceTemperatureSensor->set_unit_of_measurement("\302\260C");
        serviceTemperatureSensor->set_accuracy_decimals(1);
        serviceTemperatureSensor->set_force_update(false);

        ecoModeSwitch = new LambdaSwitch([=](bool state) {
            ESP_LOGD("custom", "Switching eco mode");
            sendCommand(ESP_SYSTEM_COMMAND_SET_ECO_MODE, state);
        });
        ecoModeSwitch->set_name("Eco mode");
        ecoModeSwitch->set_disabled_by_default(false);

        sleepSwitch = new LambdaSwitch([=](bool state) {
            ESP_LOGD("custom", "Switching sleep mode");
            sendCommand(ESP_SYSTEM_COMMAND_SET_SLEEP_MODE, state);
        });
        sleepSwitch->set_name("Sleep mode");
        sleepSwitch->set_disabled_by_default(false);

        brewTemperatureSetPoint = new LambdaNumber();
        brewTemperatureSetPoint->set_name("Brew temperature set point");
        brewTemperatureSetPoint->set_disabled_by_default(false);
        brewTemperatureSetPoint->traits.set_min_value(0);
        brewTemperatureSetPoint->traits.set_max_value(100);
        brewTemperatureSetPoint->traits.set_step(0.1f);
        brewTemperatureSetPoint->traits.set_unit_of_measurement("\302\260C");

        serviceTemperatureSetPoint = new LambdaNumber();
        serviceTemperatureSetPoint->set_name("Service temperature set point");
        serviceTemperatureSetPoint->set_disabled_by_default(false);
        serviceTemperatureSetPoint->traits.set_min_value(0);
        serviceTemperatureSetPoint->traits.set_max_value(130);
        serviceTemperatureSetPoint->traits.set_step(0.1f);
        serviceTemperatureSetPoint->traits.set_unit_of_measurement("\302\260C");

        brewTemperatureOffset = new LambdaNumber();
        brewTemperatureOffset->set_name("Brew temperature offset");
        brewTemperatureOffset->set_disabled_by_default(true);
        brewTemperatureOffset->traits.set_min_value(-20);
        brewTemperatureOffset->traits.set_max_value(20);
        brewTemperatureOffset->traits.set_step(0.1f);
        brewTemperatureOffset->traits.set_unit_of_measurement("\302\260C");

        waterTankLowSensor = new esphome::binary_sensor::BinarySensor();
        waterTankLowSensor->set_name("Water tank low");
        waterTankLowSensor->set_disabled_by_default(false);

        currentlyBrewingSensor = new esphome::binary_sensor::BinarySensor();
        currentlyBrewingSensor->set_name("Currently Brewing");
        currentlyBrewingSensor->set_disabled_by_default(false);

        currentlyFillingServiceBoilerSensor = new esphome::binary_sensor::BinarySensor();
        currentlyFillingServiceBoilerSensor->set_name("Currently filling service boiler");
        currentlyFillingServiceBoilerSensor->set_disabled_by_default(true);
    }

    void setup() override {
        ESP_LOGD("custom", "Component booting up");
        // nothing to do here
    }

    void loop() override {
        if (this->available()) {
            ESPMessageHeader header{};
            bool success = this->read_array(reinterpret_cast<uint8_t *>(&header), sizeof(ESPMessageHeader));

            if (!success) {
                ESP_LOGD("custom", "There was stuff on the UART, but we didn't get a message");
                return;
            }

            switch (header.type) {
                case ESP_MESSAGE_PING:
                    return handlePingMessage(&header);
                case ESP_MESSAGE_SYSTEM_STATUS:
                    return handleSystemStatusMessage(&header);
                case ESP_MESSAGE_PONG:
                case ESP_MESSAGE_ACK:
                case ESP_MESSAGE_NACK:
                case ESP_MESSAGE_SYSTEM_COMMAND:
                    ESP_LOGD("custom", "Non-ping message");
                    break;
            }
        }
    }

    void handlePingMessage(ESPMessageHeader* header) {
        ESP_LOGD("custom", "Ping message");
        if (header->length == sizeof(ESPPingMessage)) {
            ESPPingMessage message{};
            bool success = this->read_array(reinterpret_cast<uint8_t *>(&message), sizeof(message));

            if (success) {
                if (message.version == 0x0001) {
                    ESP_LOGD("custom", "Sending Pong");
                    ESPMessageHeader responseHeader{
                            .direction = ESP_DIRECTION_ESP32_TO_RP2040,
                            .id = static_cast<uint16_t>(esp_random()),
                            .responseTo = header->id,
                            .type = ESP_MESSAGE_PONG,
                            .error = ESP_ERROR_NONE,
                            .length = sizeof(ESPPongMessage),
                    };
                    ESPPongMessage responseMessage{.version = 0x0001};

                    this->write_array(reinterpret_cast<const uint8_t *>(&responseHeader), sizeof(ESPMessageHeader));
                    this->write_array(reinterpret_cast<const uint8_t *>(&responseMessage), sizeof(ESPPongMessage));
                } else {
                    ESP_LOGD("custom", "Incorrect message, sending error");
                    ESPMessageHeader responseHeader{
                            .direction = ESP_DIRECTION_ESP32_TO_RP2040,
                            .id = static_cast<uint16_t>(esp_random()),
                            .responseTo = header->id,
                            .type = ESP_MESSAGE_NACK,
                            .error = ESP_ERROR_PING_WRONG_VERSION,
                            .length = 0,
                    };

                    this->write_array(reinterpret_cast<const uint8_t *>(&responseHeader), sizeof(ESPMessageHeader));
                }
            } else {
                ESP_LOGD("custom", "Didn't get complete data");
                nackMessage(header);
            }
        }
    }

    void handleSystemStatusMessage(ESPMessageHeader* header) {
        ESP_LOGD("custom", "System status message");
        if (header->length == sizeof(ESPSystemStatusMessage)) {
            ESPSystemStatusMessage message{};
            bool success = this->read_array(reinterpret_cast<uint8_t *>(&message), sizeof(message));

            if (success) {
                ackMessage(header);

                float brewTemp = ((float)message.brewBoilerTemperatureMilliCelsius) / 1000.f;
                float serviceTemp = ((float)message.serviceBoilerTemperatureMilliCelsius) / 1000.f;

                if (!brewTemperatureSensor->has_state() || fabs(brewTemp - brewTemperatureSensor->state) > 0.05) {
                    brewTemperatureSensor->publish_state(brewTemp);
                }

                if (!serviceTemperatureSensor->has_state() || fabs(serviceTemp - serviceTemperatureSensor->state) > 0.05) {
                    serviceTemperatureSensor->publish_state(serviceTemp);
                }

                float brewSetPoint = ((float)message.brewBoilerSetPointMilliCelsius) / 1000.f;
                float serviceSetPoint = ((float)message.serviceBoilerSetPointMilliCelsius) / 1000.f;
                float tempOffset = ((float)message.brewTemperatureOffsetMilliCelsius) / 1000.f;

                if (!brewTemperatureSetPoint->has_state() || fabs(brewSetPoint - brewTemperatureSetPoint->state) > 0.05) {
                    brewTemperatureSetPoint->publish_state(brewSetPoint);
                }

                if (!serviceTemperatureSetPoint->has_state() || fabs(serviceSetPoint - serviceTemperatureSetPoint->state) > 0.05) {
                    serviceTemperatureSetPoint->publish_state(serviceSetPoint);
                }

                if (!brewTemperatureOffset->has_state() || fabs(tempOffset - brewTemperatureOffset->state) > 0.05) {
                    brewTemperatureOffset->publish_state(tempOffset);
                }

                if (!waterTankLowSensor->has_state() || message.waterTankLow != waterTankLowSensor->state) {
                    waterTankLowSensor->publish_state(message.waterTankLow);
                }

                if (!currentlyBrewingSensor->has_state() || message.currentlyBrewing != currentlyBrewingSensor->state) {
                    currentlyBrewingSensor->publish_state(message.currentlyBrewing);
                }

                if (!currentlyFillingServiceBoilerSensor->has_state() || message.currentlyFillingServiceBoiler != currentlyFillingServiceBoilerSensor->state) {
                    currentlyFillingServiceBoilerSensor->publish_state(message.currentlyFillingServiceBoiler);
                }

                if (message.ecoMode != ecoModeSwitch->state) {
                    ecoModeSwitch->publish_state(message.ecoMode);
                }

                if (message.sleepMode != sleepSwitch->state) {
                    sleepSwitch->publish_state(message.sleepMode);
                }
            } else {
                ESP_LOGD("custom", "Didn't get complete data");

                nackMessage(header);
            }
        }
    }

    void ackMessage(ESPMessageHeader *header)
    {
        ESPMessageHeader responseHeader{
                .direction = ESP_DIRECTION_ESP32_TO_RP2040,
                .id = static_cast<uint16_t>(esp_random()),
                .responseTo = header->id,
                .type = ESP_MESSAGE_ACK,
                .error = ESP_ERROR_NONE,
                .length = 0,
        };

        this->write_array(reinterpret_cast<const uint8_t *>(&responseHeader), sizeof(ESPMessageHeader));
    }

    void nackMessage(ESPMessageHeader *header)
    {
        ESPMessageHeader responseHeader{
                .direction = ESP_DIRECTION_ESP32_TO_RP2040,
                .id = esp_random(),
                .responseTo = header->id,
                .type = ESP_MESSAGE_NACK,
                .error = ESP_ERROR_UNEXPECTED_MESSAGE_LENGTH,
                .length = 0,
        };

        this->write_array(reinterpret_cast<const uint8_t *>(&responseHeader), sizeof(ESPMessageHeader));
    }

    void sendCommand(ESPSystemCommandType type, bool boolArg)
    {
        ESPSystemCommandPayload payload{
            .type = type,
            .bool1 = boolArg
        };

        sendCommand(payload);
    }

    void sendCommand(ESPSystemCommandPayload payload)
    {
        ESPMessageHeader commandHeader{
                .direction = ESP_DIRECTION_ESP32_TO_RP2040,
                .id = esp_random(),
                .responseTo = 0,
                .type = ESP_MESSAGE_SYSTEM_COMMAND,
                .error = ESP_ERROR_NONE,
                .length = sizeof(ESPSystemCommandMessage),
        };

        ESPSystemCommandMessage commandMessage{
            .checksum = crc32(&payload, sizeof(ESPSystemCommandPayload)),
            .payload = payload,
        };

        this->write_array(reinterpret_cast<const uint8_t *>(&commandHeader), sizeof(ESPMessageHeader));
        this->write_array(reinterpret_cast<const uint8_t *>(&commandMessage), sizeof(ESPSystemCommandMessage));
    }

    uint32_t crc32_for_byte(uint32_t r) {
        for(int j = 0; j < 8; ++j)
            r = (r & 1? 0: (uint32_t)0xEDB88320L) ^ r >> 1;
        return r ^ (uint32_t)0xFF000000L;
    }

    uint32_t crc32(const void *data, size_t n_bytes) {
        uint32_t crc = 0;
        static uint32_t table[0x100];
        if(!*table)
            for(size_t i = 0; i < 0x100; ++i)
                table[i] = crc32_for_byte(i);
        for(size_t i = 0; i < n_bytes; ++i)
            crc = table[(uint8_t)crc ^ ((uint8_t*)data)[i]] ^ crc >> 8;

        return crc;
    }
};