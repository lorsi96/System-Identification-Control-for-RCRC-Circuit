#ifndef __DATA_PUBLISHER__
#define __DATA_PUBLISHER__

#include "stdint.h"
#include "sapi.h"
#include "string.h"
#include <stdlib.h>

#define DATA_PUBLISHER_EXTRA_METADATA_SZ  1
#define DATA_PUBLISHER_DATA_BUFFER_SZ  512
#define UART_BAUDRATE 460800

typedef struct {
    char head[4];
    uint32_t id;
    uint16_t N;
    uint16_t fs;
    uint16_t extra_data[DATA_PUBLISHER_EXTRA_METADATA_SZ];
    char tail[4];
} data_publisher_header_t;

typedef struct {
    data_publisher_header_t header;
    uint16_t data_buffer[DATA_PUBLISHER_DATA_BUFFER_SZ];
    uint16_t data_buffer_b[DATA_PUBLISHER_DATA_BUFFER_SZ];
    uint16_t buffer_i;
} data_publisher_t;

void data_publisher_init(data_publisher_t* instance, uint16_t sr_hz); 

void data_publisher_update_samples(data_publisher_t* instance, uint16_t sample, uint16_t sampleb); 

void data_publisher_update_extra_data(data_publisher_t* instance, uint16_t extra_data, uint16_t n_extra_data); 


#endif