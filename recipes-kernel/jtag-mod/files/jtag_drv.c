/********************************************************************************
* Description   : AST JTAG driver
*
* Copyright (C) 2017 Intel Corporation
* Copyright (C) 2012-2020  ASPEED Technology Inc.
* Copyright (C) 2018 NuvoTon Corporation
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by the Free Software Foundation;
* either version 2 of the License, or (at your option) any later version.
* This program is distributed in the hope that it will be useful,  but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/clk.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include "jtag_drv.h"

static struct regmap *gcr_regmap;
#define MFSEL1_OFFSET 0x00C
#define MFSEL3_OFFSET 0x064

#define JTAG_DEV_MAJOR    252

#define AST_JTAG_DEBUG

#ifdef AST_JTAG_DEBUG
#define JTAG_DBUG(fmt, args...) printk(KERN_DEBUG "%s() " fmt,__FUNCTION__, ## args)
#else
#define JTAG_DBUG(fmt, args...)
#endif

#define JTAG_MSG(fmt, args...) printk(fmt, ## args)

static DEFINE_SPINLOCK(jtag_state_lock);

/* Driver name will passed by insmod command */
static char *driver_name = JTAG_DRIVER_NAME;

/* Sysfs class structure. */
static struct class *sys_class;

/* added to satisfy found hig coverity issue. */
static struct device *jtagdev;

struct gpio_jtag {
	const char *name;
	unsigned gpio;
	struct gpio_desc *gpiod;
};

struct gpio_jtag jtags[pin_NUM];
#define GPIO_ARRAY_NUM	    (pin_NUM - 1)

#define JTAG_PSPI

#if defined(JTAG_PSPI)
static unsigned int pspi_virt_addr;
static struct clk *apb_clk;
static u32 apb_clk_src;
static unsigned char reverse[16] = {
	0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE,
	0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF
};
#define REVERSE(x)  ((reverse[(x & 0x0f)] << 4) | reverse[(x & 0xf0) >> 4])

/* Set 4MHz */
#define JTAG_PSPI_SPEED		(4 * 1000000)

#define JTAG_PSPI_BASE_ADDR	(pspi_virt_addr)
#define PSPI_DATA		(JTAG_PSPI_BASE_ADDR + 0x00)
#define PSPI_CTL1		(JTAG_PSPI_BASE_ADDR + 0x02)
#define PSPI_STAT		(JTAG_PSPI_BASE_ADDR + 0x04)

#define PSPI_CTL1_SCDV6_0	9
#define PSPI_CTL1_SCIDL		8
#define PSPI_CTL1_SCM		7
#define PSPI_CTL1_EIW		6
#define PSPI_CTL1_EIR		5
#define PSPI_CTL1_SPIEN		0

#define PSPI_STAT_RBF		1
#define PSPI_STAT_BSY		0

#define PSPI_8BIT		8
#define PSPI_16BIT		16
#endif

static u32 high = 1, low = 0;

/* Linux device data satructure */
dev_t cpdev;

/* Pablo: Removed references to AST-SCU hardare state tracking */
struct ast_jtag_info {
	u8                  tapstate;       // see enum JtagStateEncode
	bool                is_open;
	struct cdev         cdev;
};

/* this structure represents a TMS cycle, as expressed in a set of bits and
   a count of bits (note: there are no start->end state transitions that
   require more than 1 byte of TMS cycles) */
typedef struct {
	unsigned char tmsbits;
	unsigned char count;
} TmsCycle;

/* these are the string representations of the TAP states corresponding to
   the enums literals in JtagStateEncode */
const char* const c_statestr[] = {"TLR", "RTI", "SelDR", "CapDR",
				"ShfDR", "Ex1DR", "PauDR", "Ex2DR", "UpdDR",
				"SelIR", "CapIR", "ShfIR", "Ex1IR", "PauIR",
				"Ex2IR", "UpdIR"};

struct ast_jtag_info *ast_jtag;

