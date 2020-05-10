#include <rtthread.h>
#include "soft_sccb.h"

#define DBG_TAG               "SCCB"
#ifdef RT_SCCB_DEBUG
#define DBG_LVL               DBG_LOG
#else
#define DBG_LVL               DBG_INFO
#endif
#include <rtdbg.h>

#define SET_SDA(ops, val)   ops->set_sda(ops->data, val)
#define SET_SCL(ops, val)   ops->set_scl(ops->data, val)
#define GET_SDA(ops)        ops->get_sda(ops->data)
#define GET_SCL(ops)        ops->get_scl(ops->data)

rt_inline void sccb_delay(struct rt_sccb_ops *ops)
{
    ops->udelay((ops->delay_us + 1) >> 1);
}

rt_inline void sccb_delay2(struct rt_sccb_ops *ops)
{
    ops->udelay(ops->delay_us);
}

#define SDA_L(ops)          SET_SDA(ops, 0)
#define SDA_H(ops)          SET_SDA(ops, 1)
#define SCL_L(ops)          SET_SCL(ops, 0)

/**
 * release scl line, and wait scl line to high.
 */
static rt_err_t SCL_H(struct rt_sccb_ops *ops)
{
    rt_tick_t start;

    SET_SCL(ops, 1);

    if (!ops->get_scl)
        goto done;

    start = rt_tick_get();
    while (!GET_SCL(ops))
    {
        if ((rt_tick_get() - start) > ops->timeout)
            return -RT_ETIMEOUT;
        rt_thread_delay((ops->timeout + 1) >> 1);
    }
#ifdef RT_SCCB_DEBUG
    if (rt_tick_get() != start)
    {
        LOG_D("wait %ld tick for SCL line to go high",
              rt_tick_get() - start);
    }
#endif

done:
    sccb_delay(ops);

    return RT_EOK;
}

static void sccb_start(struct rt_sccb_ops *ops)
{
    SDA_H(ops);
    SCL_H(ops);
    sccb_delay(ops);
    SDA_L(ops);
    sccb_delay(ops);
    SCL_L(ops);
}

static void sccb_stop(struct rt_sccb_ops *ops)
{
    SDA_L(ops);
    sccb_delay(ops);
    SCL_H(ops);
    sccb_delay(ops);
    SDA_H(ops);
    sccb_delay(ops);
}

static void sccb_no_ack(struct rt_sccb_ops *ops)
{
    sccb_delay(ops);
    SDA_H(ops);
    SCL_H(ops);
    sccb_delay(ops);
    SCL_L(ops);
    sccb_delay(ops);
    SDA_L(ops);
    sccb_delay(ops);
}

static rt_int32_t sccb_writeb(struct rt_sccb_bus_device *bus, rt_uint8_t data)
{
    rt_int32_t i;
    rt_uint8_t bit;
    rt_uint8_t res;

    struct rt_sccb_ops *ops = (struct rt_sccb_ops *)bus->priv;

    for (i = 7; i >= 0; i--)
    {
        SCL_L(ops);
        bit = (data >> i) & 1;
        SET_SDA(ops, bit);
        sccb_delay(ops);
        if (SCL_H(ops) < 0)
        {
            LOG_D("sccb_writeb: 0x%02x, "
                    "wait scl pin high timeout at bit %d",
                    data, i);

            return -RT_ETIMEOUT;
        }
    }
    SDA_H(ops);
    sccb_delay(ops);
    SCL_H(ops);
    sccb_delay(ops);
    res=!GET_SDA(ops);
    SCL_L(ops);

    return res;
}

