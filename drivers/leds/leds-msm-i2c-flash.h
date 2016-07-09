#ifndef __LEDS_MSM_I2C_FLASH__
#define __LEDS_MSM_I2C_FLASH__

enum torch_current_level {
    // in the unit of mA
    TORCH_I_48P4, //0
    TORCH_I_93P74, // 1
    TORCH_I_140P63, // 2
    TORCH_I_187P5, // 3
    TORCH_I_234P38, // 4
    TORCH_I_281P25, // 5
    TORCH_I_328P13, // 6
    TORCH_I_375, //7
    TORCH_I_MAX = TORCH_I_375,
    TORCH_I_DEFAULT = TORCH_I_48P4,
};

enum flash_current_level {
    // in the unit of mA
    FLASH_I_93P75, // 0
    FLASH_I_187P5, // 1
    FLASH_I_281P25, // 2
    FLASH_I_375, // 3
    FLASH_I_468P75, // 4
    FLASH_I_562P5, // 5
    FLASH_I_656P25, // 6
    FLASH_I_750, // 7
    FLASH_I_843P75, // 8
    FLASH_I_937P5, // 9
    FLASH_I_1031P25, // 10
    FLASH_I_1125, // 11
    FLASH_I_1218P75, // 12
    FLASH_I_1312P5, // 13
    FLASH_I_1406P25, // 14
    FLASH_I_1500, // 
    FLASH_I_MAX = FLASH_I_1031P25,//max value
    FLASH_I_DEFAULT = FLASH_I_1031P25,//default value
};

enum flash_timeout {
    // in the unit of milli-second
    FLASH_TIMEOUT_100,
    FLASH_TIMEOUT_200,
    FLASH_TIMEOUT_300,
    FLASH_TIMEOUT_400,
    FLASH_TIMEOUT_500,
    FLASH_TIMEOUT_600,
    FLASH_TIMEOUT_700,
    FLASH_TIMEOUT_800,
    FLASH_TIMEOUT_MAX = FLASH_TIMEOUT_800,
    FLASH_TIMEOUT_DEFAULT = FLASH_TIMEOUT_300,
};

enum torch_status {
    TORCH_ON,
    TORCH_OFF,
};

struct lm3642_platform_data {
    enum torch_current_level torch_current;
    enum torch_status status;
    int flash; // pin number
    enum flash_current_level flash_current;
    enum flash_timeout timeout;    
    int brightness;
	struct work_struct work;
    struct led_classdev cdev;
};

struct lm3642_data {
    struct i2c_client *client;
    struct lm3642_platform_data *pdata;
};

#endif // end of __LEDS_MSM_I2C_FLASH__
