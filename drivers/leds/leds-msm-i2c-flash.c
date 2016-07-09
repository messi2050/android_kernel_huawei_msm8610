#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/leds.h>

#include "leds-msm-i2c-flash.h"

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <linux/hw_dev_dec.h>
#endif

#undef CDBG
#ifdef CONFIG_MSMB_CAMERA_DEBUG
#define CDBG(fmt, args...) pr_err(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#endif


#define ENABLE_REGISTER 0x0A
#define MODE_BIT_MASK 0x03
#define MODE_BIT_STANDBY 0x00
#define MODE_BIT_INDICATOR 0x01
#define MODE_BIT_TORCH 0x02
#define MODE_BIT_FLASH 0x03
#define ENABLE_BIT_FLASH 0x20
#define ENABLE_BIT_TORCH 0x10

#define CURRENT_REGISTER 0x09
#define CURRENT_TORCH_MASK 0x70
#define CURRENT_TORCH_SHIFT 4
#define CURRENT_FLASH_MASK 0x0F
#define CURRENT_FLASH_SHIFT 0

#define FLASH_FEATURE_REGISTER 0x08
#define FLASH_TIMEOUT_MASK 0x07
#define FLASH_TIMEOUT_SHIFT 0

#define FLASH_CHIP_ID_MASK 0x07
#define FLASH_CHIP_ID 0x0

#define LED_TRIGGER_DEFAULT		"none"

int turn_on_torch(struct lm3642_data* data);
int turn_off_torch(struct lm3642_data* data);
int set_flash_current(struct lm3642_data* data, enum flash_current_level i);
static struct lm3642_data* lm3642;

static void lm3642_clear_error_flag(struct lm3642_data* data)
{
	int err = 0;
    struct i2c_client* client = data->client;

	err = i2c_smbus_read_byte_data(client, 0x0B);
    if(err < 0) {
        printk("read current register fail!\n");
    } else {
        printk("lm3642. [%02X] = %02X\n", 0x0B, err);
    }
}

static int atoi(const char *psz_buf)
{
	const char *pch = psz_buf;
	int ret = 0;

	while (' ' == *pch)
		pch++;

	while(pch != NULL && *pch >= '0' && *pch <= '9') {
        ret = ret * 10 + (*pch - '0');
        pch++;
    }

	return ret;
}

int convert_flash_timeout(enum flash_timeout to) {
    int timeout = 300;
    if(to <= FLASH_TIMEOUT_MAX) {
        timeout = 100 * (1 + (int)to);
    }
    return timeout;
}

int turn_on_torch(struct lm3642_data* data) {
    int err = 0;
    struct i2c_client* client = data->client;
    struct lm3642_platform_data* pdata = data->pdata;

    if(pdata->status == TORCH_ON) {
        return 0;
    }
	
	lm3642_clear_error_flag(data);
    // Set torch mode. 
    err = i2c_smbus_write_byte_data(client, ENABLE_REGISTER, MODE_BIT_TORCH/* | ENABLE_BIT_TORCH*/);
    if(err < 0) {
        printk("write i2c register failed! line: %d\n", __LINE__);
        return err;
    }


    pdata->status = TORCH_ON;

    return err;
}

int turn_off_torch(struct lm3642_data* data) {
    int err = 0;
    struct i2c_client* client = data->client;
    struct lm3642_platform_data* pdata = data->pdata;

    if(pdata->status == TORCH_OFF) {
        return 0;
    }

	lm3642_clear_error_flag(data);
    // Set torch mode. 
    err = i2c_smbus_write_byte_data(client, ENABLE_REGISTER, MODE_BIT_STANDBY);
    if(err < 0) {
        printk("write i2c register failed! line: %d\n", __LINE__);
        return err;
    }
	
    // pull down flash pins
    err = gpio_direction_output(pdata->flash, 0);
    if (err) {
        printk("%s: Failed to set gpio %d\n", __func__,
               pdata->flash);
        return err;
    }

    pdata->status = TORCH_OFF;

    return err;
}

