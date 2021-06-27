/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#include <cstdio>
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "pico/timeout_helper.h"

#include "control_board_protocol.h"
#include "utils/uart_extras.h"

/// \tag::main[]

#define CB_UART uart0
#define CB_BAUD_RATE 9600
#define CB_TX_PIN 0
#define CB_RX_PIN 1

/*
void loop(void) {
    uart_write_blocking(CB_UART, &lccVariations[currVariation*5], 5);
    if (!uart_read_blocking_timeout(CB_UART, cbData, sizeof(cbData), make_timeout_time_ms(200))) {
        return;
    }

    bool brew = cbData[1] & 0x02;
    bool tank = cbData[1] & 0x40;
    printf("V: %u A: %hu B: %hu C: %hu D: %hu E: %hu BS: %s TS: %s\n",
           currVariation,
           tripletToInt(&cbData[2]),
           tripletToInt(&cbData[5]),
           tripletToInt(&cbData[8]),
           tripletToInt(&cbData[11]),
           tripletToInt(&cbData[14]),
           brew ? "true" : "false",
           tank ? "true" : "false");

    if (currVariation < 2) {
        currVariation++;
    } else if (currVariation == 2) {
        currVariation = 0;
    }
}


void loop(void) {
    uart_read_blocking(LCC_UART, lccData, sizeof(lccData));

    uint8_t safeLcc[5] = {0x80, 0x00, 0x00, 0x00, 0x00};

    uart_write_blocking(CB_UART, safeLcc, sizeof(lccData));
    size_t lccsz = 3*sizeof(lccData);
    char lccStr[lccsz];
    tohex(lccData, sizeof(lccData), lccStr, lccsz);
//    printf("%s\n", lccStr);

    if (!uart_read_blocking_timeout(CB_UART, cbData, sizeof(cbData), make_timeout_time_ms(200))) {
        return;
    }
    uart_write_blocking(LCC_UART, cbData, sizeof(cbData));
    size_t cbsz = 3*sizeof(cbData);
    // size_t cbsz = 9;
    char cbstr[cbsz];
    tohex(cbData, sizeof(cbData), cbstr, cbsz);
//    tohex(&cbData[8], 3, cbstr, cbsz);
    //printf("%s\n", cbstr);
    bool brew = cbData[1] & 0x02;
    bool tank = cbData[1] & 0x40;
    printf("BrewS: %.2f BrewL: %.2f ServiceS %.2f ServiceL %.2f A: %hu B: %hu C: %hu D: %hu E: %hu BS: %s TS: %s\n",
           adcSmallToTemp(tripletToInt(&cbData[2])),
           adcLargeToTemp(tripletToInt(&cbData[8])),
           low_gain_adc_to_float(tripletToInt(&cbData[5])),
           high_gain_adc_to_float(tripletToInt(&cbData[11])),
           tripletToInt(&cbData[2]),
           tripletToInt(&cbData[5]),
           tripletToInt(&cbData[8]),
           tripletToInt(&cbData[11]),
           tripletToInt(&cbData[14]),
           brew ? "true" : "false",
           tank ? "true" : "false");
}

void loop(void) {
    uart_read_blocking(LCC_UART, lccData, sizeof(lccData));

    bool pumpOn = lccData[1] & 0x10;
    bool serviceBoilerOn = lccData[1] & 0x01;
    bool electroValve = lccData[2] & 0x10;
    bool coffeeBoiler = lccData[2] & 0x08;

    if (!gpio_get(20)) {
        cb.brewSwitch = true;
    } else {
        cb.brewSwitch = false;
    }

    if (!gpio_get(21)) {
        cb.serviceTapOpen = true;
    } else {
        cb.serviceTapOpen = false;
    }

    if (!gpio_get(22)) {
        cb.tankFull = false;
    } else {
        cb.tankFull = true;
    }


    int64_t timePassed = cb.update(get_absolute_time(), pumpOn, electroValve, coffeeBoiler, serviceBoilerOn);

    uint8_t cbVal[18];
    cb.copyPacket(cbVal);

    uart_write_blocking(LCC_UART, cbVal, sizeof(cbVal));

    bool fault = false;

    if (lccData[3] != 0x00) {
        fault = true;
        printf("Weird number found in LCC byte 3\n");
    }

    if (timePassed > 60) {
        fault = true;
        printf("Time passed is long\n");
    }

    if (timePassed <30 ) {
        fault = true;
        printf("Time passed is short\n");
    }

    if (fault) {
        size_t lccsz = 3*sizeof(lccData);
        char lccStr[lccsz];
        tohex(lccData, sizeof(lccData), lccStr, lccsz);

        size_t cbsz = 3*sizeof(cbVal);
        char cbstr[cbsz];
        tohex(cbVal, sizeof(cbVal), cbstr, cbsz);

        printf("%s\n", lccStr);
        printf("%s\n", cbstr);
    }

    printf("TP: %lld P: %s SB: %s EV: %s CB: %s WN: %u CT: %.1lf ST: %.1lf CL: %.0lf SL: %.0lf CA: %.1lf ST %.1l                     ",
           timePassed,
           pumpOn ? "Y" : "N",
           serviceBoilerOn ? "Y" : "N",
           electroValve ? "Y" : "N",
           coffeeBoiler ? "Y" : "N",
           lccData[3],
           cb.coffeeTempC(),
           cb.serviceTempC(),
           cb.coffeeLevelMl(),
           cb.serviceLevelMl(),
           cb.coffeeAmbientC(),
           cb.serviceAmbientC()
    );

    if (fault) {
        printf("\n");
    } else {
        printf("\x1b[1000D");
    }
}

*/

void loop() {
    uint8_t safeLcc[5] = {0x80, 0x00, 0x00, 0x00, 0x00};

    uart_write_blocking(CB_UART, safeLcc, sizeof(safeLcc));
    ControlBoardRawPacket control_board_packet;
    if (!uart_read_blocking_timeout(CB_UART, (uint8_t*)&control_board_packet, sizeof(control_board_packet), make_timeout_time_ms(200))) {
        printf("Can't read Control Board\n");

        return;
    }

    if (!validate_raw_packet(control_board_packet)) {
        printf("Control Board packet invalid\n");
    }

    ControlBoardParsedPacket parsed_packet = convert_raw_packet(control_board_packet);

    printf(
            "Brew temp: %.02f Service temp: %.02f Brew switch: %s Service boiler low: %s Water tank low &s     \x1b[1000D",
            parsed_packet.brew_boiler_temperature,
            parsed_packet.service_boiler_temperature,
            parsed_packet.brew_switch ? "Y" : "N",
            parsed_packet.service_boiler_low ? "Y" : "N",
            parsed_packet.water_tank_empty ? "Y" : "N"
            );
}

void setup() {
    stdio_init_all();

    // Initialize the UARTs
    uart_init(CB_UART, CB_BAUD_RATE);

    gpio_set_function(CB_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(CB_RX_PIN, GPIO_FUNC_UART);

    // The Bianca uses inverted UART
    gpio_set_outover(CB_TX_PIN, GPIO_OVERRIDE_INVERT);
    gpio_set_inover(CB_RX_PIN, GPIO_OVERRIDE_INVERT);
}

int main() {
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (1) {
        loop();
    }
#pragma clang diagnostic pop
}

/// \end::main[]
