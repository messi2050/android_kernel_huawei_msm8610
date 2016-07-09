/* Copyright (c) 2009, Code HUAWEI. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
/* recreat lcd dts in fc1 baseline */
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>

	 
#include<linux/init.h>
#include<linux/module.h>
	 
#include <linux/hw_lcd_common.h>
#include <mdss_dsi.h>
#include <linux/of.h>


//two gpios use to identify lcd module for 8x12 platform
#define LCD_ID_0_GPIO    76
#define LCD_ID_1_GPIO    93


#define GET_GPIO_FAIL  -1
#define GET_LCD_ID_FAIL  -1 
#define LCD_ID_PULL_UP  1
#define LCD_ID_PULL_DOWN  0
//esd read error times
#define REPEAT_COUNT_ESD 3
//ESD read , return byte count ,max = 4
#define MAX_RETURN_COUNT_ESD 4
/* delete codes which not use */

struct of_device_id huawei_mdss_dsi_panel_match[] = {
	{.compatible = "huawei,lcd_panel_id"},
	{}
};
/****************************************************************
function: get lcd id by gpio

*data structure*
*	   ID1	  ID0   * 
 *	----------------- * 
 *	|   |   |   |   | * 
 *	|   |   |   |   | * 
 *	----------------- *
 For each Gpio :
		00 means low  ,
		01 means high ,
		10 means float,
		11 is not defined,

 lcd id(hex):
 0	:ID0 low,	ID1 low
 1	:ID0 high,	ID1 low
 2	:ID0 float,	ID1 low

 4	:ID0 low,	ID1 high
 5	:ID0 high,	ID1 high
 6	:ID0 float,	ID1 high

 8	:ID0 low,	ID1 float
 9	:ID0 high,	ID1 float
 A	:ID0 float,	ID1 float, used for emulator
 ***************************************************************/