static int turn_on_flash(struct lm3642_data* data)
{
    int err = 0;
    struct i2c_client* client = data->client;
    struct lm3642_platform_data* pdata = data->pdata;

	lm3642_clear_error_flag(data);
    // Set flash mode and only enable strobe pin
    err = i2c_smbus_write_byte_data(client, ENABLE_REGISTER, MODE_BIT_FLASH | ENABLE_BIT_FLASH);
    if(err < 0) {
        printk("write i2c register failed! line: %d\n", __LINE__);
        return err;
    }
    //set flash current to 1031mA
    err = set_flash_current(data,FLASH_I_1031P25);
    //error process
    if(err < 0) {
        printk("set_flash_current failed! line: %d\n", __LINE__);
        return err;
    }
    err = gpio_direction_output(pdata->flash, 1);
    if (err) {
        printk("%s: Failed to set gpio %d\n", __func__,
               pdata->flash);
        return err;
    }

    return err;
}

static int turn_off_flash(struct lm3642_data* data)
{
    int err = 0;
    struct i2c_client* client = data->client;
    struct lm3642_platform_data* pdata = data->pdata;

	lm3642_clear_error_flag(data);
    // Set flash mode and only enable strobe pin
    err = i2c_smbus_write_byte_data(client, ENABLE_REGISTER, MODE_BIT_STANDBY);
    if(err < 0) {
        printk("write i2c register failed! line: %d\n", __LINE__);
        return err;
    }

    err = gpio_direction_output(pdata->flash, 0);
    if (err) {
        printk("%s: Failed to set gpio %d\n", __func__,
               pdata->flash);
        return err;
    }

    return err;
}



int trigger_flash(struct lm3642_data* data) {
    int err = 0;
    struct i2c_client* client = data->client;
    struct lm3642_platform_data* pdata = data->pdata;

	lm3642_clear_error_flag(data);
    // Set flash mode and only enable strobe pin
    err = i2c_smbus_write_byte_data(client, ENABLE_REGISTER, MODE_BIT_FLASH | ENABLE_BIT_FLASH);
    if(err < 0) {
        printk("write i2c register failed! line: %d\n", __LINE__);
        return err;
    }

    err = gpio_direction_output(pdata->flash, 1);
    if (err) {
        printk("%s: Failed to set gpio %d\n", __func__,
               pdata->flash);
        return err;
    }

    msleep(convert_flash_timeout(pdata->timeout));

    err = gpio_direction_output(pdata->flash, 0);
    if (err) {
        printk("%s: Failed to set gpio %d\n", __func__,
               pdata->flash);
        return err;
    }

    return err;
}

int set_flash_current(struct lm3642_data* data, enum flash_current_level i)
{
    int err = 0;
    struct i2c_client* client = data->client;
    struct lm3642_platform_data* pdata = data->pdata;

    pdata->flash_current = i;
	
	lm3642_clear_error_flag(data);
	
    err = i2c_smbus_write_byte_data(client, CURRENT_REGISTER,
        (pdata->flash_current << CURRENT_FLASH_SHIFT) | (pdata->torch_current << CURRENT_TORCH_SHIFT));
    if(err < 0) {
        printk("lm3642 write i2c faile\n");
    }

    return err;
}

int set_torch_current(struct lm3642_data* data, enum torch_current_level i)
{
    int err = 0;
    struct i2c_client* client = data->client;
    struct lm3642_platform_data* pdata = data->pdata;

    pdata->torch_current = i;
	
	lm3642_clear_error_flag(data);
	
    err = i2c_smbus_write_byte_data(client, CURRENT_REGISTER,
        (pdata->flash_current << CURRENT_FLASH_SHIFT) | (pdata->torch_current << CURRENT_TORCH_SHIFT));
    if(err < 0) {
        printk("lm3642 write i2c faile\n");
    }

    return err;
}

