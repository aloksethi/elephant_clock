#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/datetime.h"
#include "hardware/rtc.h"
#include "hardware.h"

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

volatile uint8_t TIME_DATA[4] = {0,0,0,0};

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


bool repeating_timer_callback(struct repeating_timer *t) 
{
	datetime_t rtc_time;
	uint8_t hr_ld,hr_hd, min_hd,min_ld;
	bool ret;

	ret = rtc_get_datetime(&rtc_time);

	if (ret)
	{
	hr_hd = (uint8_t)(rtc_time.hour/10);
	hr_ld = (uint8_t)(rtc_time.hour%10);
	min_hd = (uint8_t)(rtc_time.min/10);
	min_ld = (uint8_t)(rtc_time.min%10);
//TIME_DATA[HR_HD]
	TIME_DATA[HR_HD_IDX] = hr_hd;
	TIME_DATA[HR_LD_IDX] = hr_ld;
	TIME_DATA[MIN_HD_IDX] = min_hd;
	TIME_DATA[MIN_LD_IDX] = min_ld;
	}
    return true;
}

void display_time(void);

int main()
{
	struct repeating_timer timer;
        char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];

    // Start on Friday 5th of June 2020 15:45:00
    datetime_t t = {
            .year  = 2023,
            .month = 10,
            .day   = 29,
            .dotw  = 1, // 0 is Sunday, so 5 is Friday
            .hour  = 20,
            .min   = 55,
            .sec   = 00
    };


	pin_init();
	// Initialize chosen serial port
	stdio_init_all();
	multicore_reset_core1();
    rtc_init();
	sleep_us(100);

#if 1
	rtc_set_datetime(&t);
#endif

add_repeating_timer_ms(60000, repeating_timer_callback, NULL, &timer); //fire every minute
	    multicore_launch_core1(display_time);
	// Loop forever
	while (true)
	{
		bool r;
		r = rtc_running();
		printf("is rtc running: %d\n", r);
		rtc_get_datetime(&t);
        datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
        printf("\r%s  %d    ", datetime_str, r);
	
		sleep_ms(1000);
		tight_loop_contents();
	}
}
