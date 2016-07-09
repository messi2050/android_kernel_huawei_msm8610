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
#define IMX134_LITEON_13P1_GPIO 98
#define IMX134_LITEON_13P1_MODULE_ID  0
#define FUNC_SUCCESS 0


#define IMX134_LITEON_13P1_SENSOR_NAME "imx134_liteon_13p1"
DEFINE_MSM_MUTEX(imx134_liteon_13p1_mut);

#define GPIO_CAM_DVDD_EN 101

#undef CDBG
#define IMX134_LITEON_13P1_DEBUG
#ifdef IMX134_LITEON_13P1_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif

static struct msm_sensor_ctrl_t imx134_liteon_13p1_s_ctrl;

/*use its own power up&down sequence*/
/*change the order of the DOVDD and AVDD, to avoid ov5648 voltage step */
static struct msm_sensor_power_setting imx134_liteon_13p1_power_setting[] = {
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VDIG,
		.config_val = GPIO_OUT_HIGH,
		.delay = 1,
	},
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
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_HIGH,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_AF_PWDM,
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

static struct msm_sensor_power_setting imx134_liteon_13p1_power_down_setting[] = {
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
		.seq_val = SENSOR_GPIO_AF_PWDM,
		.config_val = GPIO_OUT_LOW,
		.delay = 5,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_RESET,
		.config_val = GPIO_OUT_LOW,
		.delay = 5,
	},
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
		.delay = 1,
	},
	{
		.seq_type = SENSOR_GPIO,
		.seq_val = SENSOR_GPIO_VDIG,
		.config_val = GPIO_OUT_LOW,
		.delay = 20,
	},
};

static struct v4l2_subdev_info imx134_liteon_13p1_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_SBGGR10_1X10, //chage for camera flip/mirror
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
};

static const struct i2c_device_id imx134_liteon_13p1_i2c_id[] = {
	{IMX134_LITEON_13P1_SENSOR_NAME, (kernel_ulong_t)&imx134_liteon_13p1_s_ctrl},
	{ }
};

static int32_t msm_imx134_liteon_13p1_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	return msm_sensor_i2c_probe(client, id, &imx134_liteon_13p1_s_ctrl);
}

