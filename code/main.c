#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/datetime.h"
#include "hardware/rtc.h"
#include "hardware.h"

typedef enum
{
	NORMAL,
	SET_CLK
} sm_state_t;

volatile uint8_t TIME_DATA[4] = {0,0,0,0};
volatile bool BLINK_DATA[4] = {0,0,0,0};
volatile sm_state_t SM_STATE = NORMAL;


int64_t alarm_callback(alarm_id_t id, void *user_data) 
{
	bool gpio_val;
    printf("Timer %d fired!\n", (int) id);
    gpio_set_irq_enabled(SW2, GPIO_IRQ_EDGE_FALL, true);
    // Can return a value here in us to fire in the future
	printf("GPIO %d irq enabled\n", SW2);
	gpio_val = gpio_get(SW2); 	
	
	if (gpio_val == false)
	{
		SM_STATE = SET_CLK;
	}
    return 0;
}

void gpio_callback(uint gpio, uint32_t events)
{
//	uint64_t tdiff;
//	static absolute_time_t from_time;
//	static absolute_time_t to_time;
//	static int first_time = 1;
	// Put the GPIO event(s) that just happened into event_str
	// so we can print it
	alarm_id_t alarm_id;
	gpio_acknowledge_irq(gpio, events);

	gpio_set_irq_enabled(SW2, GPIO_IRQ_EDGE_FALL, false);


    // Call alarm_callback in 2 seconds
    alarm_id = add_alarm_in_ms(2000, alarm_callback, NULL, false);
	if (alarm_id <= 0)
	{
		// failed to create alarm
		gpio_set_irq_enabled(SW2, GPIO_IRQ_EDGE_FALL, true);
	}
	printf("GPIO %d irq disabled\n", gpio);
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

	gpio_init(SW1);
	gpio_set_dir(SW1, GPIO_IN);
	gpio_init(SW2);
	gpio_set_dir(SW2, GPIO_IN);

	// can register only one callback
	gpio_set_irq_enabled_with_callback(SW2, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
	

	gpio_set_slew_rate(SW1, GPIO_SLEW_RATE_SLOW);
	gpio_set_slew_rate(SW2, GPIO_SLEW_RATE_SLOW);
#if 0
	gpio_disable_pulls(SW1);
	gpio_disable_pulls(SW2);
	gpio_set_input_hysteresis_enabled(SW1, true);
	gpio_set_input_hysteresis_enabled(SW2, true);
#endif
	gpio_put(CLCK, false);
	gpio_put(SDATA, false);
	gpio_put(LATCH, false);
	gpio_put(OE_N, false);
}
void brk_fxn(void)
{
	// printf(".");
}

void set_time_from_rtc(void)
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
	return;
}

bool repeating_timer_callback(struct repeating_timer *t) 
{
	set_time_from_rtc();
    return true;
}

void display_time(void);

int main()
{
	struct repeating_timer timer;
#if 0	
    char datetime_buf[256];
    char *datetime_str = &datetime_buf[0];
#endif
    // Start on Friday 5th of June 2020 15:45:00
    datetime_t t = {
            .year  = 2023,
            .month = 10,
            .day   = 31,
            .dotw  = 1, // 0 is Sunday, so 5 is Friday
            .hour  = 12,
            .min   = 00,
            .sec   = 00
    };


	pin_init();
	// Initialize chosen serial port
	stdio_init_all();
	multicore_reset_core1();
    rtc_init();
	sleep_us(100);
	rtc_set_datetime(&t);
	sleep_us(1000);	
	set_time_from_rtc();

	add_repeating_timer_ms(60000, repeating_timer_callback, NULL, &timer); //fire every minute
	    multicore_launch_core1(display_time);
	// Loop forever
	while (true)
	{
#if 0
		bool r;
		r = rtc_running();
		printf("is rtc running: %d\n", r);
		rtc_get_datetime(&t);
        datetime_to_str(datetime_str, sizeof(datetime_buf), &t);
    //    printf("\r%s  %d    ", datetime_str, r);
#endif

	if (SM_STATE == NORMAL)
	{	
		sleep_ms(1000);
	}
	else if (SM_STATE == SET_CLK)
	{
		bool gv1, gv2;
		static uint8_t local_idx = 0;
		static uint16_t local_tick = 0;
		uint8_t lcl_hr, lcl_min;

		BLINK_DATA[local_idx] = true;

   		gv1 = gpio_get(SW1);
		sleep_ms(125);
		gv2 = gpio_get(SW1);

		if ((false == gv1) && (false == gv2))
		{
			uint8_t max_digit_val; // max values of individual digits, hr digit can be 0,1,2 only min digit can be 0...5
			local_tick = 0;

			

			if (local_idx == 0)
				max_digit_val = 3;
			else if (local_idx == 2)
				max_digit_val = 6;
			else
				max_digit_val = 10;

			TIME_DATA[local_idx] = (TIME_DATA[local_idx] + 1)%max_digit_val;
			
			lcl_hr = TIME_DATA[HR_HD_IDX]*10 + TIME_DATA[HR_LD_IDX];
			lcl_min = TIME_DATA[MIN_HD_IDX]*10 + TIME_DATA[MIN_LD_IDX];

			lcl_hr %= 24;
			lcl_min %= 60;

			printf("set %d:%d\n",lcl_hr,lcl_min);
			TIME_DATA[HR_HD_IDX] = (uint8_t)(lcl_hr/10);
			TIME_DATA[HR_LD_IDX] = (uint8_t)(lcl_hr%10);
			TIME_DATA[MIN_HD_IDX] = (uint8_t)(lcl_min/10);
			TIME_DATA[MIN_LD_IDX] = (uint8_t)(lcl_min%10);
		
		}
		if (local_tick > 15)
		{
				local_tick = 0;

				BLINK_DATA[local_idx] = false;
				local_idx++;
				printf("next digit\n");
				if (local_idx > 3)
				{
					bool ret;
					datetime_t t;
					local_idx = 0;
					SM_STATE = NORMAL;

					lcl_hr = TIME_DATA[HR_HD_IDX]*10 + TIME_DATA[HR_LD_IDX];
					lcl_min = TIME_DATA[MIN_HD_IDX]*10 + TIME_DATA[MIN_LD_IDX];

					ret = rtc_get_datetime(&t);
					t.hour = lcl_hr;
					t.min = lcl_min;
					ret = rtc_set_datetime(&t);
					if (true == ret)
						printf("rtc set\n");
					else
						printf("failed to set rtc\n");

					BLINK_DATA[local_idx] = false;	
				}
		}
		local_tick++;
		sleep_ms(100);
	}
	//	tight_loop_contents();
	}
}