/* this is the complete set TMS cycles for going from any TAP state to
   any other TAP state, following a “shortest path” rule */
const TmsCycle _tmsCycleLookup[][16] = {
/*   start*/ /*TLR      RTI      SelDR    CapDR    SDR      Ex1DR    PDR      Ex2DR    UpdDR    SelIR    CapIR    SIR      Ex1IR    PIR      Ex2IR    UpdIR    destination*/
/*     TLR*/{ {0x01,1},{0x00,1},{0x02,2},{0x02,3},{0x02,4},{0x0a,4},{0x0a,5},{0x2a,6},{0x1a,5},{0x06,3},{0x06,4},{0x06,5},{0x16,5},{0x16,6},{0x56,7},{0x36,6} },
/*     RTI*/{ {0x07,3},{0x00,1},{0x01,1},{0x01,2},{0x01,3},{0x05,3},{0x05,4},{0x15,5},{0x0d,4},{0x03,2},{0x03,3},{0x03,4},{0x0b,4},{0x0b,5},{0x2b,6},{0x1b,5} },
/*   SelDR*/{ {0x03,2},{0x03,3},{0x00,0},{0x00,1},{0x00,2},{0x02,2},{0x02,3},{0x0a,4},{0x06,3},{0x01,1},{0x01,2},{0x01,3},{0x05,3},{0x05,4},{0x15,5},{0x0d,4} },
/*   CapDR*/{ {0x1f,5},{0x03,3},{0x07,3},{0x00,0},{0x00,1},{0x01,1},{0x01,2},{0x05,3},{0x03,2},{0x0f,4},{0x0f,5},{0x0f,6},{0x2f,6},{0x2f,7},{0xaf,8},{0x6f,7} },
/*     SDR*/{ {0x1f,5},{0x03,3},{0x07,3},{0x07,4},{0x00,1},{0x01,1},{0x01,2},{0x05,3},{0x03,2},{0x0f,4},{0x0f,5},{0x0f,6},{0x2f,6},{0x2f,7},{0xaf,8},{0x6f,7} },
/*   Ex1DR*/{ {0x0f,4},{0x01,2},{0x03,2},{0x03,3},{0x02,3},{0x00,0},{0x00,1},{0x02,2},{0x01,1},{0x07,3},{0x07,4},{0x07,5},{0x17,5},{0x17,6},{0x57,7},{0x37,6} },
/*     PDR*/{ {0x1f,5},{0x03,3},{0x07,3},{0x07,4},{0x01,2},{0x05,3},{0x00,1},{0x01,1},{0x03,2},{0x0f,4},{0x0f,5},{0x0f,6},{0x2f,6},{0x2f,7},{0xaf,8},{0x6f,7} },
/*   Ex2DR*/{ {0x0f,4},{0x01,2},{0x03,2},{0x03,3},{0x00,1},{0x02,2},{0x02,3},{0x00,0},{0x01,1},{0x07,3},{0x07,4},{0x07,5},{0x17,5},{0x17,6},{0x57,7},{0x37,6} },
/*   UpdDR*/{ {0x07,3},{0x00,1},{0x01,1},{0x01,2},{0x01,3},{0x05,3},{0x05,4},{0x15,5},{0x00,0},{0x03,2},{0x03,3},{0x03,4},{0x0b,4},{0x0b,5},{0x2b,6},{0x1b,5} },
/*   SelIR*/{ {0x01,1},{0x01,2},{0x05,3},{0x05,4},{0x05,5},{0x15,5},{0x15,6},{0x55,7},{0x35,6},{0x00,0},{0x00,1},{0x00,2},{0x02,2},{0x02,3},{0x0a,4},{0x06,3} },
/*   CapIR*/{ {0x1f,5},{0x03,3},{0x07,3},{0x07,4},{0x07,5},{0x17,5},{0x17,6},{0x57,7},{0x37,6},{0x0f,4},{0x00,0},{0x00,1},{0x01,1},{0x01,2},{0x05,3},{0x03,2} },
/*     SIR*/{ {0x1f,5},{0x03,3},{0x07,3},{0x07,4},{0x07,5},{0x17,5},{0x17,6},{0x57,7},{0x37,6},{0x0f,4},{0x0f,5},{0x00,1},{0x01,1},{0x01,2},{0x05,3},{0x03,2} },
/*   Ex1IR*/{ {0x0f,4},{0x01,2},{0x03,2},{0x03,3},{0x03,4},{0x0b,4},{0x0b,5},{0x2b,6},{0x1b,5},{0x07,3},{0x07,4},{0x02,3},{0x00,0},{0x00,1},{0x02,2},{0x01,1} },
/*     PIR*/{ {0x1f,5},{0x03,3},{0x07,3},{0x07,4},{0x07,5},{0x17,5},{0x17,6},{0x57,7},{0x37,6},{0x0f,4},{0x0f,5},{0x01,2},{0x05,3},{0x00,1},{0x01,1},{0x03,2} },
/*   Ex2IR*/{ {0x0f,4},{0x01,2},{0x03,2},{0x03,3},{0x03,4},{0x0b,4},{0x0b,5},{0x2b,6},{0x1b,5},{0x07,3},{0x07,4},{0x00,1},{0x02,2},{0x02,3},{0x00,0},{0x01,1} },
/*   UpdIR*/{ {0x07,3},{0x00,1},{0x01,1},{0x01,2},{0x01,3},{0x05,3},{0x05,4},{0x15,5},{0x0d,4},{0x03,2},{0x03,3},{0x03,4},{0x0b,4},{0x0b,5},{0x2b,6},{0x00,0} },
};

