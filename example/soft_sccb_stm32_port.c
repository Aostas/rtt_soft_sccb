/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-08     balanceTWK   first version
 */

#include <board.h>
#include "soft_sccb_stm32_port.h"

#ifdef PKG_USING_SOFT_SCCB

//#define DRV_DEBUG
#define LOG_TAG              "drv.sccb"
#include <drv_log.h>

static const struct stm32_soft_sccb_config soft_sccb_config =SCCB_BUS_CONFIG;

static struct stm32_sccb sccb_obj;

/**
 * This function initializes the sccb pin.
 *
 * @param Stm32 sccb dirver class.
 */
static void stm32_sccb_gpio_init(struct stm32_sccb *sccb)
{
    struct stm32_soft_sccb_config* cfg = (struct stm32_soft_sccb_config*)sccb->ops.data;

    rt_pin_mode(cfg->scl, PIN_MODE_OUTPUT_OD);
    rt_pin_mode(cfg->sda, PIN_MODE_OUTPUT_OD);

    rt_pin_write(cfg->scl, PIN_HIGH);
    rt_pin_write(cfg->sda, PIN_HIGH);
}

/**
 * This function sets the sda pin.
 *
 * @param Stm32 config class.
 * @param The sda pin state.
 */
static void stm32_set_sda(void *data, rt_int32_t state)
{
    struct stm32_soft_sccb_config* cfg = (struct stm32_soft_sccb_config*)data;
    if (state)
    {
        rt_pin_write(cfg->sda, PIN_HIGH);
    }
    else
    {
        rt_pin_write(cfg->sda, PIN_LOW);
    }
}

/**
 * This function sets the scl pin.
 *
 * @param Stm32 config class.
 * @param The scl pin state.
 */
static void stm32_set_scl(void *data, rt_int32_t state)
{
    struct stm32_soft_sccb_config* cfg = (struct stm32_soft_sccb_config*)data;
    if (state)
    {
        rt_pin_write(cfg->scl, PIN_HIGH);
    }
    else
    {
        rt_pin_write(cfg->scl, PIN_LOW);
    }
}

/**
 * This function gets the sda pin state.
 *
 * @param The sda pin state.
 */
static rt_int32_t stm32_get_sda(void *data)
{
    struct stm32_soft_sccb_config* cfg = (struct stm32_soft_sccb_config*)data;
    return rt_pin_read(cfg->sda);
}

/**
 * This function gets the scl pin state.
 *
 * @param The scl pin state.
 */
static rt_int32_t stm32_get_scl(void *data)
{
    struct stm32_soft_sccb_config* cfg = (struct stm32_soft_sccb_config*)data;
    return rt_pin_read(cfg->scl);
}
/**
 * The time delay function.
 *
 * @param microseconds.
 */
static void stm32_udelay(rt_uint32_t us)
{
    rt_uint32_t ticks;
    rt_uint32_t told, tnow, tcnt = 0;
    rt_uint32_t reload = SysTick->LOAD;

    ticks = us * reload / (1000000 / RT_TICK_PER_SECOND);
    told = SysTick->VAL;
    while (1)
    {
        tnow = SysTick->VAL;
        if (tnow != told)
        {
            if (tnow < told)
            {
                tcnt += told - tnow;
            }
            else
            {
                tcnt += reload - tnow + told;
            }
            told = tnow;
            if (tcnt >= ticks)
            {
                break;
            }
        }
    }
}

static const struct rt_sccb_ops stm32_bit_ops_default =
{
    .data     = RT_NULL,
    .set_sda  = stm32_set_sda,
    .set_scl  = stm32_set_scl,
    .get_sda  = stm32_get_sda,
    .get_scl  = stm32_get_scl,
    .udelay   = stm32_udelay,
    .delay_us = 1,
    .timeout  = 100
};

/**
 * if sccb is locked, this function will unlock it
 *
 * @param stm32 config class
 *
 * @return RT_EOK indicates successful unlock.
 */
static rt_err_t stm32_sccb_bus_unlock(const struct stm32_soft_sccb_config *cfg)
{
    rt_int32_t i = 0;

    if (PIN_LOW == rt_pin_read(cfg->sda))
    {
        while (i++ < 9)
        {
            rt_pin_write(cfg->scl, PIN_HIGH);
            stm32_udelay(100);
            rt_pin_write(cfg->scl, PIN_LOW);
            stm32_udelay(100);
        }
    }
    if (PIN_LOW == rt_pin_read(cfg->sda))
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

/* SCCB initialization function */
int rt_hw_sccb_init(void)
{
    rt_err_t result;

    sccb_obj.ops = stm32_bit_ops_default;
    sccb_obj.ops.data = (void*)&soft_sccb_config;
    sccb_obj.sccb_bus.priv = &sccb_obj.ops;
    stm32_sccb_gpio_init(&sccb_obj);
    result = rt_sccb_add_bus(&sccb_obj.sccb_bus, soft_sccb_config.bus_name);
    RT_ASSERT(result == RT_EOK);
    stm32_sccb_bus_unlock(&soft_sccb_config);

    LOG_D("software simulation %s init done, pin scl: %d, pin sda %d",
    soft_sccb_config.bus_name,
    soft_sccb_config.scl,
    soft_sccb_config.sda);

    return RT_EOK;
}
INIT_BOARD_EXPORT(rt_hw_sccb_init);

#endif /* PKG_USING_SOFT_SCCB */