/*
G730:
BOE:     ID[1:0]   {1:1} --->is is 5
tianma:  ID[1:0]   {0:0} --->id is 0
youda:   ID[1:0]   {1:0} --->id is 4
BYD:     ID[1:0]   {0:1} --->id is 1


BOE    IC     NT35517 
tianma IC     HX8389-B 
youda  IC     RM68190 
BYD    IC     NT35517 
*/
int hw_get_lcd_id(void)
{
	int ret = 0;
	int id0,id1;
	int gpio_id0,gpio_id1;
	int pullup_read,pulldown_read;
	static int lcd_id = GET_LCD_ID_FAIL;
	
	id0=0;
	id1=0;
	pullup_read = 0;
	pulldown_read = 0;
	gpio_id0 = LCD_ID_0_GPIO;
	gpio_id1 = LCD_ID_1_GPIO;

	
	if( (lcd_id >= 0x0) && (lcd_id <= 0xA) )//if lcd_id had read successfully,just return lcd_id.
		return lcd_id;

	if(gpio_id0 <= GET_GPIO_FAIL ||gpio_id1 <= GET_GPIO_FAIL)
		return GET_LCD_ID_FAIL;

    LCD_LOG_INFO("gpio_lcd_id0:%d gpio_lcd_id1:%d\n",gpio_id0,gpio_id1);

    ret = gpio_request(gpio_id0, "lcd_id0");
      if (ret) {
         LCD_LOG_ERR("lcd_id0 gpio[%d] request failed\n", gpio_id0);
         goto lcd_id0_req_fail;
          }

    ret = gpio_request(gpio_id1, "lcd_id1");
	if (ret) {
	     LCD_LOG_ERR("lcd_id1 gpio[%d] request failed\n", gpio_id1);
         goto lcd_id1_req_fail;
	}

	/*config id0 to pull down and read*/
	ret = gpio_tlmm_config(GPIO_CFG(gpio_id0,0,GPIO_CFG_INPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	  if (ret) {
	     LCD_LOG_ERR("config id0 to pull down failed\n");
	     goto get_lcd_id_fail;
	    }
	udelay(10);
	pulldown_read = gpio_get_value(gpio_id0);

	/*config id0 to pull up and read*/
	ret = gpio_tlmm_config(GPIO_CFG(gpio_id0,0,GPIO_CFG_INPUT,GPIO_CFG_PULL_UP,GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	if (ret) {
		LCD_LOG_ERR("config id0 to pull up failed\n");
		goto get_lcd_id_fail;
		}
	udelay(10);
	pullup_read = gpio_get_value(gpio_id0);
	if(pulldown_read != pullup_read)//float
	{
		id0 = BIT(1);
		gpio_tlmm_config(GPIO_CFG(gpio_id0,0,GPIO_CFG_INPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	}
	else//connect 
	{
		id0 = pullup_read;//pullup_read==pulldown_read
		switch(id0)
		{
			case LCD_ID_PULL_DOWN:
			case LCD_ID_PULL_UP:
			default:
				gpio_tlmm_config(GPIO_CFG(gpio_id0,0,GPIO_CFG_INPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),GPIO_CFG_ENABLE);
				break;
		}

	}

	/*config id1 to pull down and read*/
	ret = gpio_tlmm_config(GPIO_CFG(gpio_id1,0,GPIO_CFG_INPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	if (ret) {
		LCD_LOG_ERR("config id1 to pull down failed\n");
		goto get_lcd_id_fail;
		}
	udelay(10);
	pulldown_read = gpio_get_value(gpio_id1);

	/*config id1 to pull up and read*/
	ret = gpio_tlmm_config(GPIO_CFG(gpio_id1,0,GPIO_CFG_INPUT,GPIO_CFG_PULL_UP,GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	if (ret) {
		LCD_LOG_ERR("config id1 to pull up failed\n");
		goto get_lcd_id_fail;
		}
	udelay(10);
	pullup_read = gpio_get_value(gpio_id1);
	if(pulldown_read != pullup_read)//float
	{
		id1 = BIT(1);
		gpio_tlmm_config(GPIO_CFG(gpio_id1,0,GPIO_CFG_INPUT,GPIO_CFG_PULL_DOWN,GPIO_CFG_2MA),GPIO_CFG_ENABLE);
	}
	else//connect
	{
		id1 = pullup_read;//pullup_read==pulldown_read
		switch(id1)
		{
			case LCD_ID_PULL_DOWN:
			case LCD_ID_PULL_UP:
			default:
				gpio_tlmm_config(GPIO_CFG(gpio_id1,0,GPIO_CFG_INPUT,GPIO_CFG_NO_PULL,GPIO_CFG_2MA),GPIO_CFG_ENABLE);
				break;
		}
	}

	gpio_free(gpio_id0);
	gpio_free(gpio_id1);

	lcd_id = (id1<<2) | id0;
	return lcd_id;
get_lcd_id_fail:
	gpio_free(gpio_id1);
lcd_id1_req_fail:
	gpio_free(gpio_id0);
lcd_id0_req_fail:
	return GET_LCD_ID_FAIL;
}

void hw_get_lcd_panel(void)
{
	char *psKey = NULL;
	int id = 0;
	int ret = 0;
	psKey = kmalloc(30, GFP_KERNEL);
    if (NULL == psKey)  
    {
		ret = false;
		return ;
    }
    memset(psKey, 0, 30);

	id = hw_get_lcd_id();
	sprintf(psKey, "huawei,lcd_panel_id%X", id);
	strcpy(huawei_mdss_dsi_panel_match->compatible,psKey);

   kfree(psKey);
	return;
	
};
/***************************************************************
Function: process_mipi_read_table
Description: read the required register and return register value
Parameters:
    struct mdss_dsi_ctrl_pdata *ctrl: dsi control parameter struct
    struct dsi_cmd_desc* dcs_read_cmd: register required to be read
    char *rx_buf: the returned register value will be saved in this buffer
    u32 rx_len: the expect number of returned register value
Return:0:success
***************************************************************/
int process_mipi_read_table(struct mdss_dsi_ctrl_pdata *ctrl,struct dsi_cmd_desc* dcs_read_cmd,char *rx_buf, u32 rx_len)
{
	struct dcs_cmd_req cmdreq;

	memset(&cmdreq, 0, sizeof(cmdreq));
	cmdreq.cmds = dcs_read_cmd;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_RX | CMD_REQ_COMMIT;
	cmdreq.rlen = rx_len;
	cmdreq.rbuf = rx_buf;
	cmdreq.cb = NULL; //call back

	mdss_dsi_cmdlist_put(ctrl, &cmdreq);

	return 0;
}
/***************************************************************
Function: lcd_monitor_reg_status
Description: monitor the panel status by read and compare the value of IC registers
Parameters:
    struct mdss_dsi_ctrl_pdata *ctrl: dsi control parameter struct
Return:0:fail
Return:1:success
***************************************************************/
int lcd_monitor_reg_status(struct mdss_dsi_ctrl_pdata * ctrl)
{

	char esd_buf[MAX_RETURN_COUNT_ESD]={0};
	u8 ret = 1;//0 fail 1 success

	u32 i = 0,j = 0;
	//pr_err("%s: begin to trace this function\n",__func__);
	//dump_stack();
	//pr_err("%s: end to trace this function\n",__func__);
	if(NULL==ctrl->esd_cmds.cmds)
	{
		//enable the BTA check
		LCD_LOG_DBG("%s: esd_cmds is NULL, just do BTA check\n", __func__);
		return 1;
	}
	for(i=0;i<ctrl->esd_cmds.cmd_cnt;i++)
	{
		process_mipi_read_table(ctrl,&ctrl->esd_cmds.cmds[i],esd_buf,ctrl->esd_cmds.rx_cmds[i].elen);

		for(j=0;j<ctrl->esd_cmds.rx_cmds[i].elen;j++)
		{
                     //delete unwanted log
			if(esd_buf[j] != ctrl->esd_cmds.rx_cmds[i].excpet_result[j])
			{
				pr_err("%s: lcd register esd error , esd_buf[%d]=%x , excpet_result[%d]=%x\n",__func__,
						j,esd_buf[j],j,ctrl->esd_cmds.rx_cmds[i].excpet_result[j]);

				ret = 0;
				break;
			}

		}
	}
	return ret;
}
/***************************************************************
Function: lcd_esd_check
Description: determine the panel final status according the panel error times
Parameters:
    struct mdss_dsi_ctrl_pdata *ctrl: dsi control parameter struct
Return:0:fail
Return:1:success
***************************************************************/
int lcd_esd_check(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int ret = 0;//0 fail 1 success
	int error_time = 0;
       //delete unwanted log
	if (ctrl_pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return 0;
	}

	while((error_time < REPEAT_COUNT_ESD) && ( 1 !=ret ))
	{
		ret = lcd_monitor_reg_status(ctrl_pdata);
		error_time++;
	}
	return ret;
}