/* Pablo: Modified this function to access GPIOs directly and only read TDO when needed */
static u8 TCK_Cycle(unsigned char no_tdo, unsigned char TMS, unsigned char TDI)
{
	u32 tdo = 0;

	/* Faster implementationi (1.66MHz). Since all GPIOs are on the same group one write is enough to set TMS, TDI and CLK
	 * TODO: Need to revisit if doing iowrite32 and ioread32 is ok here. Also, change access to GPnDOS and GPnDOC
	 * registers not to touch all other GPIOs in this bank that are defines as output (none affected now but
	 * could in the future */
	gpiod_set_value(jtags[pin_TDI].gpiod, (int)TDI);
	gpiod_set_value(jtags[pin_TMS].gpiod, (int)TMS);
	gpiod_set_value(jtags[pin_TCK].gpiod, (int)low);

	gpiod_set_value(jtags[pin_TDI].gpiod, (int)TDI);
	gpiod_set_value(jtags[pin_TMS].gpiod, (int)TMS);
	gpiod_set_value(jtags[pin_TCK].gpiod, (int)high);
	if (!no_tdo)
		tdo = gpiod_get_value(jtags[pin_TDO].gpiod);

	return (tdo);
}

/* Pablo: Removed jtag_wait_instruction_complete, wait_data_pause_complete and wait_data_complete
 * not needed when bitbanging */

/*************************************************************************************/
/* Pablo: added TDO needed */
void ast_jtag_bitbang(struct tck_bitbang *bitbang)
{
	bitbang->tdo = TCK_Cycle(0, bitbang->tms, bitbang->tdi);
}

