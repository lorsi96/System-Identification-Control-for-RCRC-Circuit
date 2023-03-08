/* ************************************************************************* */
/*                             Public Inclusions                             */
/* ************************************************************************* */
#include "arm_const_structs.h"
#include "arm_math.h"
#include "sapi.h"

/* ************************************************************************* */
/*                               Configuration                               */
/* ************************************************************************* */
#define PROG_LOOP_HZ 1000
#define SQ_WAVE_HZ 10
#define PROG_FREQ_CYCLES (EDU_CIAA_NXP_CLOCK_SPEED / PROG_LOOP_HZ)

#define UART_BAUDRATE 460800


typedef int16_t(*DacSampleProvider_t)();

/* ************************************************************************* */
typedef struct {
    uint32_t ticksThreshold; 
    uint32_t ticksThresholdHalf;
    uint32_t tickCount;
} SquareWaveDacProvider_t;

int16_t SquareWaveDacProvider_getSample() {
    static SquareWaveDacProvider_t self = {
        .ticksThreshold = PROG_LOOP_HZ / SQ_WAVE_HZ, 
        .ticksThresholdHalf = PROG_LOOP_HZ / SQ_WAVE_HZ / 2,
        .tickCount = 0
    };

    if(self.tickCount >= self.ticksThreshold) {
        self.tickCount = 0;
    }
    return self.tickCount++ > self.ticksThresholdHalf ? 0x7FFF : 0x8000;
}

#define ADC_DATA_SZ 512
/* ***************************** Data Transfer ***************************** */
static struct header_struct {
    char head[4];
    uint32_t id;
    uint16_t N;
    uint16_t fs;
    char tail[4];
} header = {
    "head", 0, ADC_DATA_SZ, PROG_LOOP_HZ, "tail"};

q15_t adc_sample = 0;
uint16_t dac_sample = 512;

/* ************************************************************************* */
/*                                    Code                                   */
/* ************************************************************************* */
static DacSampleProvider_t dacProvider = SquareWaveDacProvider_getSample; 


int main(void) {



    boardConfig();
    uartConfig(UART_USB, UART_BAUDRATE);
    adcConfig(ADC_ENABLE);
    dacConfig(DAC_ENABLE);
    cyclesCounterInit(EDU_CIAA_NXP_CLOCK_SPEED);
    dacWrite(DAC, 512);  // Start at 0.
    static uint32_t uartSamplesCount = 0;
    for (;;) {
        cyclesCounterReset();

        /* Send ADC data through UART. */
        uint16_t raw_adc = adcRead(CH2);
        adc_sample = (((raw_adc - 512)) << 6);
        uartWriteByteArray(UART_USB, (uint8_t*)&adc_sample, sizeof(adc_sample));
        // uartReadByte(UART_USB, &recv_by);

        dacWrite(DAC, (dacProvider() >> 6) + 512);

        if(++uartSamplesCount == ADC_DATA_SZ) {
            uartWriteByteArray(UART_USB, (uint8_t*)&header, sizeof(header));
            uartSamplesCount = 0;
        }

        while (cyclesCounterRead() < PROG_FREQ_CYCLES)
            ;
    }
}