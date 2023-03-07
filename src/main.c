/* ************************************************************************* */
/*                             Public Inclusions                             */
/* ************************************************************************* */
#include "arm_const_structs.h"
#include "arm_math.h"
#include "sapi.h"
/* ************************************************************************* */
/*                               Configuration                               */
/* ************************************************************************* */
#define PROG_LOOP_HZ 8000
#define PROG_FREQ_CYCLES (EDU_CIAA_NXP_CLOCK_SPEED / PROG_LOOP_HZ)

#define UART_BAUDRATE 460800


#define ADC_DATA_SZ 512
/* ***************************** Data Transfer ***************************** */
static struct header_struct {
    char head[4];
    uint32_t id;
    uint16_t N;
    uint16_t fs;
    uint16_t dbg1;
    uint16_t dbg2;
    uint16_t dbg3;
    char tail[4];
} header = {
    "head", 0, ADC_DATA_SZ, PROG_LOOP_HZ, 0, 0, 0, "tail"};

q15_t adc_sample = 0;
uint16_t dac_sample = 512;

/* ************************************************************************* */
/*                                    Code                                   */
/* ************************************************************************* */

int main(void) {
    boardConfig();
    uartConfig(UART_USB, UART_BAUDRATE);
    adcConfig(ADC_ENABLE);
    dacConfig(DAC_ENABLE);
    cyclesCounterInit(EDU_CIAA_NXP_CLOCK_SPEED);
    dacWrite(DAC, 512);  // Start at 0.
    for (;;) {
        cyclesCounterReset();

        /* Send ADC data through UART. */
        uint16_t raw_adc = adcRead(CH1);
        adc_sample = (((raw_adc - 512)) << 6);
        uartWriteByteArray(UART_USB, (uint8_t*)&adc_sample, sizeof(adc_sample));

        // uartWriteByteArray(UART_USB, (uint8_t*)&header, sizeof(header));
        // uartReadByte(UART_USB, &recv_by);

        // dacWrite(DAC, (modulator_get_out_sample(&mod) >> 6) + 512);
        while (cyclesCounterRead() < PROG_FREQ_CYCLES)
            ;
    }
}