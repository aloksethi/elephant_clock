#include <stdio.h>
#include "pico/stdlib.h"

#define LATCH 7
#define OE_N 3
#define SW1 5
#define CLCK 4

#define SDATA 6
#define SW2 2

//#define DBG_PRINTS
#define BLINK_BLED 0
#define BLINK_GLED 0
static char event_str[128];
static const char *gpio_irq_str[] = {
	"LEVEL_LOW",  // 0x1
	"LEVEL_HIGH", // 0x2
	"EDGE_FALL",  // 0x4
	"EDGE_RISE"	  // 0x8
};
uint8_t LED_SEL[] = {0x70, 0xb0, 0xd0, 0xe0};
uint8_t DIG_VAL[] = {0x3, 0xF3, 0x25, 0x0D, 0x99, 0x49, 0x41, 0x1F, 0x1, 0x9};
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

void gpio_event_string(char *buf, uint32_t events)
{
	for (uint i = 0; i < 4; i++)
	{
		uint mask = (1 << i);
		if (events & mask)
		{
			// Copy this event string into the user string
			const char *event_str = gpio_irq_str[i];
			while (*event_str != '\0')
			{
				*buf++ = *event_str++;
			}
			events &= ~mask;

			// If more events add ", "
			if (events)
			{
				*buf++ = ',';
				*buf++ = ' ';
			}
		}
	}
	*buf++ = '\0';
}

void gpio1_callback(uint gpio, uint32_t events)
{
	uint64_t tdiff;
	static absolute_time_t from_time;
	static absolute_time_t to_time;
	static int first_time = 1;
	// Put the GPIO event(s) that just happened into event_str
	// so we can print it
	gpio_acknowledge_irq(gpio, events);
	gpio_event_string(event_str, events);

	if (first_time)
	{
		from_time = get_absolute_time();
		to_time = get_absolute_time();
		first_time = 0;
	}

	to_time = get_absolute_time();
	tdiff = absolute_time_diff_us(from_time, to_time);
	from_time = to_time;

	printf("GPIO1 %d %lld %s\n", gpio, tdiff, event_str);
}

void gpio2_callback(uint gpio, uint32_t events)
{
	// Put the GPIO event(s) that just happened into event_str
	// so we can print it
	static uint32_t num = 0;
	gpio_acknowledge_irq(gpio, events);
	gpio_event_string(event_str, events);
	printf("GPIO2 %d %u %s\n", gpio, num, event_str);
	num++;
}

void pin_init(void)
{
	gpio_init(OE_N);
	gpio_set_dir(OE_N, GPIO_OUT);
	gpio_init(SDATA);
	gpio_set_dir(SDATA, GPIO_OUT);

	gpio_init(CLCK);
	gpio_set_dir(CLCK, GPIO_OUT);
	gpio_init(LATCH);
	gpio_set_dir(LATCH, GPIO_OUT);
#if 0
	gpio_init(SW1);
	gpio_set_dir(SW1, GPIO_IN);
	gpio_init(SW2);
	gpio_set_dir(SW2, GPIO_IN);
#endif
	// gpio_set_irq_enabled_with_callback(SW1, GPIO_IRQ_EDGE_FALL, true, &gpio1_callback);
	// gpio_set_irq_enabled_with_callback(SW2, GPIO_IRQ_EDGE_FALL, true, &gpio2_callback);

	gpio_set_slew_rate(SW1, GPIO_SLEW_RATE_SLOW);
	gpio_set_slew_rate(SW2, GPIO_SLEW_RATE_SLOW);
	gpio_disable_pulls(SW1);
	gpio_disable_pulls(SW2);
	gpio_set_input_hysteresis_enabled(SW1, true);
	gpio_set_input_hysteresis_enabled(SW2, true);

	gpio_put(CLCK, false);
	gpio_put(SDATA, false);
	gpio_put(LATCH, false);
	gpio_put(OE_N, false);
}
void brk_fxn(void)
{
	// printf(".");
}

volatile uint8_t dig_val=0;


bool repeating_timer_callback(struct repeating_timer *t) {
    printf("Repeat at %lld\n", time_us_64());
dig_val +=1;
dig_val = dig_val % 10;
    return true;
}


int main()
{

	const uint led_pin = 25;
	uint8_t data_u3 = 0x0;
	uint8_t data_u6;
	int i;
	uint16_t data;
	uint8_t data_in = 0;
	// Initialize LED pin
	gpio_init(led_pin);
	gpio_set_dir(led_pin, GPIO_OUT);
	pin_init();
	// Initialize chosen serial port
	stdio_init_all();

	clk_in_data(0x0);

	// Loop forever
	while (true)
	{
#if BLINK_BLED

		// Blink LED
		gpio_put(led_pin, true);
		sleep_ms(500);
		gpio_put(led_pin, false);
		sleep_ms(500);
#endif
#if 0
		data_u3 = 0x0;
		data_u6 = 0x00;
		data = data_u3<<8 | data_u6;
		clk_in_data(data);
		sleep_ms(500);
#endif
#if BLINK_GLED
		data_u3 = 0xff;
		data_u6 = 0xf0;
		data = data_u3 << 8 | data_u6;
		clk_in_data(data);
		sleep_ms(500);

		data_u3 = 0xff;
		data_u6 = 0xf1;
		data = data_u3 << 8 | data_u6;
		clk_in_data(data);
		sleep_ms(500);
#endif

#if 0
		data_u3 = 0x0;
		data_u6 = 0xf0;
		data = data_u3<<8 | data_u6;
		if (data_in == 0){
			clk_in_data(data);
		}
		data_in = 1;
		sleep_ms(500);
#endif
#if 0 // old testing loop
	for (int j=0;j<4;j++)
	{
		//uint8_t tmp = 0;
		//tmp = ~(1<<(j+4));
		//tmp = 0xff;//tmp | 0xf0;
		for (int i=0;i<10;i++)
		{
			
			//data_u3 = (~(1<<i)) & 0xff;
			data_u3= DIG_VAL[i];
			data_u6 = LED_SEL[j];//0xef; //f0
			data = data_u3<<8 | data_u6;
			clk_in_data(data);
			brk_fxn();
			sleep_ms(500);
		}
	}
#endif
	struct repeating_timer timer;
    add_repeating_timer_ms(1000, repeating_timer_callback, NULL, &timer);
		//for (int i = 0; i < 10; i++)
		while(1)
		{ 
			for (int j = 0; j < 4; j++)
			{
				int i = 0;
				i = (dig_val + j)%10;
				data_u3 = DIG_VAL[i];
				data_u6 = LED_SEL[j]; // 0xef; //f0
				data = data_u3 << 8 | data_u6;
				clk_in_data(data);
				
				sleep_ms(0.5);
			}
		}
	}
}