int set_flash_timeout(struct lm3642_data* data, enum flash_timeout to)
{
    int err = 0, tmp = 0;
    struct i2c_client* client = data->client;
    struct lm3642_platform_data* pdata = data->pdata;

    pdata->timeout = to;

    tmp = i2c_smbus_read_byte_data(client, FLASH_FEATURE_REGISTER);
    if(tmp < 0) {
        printk("%s read i2c fail\n", __func__);
        return tmp;
    }
	
	err = i2c_smbus_read_byte_data(client, 0x0B);
    if(err < 0) {
        printk("read current register fail!\n");
    } else {
        printk("lm3642. [%02X] = %02X\n", 0x0B, err);
    }

    err = i2c_smbus_write_byte_data(client, FLASH_FEATURE_REGISTER,
        (((int)pdata->timeout << FLASH_TIMEOUT_SHIFT) & FLASH_TIMEOUT_MASK)
        | (tmp & ~FLASH_TIMEOUT_MASK));
    if(err < 0) {
        printk("%s write i2c faile\n", __func__);
    }

    return err;
}


static ssize_t lm3642_flash_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
    struct lm3642_data* data = dev_get_drvdata(dev);
    struct i2c_client* client = data->client;
    int ret = 0, count = 0;;

    ret = i2c_smbus_read_byte_data(client, CURRENT_REGISTER);
    if(ret < 0) {
        printk("read current register fail!\n");
    } else {
        ret = ret & CURRENT_FLASH_MASK;
        count = sprintf(buf, "%d\n", ret);
    }

    return count;
}

static ssize_t lm3642_flash_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
    struct lm3642_data* data = dev_get_drvdata(dev);

    trigger_flash(data);

	return size;
}

static ssize_t lm3642_torch_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
    struct lm3642_data* data = dev_get_drvdata(dev);
    int count = 0;

	lm3642_clear_error_flag(data);

    switch(data->pdata->status) {
    case TORCH_ON:
        count = 3; 
        strncpy(buf, "ON\n", count);
        break;
    case TORCH_OFF:
        count = 4;
        strncpy(buf, "OFF\n", count);
        break;
    }

    return count;
}
static ssize_t lm3642_torch_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
    struct lm3642_data* data = dev_get_drvdata(dev);

    if(0 == strncmp("0\n", buf, 2) ) {
        // input "0", turn off torch
        turn_off_torch(data);
    } else {
        // input non-"0", 
        turn_on_torch(data);
    }

    return size;
}

static ssize_t lm3642_torch_current_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
    char TORCH_CURRENTS[8][10] = {"48.4mA", "93.74mA", "140.63mA", "187.5mA", "234.38mA", "281.25mA", "328.13mA", "375mA"};
    struct lm3642_data* data = dev_get_drvdata(dev);
    struct i2c_client* client = data->client;
    int ret = 0, count = 0;

    ret = i2c_smbus_read_byte_data(client, CURRENT_REGISTER);
    if(ret < 0) {
        printk("read current register fail!\n");
    } else {
        ret = (ret & CURRENT_TORCH_MASK) >> CURRENT_TORCH_SHIFT ;
        count = sprintf(buf, "%s\n", TORCH_CURRENTS[ret]);
    }

    return count;
}

// the input should be 1-8. 0 is NOT used since atoi would return 0 if non-numeric
static ssize_t lm3642_torch_current_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
    struct lm3642_data* data = dev_get_drvdata(dev);
    int ret = 0;

    if(buf == NULL) {
        printk("buf NULL\n");
        return size;
    }

    ret = atoi(buf);
    if(ret > (1 + (int)TORCH_I_MAX) || ret < 1) {
        printk("invalid input: %s\n", buf);
    } else {
        set_torch_current(data, (enum torch_current_level) ret - 1);
    }

    return size;
}

