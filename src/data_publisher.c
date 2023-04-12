#include "data_publisher.h"

static const data_publisher_t __default = {
    .header = {.head = {'h', 'e', 'a', 'd'},
               .id = 0,
               .N = DATA_PUBLISHER_DATA_BUFFER_SZ,
               .fs = 1000,
               .extra_data = {0},
               .tail = {'t', 'a', 'i', 'l'}},
    .data_buffer = {0},
    .buffer_i = 0};

static void __data_publisher_send_data(uint8_t* data, uint16_t size_bytes) {
    uartWriteByteArray(UART_USB, data, size_bytes);
}

static void __data_publisher_init_transport() {
    uartConfig(UART_USB, UART_BAUDRATE);
}

void data_publisher_init(data_publisher_t* instance, uint16_t sr_hz) {
    __data_publisher_init_transport();
    memcpy(instance, &__default, sizeof(data_publisher_t));
}

void data_publisher_update_extra_data(data_publisher_t* instance,
                                      uint16_t extra_data,
                                      uint16_t n_extra_data) {
    instance->header.extra_data[n_extra_data] = extra_data;
}

void data_publisher_update_samples(data_publisher_t* instance, uint16_t sample,
                                   uint16_t sampleb) {
    __data_publisher_send_data((uint8_t*)&sample, sizeof(sample));
    __data_publisher_send_data((uint8_t*)&sampleb, sizeof(sampleb));
    instance->buffer_i++;
    if (instance->buffer_i == DATA_PUBLISHER_DATA_BUFFER_SZ) {
        __data_publisher_send_data((uint8_t*)&instance->header,
                                   sizeof(data_publisher_header_t));
        instance->buffer_i = 0;
    }
}