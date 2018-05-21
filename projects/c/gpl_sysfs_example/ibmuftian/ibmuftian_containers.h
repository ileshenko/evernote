#ifndef _IBMUFTIAN_CONTAINERS_H_
#define _IBMUFTIAN_CONTAINERS_H_
#include <linux/list.h>

#include "ibmuftian_utils.h"
#include "ibmuftian_board.h"


typedef struct ibmuftian_board_container
{
    atomic_t count;
    struct kset *kobjset;
    struct list_head board_l;    
} ibmuftian_board_container;

typedef struct ibmuftian_fpga_container
{
    atomic_t count;
    struct kset *kobjset;
    struct list_head fpga_l;    
} ibmuftian_fpga_container;

typedef struct ibmuftian_engn_container
{
    atomic_t count;
    struct kset *kobjset;
    struct list_head engn_l;    
} ibmuftian_engn_container;

#endif