static ssize_t lm3642_flash_current_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
    char FLASH_CURRENTS[16][16] = {"93.75mA", "187.5mA", "281.25mA", "375mA",
        "468.75mA", "562.5mA", "656.25mA", "750mA",
        "843.75mA", "937.5mA", "1031.25mA", "1125mA",
        "1218.75mA", "1312.5mA", "1406.25mA", "1500mA"
        };
    struct lm3642_data* data = dev_get_drvdata(dev);
    struct i2c_client* client = data->client;
    int ret = 0, count = 0;

    ret = i2c_smbus_read_byte_data(client, CURRENT_REGISTER);
    if(ret < 0) {
        printk("read current register fail!\n");
    } else {
        ret = (ret & CURRENT_FLASH_MASK) >> CURRENT_FLASH_SHIFT ;
        count = sprintf(buf, "%s\n", FLASH_CURRENTS[ret]);
    }

    return count;
}
static ssize_t lm3642_flash_current_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
    struct lm3642_data* data = dev_get_drvdata(dev);
    int ret = 0;

    if(buf == NULL) {
        printk("buf NULL. %s\n", __func__);
        return size;
    }

    ret = atoi(buf);
    if(ret > (1 + (int)FLASH_I_MAX) || ret < 1) {
        printk("invalid input: %s\n", buf);
    } else {
        set_flash_current(data, (enum flash_current_level) ret - 1);
    }

    return size;
}

static ssize_t lm3642_flash_timeout_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
    char FLASH_TIMEOUTS[8][8] = {
        "100ms", "200ms", "300ms", "400ms", "500ms", "600ms", "700ms", "800ms"
        };
    struct lm3642_data* data = dev_get_drvdata(dev);
    struct i2c_client* client = data->client;
    int ret = 0, count = 0;

    ret = i2c_smbus_read_byte_data(client, FLASH_FEATURE_REGISTER);
    if(ret < 0) {
        printk("read current register fail!\n");
    } else {
        ret = (ret & FLASH_TIMEOUT_MASK) >> FLASH_TIMEOUT_SHIFT ;
        count = sprintf(buf, "%s\n", FLASH_TIMEOUTS[ret]);
    }

    return count;
}

static ssize_t lm3642_flash_timeout_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
    struct lm3642_data* data = dev_get_drvdata(dev);
    int ret = 0;

    if(buf == NULL) {
        printk("buf NULL. %s\n", __func__);
        return size;
    }

    ret = atoi(buf);
    if(ret > (1 + (int)FLASH_TIMEOUT_MAX) || ret < 1) {
        printk("invalid input: %s\n", buf);
    } else {
        set_flash_timeout(data, (enum flash_timeout) ret - 1);
    }

    return size;
}

static void led_i2c_brightness_set_led_work(struct work_struct *work)
{
    if(lm3642->pdata->brightness > LED_HALF) {
        turn_off_torch(lm3642);
        turn_on_flash(lm3642);
    } else if(lm3642->pdata->brightness > LED_OFF) {
        turn_off_flash(lm3642);
        turn_on_torch(lm3642);
    } else {
        turn_off_flash(lm3642);
        turn_off_torch(lm3642);
    }
}

static void led_i2c_brightness_set(struct led_classdev *led_cdev,
				    enum led_brightness value)
{
	lm3642->pdata->brightness = value;
	schedule_work(&lm3642->pdata->work);
}

static enum led_brightness led_i2c_brightness_get(struct led_classdev *led_cdev)
{
    struct lm3642_platform_data *flash_led =
	    container_of(led_cdev, struct lm3642_platform_data, cdev);
	return flash_led->brightness;
}

static DEVICE_ATTR(flashlight, 0664, lm3642_flash_show, lm3642_flash_store); // for test only
static DEVICE_ATTR(torch, 0664, lm3642_torch_show, lm3642_torch_store);
static DEVICE_ATTR(torch_current, 0664, lm3642_torch_current_show, lm3642_torch_current_store);
static DEVICE_ATTR(flash_current, 0664, lm3642_flash_current_show, lm3642_flash_current_store);
static DEVICE_ATTR(flash_timeout, 0664, lm3642_flash_timeout_show, lm3642_flash_timeout_store);