static rt_int32_t sccb_readb(struct rt_sccb_bus_device *bus)
{
    rt_uint8_t i;
    rt_uint8_t data = 0;
    struct rt_sccb_ops *ops = (struct rt_sccb_ops *)bus->priv;

    SDA_H(ops);
    sccb_delay(ops);
    for (i = 0; i < 8; i++)
    {
        data <<= 1;

        if (SCL_H(ops) < 0)
        {
            LOG_D("sccb_readb: wait scl pin high "
                    "timeout at bit %d", 7 - i);

            return -RT_ETIMEOUT;
        }

        if (GET_SDA(ops))
            data |= 1;
        SCL_L(ops);
        sccb_delay(ops);
    }

    return data;
}

static rt_size_t sccb_write_reg(struct rt_sccb_bus_device *bus,
                                struct rt_sccb_msg        *msg)
{
    rt_int32_t ret;

    ret = sccb_writeb(bus, *(msg->data));

    if (ret == 0)
    {
        LOG_E("send bytes: error %d", ret);

        return ret;
    }

    return 1;
}

static rt_size_t sccb_read_reg(struct rt_sccb_bus_device *bus,
                                struct rt_sccb_msg        *msg)
{
    rt_int32_t val;
    struct rt_sccb_ops *ops = (struct rt_sccb_ops *)bus->priv;

    val = sccb_readb(bus);
    if (val >= 0)
    {
        *(msg->data) = val;
    }

    LOG_D("recieve byte: 0x%02x", val);

    sccb_no_ack(ops);

    return 1;
}

static rt_int32_t sccb_send_dev_address(struct rt_sccb_bus_device *bus,
                                   rt_uint8_t                addr,
                                   rt_int32_t                retries)
{
    struct rt_sccb_ops *ops = (struct rt_sccb_ops *)bus->priv;
    rt_int32_t i;
    rt_err_t ret = 0;

    for (i = 0; i <= retries; i++)
    {
        ret = sccb_writeb(bus, addr);
        if (ret == 1 || i == retries)
            break;
        LOG_D("send stop condition");
        sccb_stop(ops);
        sccb_delay(ops);
        LOG_D("send start condition");
        sccb_start(ops);
    }

    return ret;
}

static rt_err_t sccb_send_address(struct rt_sccb_bus_device *bus,
                                     struct rt_sccb_msg        *msg)
{
    rt_uint16_t flags = msg->flags;

    rt_uint8_t addr;
    rt_int32_t retries;
    rt_err_t ret;

    retries = bus->retries;

    /* 7-bit addr */
    addr = msg->addr << 1;
    if (flags & RT_SCCB_RD)
        addr |= 1;
    ret = sccb_send_dev_address(bus, addr, retries);
    if (ret != 1)
        return -RT_EIO;

    return RT_EOK;
}

static rt_size_t sccb_xfer(struct rt_sccb_bus_device *bus, struct rt_sccb_msg *msg)
{
    struct rt_sccb_ops *ops = (struct rt_sccb_ops *)bus->priv;
    rt_int32_t ret;
    LOG_D("send start condition");
    sccb_start(ops);
    if (!(msg->flags))
    {
        ret = sccb_send_address(bus, msg);
        if (ret != RT_EOK)
        {
            LOG_D("receive NACK from device addr 0x%02x msg %d",
                    msgs[i].addr, i);
            goto out;
        }
    }
    sccb_delay2(ops);
    if (msg->flags & RT_SCCB_RD)
    {
        ret = sccb_read_reg(bus, msg);
        if (ret == 0)
        {
            ret = -RT_EIO;
            goto out;
        }
    }
    else
    {
        ret = sccb_write_reg(bus, msg);
        if (ret == 0)
        {
            ret = -RT_ERROR;
            goto out;
        }
    }

out:
    LOG_D("send stop condition");
    sccb_stop(ops);

    return 1;
}

static const struct rt_sccb_bus_device_ops sccb_bus_ops =
{
    sccb_xfer,
    RT_NULL
};

rt_err_t rt_sccb_add_bus(struct rt_sccb_bus_device *bus,
                            const char               *bus_name)
{
    bus->ops = &sccb_bus_ops;

    return rt_sccb_bus_device_register(bus, bus_name);
}
