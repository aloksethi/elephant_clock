#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware.h"

//#define DBG_PRINTS

uint8_t LED_SEL[] = {0x70, 0xb0, 0xd0, 0xe0};
//  values from 0 to 9. value of 0xff for turning the digit off.
#define BLANK_DATA 0xff
uint8_t DIG_VAL[] = {0x3, 0x9F, 0x25, 0x0D, 0x99, 0x49, 0x41, 0x1F, 0x1, 0x9};



uint32_t sleep_time = 1000; // 5ms is the sleep. every digit will be illuminated for 5ms. reduce it to 1 if you want to reduce the brighteness.
#define BLANK_T1 ((uint32_t)(20)) /* it is the OFF time. due to cyclic nature, a digit is 3/4 times off and 1/4 times on */
#define BLANK_T2 ((uint32_t)(BLANK_T1 + 20)) /* it is the ON time. summing two uint32s be vary of overflow */
//#define BLANK_T1 ((uint32_t)(500000 - (3*sleep_time))/sleep_time) /* it is the OFF time. due to cyclic nature, a digit is 3/4 times off and 1/4 times on */
//#define BLANK_T2 (uint32_t)((BLANK_T1*sleep_time + (uint32_t)(500000 - (1*sleep_time)))/sleep_time) /* it is the ON time. summing two uint32s be vary of overflow */

extern volatile uint8_t TIME_DATA[];
extern volatile bool BLINK_DATA[];

void clk_in_data(uint16_t data)
{
	uint8_t i;
//	const int delay_ms = 0;

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

//		sleep_ms(delay_ms);

		gpio_put(CLCK, true);
//		sleep_ms(delay_ms);
		gpio_put(CLCK, false);
		//		gpio_put(SDATA, false);
	}
	gpio_put(LATCH, true);
//	sleep_ms(delay_ms);
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
	//uint8_t slp_time_idx;
	//uint32_t slp_time[]={1, 10};
	//uint8_t counter = 0;
	// so the timer tick for this fxn is approx 5ms, but it goes through digits cyclyly, so a digit will be changed after every 20ms. 
	// thus a digit will saso when a digit blinks, then it should stay illuminated for Xms and off for Xms
	// X = 200
	while (1)
	{
		//counter ++;
		for (int j = 0; j < MAX_DIGITS; j++) // scroll through the four digits
		{
			int i = 0;
			//uint8_t sleep_time;
			//static uint8_t toggle = 0;

			i = TIME_DATA[j];

			if (BLINK_DATA[j] == true)
			{
				static uint32_t blnk_counter = 0;

				blnk_counter++;
				if (blnk_counter < BLANK_T1)
					data_u3 = BLANK_DATA;
				else if (blnk_counter <BLANK_T2)
					data_u3 = DIG_VAL[i];
				else
				{
					data_u3 = DIG_VAL[i];
					blnk_counter = 0;
				}
			}
			else
				data_u3 = DIG_VAL[i];

			data_u6 = LED_SEL[j]; // 0xef; //f0
			data = data_u3 << 8 | data_u6;
			clk_in_data(data);

			//sleep_us(slp_time[slp_time_idx]);
			sleep_us(sleep_time);
		}
	}

}
#if 0
function for testing sleep_us values
void display_time(void)
{
	uint8_t data_u3, data_u6;
	uint16_t data;
	uint8_t slp_time_idx;
	uint32_t slp_time[]={1, 10};
	while (1)
	{
		for (slp_time_idx = 0;slp_time_idx<(sizeof(slp_time)/sizeof(slp_time[0]));slp_time_idx++)
		{
			int i=0;
			printf("slp_time value is %d\n", slp_time[slp_time_idx]);
			while (i<100000)
			{
				i++;
				for (int j = 0; j < MAX_DIGITS; j++) // scroll through the four digits
				{
					int i = 0;
					uint8_t sleep_time;
					static uint8_t toggle = 0;

					i = TIME_DATA[j];
					
					if (BLINK_DATA[j] == true)
					{
						data_u3 = 0xff;
					}
					else
						data_u3 = DIG_VAL[i];

					data_u6 = LED_SEL[j]; // 0xef; //f0
					data = data_u3 << 8 | data_u6;
					clk_in_data(data);
#if 0
					if (BLINK_DATA[j] == true)
					{

						if (toggle )
						{
							sleep_time = 0;
							toggle = 0;
						}
						else
						{
							sleep_time = 50;
							toggle = 1;
						}

					}
#endif					
					//			else
					//sleep_time = .1;
					//5 ms is the max to sleep otherwise the digits will flicker --> gives max intensity
					//.01 ms  --?> barely visible
					sleep_us(slp_time[slp_time_idx]);
					//sleep_ms(sleep_time);
				}
			}
		}
	}
}
#endif
