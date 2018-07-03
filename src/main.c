#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/dma.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

int _write(int file, char *ptr, int len);

void hardware_setup(void) {
	rcc_clock_setup_in_hse_8mhz_out_72mhz();

	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_AFIO);
	rcc_periph_clock_enable(RCC_USART2);
	rcc_periph_clock_enable(RCC_SPI1);
	rcc_periph_clock_enable(RCC_DMA1);

	gpio_set_mode(
		GPIOA, 
		GPIO_MODE_OUTPUT_2_MHZ, 
		GPIO_CNF_OUTPUT_PUSHPULL,
		GPIO8);

		/* Setup GPIO pin GPIO_USART2_TX and GPIO_USART2_RX. */
	gpio_set_mode(GPIO_BANK_USART2_TX, GPIO_MODE_OUTPUT_2_MHZ,
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART2_TX);
	gpio_set_mode(GPIO_BANK_USART2_RX, GPIO_MODE_INPUT,
		GPIO_CNF_INPUT_FLOAT, GPIO_USART2_RX);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, 
		GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO7 | GPIO4 | GPIO5);
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, 
		GPIO_CNF_INPUT_FLOAT, GPIO6);

	/* Setup UART parameters. */
	usart_set_baudrate(USART2, 115200);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_STOPBITS_1);
	usart_set_mode(USART2, USART_MODE_TX_RX);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);

	/* Finally enable the USART. */
	usart_enable(USART2);

	spi_reset(SPI1);
	spi_disable(SPI1);
	spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_4, SPI_CR1_CPOL_CLK_TO_1_WHEN_IDLE,
		SPI_CR1_CPHA_CLK_TRANSITION_2, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);
	spi_enable_software_slave_management(SPI1);
	spi_set_nss_high(SPI1);
	spi_enable(SPI1);

}

int _write(int file, char *ptr, int len) {
	int i;

	if (file == 1) {
		for (i = 0; i < len; i++)
			usart_send_blocking(USART2, ptr[i]);
		return i;
	}

	errno = EIO;
	return -1;
}

int count = 0;


#define dma_length (2250)
#define samples_per_period (dma_length * 8)

volatile uint8_t dma_buffer[dma_length] __attribute__ ((aligned (4)));

void start_dma() {
	dma_channel_reset(DMA1, DMA_CHANNEL3);
	dma_set_peripheral_address(DMA1, DMA_CHANNEL3, (uint32_t)&SPI1_DR);
	dma_set_memory_address(DMA1, DMA_CHANNEL3, dma_buffer);
	dma_set_number_of_data(DMA1, DMA_CHANNEL3, dma_length);
	dma_enable_circular_mode(DMA1, DMA_CHANNEL3);
	dma_set_read_from_memory(DMA1, DMA_CHANNEL3);
	dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL3);
	dma_set_peripheral_size(DMA1, DMA_CHANNEL3, DMA_CCR_PSIZE_8BIT);
	dma_set_memory_size(DMA1, DMA_CHANNEL3, DMA_CCR_MSIZE_8BIT);
	dma_set_priority(DMA1, DMA_CHANNEL3, DMA_CCR_PL_HIGH);
	dma_enable_channel(DMA1, DMA_CHANNEL3);
	spi_enable_tx_dma(SPI1);
}

int main(void) {
	float error = 0.0f;
	for (int i = 0; i < dma_length; i++) {
		uint8_t byte = 0;
		for (int j = 0; j < 8; j++) {
			float sample = (cosf((float)(i * 8 + j) * 2. * 3.14159265f / samples_per_period) / 2. + 0.5);
			int8_t quantized = sample + error > 0.5 ? 1 : 0;
			error += sample - quantized;
			byte = byte << 1;
			byte |= quantized;
		}
		dma_buffer[i] = byte;
	}
	/*for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 1000; j++) {
			dma_buffer[i] = 0xFF & ((1 << i) - 1);
		}
	}
	for (int i = 8; i > 0; i++) {
		for (int j = 0; j < 1000; j++) {
			dma_buffer[i] = 0xFF & ((1 << i) - 1);
		}
	}*/
	hardware_setup();
	start_dma();
	while (1) {
		/*if (dma_buffer_idx >= dma_length) dma_buffer_idx = 0;
		spi_send(SPI1, dma_buffer[dma_buffer_idx]);
		spi_send(SPI1, dma_buffer[dma_buffer_idx]);
		spi_send(SPI1, dma_buffer[dma_buffer_idx]);
		spi_send(SPI1, dma_buffer[dma_buffer_idx]);
		dma_buffer_idx++;*/
	}

	while (1) {};
	return 0;
}