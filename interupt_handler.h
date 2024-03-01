//
// Created by Aleksi Merilainen on 1.3.2024.
//

#ifndef VENTILATION_CONTROLLER_PROJECT_INTERUPT_HANDLER_H
#define VENTILATION_CONTROLLER_PROJECT_INTERUPT_HANDLER_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"


class InterruptHandler {
private:
    static uint8_t pinA;
    static uint8_t pinB;
    static uint8_t pinC;
    static int8_t lastEncoded;
    static int8_t encoderValue;

public:
    InterruptHandler(uint8_t PinA0, uint8_t pinB0, uint8_t pinC0)
    {
        pinA = PinA0;
        pinB = pinB0;
        pinC = pinC0;
        lastEncoded = 0;
        encoderValue = 0;
    }
    void handle_interrupts();
   void setupPin(uint8_t pin) {
        gpio_init(pin);
        gpio_set_dir(pin, GPIO_IN);
        gpio_pull_up(pin);
        gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_RISE, true, &handle_interrupts);

    }
};

#endif //VENTILATION_CONTROLLER_PROJECT_INTERUPT_HANDLER_H
