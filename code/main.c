#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/datetime.h"
#include "hardware/rtc.h"
#include "hardware.h"
#include "hardware/i2c.h"

typedef enum
{
	NORMAL,
	SET_CLK
} sm_state_t;

volatile uint8_t TIME_DATA[4] = {0,0,0,0};
volatile bool BLINK_DATA[4] = {false,false,false,false};
volatile sm_state_t SM_STATE = NORMAL;



int64_t alarm_callback(alarm_id_t id, void *user_data) 
{
	bool gpio_val;
    printf("Timer %d fired!\n", (int) id);
    // Can return a value here in us to fire in the future
	printf("GPIO %d irq enabled\n", SW2);
	gpio_val = gpio_get(SW2); 	
	
	if (gpio_val == false)
	{
		SM_STATE = SET_CLK;
	}
    gpio_set_irq_enabled(SW2, GPIO_IRQ_EDGE_FALL, true);

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


    // Call alarm_callback in 1 seconds
    alarm_id = add_alarm_in_ms(1000, alarm_callback, NULL, false);
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

void enable_repeating_timer(bool val)
{
	static struct repeating_timer timer;

	if (val == true)
		add_repeating_timer_ms(60000, repeating_timer_callback, NULL, &timer); //fire every minute
	else
		cancel_repeating_timer (&timer);

	return;	
}

static void setup_ext_rtc() 
{
    gpio_init(EXT_RTC_I2C_SDA_PIN);
    gpio_set_function(EXT_RTC_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(EXT_RTC_I2C_SDA_PIN);

    gpio_init(EXT_RTC_I2C_SCL_PIN);
    gpio_set_function(EXT_RTC_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(EXT_RTC_I2C_SCL_PIN);

    i2c_init(EXT_RTC_I2C_DEV, EXT_RTC_I2C_BAUDRATE);

	return;
}

#if 0
static void write_ext_rtc_reg(uint8_t reg, uint8_t val)
{
		int ret;
uint8_t tmp[2];
tmp[0] = reg;
tmp[1] = val;

	ret = i2c_write_blocking(EXT_RTC_I2C_DEV, EXT_RTC_I2C_ADDRESS, &tmp[0], 2, false);  // true to keep master control of bus
    if (ret == PICO_ERROR_GENERIC)
		printf("failed to wirite ext rtc\n");

#if 0
	ret = i2c_write_blocking(i2c0, I2C_SLAVE_ADDRESS, &reg, 1, true);  // true to keep master control of bus
    if (ret == PICO_ERROR_GENERIC)
		printf("failed to wirite ext rtc\n");
	
	ret = i2c_write_blocking(i2c0, I2C_SLAVE_ADDRESS, &val, 1, false);  // true to keep master control of bus
    if (ret == PICO_ERROR_GENERIC)
		printf("failed to wirite value to ext rtc\n");

#endif
	
}
#endif
#if 0
static uint8_t conv_hr_to_ds3231hr(uint8_t hr)
{
	uint8_t hr_code = 0;

	if (hr >= 20)
		hr_code = 0x20 | ((hr - 20) & 0x0f);
	else if (hr_code >= 10)
		hr_code = 0x10 | ((hr -10) & 0x0f);
	else
		hr_code = hr & 0x0f;

	return hr_code;
}

static uint8_t conv_ds3231hr_to_hr(uint8_t hr_code)
{
	uint8_t hr = 0;

	if (hr_code & 0x20)
		hr = 20;
	else if (hr_code & 0x10)
		hr = 10;
	else if (hr_code & 0x30)
		printf("wtf: both 20 n 10 hr set\n");

	hr = hr + (hr_code & 0x0f);

	return hr;
}
#endif
static uint8_t conv_val_to_bcd(uint8_t val)
{
	uint8_t bcd = 0;

	if (val > 99)
		printf("error");

	bcd = ((uint8_t)(val / 10))<< 4;
	bcd |= ((uint8_t)(val % 10));
	
	return bcd;
}

static uint8_t conv_bcd_to_val(uint8_t code)
{
	uint8_t val = 0;

	val = ((code & 0xf0)>>4)*10 + ((code & 0x0f));
	
	return val;
}

static void write_ext_rtc(datetime_t *t)
{
	// lets write only the hours and mins
	uint8_t buf[3];  // two bytes of data and one register address
    int ret;

	buf[0] = EXT_RTC_MIN_REG;
	buf[1] = conv_val_to_bcd(t->min);
	buf[2] = conv_val_to_bcd(t->hour);
	ret = i2c_write_blocking(EXT_RTC_I2C_DEV, EXT_RTC_I2C_ADDRESS, &buf[0], 3, false);  
    if (ret == PICO_ERROR_GENERIC)
		printf("failed to wirite ext rtc\n");

}

static void read_ext_rtc(datetime_t *t)
{
 	uint8_t buf[7];
    uint8_t reg = EXT_RTC_SEC_REG;
	int ret;
	//uint8_t data;
	uint8_t hr;

	/* in DS3231, time registers start from 0 to 0x6, so need to read only 7 uint8s from the i2c*/
	assert(EXT_RTC_YR_REG < 7);

    ret = i2c_write_blocking(EXT_RTC_I2C_DEV, EXT_RTC_I2C_ADDRESS, &reg, 1, true);  // true to keep master control of bus
    if (ret == PICO_ERROR_GENERIC)
		printf("failed to wirite ext rtc\n");
	
	ret = i2c_read_blocking(EXT_RTC_I2C_DEV, EXT_RTC_I2C_ADDRESS, &buf[0], 7, false);  // false - finished with bus
	if (ret == PICO_ERROR_GENERIC)
		printf("failed to read ext rtc\n");

	// hour is in reg 0x2
	hr = conv_bcd_to_val(buf[EXT_RTC_HR_REG]);
	printf("bcd Time:%02d:%02x:%02x  Day:%1d Date:%02x-%02x-20%02x\n",hr, buf[1], buf[0] , buf[3], buf[4], buf[5], buf[6]);

	t->year = 2000 + conv_bcd_to_val(buf[EXT_RTC_YR_REG]);  //device returns only last two digits of the year. not handling the rolloever to next centuary
	t->month = conv_bcd_to_val(buf[EXT_RTC_MON_REG]);
	t->day = conv_bcd_to_val(buf[EXT_RTC_DAY_REG]);
	t->dotw = conv_bcd_to_val(buf[EXT_RTC_DOW_REG]);
	t->hour = conv_bcd_to_val(buf[EXT_RTC_HR_REG]);
	t->min = conv_bcd_to_val(buf[EXT_RTC_MIN_REG]);
	t->sec = conv_bcd_to_val(buf[EXT_RTC_SEC_REG]);

	printf("Time:%02d:%02d:%02d  Day:%1d Date:%02d-%02d-%02d\n",t->hour, t->min, t->sec , t->dotw, t->day, t->month, t->year);
	return;
}
//volatile uint8_t call_write = 0;
int main()
{
    datetime_t t;

	pin_init();
	// Initialize chosen serial port
	stdio_init_all();
	multicore_reset_core1();
    rtc_init();
	setup_ext_rtc();
	read_ext_rtc(&t);

	sleep_us(100);
	rtc_set_datetime(&t);
	sleep_us(1000);	
	set_time_from_rtc();

	enable_repeating_timer(true);
	//add_repeating_timer_ms(60000, repeating_timer_callback, NULL, &timer); //fire every minute

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
		
		enable_repeating_timer(false);

   		gv1 = gpio_get(SW1);
		sleep_ms(175);
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

			printf("set %02d:%02d\n",lcl_hr,lcl_min);
			TIME_DATA[HR_HD_IDX] = (uint8_t)(lcl_hr/10);
			TIME_DATA[HR_LD_IDX] = (uint8_t)(lcl_hr%10);
			TIME_DATA[MIN_HD_IDX] = (uint8_t)(lcl_min/10);
			TIME_DATA[MIN_LD_IDX] = (uint8_t)(lcl_min%10);
			printf("set_read %1d%1d:%1d%1d\n",TIME_DATA[HR_HD_IDX], TIME_DATA[HR_LD_IDX], TIME_DATA[MIN_HD_IDX], TIME_DATA[MIN_LD_IDX]   );

		}
		if (local_tick > 20) //2 seconds for each digit?
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
					
					printf("final_read %1d%1d:%1d%1d\n",TIME_DATA[HR_HD_IDX], TIME_DATA[HR_LD_IDX], TIME_DATA[MIN_HD_IDX], TIME_DATA[MIN_LD_IDX]   );

					lcl_hr = TIME_DATA[HR_HD_IDX]*10 + TIME_DATA[HR_LD_IDX];
					lcl_min = TIME_DATA[MIN_HD_IDX]*10 + TIME_DATA[MIN_LD_IDX];

					ret = rtc_get_datetime(&t);
					t.hour = lcl_hr;
					t.min = lcl_min;
					ret = rtc_set_datetime(&t);
					write_ext_rtc(&t);

					if (true == ret)
					{
						printf("rtc set %d:%d\n",t.hour, t.min);
					}
					else
						printf("failed to set rtc\n");

					BLINK_DATA[local_idx] = false;	
					enable_repeating_timer(true);

				}
		}
		local_tick++;
		sleep_ms(100);
	}
	//	tight_loop_contents();
	}
}