/******************************************************************************/
static int ast_jtag_set_tapstate(struct ast_jtag_info *ast_jtag, unsigned int tapstate)
{
	unsigned char i;
	unsigned char tmsbits;
	unsigned char count;

	// ensure that the requested and current tap states are within 0 to 15.
	if ((ast_jtag->tapstate >= sizeof(_tmsCycleLookup[0]) / sizeof(_tmsCycleLookup[0][0])) ||  // Column
		(tapstate >= sizeof(_tmsCycleLookup) / sizeof _tmsCycleLookup[0]))  // row
		return -1;

	if (tapstate == JtagTLR) {
		// clear tap state and go back to TLR
		for (i = 0; i < 9; i++) {
			/* Pablo: No TDO needed x2 faster */
			TCK_Cycle(1, 1, 0);
		}
		ast_jtag->tapstate = JtagTLR;
		return 0;
	}

	tmsbits = _tmsCycleLookup[ast_jtag->tapstate][tapstate].tmsbits;
	count   = _tmsCycleLookup[ast_jtag->tapstate][tapstate].count;

	if (count == 0)
		return 0;

	for (i = 0; i < count; i++) {
		/* Pablo: No TDO needed x2 faster */
		TCK_Cycle(1, (tmsbits & 1), 0);
		tmsbits >>= 1;
	}
	ast_jtag->tapstate = tapstate;
	return 0;
}
#if defined(JTAG_PSPI)
static void jtag_switch_pspi(int enable)
{
	int divisor, count;

	if (enable) {
		divisor = (apb_clk_src / (2 * JTAG_PSPI_SPEED)) - 1;
		if (divisor <= 0) {
			printk(KERN_ERR "Requested PSPI frequency is too large.\n");
			return;
		}

		gpiod_set_value(jtags[pin_TMS].gpiod, 0);
		gpiod_set_value(jtags[pin_TDI].gpiod, 0);
		gpiod_set_value(jtags[pin_TCK].gpiod, 0);

		writew(readw(PSPI_CTL1) & ~(0x1 << PSPI_CTL1_SPIEN), PSPI_CTL1);

		regmap_update_bits(gcr_regmap, MFSEL3_OFFSET, (0x3 << 3),
				(0x2 << 3));

		writew((readw(PSPI_CTL1) & ~(0x7f << PSPI_CTL1_SCDV6_0)) |
				(divisor << PSPI_CTL1_SCDV6_0),
				PSPI_CTL1);

		writew(readw(PSPI_CTL1) &
				~(0x1 << PSPI_CTL1_SCIDL),
				PSPI_CTL1);

		writew(readw(PSPI_CTL1) &
				~(0x1 << PSPI_CTL1_SCM),
				PSPI_CTL1);

		writew(readw(PSPI_CTL1) | (0x1 << PSPI_CTL1_SPIEN),
				PSPI_CTL1);

		while (readb(PSPI_STAT) & (0x1 << PSPI_STAT_RBF))
			readb(PSPI_STAT);
	} else {
		writew(readw(PSPI_CTL1) & ~(0x1 << PSPI_CTL1_SPIEN), PSPI_CTL1);
		regmap_update_bits(gcr_regmap, MFSEL3_OFFSET, (0x3 << 3),
				(0x0 << 3));
	}
}
#endif

