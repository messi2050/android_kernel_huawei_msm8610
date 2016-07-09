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
 *
 */
#include <mach/gpiomux.h>
#include "msm_sensor.h"
#include "msm_sd.h"
#include "camera.h"
#include "msm_cci.h"
#include "msm_camera_io_util.h"
#include "msm_camera_i2c_mux.h"
#include <mach/rpm-regulator.h>
#include <mach/rpm-regulator-smd.h>
#include <linux/regulator/consumer.h>
#include <misc/app_info.h>

#define S5K4E1_SUNNY_SENSOR_NAME "s5k4e1_sunny"
DEFINE_MSM_MUTEX(s5k4e1_sunny_mut);

#undef CDBG
#define S5K4E1_SUNNY_DEBUG
#ifdef S5K4E1_SUNNY_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif


static struct msm_sensor_ctrl_t s5k4e1_sunny_s_ctrl;

/*change the order of the DOVDD and AVDD, to avoid ov5648 voltage step */
static struct msm_sensor_power_setting s5k4e1_sunny_power_setting[] = {
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VIO,
		.config_val = 0,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VANA,
		.config_val = 0,
		.delay = 1,
	},
	//delete VDIG not used
	/*
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VDIG,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
	*/
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 24000000,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},
};

static struct msm_sensor_power_setting s5k4e1_sunny_power_down_setting[] = {
	{
		.seq_type = SENSOR_I2C_MUX,
		.seq_val = 0,
		.config_val = 0,
		.delay = 0,
	},
	{
		.seq_type = SENSOR_CLK,
		.seq_val = SENSOR_CAM_MCLK,
		.config_val = 24000000,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 5,
	},
	//delete VDIG not used
	/*
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VDIG,
		.config_val = GPIO_OUT_LOW,
		.delay = 1,
	},
	*/
	/*change the order of the DOVDD and AVDD, to avoid ov5648 voltage step */
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VANA,
		.config_val = 0,
		.delay = 1,
	},
	{
		.seq_type = SENSOR_VREG,
		.seq_val = CAM_VIO,
		.config_val = 0,
		.delay = 20,
	},
};

static struct v4l2_subdev_info s5k4e1_sunny_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_SGRBG10_1X10,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
};

static const struct i2c_device_id s5k4e1_sunny_i2c_id[] = {
	{S5K4E1_SUNNY_SENSOR_NAME, (kernel_ulong_t)&s5k4e1_sunny_s_ctrl},
	{ }
};

static int32_t msm_s5k4e1_sunny_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	return msm_sensor_i2c_probe(client, id, &s5k4e1_sunny_s_ctrl);
}

static struct i2c_driver s5k4e1_sunny_i2c_driver = {
	.id_table = s5k4e1_sunny_i2c_id,
	.probe  = msm_s5k4e1_sunny_i2c_probe,
	.driver = {
		.name = S5K4E1_SUNNY_SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client s5k4e1_sunny_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static const struct of_device_id s5k4e1_sunny_dt_match[] = {
	{.compatible = "qcom,s5k4e1_sunny", .data = &s5k4e1_sunny_s_ctrl},
	{}
};

MODULE_DEVICE_TABLE(of, s5k4e1_sunny_dt_match);

static struct platform_driver s5k4e1_sunny_platform_driver = {
	.driver = {
		.name = "qcom,s5k4e1_sunny",
		.owner = THIS_MODULE,
		.of_match_table = s5k4e1_sunny_dt_match,
	},
};

static int32_t s5k4e1_sunny_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	const struct of_device_id *match;
	match = of_match_device(s5k4e1_sunny_dt_match, &pdev->dev);
	rc = msm_sensor_platform_probe(pdev, match->data);
	return rc;
}

static int __init s5k4e1_sunny_init_module(void)
{
	int32_t rc = 0;
	pr_info("%s:%d\n", __func__, __LINE__);
	rc = platform_driver_probe(&s5k4e1_sunny_platform_driver,
		s5k4e1_sunny_platform_probe);
	if (!rc)
		return rc;
	pr_err("%s:%d rc %d\n", __func__, __LINE__, rc);
	return i2c_add_driver(&s5k4e1_sunny_i2c_driver);
}

static void __exit s5k4e1_sunny_exit_module(void)
{
	pr_info("%s:%d\n", __func__, __LINE__);
	if (s5k4e1_sunny_s_ctrl.pdev) {
		msm_sensor_free_sensor_data(&s5k4e1_sunny_s_ctrl);
		platform_driver_unregister(&s5k4e1_sunny_platform_driver);
	} else
		i2c_del_driver(&s5k4e1_sunny_i2c_driver);
	return;
}

#ifdef CONFIG_HUAWEI_KERNEL_CAMERA
/****************************************************************************
* FunctionName: s5k4e1_sunny_add_project_name;
* Description :  add the project name and app_info display;
***************************************************************************/
static int s5k4e1_sunny_add_project_name(struct msm_sensor_ctrl_t *s_ctrl)
{
    int32_t rc = 0;
    /*Todo: check the module before cp project name, when we use two sensors with the same IC*/

    /*add project name for the project menu*/
    strncpy(s_ctrl->sensordata->sensor_info->sensor_project_name, "23060132FF-SAM-S", strlen("23060132FF-SAM-S")+1);

    pr_info("%s %d : s5k4e1_sunny_add_project_name sensor_project_name=%s \n",  __func__, __LINE__,
            s_ctrl->sensordata->sensor_info->sensor_project_name);

    /*add the app_info*/
    rc = app_info_set("camera_slave", "s5k4e1_sunny");

    if(0 != rc)
    {
        pr_err("s5k4e1_sunny_add_project_name failed\n");
    }
    else
    {
        pr_info("s5k4e1_sunny_add_project_name OK\n");
    }

    return rc;
}
#endif

static struct msm_sensor_fn_t s5k4e1_sunny_sensor_func_tbl = {
	.sensor_config = msm_sensor_config,
	.sensor_power_up = msm_sensor_power_up,
	.sensor_power_down = msm_sensor_power_down,
	.sensor_match_id = msm_sensor_match_id,
#ifdef CONFIG_HUAWEI_KERNEL_CAMERA
	.sensor_add_project_name = s5k4e1_sunny_add_project_name,
#endif
};

static struct msm_sensor_ctrl_t s5k4e1_sunny_s_ctrl = {
	.sensor_i2c_client = &s5k4e1_sunny_sensor_i2c_client,
	.power_setting_array.power_setting = s5k4e1_sunny_power_setting,
	.power_setting_array.size = ARRAY_SIZE(s5k4e1_sunny_power_setting),
	.power_setting_array.power_down_setting = s5k4e1_sunny_power_down_setting,
	.power_setting_array.size_down = ARRAY_SIZE(s5k4e1_sunny_power_down_setting),
	.msm_sensor_mutex = &s5k4e1_sunny_mut,
	.sensor_v4l2_subdev_info = s5k4e1_sunny_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(s5k4e1_sunny_subdev_info),
#ifdef CONFIG_HUAWEI_KERNEL_CAMERA
    .func_tbl = &s5k4e1_sunny_sensor_func_tbl,
#endif
};

module_init(s5k4e1_sunny_init_module);
module_exit(s5k4e1_sunny_exit_module);
MODULE_DESCRIPTION("s5k4e1_sunny");
MODULE_LICENSE("GPL v2");

