#ifndef __SOFT_SCCB_DEV_H__
#define __SOFT_SCCB_DEV_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "soft_sccb_core.h"

#define RT_SCCB_DEV_CTRL_ADDR         0x20
#define RT_SCCB_DEV_CTRL_TIMEOUT      0x21
#define RT_SCCB_DEV_CTRL_RW           0x22

struct rt_sccb_priv_data
{
    struct rt_sccb_msg  *msg;
};

rt_err_t rt_sccb_bus_device_device_init(struct rt_sccb_bus_device *bus, const char *name);

#ifdef __cplusplus
}
#endif

#endif
