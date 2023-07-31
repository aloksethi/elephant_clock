#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware.h"

uint8_t LED_SEL[] = {0x70, 0xb0, 0xd0, 0xe0};
uint8_t DIG_VAL[] = {0x3, 0x9F, 0x25, 0x0D, 0x99, 0x49, 0x41, 0x1F, 0x1, 0x9};

extern volatile uint8_t TIME_DATA[];

void clk_in_data(uint16_t data)
{
	uint8_t i;
	const int delay_ms = 0;

	gpio_put(OE_N, true);
#ifdef DBG_PRINTS
	printf("data is: %x\r\n", data);
#endif
	for (i = 0; i < sizeof(data) * 8; i++)
	{
		uint8_t tmp;

		tmp = (data & (0x1 << i)) >> i;
		if (tmp)
		{
			gpio_put(SDATA, true);
#ifdef DBG_PRINTS
			printf(" 1 ");
#endif
		}
		else
		{
			gpio_put(SDATA, false);
#ifdef DBG_PRINTS
			printf(" 0 ");
#endif
		}

		sleep_ms(delay_ms);

		gpio_put(CLCK, true);
		sleep_ms(delay_ms);
		gpio_put(CLCK, false);
		//		gpio_put(SDATA, false);
	}
	gpio_put(LATCH, true);
	sleep_ms(delay_ms);
	gpio_put(LATCH, false);

	gpio_put(OE_N, false);
#ifdef DBG_PRINTS
	printf("\n");
#endif
	return;
}

void display_time(void)
{
	uint8_t data_u3, data_u6;
	uint16_t data;

	while (1)
	{
		for (int j = 0; j < 4; j++)
		{
			int i = 0;
			i = TIME_DATA[j];
			data_u3 = DIG_VAL[i];
			data_u6 = LED_SEL[j]; // 0xef; //f0
			data = data_u3 << 8 | data_u6;
			clk_in_data(data);

//5 ms is the max to sleep otherwise the digits will flicker --> gives max intensity
//.01 ms  --?> barely visible
			sleep_ms(5);
		}
	}
}