static struct i2c_driver imx134_liteon_13p1_i2c_driver = {
	.id_table = imx134_liteon_13p1_i2c_id,
	.probe  = msm_imx134_liteon_13p1_i2c_probe,
	.driver = {
		.name = IMX134_LITEON_13P1_SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client imx134_liteon_13p1_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static const struct of_device_id imx134_liteon_13p1_dt_match[] = {
	{.compatible = "qcom,imx134_liteon_13p1", .data = &imx134_liteon_13p1_s_ctrl},
	{}
};

/****************************************************************************
* FunctionName: hw_imx134_match_module;
* Description : Check which manufacture this module based Sony image sensor imx134 belong to;
***************************************************************************/
static int imx134_liteon_13p1_match_module(struct msm_sensor_ctrl_t *s_ctrl)
{
    int rc = 0; //return value
    int pullup_read = -ENODEV; //pullup id value
    int pulldown_read = -ENODEV ; //pulldown id value
    int module_gpio = IMX134_LITEON_13P1_GPIO ; //gpio num of id pin
    pr_info("%s %d :  sensor_name=%s\n",  __func__, __LINE__,IMX134_LITEON_13P1_SENSOR_NAME);

    rc = gpio_request(module_gpio, "imx134_idpin");
	//gpio request fail
    if (rc < 0) 
    {
        pr_err("%s %d :  gpio request fail\n",  __func__, __LINE__);
        rc = -ENODEV;
        return rc;
    }

    /*config id to pull down and read*/
    rc = gpio_tlmm_config(GPIO_CFG(module_gpio,0,GPIO_CFG_INPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_2MA),GPIO_CFG_ENABLE);
    //config fail
    if (rc < 0) 
    {
        pr_err("%s %d :  gpio config fail\n",  __func__, __LINE__);
        goto get_module_id_fail;	
    }
    mdelay(1);
    //read id pin
    pulldown_read = gpio_get_value(module_gpio);
	
    /*config id to pull up and read*/
    rc = gpio_tlmm_config(GPIO_CFG(module_gpio,0,GPIO_CFG_INPUT,GPIO_CFG_PULL_UP,GPIO_CFG_2MA),GPIO_CFG_ENABLE);
    //config fail
    if (rc < 0) 
    {
        pr_err("%s %d :  gpio config fail\n",  __func__, __LINE__);

        goto get_module_id_fail;	
    }

    mdelay(1);
    //read id pin
    pullup_read = gpio_get_value(module_gpio);
    
    if(pulldown_read != pullup_read)//float
    {
        pr_err("%s %d : camera module pin float!\n",  __func__, __LINE__);

        gpio_tlmm_config(GPIO_CFG(module_gpio,0,GPIO_CFG_INPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),GPIO_CFG_ENABLE);
        goto get_module_id_fail;	

    }
    else//connect 
    {
        //pullup_read==pulldown_read

        /* if the moudle is liteon, the value is 0, if the moudle is sunny ,the value is 1 */
        if(IMX134_LITEON_13P1_MODULE_ID == pulldown_read)
        {
            //rc = IMX134_LITEON_13P1_MODULE_ID detect success
            rc = FUNC_SUCCESS;
            pr_info("check module id from camera module id PIN:OK \n");
            gpio_tlmm_config(GPIO_CFG(module_gpio,0,GPIO_CFG_INPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),GPIO_CFG_ENABLE);
            gpio_free(module_gpio);
			//return success
            return rc;
        }
        else
        {
            //detect fail not match
            pr_err("%s %d :  module id from camera module id not match!  \n",  __func__, __LINE__);
            gpio_tlmm_config(GPIO_CFG(module_gpio,0,GPIO_CFG_INPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),GPIO_CFG_ENABLE);
            goto get_module_id_fail;	
        }

    }

//error handler
get_module_id_fail:

    gpio_free(module_gpio);
    rc = -ENODEV;

    return rc;

    
}

#ifdef CONFIG_HUAWEI_KERNEL_CAMERA
/****************************************************************************
* FunctionName: s5k4e1_liteon_add_project_name;
* Description :  add the project name and app_info display;
***************************************************************************/
static int imx134_liteon_13p1_add_project_name(struct msm_sensor_ctrl_t *s_ctrl)
{
    int rc = 0;
    /*add project name for the project menu*/
    strncpy(s_ctrl->sensordata->sensor_info->sensor_project_name, "23060131FA-IMX-L", strlen("23060131FA-IMX-L")+1);
    pr_info("%s %d : imx134_liteon_13p1_add_project_name sensor_project_name=%s \n",  __func__, __LINE__,
            s_ctrl->sensordata->sensor_info->sensor_project_name);
    /*add the app_info*/
    rc = app_info_set("camera_main", "imx134_liteon");
    if(FUNC_SUCCESS != rc )
    {//set fail
        pr_err("%s %d : imx134_liteon_13p1_add_project_name fail! \n",  __func__, __LINE__);
    }
    else
    {//set ok
        pr_info("imx134_liteon_13p1_add_project_name OK\n");
    }

    return rc;
}
#endif


MODULE_DEVICE_TABLE(of, imx134_liteon_13p1_dt_match);

static struct platform_driver imx134_liteon_13p1_platform_driver = {
	.driver = {
		.name = "qcom,imx134_liteon_13p1",
		.owner = THIS_MODULE,
		.of_match_table = imx134_liteon_13p1_dt_match,
	},
};

static int32_t imx134_liteon_13p1_platform_probe(struct platform_device *pdev)
{
	int32_t rc = 0;
	const struct of_device_id *match;
	match = of_match_device(imx134_liteon_13p1_dt_match, &pdev->dev);
	rc = msm_sensor_platform_probe(pdev, match->data);
	return rc;
}

static int __init imx134_liteon_13p1_init_module(void)
{
	int32_t rc = 0;
	pr_info("%s:%d\n", __func__, __LINE__);
	rc = platform_driver_probe(&imx134_liteon_13p1_platform_driver,
		imx134_liteon_13p1_platform_probe);
	if (!rc)
		return rc;
	pr_err("%s:%d rc %d\n", __func__, __LINE__, rc);
	return i2c_add_driver(&imx134_liteon_13p1_i2c_driver);
}

static void __exit imx134_liteon_13p1_exit_module(void)
{
	pr_info("%s:%d\n", __func__, __LINE__);
	if (imx134_liteon_13p1_s_ctrl.pdev) {
		msm_sensor_free_sensor_data(&imx134_liteon_13p1_s_ctrl);
		platform_driver_unregister(&imx134_liteon_13p1_platform_driver);
	} else
		i2c_del_driver(&imx134_liteon_13p1_i2c_driver);
	return;
}

static struct msm_sensor_fn_t imx134_liteon_13p1_sensor_func_tbl = {
	.sensor_config = msm_sensor_config,
	.sensor_power_up = msm_sensor_power_up,
	.sensor_power_down = msm_sensor_power_down,
	.sensor_match_id = msm_sensor_match_id,
	.sensor_match_module = imx134_liteon_13p1_match_module,
#ifdef CONFIG_HUAWEI_KERNEL_CAMERA
	.sensor_add_project_name = imx134_liteon_13p1_add_project_name,
#endif
};

static struct msm_sensor_ctrl_t imx134_liteon_13p1_s_ctrl = {
	.sensor_i2c_client = &imx134_liteon_13p1_sensor_i2c_client,
	.power_setting_array.power_setting = imx134_liteon_13p1_power_setting,
	.power_setting_array.size = ARRAY_SIZE(imx134_liteon_13p1_power_setting),
	.power_setting_array.power_down_setting = imx134_liteon_13p1_power_down_setting,
	.power_setting_array.size_down = ARRAY_SIZE(imx134_liteon_13p1_power_down_setting),
	.msm_sensor_mutex = &imx134_liteon_13p1_mut,
	.sensor_v4l2_subdev_info = imx134_liteon_13p1_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(imx134_liteon_13p1_subdev_info),
	.func_tbl = &imx134_liteon_13p1_sensor_func_tbl,
};

module_init(imx134_liteon_13p1_init_module);
module_exit(imx134_liteon_13p1_exit_module);
MODULE_DESCRIPTION("imx134_liteon_13p1");
MODULE_LICENSE("GPL v2");

