/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __LINUX_SENSORS_H_INCLUDED
#define __LINUX_SENSORS_H_INCLUDED

#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/rwsem.h>

#define SENSORS_ACCELERATION_HANDLE		0
#define SENSORS_MAGNETIC_FIELD_HANDLE		1
#define SENSORS_ORIENTATION_HANDLE		2
#define SENSORS_LIGHT_HANDLE			3
#define SENSORS_PROXIMITY_HANDLE		4
#define SENSORS_GYROSCOPE_HANDLE		5
#define SENSORS_PRESSURE_HANDLE			6

#define SENSOR_TYPE_ACCELEROMETER		1
#define SENSOR_TYPE_GEOMAGNETIC_FIELD		2
#define SENSOR_TYPE_MAGNETIC_FIELD  SENSOR_TYPE_GEOMAGNETIC_FIELD
#define SENSOR_TYPE_ORIENTATION			3
#define SENSOR_TYPE_GYROSCOPE			4
#define SENSOR_TYPE_LIGHT			5
#define SENSOR_TYPE_PRESSURE			6
#define SENSOR_TYPE_TEMPERATURE			7
#define SENSOR_TYPE_PROXIMITY			8
#define SENSOR_TYPE_GRAVITY			9
#define SENSOR_TYPE_LINEAR_ACCELERATION		10
#define SENSOR_TYPE_ROTATION_VECTOR		11
#define SENSOR_TYPE_RELATIVE_HUMIDITY		12
#define SENSOR_TYPE_AMBIENT_TEMPERATURE		13
#define SENSOR_TYPE_MAGNETIC_FIELD_UNCALIBRATED	14
#define SENSOR_TYPE_GAME_ROTATION_VECTOR	15
#define SENSOR_TYPE_GYROSCOPE_UNCALIBRATED	16
#define SENSOR_TYPE_SIGNIFICANT_MOTION		17
#define SENSOR_TYPE_STEP_DETECTOR		18
#define SENSOR_TYPE_STEP_COUNTER		19
#define SENSOR_TYPE_GEOMAGNETIC_ROTATION_VECTOR	20

/**
 * struct sensors_classdev - hold the sensor general parameters and APIs
 * @dev:		The device to register.
 * @node:		The list for the all the sensor drivers.
 * @name:		Name of this sensor.
 * @vendor:		The vendor of the hardware part.
 * @handle:		The handle that identifies this sensors.
 * @type:		The sensor type.
 * @max_range:		The maximum range of this sensor's value in SI units.
 * @resolution:		The smallest difference between two values reported by
 *			this sensor.
 * @sensor_power:	The rough estimate of this sensor's power consumption
 *			in mA.
 * @min_delay:		This value depends on the trigger mode:
 *			continuous: minimum period allowed in microseconds
 *			on-change : 0
 *			one-shot :-1
 *			special : 0, unless otherwise noted
 * @fifo_reserved_event_count:	The number of events reserved for this sensor
 *				in the batch mode FIFO.
 * @fifo_max_event_count:	The maximum number of events of this sensor
 *				that could be batched.
 * @enabled:		Store the sensor driver enable status.
 * @delay_msec:		Store the sensor driver delay value. The data unit is
 *			millisecond.
 * @sensors_enable:	The handle for enable and disable sensor.
 * @sensors_poll_delay:	The handle for set the sensor polling delay time.
 */
struct sensors_classdev {
	struct device		*dev;
	struct list_head	node;
	const char		*name;
	const char		*vendor;
	int			version;
	int			handle;
	int			type;
	const char		*max_range;
	const char		*resolution;
	const char		*sensor_power;
	int			min_delay;
	int			fifo_reserved_event_count;
	int			fifo_max_event_count;
	unsigned int		enabled;
	unsigned int		delay_msec;
	/* enable and disable the sensor handle*/
	int	(*sensors_enable)(struct sensors_classdev *sensors_cdev,
					unsigned int enabled);
	int	(*sensors_poll_delay)(struct sensors_classdev *sensors_cdev,
					unsigned int delay_msec);
};

extern int sensors_classdev_register(struct device *parent,
				 struct sensors_classdev *sensors_cdev);