static int __devexit lm3642_remove(struct i2c_client *client)
{
    struct lm3642_data* data = i2c_get_clientdata(client);

    device_remove_file(&client->dev, &dev_attr_flashlight);
	device_remove_file(&client->dev, &dev_attr_torch);
	device_remove_file(&client->dev, &dev_attr_flash_current);
	device_remove_file(&client->dev, &dev_attr_torch_current);
	device_remove_file(&client->dev, &dev_attr_flash_timeout);

    if (gpio_is_valid(data->pdata->flash))
		gpio_free(data->pdata->flash);

    kfree(data);
    return 0;
}

static int lm3642_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
    int err = 0;
    const char *temp_str;
    struct lm3642_platform_data *pdata;
    struct lm3642_data* data;
    struct device_node *node = client->dev.of_node;

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
    int tmp_data = 0;
#endif
    if (node) {
		pdata = devm_kzalloc(&client->dev,
			sizeof(struct lm3642_platform_data), GFP_KERNEL);
		if (!pdata) {
			dev_err(&client->dev, "Failed to allocate memory\n");
			return -ENOMEM;
		}
	} else {
		pdata = client->dev.platform_data;
    }

	if (!pdata) {
		dev_err(&client->dev, "Invalid pdata\n");
		return -EINVAL;
	}

	INIT_WORK(&pdata->work, led_i2c_brightness_set_led_work);

	pdata->brightness = 0;
    pdata->flash_current = FLASH_I_DEFAULT;
    pdata->torch_current = TORCH_I_DEFAULT;
    pdata->timeout = FLASH_TIMEOUT_DEFAULT;
    pdata->status = TORCH_OFF;
    pdata->cdev.default_trigger = LED_TRIGGER_DEFAULT;

    err = of_property_read_string(node, "linux,default-trigger", &temp_str);
	if (!err) {
		pdata->cdev.default_trigger = temp_str;
    }

    err = of_property_read_string(node, "linux,name", &pdata->cdev.name);
	if (err) {
		dev_err(&client->dev, "%s: Failed to read linux name. rc = %d\n",
			__func__, err);
		goto free_platform_data;
	}

    pdata->cdev.max_brightness = LED_FULL;
	pdata->cdev.brightness_set = led_i2c_brightness_set;
	pdata->cdev.brightness_get = led_i2c_brightness_get;

    err = led_classdev_register(&client->dev, &pdata->cdev);
	if (err) {
		dev_err(&client->dev, "%s: Failed to register led dev. rc = %d\n",
			__func__, err);
		goto free_platform_data;
	}

    // Get gpio number of flash from dts(i)
    pdata->flash = of_get_named_gpio(node, "qcom,gpio-flash", 0);
    if (pdata->flash < 0) {
        printk("of_get_named_gpio failed. Line: %d. %s\n", __LINE__, __func__);
		goto free_platform_data;
	} else {
		err = gpio_request(pdata->flash, "FLASH_NOW");
        printk("flash = %d\n", pdata->flash);
		if (err) {
            printk("gpio_request failed. Line: %d. %s\n", __LINE__, __func__);
			goto free_platform_data;
		}
	}

    // set gpio of flash as output. 
	gpio_tlmm_config(GPIO_CFG(pdata->flash, 0,
				  GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
				  GPIO_CFG_2MA), GPIO_CFG_ENABLE);

    err = gpio_direction_output(pdata->flash, 0);
	if (err) {
		printk("%s: Failed to set gpio %d\n", __func__,
		       pdata->flash);
		goto free_platform_data;
	}

    // Check and setup i2c client
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "I2C not supported\n");
		goto free_platform_data;
	}

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK))
	{
	    dev_err(&client->dev, "%s: I2C error2!\n", __func__);
		goto free_platform_data;
	}

	data = kzalloc(sizeof(struct lm3642_data), GFP_KERNEL);
    if (!data) {
		//dev_err(&client->dev, "Not enough memory\n");
		printk("%s: Not enough memory\n", __func__);
		goto free_platform_data;
	}

	data->client = client;
	data->pdata = pdata;

	i2c_set_clientdata(client, data);
    lm3642 = data;

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
    /* read chip id */
    tmp_data = i2c_smbus_read_byte_data(client, 0x00);
    if(tmp_data < 0) 
    {
        CDBG("%s: FLASHCHIP READ I2C error!\n", __func__);
        err = tmp_data;
        goto free_platform_data;
    }

    if ( FLASH_CHIP_ID == (tmp_data & FLASH_CHIP_ID_MASK) )
    {
    	CDBG("%s : Read chip id ok!Chip ID is %d.\n", __func__, tmp_data);
    	/* detect current device successful, set the flag as present */
    	set_hw_dev_flag(DEV_I2C_FLASH);
    	CDBG("%s : LM3642 probe succeed!\n", __func__);
    } 
    else 
    {
        CDBG("%s : read chip id error!Chip ID is %d.\n", __func__, tmp_data);
        err = -ENODEV;
        goto free_platform_data;
    }
