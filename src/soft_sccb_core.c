/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
 * 2012-04-25     weety         first version
 */

#include <rtthread.h>
#include "soft_sccb_core.h"
#include "soft_sccb_dev.h"

#define DBG_TAG               "SCCB"
#ifdef RT_SCCB_DEBUG
#define DBG_LVL               DBG_LOG
#else
#define DBG_LVL               DBG_INFO
#endif
#include <rtdbg.h>

rt_err_t rt_sccb_bus_device_register(struct rt_sccb_bus_device *bus,
                                    const char               *bus_name)
{
    rt_err_t res = RT_EOK;

    rt_mutex_init(&bus->lock, "sccb_bus_lock", RT_IPC_FLAG_FIFO);

    if (bus->timeout == 0) bus->timeout = RT_TICK_PER_SECOND;

    res = rt_sccb_bus_device_device_init(bus, bus_name);

    LOG_I("SCCB bus [%s] registered", bus_name);

    return res;
}

struct rt_sccb_bus_device *rt_sccb_bus_device_find(const char *bus_name)
{
    struct rt_sccb_bus_device *bus;
    rt_device_t dev = rt_device_find(bus_name);
    if (dev == RT_NULL || dev->type != RT_Device_Class_I2CBUS)
    {
        LOG_E("SCCB bus %s not exist", bus_name);

        return RT_NULL;
    }

    bus = (struct rt_sccb_bus_device *)dev->user_data;

    return bus;
}

rt_size_t rt_sccb_transfer(struct rt_sccb_bus_device *bus,
                          struct rt_sccb_msg         *msg)
{
    rt_size_t ret;

    if (bus->ops->master_xfer)
    {
#ifdef RT_SCCB_DEBUG
        LOG_D("msg %c, addr=0x%02x, data=%d",
              (msg.flags & RT_I2C_RD) ? 'R' : 'W',
              msg.addr, msg.data);
#endif

        rt_mutex_take(&bus->lock, RT_WAITING_FOREVER);
        ret = bus->ops->master_xfer(bus, msg);
        rt_mutex_release(&bus->lock);

        return ret;
    }
    else
    {
        LOG_E("SCCB bus operation not supported");

        return 0;
    }
}

rt_size_t rt_sccb_master_send(struct rt_sccb_bus_device *bus,
                             rt_uint16_t               addr,
                             rt_uint16_t               flags,
                             rt_uint8_t                *data)
{
    rt_err_t ret;
    struct rt_sccb_msg msg;

    msg.addr  = addr;
    msg.flags = flags;
    msg.data   = data;

    ret = rt_sccb_transfer(bus, &msg);

    return ret;
}

rt_size_t rt_sccb_master_recv(struct rt_sccb_bus_device *bus,
                             rt_uint16_t               addr,
                             rt_uint16_t               flags,
                             rt_uint8_t                *data)
{
    rt_err_t ret;
    struct rt_sccb_msg msg;
    RT_ASSERT(bus != RT_NULL);

    msg.addr   = addr;
    msg.flags  = flags | RT_SCCB_RD;
    msg.data    = data;

    ret = rt_sccb_transfer(bus, &msg);

    return ret;
}

int rt_sccb_core_init(void)
{
    return 0;
}
INIT_COMPONENT_EXPORT(rt_sccb_core_init);
