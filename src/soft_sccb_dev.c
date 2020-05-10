#include <rtthread.h>
#include "soft_sccb_dev.h"
#include "soft_sccb_core.h"

#define DBG_TAG               "SCCB"
#ifdef RT_SCCB_DEBUG
#define DBG_LVL               DBG_LOG
#else
#define DBG_LVL               DBG_INFO
#endif
#include <rtdbg.h>

static rt_size_t sccb_bus_device_read(rt_device_t dev,
                                     rt_off_t    pos,
                                     void       *buffer,
                                     rt_size_t   count)
{
    rt_uint16_t addr;
    rt_uint16_t flags;
    struct rt_sccb_bus_device *bus = (struct rt_sccb_bus_device *)dev->user_data;

    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(buffer != RT_NULL);
    RT_ASSERT(count != 1);

    LOG_D("SCCB bus dev [%s] reading %u bytes.", dev->parent.name, count);

    addr = pos & 0xffff;
    flags = (pos >> 16) & 0xffff;

    return rt_sccb_master_recv(bus, addr, flags, (rt_uint8_t *)buffer);
}

static rt_size_t sccb_bus_device_write(rt_device_t dev,
                                      rt_off_t    pos,
                                      const void *buffer,
                                      rt_size_t   count)
{
    rt_uint16_t addr;
    rt_uint16_t flags;
    struct rt_sccb_bus_device *bus = (struct rt_sccb_bus_device *)dev->user_data;

    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(buffer != RT_NULL);

    LOG_D("SCCB bus dev [%s] writing %u bytes.", dev->parent.name, count);

    addr = pos & 0xffff;
    flags = (pos >> 16) & 0xffff;

    return rt_sccb_master_send(bus, addr, flags, (rt_uint8_t *)buffer);
}

static rt_err_t sccb_bus_device_control(rt_device_t dev,
                                       int         cmd,
                                       void       *args)
{
    rt_err_t ret;
    struct rt_sccb_priv_data *priv_data;
    struct rt_sccb_bus_device *bus = (struct rt_sccb_bus_device *)dev->user_data;

    RT_ASSERT(bus != RT_NULL);

    switch (cmd)
    {
    case RT_SCCB_DEV_CTRL_ADDR:
        bus->addr = *(rt_uint16_t *)args;
        break;
    case RT_SCCB_DEV_CTRL_TIMEOUT:
        bus->timeout = *(rt_uint32_t *)args;
        break;
    case RT_SCCB_DEV_CTRL_RW:
        priv_data = (struct rt_sccb_priv_data *)args;
        ret = rt_sccb_transfer(bus, priv_data->msg);
        if (ret < 0)
        {
            return -RT_EIO;
        }
        break;
    default:
        break;
    }

    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops sccb_ops =
{
    RT_NULL,
    RT_NULL,
    RT_NULL,
    sccb_bus_device_read,
    sccb_bus_device_write,
    sccb_bus_device_control
};
#endif

rt_err_t rt_sccb_bus_device_device_init(struct rt_sccb_bus_device *bus, const char *name)
{
    struct rt_device *device;
    RT_ASSERT(bus != RT_NULL);

    device = &bus->parent;

    device->user_data = bus;

    /* set device type */
    device->type    = RT_Device_Class_SCCB;
    /* initialize device interface */
#ifdef RT_USING_DEVICE_OPS
    device->ops     = &sccb_ops;
#else
    device->init    = RT_NULL;
    device->open    = RT_NULL;
    device->close   = RT_NULL;
    device->read    = sccb_bus_device_read;
    device->write   = sccb_bus_device_write;
    device->control = sccb_bus_device_control;
#endif

    /* register to device manager */
    rt_device_register(device, name, RT_DEVICE_FLAG_RDWR);

    return RT_EOK;
}