/* Pablo: Removed ast_jtag_interrupt. No HW so not needed */
/******************************************************************************/
void ast_jtag_readwrite_scan(struct ast_jtag_info *ast_jtag, struct scan_xfer *scan_xfer)
{
	unsigned int  bit_index = 0;
	unsigned char *tdi_p = scan_xfer->tdi;
	unsigned char *tdo_p = scan_xfer->tdo;
#if defined(JTAG_PSPI)
	int remain_bits = scan_xfer->length;
	unsigned int pspi = 0;
#endif

	if ((ast_jtag->tapstate != JtagShfDR) && (ast_jtag->tapstate != JtagShfIR)) {
		if ((ast_jtag->tapstate >= 0) && (ast_jtag->tapstate < sizeof(c_statestr) / sizeof(c_statestr[0]))) {
			printk(KERN_ERR "readwrite_scan bad current tapstate = %s\n", c_statestr[ast_jtag->tapstate]);
		}
		return;
	}
	if (scan_xfer->length == 0) {
		printk(KERN_ERR "readwrite_scan bad length 0\n");
		return;
	}

	if (scan_xfer->tdi == NULL && scan_xfer->tdi_bytes != 0) {
		printk(KERN_ERR "readwrite_scan null tdi with nonzero length %u!\n", scan_xfer->tdi_bytes);
		return;
	}

	if (scan_xfer->tdo == NULL && scan_xfer->tdo_bytes != 0) {
		printk(KERN_ERR "readwrite_scan null tdo with nonzero length %u!\n", scan_xfer->tdo_bytes);
		return;
	}

#if defined(JTAG_PSPI)
	if (remain_bits > PSPI_8BIT) {
		jtag_switch_pspi(1);
		pspi = 1;
	}
#endif
	while (bit_index < scan_xfer->length) {
		int bit_offset = (bit_index % 8);
		int this_input_bit = 0;
		int tms_high_or_low;
		int this_output_bit;
#if defined (JTAG_PSPI)
		unsigned char tdo;

		if (remain_bits <= PSPI_8BIT)
#endif
		{
#if defined (JTAG_PSPI)
			if (pspi) {
				jtag_switch_pspi(0);
				pspi = 0;
			}
#endif
			if (bit_index / 8 < scan_xfer->tdi_bytes) {
				// If we are on a byte boundary, increment the byte pointers
				// Don't increment on 0, pointer is already on the first byte
				if (bit_index % 8 == 0 && bit_index != 0)
					tdi_p++;

				this_input_bit = (*tdi_p >> bit_offset) & 1;
			}
			// If this is the last bit, leave TMS high
			tms_high_or_low = (bit_index == scan_xfer->length - 1) &&
				(scan_xfer->end_tap_state != JtagShfDR) &&
				(scan_xfer->end_tap_state != JtagShfIR);
			this_output_bit = TCK_Cycle(0, tms_high_or_low, this_input_bit);
			// If it was the last bit in the scan and the end_tap_state is something other than shiftDR or shiftIR then go to Exit1.
			// IMPORTANT Note: if the end_tap_state is ShiftIR/DR and the next call to this function is a shiftDR/IR then the driver will not change state!
			if (tms_high_or_low) {
				ast_jtag->tapstate = (ast_jtag->tapstate == JtagShfDR) ?
					JtagEx1DR : JtagEx1IR;
			}
			if (bit_index / 8 < scan_xfer->tdo_bytes) {
				if (bit_index % 8 == 0) {
					if (bit_index != 0) {
						tdo_p++;
					}
					// Zero the output buffer before we write data to it
					*tdo_p = 0;
				}
				*tdo_p |= this_output_bit << bit_offset;
			}
			bit_index++;
		}
#if defined(JTAG_PSPI)
		else {
			while (readb(PSPI_STAT) & (0x1 << PSPI_STAT_BSY));

			if (bit_index / 8 < scan_xfer->tdi_bytes)
				writeb(REVERSE(*tdi_p), PSPI_DATA);
			else
				writeb(0x0, PSPI_DATA);

			while (readb(PSPI_STAT) & (0x1 << PSPI_STAT_BSY));

			while (!(readb(PSPI_STAT) & (0x1 << PSPI_STAT_RBF)));

			if (bit_index / 8 < scan_xfer->tdo_bytes) {
				tdo = readb(PSPI_DATA);
				*tdo_p = REVERSE(tdo);
			}

			bit_index += 8;
			remain_bits -= 8;
			if (remain_bits > PSPI_8BIT) {
				tdo_p++;
				tdi_p++;
			}
		}
#endif
	}
	// Go to the requested tap state (may be a NOP, as we may be already in the JtagShf state)
	ast_jtag_set_tapstate(ast_jtag, scan_xfer->end_tap_state);
}

/*************************************************************************************/
/* Pablo: Set All output GPIOs as inputs */
static inline void ast_jtag_slave(void)
{
	regmap_update_bits(gcr_regmap, MFSEL1_OFFSET, (0x1 << 14),
			(0x1 << 14));
	regmap_update_bits(gcr_regmap, MFSEL3_OFFSET, (0x3 << 3),
			(0x0 << 3));
}


