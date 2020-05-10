/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-08     balanceTWK   first version
 */

#ifndef __DRV_SCCB__
#define __DRV_SCCB__

#include <rtthread.h>
#include <rthw.h>
#include "soft_sccb.h"
#include "soft_sccb_core.h"

/* stm32 config class */
struct stm32_soft_sccb_config
{
    rt_uint8_t scl;
    rt_uint8_t sda;
    const char *bus_name;
};
/* stm32 i2c dirver class */
struct stm32_sccb
{
    struct rt_sccb_ops ops;
    struct rt_sccb_bus_device sccb_bus;
};

#define SCCB_BUS_CONFIG                                  \
    {                                                    \
        .scl = BSP_SCCB_SCL_PIN,                         \
        .sda = BSP_SCCB_SDA_PIN,                         \
        .bus_name = "sccb",                              \
    }

int rt_hw_sccb_init(void);

#endif