extern void sensors_classdev_unregister(struct sensors_classdev *sensors_cdev);

#endif		/* __LINUX_SENSORS_H_INCLUDED */
/************************************************************
  Copyright (C), 1988-1999, Huawei Tech. Co., Ltd.
  FileName: board_sensors.h
  Author: hantao(00185954)       Version : 0.1      Date:  2011-07-11
  Description:	.h file for sensors
  Version:
  Function List:
  History:
  <author>  <time>   <version >   <desc>
***********************************************************/
/*==============================================================================
History

Problem NO.         Name        Time         Reason

==============================================================================*/

#ifndef	__BOARD_SENSORS_H__
#define	__BOARD_SENSORS_H__

/*sunlibin added*/
#define GS_SUSPEND  0
#define GS_RESUME   1
#define IC_PM_ON   1
#define IC_PM_OFF  0

#define WHOAMI_LIS3DH_ACC	0x33	/* St Expected content for WAI */

/******************************************************************************
 * Rohm Accelerometer WHO_AM_I return value
 *****************************************************************************/
#define KIONIX_ACCEL_WHO_AM_I_KXTE9 		0x00
#define KIONIX_ACCEL_WHO_AM_I_KXTF9 		0x01
#define KIONIX_ACCEL_WHO_AM_I_KXTI9_1001 	0x04
#define KIONIX_ACCEL_WHO_AM_I_KXTIK_1004 	0x05
#define KIONIX_ACCEL_WHO_AM_I_KXTJ9_1005 	0x07
#define KIONIX_ACCEL_WHO_AM_I_KXTJ9_1007 	0x08
#define KIONIX_ACCEL_WHO_AM_I_KXCJ9_1008 	0x0A
#define KIONIX_ACCEL_WHO_AM_I_KXTJ2_1009 	0x09
#define KIONIX_ACCEL_WHO_AM_I_KXCJK_1013 	0x11
#define KIONIX_ACCEL_WHO_AM_I_KX023		0x15


enum input_name {
	ACC,
	AKM,
	GYRO,
	ALS,
	PS,
	SENSOR_MAX
};

/*sunlibin added*/
typedef enum
{
	COMPASS_TOP_GS_TOP 			=0,
	COMPASS_TOP_GS_BOTTOM 		=1,
	COMPASS_BOTTOM_GS_TOP 		=2,
	COMPASS_BOTTOM_GS_BOTTOM	=3,
	COMPASS_NONE_GS_BOTTOM		=4,
	COMPASS_NONE_GS_TOP			=5,
	COMPASS_GS_POSITION_MAX,
}compass_gs_position_type;

typedef enum
{
	GS_ADIX345 	= 0x01,
	GS_ST35DE	= 0x02,
	GS_ST303DLH = 0X03,
	GS_MMA8452  = 0x04,
	GS_BMA250   = 0x05,
	GS_STLIS3XH	= 0x06,
	GS_ADI346   = 0x07,
	GS_KXTIK1004= 0x08,
}hw_gs_type;

/*sunlibin added*/
/* Use this variable to control the number of
 * effective bits of the accelerometer output.
 * Use the macro definition to select the desired
 * number of effective bits. */
#define KIONIX_ACCEL_RES_12BIT	0
#define KIONIX_ACCEL_RES_8BIT	1
#define KIONIX_ACCEL_RES_6BIT	2
#define KIONIX_ACCEL_RES_16BIT	3	//KX023

/* Use this variable to control the G range of
 * the accelerometer output. Use the macro definition
 * to select the desired G range.*/
#define KIONIX_ACCEL_G_2G		0
#define KIONIX_ACCEL_G_4G		1
#define KIONIX_ACCEL_G_6G		2
#define KIONIX_ACCEL_G_8G		3

struct gs_platform_data {
	int (*adapt_fn)(void);	/* fucntion is suported in some product */
	int slave_addr;     /*I2C slave address*/
	int dev_id;         /*who am I*/
	int *init_flag;     /*Init*/
	int (*gs_power)(int on);
	int int1_gpio;
	int int2_gpio;
	int compass_gs_position;
	u8 accel_res;
	u8 accel_g_range;
};



int set_sensor_input(enum input_name name, const char *input_num);
#endif