/* Pablo: Set All output GPIOs as outputs */
static inline void ast_jtag_master(void)
{
	int count;

	regmap_update_bits(gcr_regmap, MFSEL1_OFFSET, (0x1 << 14),
			(0x0 << 14));
	regmap_update_bits(gcr_regmap, MFSEL3_OFFSET, (0x3 << 3),
			(0x0 << 3));

	for (count = 0; count < pin_NUM; count ++) {
		if (count == pin_TDO)
			gpiod_direction_input(jtags[count].gpiod);
		else
			gpiod_direction_output(jtags[count].gpiod, high);
	}

	ast_jtag_set_tapstate(ast_jtag, JtagTLR);
}

static long jtag_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct ast_jtag_info *ast_jtag = file->private_data;
	void __user *argp = (void __user *)arg;
	struct tck_bitbang bitbang;
	struct scan_xfer scan_xfer;

	switch (cmd) {
	case AST_JTAG_SIOCFREQ:
		/* Pablo: Do nothing no AST-SCU */
		break;
	case AST_JTAG_GIOCFREQ:
		/* Pablo: return dummy 1.6MHz (Not used in this version)*/
		ret = 1666666;
		break;
	case AST_JTAG_BITBANG:
		if (copy_from_user(&bitbang, argp, sizeof(struct tck_bitbang)))
			ret = -EFAULT;
		else
			ast_jtag_bitbang(&bitbang);
		if (copy_to_user(argp, &bitbang, sizeof(struct tck_bitbang)))
			ret = -EFAULT;
		break;
	case AST_JTAG_SET_TAPSTATE:
		ast_jtag_set_tapstate(ast_jtag, (unsigned int)arg);
		break;
	case AST_JTAG_READWRITESCAN:
		if (copy_from_user(&scan_xfer, argp, sizeof(struct scan_xfer)))
			ret = -EFAULT;
		else {
			ast_jtag_readwrite_scan(ast_jtag, &scan_xfer);
		}
		if (copy_to_user(argp, &scan_xfer, sizeof(struct scan_xfer)))
			ret = -EFAULT;
		break;
	case AST_JTAG_SLAVECONTLR:
		if (arg)
			ast_jtag_slave();
		else
			ast_jtag_master();
		break;
	default:
		return -ENOTTY;
	}

	return ret;
}

static struct platform_driver ast_jtag_driver = {
	.driver = {
		.name   = "jtag_drv",
		.owner  = THIS_MODULE,
	}
};

static int jtag_open(struct inode *inode, struct file *file)
{
	spin_lock(&jtag_state_lock);
	if (ast_jtag->is_open) {
		spin_unlock(&jtag_state_lock);
		return -EBUSY;
	}

	ast_jtag->is_open = true;
	file->private_data = ast_jtag;

	spin_unlock(&jtag_state_lock);

	return 0;
}

static int jtag_release(struct inode *inode, struct file *file)
{
	struct ast_jtag_info *drvdata = file->private_data;

	spin_lock(&jtag_state_lock);

	drvdata->is_open = false;

	spin_unlock(&jtag_state_lock);

	return 0;
}

static ssize_t show_tdo(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	u32 tdo = 0;

	tdo = gpiod_get_value(jtags[pin_TDO].gpiod);

	return sprintf(buf, "%u\n", tdo);
}

static DEVICE_ATTR(tdo, S_IRUGO, show_tdo, NULL);

static ssize_t store_tdi(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int tdi;

	tdi = (unsigned int)simple_strtoul(buf, NULL, 10);

	gpiod_set_value(jtags[pin_TDI].gpiod, tdi);

	return count;
}

static DEVICE_ATTR(tdi, S_IWUSR, NULL, store_tdi);

static ssize_t store_tms(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int tms;

	tms = simple_strtoul(buf, NULL, 10);
	gpiod_set_value(jtags[pin_TMS].gpiod, tms);

	return count;
}

static DEVICE_ATTR(tms, S_IWUSR, NULL, store_tms);

static ssize_t store_tck(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int tck;

	tck = simple_strtoul(buf, NULL, 10);
	gpiod_set_value(jtags[pin_TCK].gpiod, tck);

	return count;
}

