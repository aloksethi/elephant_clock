#ifndef __HARDHARE_H
#define __HARDWARE_H

// define the GPIO mappings
#define LATCH   7
#define OE_N    3
#define CLCK    4
#define SDATA   6

#define SW1     5
#define SW2     2

#define HR_HD_IDX   0
#define HR_LD_IDX   1
#define MIN_HD_IDX   2
#define MIN_LD_IDX   3
#define MAX_DIGITS  4


/* external RTC connections*/
#define EXT_RTC_I2C_DEV         (i2c0)
#define EXT_RTC_I2C_ADDRESS     (0x68)
#define EXT_RTC_I2C_BAUDRATE    (100000)
#define EXT_RTC_I2C_SDA_PIN     (16)
#define EXT_RTC_I2C_SCL_PIN     (17)

/* DS3231 internal registers*/
#define EXT_RTC_SEC_REG         (0x0)
#define EXT_RTC_MIN_REG         (0x1)
#define EXT_RTC_HR_REG          (0x2)
#define EXT_RTC_DOW_REG         (0x3)
#define EXT_RTC_DAY_REG         (0x4)
#define EXT_RTC_MON_REG         (0x5)
#define EXT_RTC_YR_REG          (0x6)

/* function definitions*/
void display_time(void);

#endif