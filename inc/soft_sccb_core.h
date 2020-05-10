#ifndef __SOFT_SCCB_CORE_H__
#define __SOFT_SCCB_CORE_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif


#define RT_SCCB_WR               (0)
#define RT_SCCB_RD               (1)

struct rt_sccb_msg
{
    rt_uint16_t addr;
    rt_uint16_t flags;
    rt_uint8_t  *data;
};

/*for sccb bus driver*/
struct rt_sccb_bus_device
{
    struct rt_device parent;
    const struct rt_sccb_bus_device_ops *ops;
    rt_uint16_t  flags;
    rt_uint16_t  addr;
    struct rt_mutex lock;
    rt_uint32_t  timeout;
    rt_uint32_t  retries;
    void *priv;
};

struct rt_sccb_bus_device_ops
{
    rt_size_t (*master_xfer)(struct rt_sccb_bus_device *bus,
                             struct rt_sccb_msg *msg);
    rt_err_t (*sccb_bus_control)(struct rt_sccb_bus_device *bus,
                                rt_uint32_t,
                                rt_uint32_t);
};


struct rt_sccb_client
{
    struct rt_device               parent;
    struct rt_sccb_bus_device       *bus;
    rt_uint16_t                    client_addr;
};

rt_err_t rt_sccb_bus_device_register(struct rt_sccb_bus_device *bus,
                                    const char               *bus_name);
struct rt_sccb_bus_device *rt_sccb_bus_device_find(const char *bus_name);
rt_size_t rt_sccb_transfer(struct rt_sccb_bus_device *bus,
                          struct rt_sccb_msg         *msg);
rt_size_t rt_sccb_master_send(struct rt_sccb_bus_device *bus,
                             rt_uint16_t               addr,
                             rt_uint16_t               flags,
                             rt_uint8_t                *data);
rt_size_t rt_sccb_master_recv(struct rt_sccb_bus_device *bus,
                             rt_uint16_t               addr,
                             rt_uint16_t               flags,
                             rt_uint8_t                *data);
int rt_sccb_core_init(void);

#ifdef __cplusplus
}
#endif

#endif