static DEVICE_ATTR(tck, S_IWUSR, NULL, store_tck);

static ssize_t show_sts(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if ((ast_jtag->tapstate >= 0) && (ast_jtag->tapstate < sizeof(c_statestr) / sizeof(c_statestr[0])))
		return sprintf(buf, "%s\n", c_statestr[ast_jtag->tapstate]);
	else
		return sprintf(buf, "ERROR\n");
}

static DEVICE_ATTR(sts, S_IRUGO, show_sts, NULL);

/* Pablo: Removed show and store frequence attributes */
static struct attribute *jtag_sysfs_entries[] = {
	&dev_attr_sts.attr,
	&dev_attr_tck.attr,
	&dev_attr_tms.attr,
	&dev_attr_tdi.attr,
	&dev_attr_tdo.attr,
	NULL
};

static struct attribute_group jtag_attribute_group = {
	.attrs = jtag_sysfs_entries,
};

static struct file_operations ast_jtag_fops = {
	.open              = jtag_open,
	.unlocked_ioctl    = jtag_ioctl,
	.release           = jtag_release,
};

static int jtag_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fwnode_handle *child;
	int count, i = 0, ret;

	/* disable for now - need to check if necessary */
	gcr_regmap = syscon_regmap_lookup_by_compatible("nuvoton,npcm750-gcr");
	if (IS_ERR(gcr_regmap)) {
		pr_err("%s: failed to find nuvoton,npcm750-gcr\n", __func__);
		return IS_ERR(gcr_regmap);
	}

	count = device_get_child_node_count(dev);
	if (!count)
		return ERR_PTR(-ENODEV);

	device_for_each_child_node(dev, child) {
		const char *name = NULL;
		struct gpio_desc *gpiod;
		ret = fwnode_property_read_string(child, "label", &name);
		if (!name) {
			fwnode_handle_put(child);
			return ERR_PTR(-EINVAL);
		}

		gpiod = devm_fwnode_get_gpiod_from_child(dev, NULL, child,
							GPIOD_ASIS, name);

		if (IS_ERR(gpiod)) {
			fwnode_handle_put(child);
			return ERR_CAST(gpiod);
		}

		if (!strcmp(name, "tdi"))
			i = pin_TDI;
		else if (!strcmp(name, "tms"))
			i = pin_TMS;
		else if (!strcmp(name, "tdo"))
			i = pin_TDO;
		else if (!strcmp(name, "tck"))
			i = pin_TCK;
		else
			return ERR_PTR(-EINVAL);

		jtags[i].name = name;
		jtags[i].gpiod = gpiod;
	}

	ast_jtag_slave();

	return 0;
}

static const struct of_device_id of_gpio_jtag_match[] = {
	{ .compatible = "gpio-jtag_drv", },
	{},
};

static struct platform_driver jtag_driver = {
	.probe          = jtag_probe,
	.driver         = {
		.name   = "jtag-gpios",
		.of_match_table = of_gpio_jtag_match,
	},
};

#if defined(JTAG_PSPI)
static int jtag_pspi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np;
	struct resource res;
	int ret = 0;

	regmap_update_bits(gcr_regmap, MFSEL3_OFFSET, (0x3 << 3),
			(0x0 << 3));

	np = dev->of_node;
	ret = of_address_to_resource(np, 0, &res);
	if (ret) {
		pr_err("\t\t\t of_address_to_resource fail ret %d\n", ret);
		return -EINVAL;
	}

	pspi_virt_addr = (int)ioremap(res.start, resource_size(&res));

	if (!pspi_virt_addr) {
		pr_info("\t\t\t pspi_virt_addr fail \n");
		return -ENOMEM;
	}

	apb_clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(apb_clk)) {
		pr_err(" JTAG PSPI probe failed: can't read clk.\n");
		return -ENODEV;
	}
	clk_prepare_enable(apb_clk);

	apb_clk_src = clk_get_rate(apb_clk);

	return 0;
}