#endif
    // create file sysfs
    err = device_create_file(&client->dev, &dev_attr_flashlight);
    if(err) {
        goto free_data;
    }

    err = device_create_file(&client->dev, &dev_attr_torch);
    if(err) {
        goto free_lm3642_sys_flashlight;
    }

    err = device_create_file(&client->dev, &dev_attr_torch_current);
    if(err) {
        goto free_lm3642_sys_torch;
    }

    err = device_create_file(&client->dev, &dev_attr_flash_current);
    if(err) {
        goto free_lm3642_sys_torch_current;
    }

    err = device_create_file(&client->dev, &dev_attr_flash_timeout);
    if(err) {
        goto free_lm3642_sys_flash_current;
    }

    return 0;
free_lm3642_sys_flash_current:
    device_remove_file(&client->dev, &dev_attr_flash_current);
free_lm3642_sys_torch_current:
    device_remove_file(&client->dev, &dev_attr_torch_current);
free_lm3642_sys_torch:
    device_remove_file(&client->dev, &dev_attr_torch);
free_lm3642_sys_flashlight:
	device_remove_file(&client->dev, &dev_attr_flashlight);
    printk("device_create_file failed\n");
free_data: 
    kfree(data);
free_platform_data:
    if(node) {
        kfree(pdata);
    }
    return err;


}


static const struct i2c_device_id lm3642_id[] = {
	{"lm3642", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, lm3642_id);

#ifdef CONFIG_OF
static struct of_device_id lm3642_match_table[] = {
	{ .compatible = "qcom,led-flash",},
	{ },
};
#else
#define lm3642_match_table NULL
#endif


static struct i2c_driver lm3642_driver = {
	.probe = lm3642_probe,
	.remove = __devexit_p(lm3642_remove),
	.driver = {
		   .name = "lm3642",
		   .owner = THIS_MODULE,
		   .of_match_table = lm3642_match_table,
/* Do NOT implements PM at first. We just verify the funciton.
#ifdef CONFIG_PM
		   .pm = &lm3642_pm_ops,
#endif
*/
	   },
	.id_table = lm3642_id,
};

static int __init lm3642_init(void)
{
	return i2c_add_driver(&lm3642_driver);
}

module_init(lm3642_init);

static void __exit lm3642_exit(void)
{
	i2c_del_driver(&lm3642_driver);
}
module_exit(lm3642_exit);

MODULE_DESCRIPTION("For lm3642 camera flash light driver");
MODULE_LICENSE("GPL v2");

