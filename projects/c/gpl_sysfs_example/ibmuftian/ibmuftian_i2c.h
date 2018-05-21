#ifndef _IBMUFTIAN_I2C_H_
#define _IBMUFTIAN_I2C_H_

#include <linux/device.h>
#include <linux/semaphore.h>
#include <linux/kernel.h>

#include "ibmuftian_utils.h"
#include "ibmuftian_pci_bar.h"

#define to_i2c(x)                          container_of((x), ibmuftian_i2c, kobj)
#define to_i2c_attr(x)                     container_of((x), i2c_attr, attr)
#define wait_for_ack(self) wait_for_acknak((self), 1)
#define wait_for_nak(self) wait_for_acknak((self), 0)


struct ibmuftian_board;

typedef union 
{
    u32 all;
    struct {
        u32 iflag : BITS( 0, 0); // The Interrupt Flag is set when:
                                 // - one byte transfer has been completed
                                 // - arbitration is lost
        u32 tip   : BITS( 1, 1); // Transfer in progress.
                                 // - '1' when transferring data
                                 // - '0' when transfer complete
        u32       : BITS( 4, 2);
        u32 al    : BITS( 5, 5); // Arbitration lost. This bit is set when the
                                 // core lost arbitration. Arbitration is lost when:
                                 // - a STOP signal is detected, but non requested
                                 // - the master drives SDA high, but SDA is low.
        u32 busy  : BITS( 6, 6); // I2C bus busy
                                 // - '1' after START signal detected
                                 // - '0' after STOP signal detected
        u32 rxack : BITS( 7, 7); // Received acknowledge from slave.
                                 // This flag represents acknowledge from the addressed slave.
                                 // - '1'  = No acknowledge received
                                 // - '0'  = Acknowledge received
        u32       : BITS(31, 8);
    } bit;
} __attribute__((packed)) i2c_stats;

typedef union
{
    u32 i2c_ctrl;
	struct
	{
	    u32 : RANGE(5, 0);
		u32 ein : RANGE(6, 6);
		u32 enable : RANGE(7, 7);
		u32 : RANGE(31, 8);
	} bits;
} __attribute__((packed)) ibmuftian_i2c_ctrl;

typedef union i2c_cmd_type
{
    char cmd_type_str[CMD_TYPE_LEN];
    int cmd_type_int;
} i2c_cmd_type;

typedef struct i2c_command
{
    unsigned params[I2C_NUM_PARAMS];
    i2c_cmd_type command_type;
} i2c_command;

typedef struct ibmuftian_i2c
{
    struct kobject kobj;
	struct semaphore i2c_lock;
    unsigned id;
    ibmuftian_bar *bar4;
    unsigned speed;
    int ready;
    i2c_command command;
    char command_buff[I2C_BUFF_LEN];
    char command_result[I2C_BUFF_LEN];    
} ibmuftian_i2c;

typedef struct i2c_bd
{
    ibmuftian_i2c *i2c;    // The i2c object context that will do the I2C operation
    unsigned i2c_addr;     // The un-shifted I2C bus address, this will get shifted left by one.
    unsigned page_size;    // The devices internal page size
    unsigned dev_offset;   // The offset of this attribute within the device.
    unsigned offset_size;  // Two byte offsets used by EEPROM
} i2c_bd;

typedef struct i2c_attr
{
    struct attribute attr;
    ssize_t (*show) (ibmuftian_i2c *, struct i2c_attr *, char *);
    ssize_t (*store)(ibmuftian_i2c *, struct i2c_attr *, const char *, size_t);
} i2c_attr;

int lock_i2c_controller(ibmuftian_i2c *);
void release_i2c_controller(ibmuftian_i2c *);
int i2c_read(i2c_bd *, void *, unsigned, unsigned *);
int i2c_write(i2c_bd *, void const *, unsigned, unsigned *);
struct ibmuftian_i2c* ibmuftian_i2c_create(struct ibmuftian_board *);
int create_i2c(struct ibmuftian_board *);
void i2c_delete(ibmuftian_i2c *);


#endif