static const struct of_device_id of_jtag_pspi_match[] = {
	{ .compatible = "nuvoton,npcm750-pspi"},
	{},
};

static struct platform_driver pspi_driver = {
	.probe		= jtag_pspi_probe,
	.driver		= {
		.name	= "jtag-pspi",
		.of_match_table = of_jtag_pspi_match,
	},
};
#endif

/* Pablo: Probe is init now. Changed to match environment */
int __init ast_jtag_init(void)
{
	int result;

	JTAG_DBUG("ast_jtag_probe load started\n");

	if (!(ast_jtag = kzalloc(sizeof(struct ast_jtag_info), GFP_KERNEL)))
		return -ENOMEM;

	/** - allocate a cedv for cpdev and dispatch ast_jtag_fops to ast_jtag->cdev->ops */
	cdev_init(&ast_jtag->cdev, &ast_jtag_fops);

	/** - register the cpdev */
	result = alloc_chrdev_region(&cpdev, 0, 1, driver_name);

	printk("mknod /dev/jtag_drv c %d 0\n", MAJOR(cpdev));

	if (result < 0) {
		printk(KERN_ERR "KN : Can't register jtag_dev, ERROR: %d\n", result);
		return result;
	}

	/* Add cpdev to system */
	cdev_add(&ast_jtag->cdev, cpdev, 1);

	/** Add debugfs functions here */

	if ((sys_class = class_create(THIS_MODULE, driver_name)) == NULL) {
		printk("KN:CLASS_CREATE Filed with %d\n", MAJOR(cpdev));
		unregister_chrdev_region(cpdev, 1);
		return -1;
	}

	if ((jtagdev = device_create(sys_class, NULL, cpdev, NULL, driver_name)) == NULL) {
		printk("KN: DEVICE_CREATE Filed with %d\n", MAJOR(cpdev));

		/** - Unregister fp_cdev */
		unregister_chrdev_region(cpdev, 1);
		cdev_del(&ast_jtag->cdev);
		class_destroy(sys_class);
		result = -1;
	}

	/* - Initial sysfs related resource */
	result = platform_driver_register(&ast_jtag_driver);
	if (result) {
		printk(KERN_ERR "%s(): can't register sysfs.\n", __FUNCTION__);
		/** - Unregister fp_cdev */
		return result;
	}

	result = sysfs_create_group(&jtagdev->kobj, &jtag_attribute_group);
	if (result) {
		printk(KERN_ERR "ast_jtag: failed to create sysfs device attributes.\n");
		return result;
	}

#if defined(JTAG_PSPI)
	result = platform_driver_register(&jtag_driver);
	if (result) {
		printk(KERN_ERR "Failed to init jtag_driver.\n");
		return result;
	}

	return platform_driver_register(&pspi_driver);
#else
	return platform_driver_register(&jtag_driver);
#endif
}

/* Pablo: Remove is exit now. Changed to match environment */
static void __exit ast_jtag_exit(void)
{
	if (ast_jtag == NULL)
		return;

	JTAG_DBUG("ast_jtag_remove\n");

	/** - Destroy sysfs class. */
	sysfs_remove_group(&jtagdev->kobj, &jtag_attribute_group);
	device_destroy(sys_class, cpdev);
	class_destroy(sys_class);

	/** - Unregister fp_cdev */
	cdev_del(&ast_jtag->cdev);
	unregister_chrdev_region(cpdev, 1);

#if defined(JTAG_PSPI)
	platform_driver_unregister(&pspi_driver);
#endif
	platform_driver_unregister(&jtag_driver);
	platform_driver_unregister(&ast_jtag_driver);

	kfree(ast_jtag);

	ast_jtag_slave();

	printk("JTAG driver removed successfully!\n");

	return;
}

MODULE_DESCRIPTION("AST JTAG LIB Driver");
MODULE_LICENSE("GPL");
module_init(ast_jtag_init);
module_exit(ast_jtag_exit);
module_param(driver_name, charp, S_IRUGO);

