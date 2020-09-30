/***************************************************************************
 *   Copyright (C) 2016 by Matthias Welwarsky                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *                                                                         *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <stdint.h>
#include "target/arm_adi_v5.h"
#include "target/arm.h"
#include "helper/list.h"
#include "helper/command.h"
#include "transport/transport.h"
#include "jtag/interface.h"
#include <helper/jep106.h>
#include "target/aji_vjtag.h"

static LIST_HEAD(all_dap);

extern const struct dap_ops swd_dap_ops;
extern const struct dap_ops jtag_dp_ops;
extern struct adapter_driver *adapter_driver;

/* DAP command support */
struct arm_vjtag_object {
	struct list_head lh;
	struct adiv5_dap dap;
	char *name;
	const struct swd_driver *swd;
};

static void vjtag_instance_init(struct adiv5_dap *dap)
{
	int i;
	/* Set up with safe defaults */
	for (i = 0; i <= DP_APSEL_MAX; i++) {
		dap->ap[i].dap = dap;
		dap->ap[i].ap_num = i;
		/* memaccess_tck max is 255 */
		dap->ap[i].memaccess_tck = 255;
		/* Number of bits for tar autoincrement, impl. dep. at least 10 */
		dap->ap[i].tar_autoincr_block = (1<<10);
		/* default CSW value */
		dap->ap[i].csw_default = CSW_AHB_DEFAULT;
	}
	INIT_LIST_HEAD(&dap->cmd_journal);
	INIT_LIST_HEAD(&dap->cmd_pool);
}
/*
const char *adiv5_dap_name(struct adiv5_dap *self)
{
	struct arm_vjtag_object *obj = container_of(self, struct arm_vjtag_object, dap);
	return obj->name;
}

const struct swd_driver *adiv5_dap_swd_driver(struct adiv5_dap *self)
{
	struct arm_vjtag_object *obj = container_of(self, struct arm_vjtag_object, dap);
	return obj->swd;
}

struct adiv5_dap *dap_instance_by_jim_obj(Jim_Interp *interp, Jim_Obj *o)
{
	struct arm_vjtag_object *obj = NULL;
	const char *name;
	bool found = false;

	name = Jim_GetString(o, NULL);

	list_for_each_entry(obj, &all_dap, lh) {
		if (!strcmp(name, obj->name)) {
			found = true;
			break;
		}
	}

	if (found)
		return &obj->dap;
	return NULL;
}
*/

static int vjtag_init_all(void)
{
	struct arm_vjtag_object *obj;
	int retval;

	LOG_DEBUG("Initializing all DAPs ...");

	list_for_each_entry(obj, &all_dap, lh) {
		struct adiv5_dap *dap = &obj->dap;

		/* with hla, dap is just a dummy */
		if (transport_is_hla())
			continue;

		/* skip taps that are disabled */
		if (!dap->tap->enabled)
			continue;

		if (transport_is_swd()) {
			dap->ops = &swd_dap_ops;
			obj->swd = adapter_driver->swd_ops;
		} else if (transport_is_dapdirect_swd()) {
			dap->ops = adapter_driver->dap_swd_ops;
		} else if (transport_is_dapdirect_jtag()) {
			dap->ops = adapter_driver->dap_jtag_ops;
		} else
			dap->ops = &jtag_dp_ops;

		retval = dap->ops->connect(dap);
		if (retval != ERROR_OK)
			return retval;
	}

	return ERROR_OK;
}

/* =================================================================================*/
/* From arm_adi_v5.c  */
#define ANY_ID 0x1000

#define ARM_ID 0x4BB

static const struct {
	uint16_t designer_id;
	uint16_t part_num;
	const char* type;
	const char* full;
} dap_partnums[] = {
	{ ARM_ID, 0x000, "Cortex-M3 SCS",              "(System Control Space)", },
	{ ARM_ID, 0x001, "Cortex-M3 ITM",              "(Instrumentation Trace Module)", },
	{ ARM_ID, 0x002, "Cortex-M3 DWT",              "(Data Watchpoint and Trace)", },
	{ ARM_ID, 0x003, "Cortex-M3 FPB",              "(Flash Patch and Breakpoint)", },
	{ ARM_ID, 0x008, "Cortex-M0 SCS",              "(System Control Space)", },
	{ ARM_ID, 0x00a, "Cortex-M0 DWT",              "(Data Watchpoint and Trace)", },
	{ ARM_ID, 0x00b, "Cortex-M0 BPU",              "(Breakpoint Unit)", },
	{ ARM_ID, 0x00c, "Cortex-M4 SCS",              "(System Control Space)", },
	{ ARM_ID, 0x00d, "CoreSight ETM11",            "(Embedded Trace)", },
	{ ARM_ID, 0x00e, "Cortex-M7 FPB",              "(Flash Patch and Breakpoint)", },
	{ ARM_ID, 0x470, "Cortex-M1 ROM",              "(ROM Table)", },
	{ ARM_ID, 0x471, "Cortex-M0 ROM",              "(ROM Table)", },
	{ ARM_ID, 0x490, "Cortex-A15 GIC",             "(Generic Interrupt Controller)", },
	{ ARM_ID, 0x4a1, "Cortex-A53 ROM",             "(v8 Memory Map ROM Table)", },
	{ ARM_ID, 0x4a2, "Cortex-A57 ROM",             "(ROM Table)", },
	{ ARM_ID, 0x4a3, "Cortex-A53 ROM",             "(v7 Memory Map ROM Table)", },
	{ ARM_ID, 0x4a4, "Cortex-A72 ROM",             "(ROM Table)", },
	{ ARM_ID, 0x4a9, "Cortex-A9 ROM",              "(ROM Table)", },
	{ ARM_ID, 0x4aa, "Cortex-A35 ROM",             "(v8 Memory Map ROM Table)", },
	{ ARM_ID, 0x4af, "Cortex-A15 ROM",             "(ROM Table)", },
	{ ARM_ID, 0x4b5, "Cortex-R5 ROM",              "(ROM Table)", },
	{ ARM_ID, 0x4c0, "Cortex-M0+ ROM",             "(ROM Table)", },
	{ ARM_ID, 0x4c3, "Cortex-M3 ROM",              "(ROM Table)", },
	{ ARM_ID, 0x4c4, "Cortex-M4 ROM",              "(ROM Table)", },
	{ ARM_ID, 0x4c7, "Cortex-M7 PPB ROM",          "(Private Peripheral Bus ROM Table)", },
	{ ARM_ID, 0x4c8, "Cortex-M7 ROM",              "(ROM Table)", },
	{ ARM_ID, 0x4e0, "Cortex-A35 ROM",             "(v7 Memory Map ROM Table)", },
	{ ARM_ID, 0x906, "CoreSight CTI",              "(Cross Trigger)", },
	{ ARM_ID, 0x907, "CoreSight ETB",              "(Trace Buffer)", },
	{ ARM_ID, 0x908, "CoreSight CSTF",             "(Trace Funnel)", },
	{ ARM_ID, 0x909, "CoreSight ATBR",             "(Advanced Trace Bus Replicator)", },
	{ ARM_ID, 0x910, "CoreSight ETM9",             "(Embedded Trace)", },
	{ ARM_ID, 0x912, "CoreSight TPIU",             "(Trace Port Interface Unit)", },
	{ ARM_ID, 0x913, "CoreSight ITM",              "(Instrumentation Trace Macrocell)", },
	{ ARM_ID, 0x914, "CoreSight SWO",              "(Single Wire Output)", },
	{ ARM_ID, 0x917, "CoreSight HTM",              "(AHB Trace Macrocell)", },
	{ ARM_ID, 0x920, "CoreSight ETM11",            "(Embedded Trace)", },
	{ ARM_ID, 0x921, "Cortex-A8 ETM",              "(Embedded Trace)", },
	{ ARM_ID, 0x922, "Cortex-A8 CTI",              "(Cross Trigger)", },
	{ ARM_ID, 0x923, "Cortex-M3 TPIU",             "(Trace Port Interface Unit)", },
	{ ARM_ID, 0x924, "Cortex-M3 ETM",              "(Embedded Trace)", },
	{ ARM_ID, 0x925, "Cortex-M4 ETM",              "(Embedded Trace)", },
	{ ARM_ID, 0x930, "Cortex-R4 ETM",              "(Embedded Trace)", },
	{ ARM_ID, 0x931, "Cortex-R5 ETM",              "(Embedded Trace)", },
	{ ARM_ID, 0x932, "CoreSight MTB-M0+",          "(Micro Trace Buffer)", },
	{ ARM_ID, 0x941, "CoreSight TPIU-Lite",        "(Trace Port Interface Unit)", },
	{ ARM_ID, 0x950, "Cortex-A9 PTM",              "(Program Trace Macrocell)", },
	{ ARM_ID, 0x955, "Cortex-A5 ETM",              "(Embedded Trace)", },
	{ ARM_ID, 0x95a, "Cortex-A72 ETM",             "(Embedded Trace)", },
	{ ARM_ID, 0x95b, "Cortex-A17 PTM",             "(Program Trace Macrocell)", },
	{ ARM_ID, 0x95d, "Cortex-A53 ETM",             "(Embedded Trace)", },
	{ ARM_ID, 0x95e, "Cortex-A57 ETM",             "(Embedded Trace)", },
	{ ARM_ID, 0x95f, "Cortex-A15 PTM",             "(Program Trace Macrocell)", },
	{ ARM_ID, 0x961, "CoreSight TMC",              "(Trace Memory Controller)", },
	{ ARM_ID, 0x962, "CoreSight STM",              "(System Trace Macrocell)", },
	{ ARM_ID, 0x975, "Cortex-M7 ETM",              "(Embedded Trace)", },
	{ ARM_ID, 0x9a0, "CoreSight PMU",              "(Performance Monitoring Unit)", },
	{ ARM_ID, 0x9a1, "Cortex-M4 TPIU",             "(Trace Port Interface Unit)", },
	{ ARM_ID, 0x9a4, "CoreSight GPR",              "(Granular Power Requester)", },
	{ ARM_ID, 0x9a5, "Cortex-A5 PMU",              "(Performance Monitor Unit)", },
	{ ARM_ID, 0x9a7, "Cortex-A7 PMU",              "(Performance Monitor Unit)", },
	{ ARM_ID, 0x9a8, "Cortex-A53 CTI",             "(Cross Trigger)", },
	{ ARM_ID, 0x9a9, "Cortex-M7 TPIU",             "(Trace Port Interface Unit)", },
	{ ARM_ID, 0x9ae, "Cortex-A17 PMU",             "(Performance Monitor Unit)", },
	{ ARM_ID, 0x9af, "Cortex-A15 PMU",             "(Performance Monitor Unit)", },
	{ ARM_ID, 0x9b7, "Cortex-R7 PMU",              "(Performance Monitor Unit)", },
	{ ARM_ID, 0x9d3, "Cortex-A53 PMU",             "(Performance Monitor Unit)", },
	{ ARM_ID, 0x9d7, "Cortex-A57 PMU",             "(Performance Monitor Unit)", },
	{ ARM_ID, 0x9d8, "Cortex-A72 PMU",             "(Performance Monitor Unit)", },
	{ ARM_ID, 0x9da, "Cortex-A35 PMU/CTI/ETM",     "(Performance Monitor Unit/Cross Trigger/ETM)", },
	{ ARM_ID, 0xc05, "Cortex-A5 Debug",            "(Debug Unit)", },
	{ ARM_ID, 0xc07, "Cortex-A7 Debug",            "(Debug Unit)", },
	{ ARM_ID, 0xc08, "Cortex-A8 Debug",            "(Debug Unit)", },
	{ ARM_ID, 0xc09, "Cortex-A9 Debug",            "(Debug Unit)", },
	{ ARM_ID, 0xc0e, "Cortex-A17 Debug",           "(Debug Unit)", },
	{ ARM_ID, 0xc0f, "Cortex-A15 Debug",           "(Debug Unit)", },
	{ ARM_ID, 0xc14, "Cortex-R4 Debug",            "(Debug Unit)", },
	{ ARM_ID, 0xc15, "Cortex-R5 Debug",            "(Debug Unit)", },
	{ ARM_ID, 0xc17, "Cortex-R7 Debug",            "(Debug Unit)", },
	{ ARM_ID, 0xd03, "Cortex-A53 Debug",           "(Debug Unit)", },
	{ ARM_ID, 0xd04, "Cortex-A35 Debug",           "(Debug Unit)", },
	{ ARM_ID, 0xd07, "Cortex-A57 Debug",           "(Debug Unit)", },
	{ ARM_ID, 0xd08, "Cortex-A72 Debug",           "(Debug Unit)", },
	{ 0x097,  0x9af, "MSP432 ROM",                 "(ROM Table)" },
	{ 0x09f,  0xcd0, "Atmel CPU with DSU",         "(CPU)" },
	{ 0x0c1,  0x1db, "XMC4500 ROM",                "(ROM Table)" },
	{ 0x0c1,  0x1df, "XMC4700/4800 ROM",           "(ROM Table)" },
	{ 0x0c1,  0x1ed, "XMC1000 ROM",                "(ROM Table)" },
	{ 0x0E5,  0x000, "SHARC+/Blackfin+",           "", },
	{ 0x0F0,  0x440, "Qualcomm QDSS Component v1", "(Qualcomm Designed CoreSight Component v1)", },
	{ 0x3eb,  0x181, "Tegra 186 ROM",              "(ROM Table)", },
	{ 0x3eb,  0x202, "Denver ETM",                 "(Denver Embedded Trace)", },
	{ 0x3eb,  0x211, "Tegra 210 ROM",              "(ROM Table)", },
	{ 0x3eb,  0x302, "Denver Debug",               "(Debug Unit)", },
	{ 0x3eb,  0x402, "Denver PMU",                 "(Performance Monitor Unit)", },
	/* legacy comment: 0x113: what? */
	{ ANY_ID, 0x120, "TI SDTI",                    "(System Debug Trace Interface)", }, /* from OMAP3 memmap */
	{ ANY_ID, 0x343, "TI DAPCTL",                  "", }, /* from OMAP3 memmap */
};

/* CID interpretation -- see ARM IHI 0029B section 3
 * and ARM IHI 0031A table 13-3.
 */
static const char* class_description[16] = {
	"Reserved", "ROM table", "Reserved", "Reserved",
	"Reserved", "Reserved", "Reserved", "Reserved",
	"Reserved", "CoreSight component", "Reserved", "Peripheral Test Block",
	"Reserved", "OptimoDE DESS",
	"Generic IP component", "PrimeCell or System component"
};

static bool is_dap_cid_ok(uint32_t cid)
{
	return (cid & 0xffff0fff) == 0xb105000d;
}

static int vjtag_read_part_id(struct adiv5_ap* ap, uint32_t component_base, uint32_t* cid, uint64_t* pid)
{
	assert((component_base & 0xFFF) == 0);
	assert(ap != NULL && cid != NULL && pid != NULL);

	uint32_t cid0, cid1, cid2, cid3;
	uint32_t pid0, pid1, pid2, pid3, pid4;
	int retval;

	/* IDs are in last 4K section */
	retval = mem_ap_read_u32(ap, component_base + 0xFE0, &pid0);
	if (retval != ERROR_OK)
		return retval;
	retval = mem_ap_read_u32(ap, component_base + 0xFE4, &pid1);
	if (retval != ERROR_OK)
		return retval;
	retval = mem_ap_read_u32(ap, component_base + 0xFE8, &pid2);
	if (retval != ERROR_OK)
		return retval;
	retval = mem_ap_read_u32(ap, component_base + 0xFEC, &pid3);
	if (retval != ERROR_OK)
		return retval;
	retval = mem_ap_read_u32(ap, component_base + 0xFD0, &pid4);
	if (retval != ERROR_OK)
		return retval;
	retval = mem_ap_read_u32(ap, component_base + 0xFF0, &cid0);
	if (retval != ERROR_OK)
		return retval;
	retval = mem_ap_read_u32(ap, component_base + 0xFF4, &cid1);
	if (retval != ERROR_OK)
		return retval;
	retval = mem_ap_read_u32(ap, component_base + 0xFF8, &cid2);
	if (retval != ERROR_OK)
		return retval;
	retval = mem_ap_read_u32(ap, component_base + 0xFFC, &cid3);
	if (retval != ERROR_OK)
		return retval;

	retval = dap_run(ap->dap);
	if (retval != ERROR_OK)
		return retval;

	*cid = (cid3 & 0xff) << 24
		| (cid2 & 0xff) << 16
		| (cid1 & 0xff) << 8
		| (cid0 & 0xff);
	*pid = (uint64_t)(pid4 & 0xff) << 32
		| (pid3 & 0xff) << 24
		| (pid2 & 0xff) << 16
		| (pid1 & 0xff) << 8
		| (pid0 & 0xff);

	return ERROR_OK;
}

static int vjtag_rom_display(struct command_invocation* cmd,
	struct adiv5_ap* ap, uint32_t dbgbase, int depth)
{
	int retval;
	uint64_t pid;
	uint32_t cid;
	char tabs[16] = "";

	if (depth > 16) {
		command_print(cmd, "\tTables too deep");
		return ERROR_FAIL;
	}

	if (depth)
		snprintf(tabs, sizeof(tabs), "[L%02d] ", depth);

	uint32_t base_addr = dbgbase & 0xFFFFF000;
	command_print(cmd, "\t\tComponent base address 0x%08" PRIx32, base_addr);

	retval = vjtag_read_part_id(ap, base_addr, &cid, &pid);
	if (retval != ERROR_OK) {
		command_print(cmd, "\t\tCan't read component, the corresponding core might be turned off");
		return ERROR_OK; /* Don't abort recursion */
	}

	if (!is_dap_cid_ok(cid)) {
		command_print(cmd, "\t\tInvalid CID 0x%08" PRIx32, cid);
		return ERROR_OK; /* Don't abort recursion */
	}

	/* component may take multiple 4K pages */
	uint32_t size = (pid >> 36) & 0xf;
	if (size > 0)
		command_print(cmd, "\t\tStart address 0x%08" PRIx32, (uint32_t)(base_addr - 0x1000 * size));

	command_print(cmd, "\t\tPeripheral ID 0x%010" PRIx64, pid);

	uint8_t class = (cid >> 12) & 0xf;
	uint16_t part_num = pid & 0xfff;
	uint16_t designer_id = ((pid >> 32) & 0xf) << 8 | ((pid >> 12) & 0xff);

	if (designer_id & 0x80) {
		/* JEP106 code */
		command_print(cmd, "\t\tDesigner is 0x%03" PRIx16 ", %s",
			designer_id, jep106_manufacturer(designer_id >> 8, designer_id & 0x7f));
	}
	else {
		/* Legacy ASCII ID, clear invalid bits */
		designer_id &= 0x7f;
		command_print(cmd, "\t\tDesigner ASCII code 0x%02" PRIx16 ", %s",
			designer_id, designer_id == 0x41 ? "ARM" : "<unknown>");
	}

	/* default values to be overwritten upon finding a match */
	const char* type = "Unrecognized";
	const char* full = "";

	/* search dap_partnums[] array for a match */
	for (unsigned entry = 0; entry < ARRAY_SIZE(dap_partnums); entry++) {

		if ((dap_partnums[entry].designer_id != designer_id) && (dap_partnums[entry].designer_id != ANY_ID))
			continue;

		if (dap_partnums[entry].part_num != part_num)
			continue;

		type = dap_partnums[entry].type;
		full = dap_partnums[entry].full;
		break;
	}

	command_print(cmd, "\t\tPart is 0x%" PRIx16", %s %s", part_num, type, full);
	command_print(cmd, "\t\tComponent class is 0x%" PRIx8 ", %s", class, class_description[class]);

	if (class == 1) { /* ROM Table */
		uint32_t memtype;
		retval = mem_ap_read_atomic_u32(ap, base_addr | 0xFCC, &memtype);
		if (retval != ERROR_OK)
			return retval;

		if (memtype & 0x01)
			command_print(cmd, "\t\tMEMTYPE system memory present on bus");
		else
			command_print(cmd, "\t\tMEMTYPE system memory not present: dedicated debug bus");

		/* Read ROM table entries from base address until we get 0x00000000 or reach the reserved area */
		for (uint16_t entry_offset = 0; entry_offset < 0xF00; entry_offset += 4) {
			uint32_t romentry;
			retval = mem_ap_read_atomic_u32(ap, base_addr | entry_offset, &romentry);
			if (retval != ERROR_OK)
				return retval;
			command_print(cmd, "\t%sROMTABLE[0x%x] = 0x%" PRIx32 "",
				tabs, entry_offset, romentry);
			if (romentry & 0x01) {
				/* Recurse */
				retval = vjtag_rom_display(cmd, ap, base_addr + (romentry & 0xFFFFF000), depth + 1);
				if (retval != ERROR_OK)
					return retval;
			}
			else if (romentry != 0) {
				command_print(cmd, "\t\tComponent not present");
			}
			else {
				command_print(cmd, "\t%s\tEnd of ROM table", tabs);
				break;
			}
		}
	}
	else if (class == 9) { /* CoreSight component */
		const char* major = "Reserved", * subtype = "Reserved";

		uint32_t devtype;
		retval = mem_ap_read_atomic_u32(ap, base_addr | 0xFCC, &devtype);
		if (retval != ERROR_OK)
			return retval;
		unsigned minor = (devtype >> 4) & 0x0f;
		switch (devtype & 0x0f) {
		case 0:
			major = "Miscellaneous";
			switch (minor) {
			case 0:
				subtype = "other";
				break;
			case 4:
				subtype = "Validation component";
				break;
			}
			break;
		case 1:
			major = "Trace Sink";
			switch (minor) {
			case 0:
				subtype = "other";
				break;
			case 1:
				subtype = "Port";
				break;
			case 2:
				subtype = "Buffer";
				break;
			case 3:
				subtype = "Router";
				break;
			}
			break;
		case 2:
			major = "Trace Link";
			switch (minor) {
			case 0:
				subtype = "other";
				break;
			case 1:
				subtype = "Funnel, router";
				break;
			case 2:
				subtype = "Filter";
				break;
			case 3:
				subtype = "FIFO, buffer";
				break;
			}
			break;
		case 3:
			major = "Trace Source";
			switch (minor) {
			case 0:
				subtype = "other";
				break;
			case 1:
				subtype = "Processor";
				break;
			case 2:
				subtype = "DSP";
				break;
			case 3:
				subtype = "Engine/Coprocessor";
				break;
			case 4:
				subtype = "Bus";
				break;
			case 6:
				subtype = "Software";
				break;
			}
			break;
		case 4:
			major = "Debug Control";
			switch (minor) {
			case 0:
				subtype = "other";
				break;
			case 1:
				subtype = "Trigger Matrix";
				break;
			case 2:
				subtype = "Debug Auth";
				break;
			case 3:
				subtype = "Power Requestor";
				break;
			}
			break;
		case 5:
			major = "Debug Logic";
			switch (minor) {
			case 0:
				subtype = "other";
				break;
			case 1:
				subtype = "Processor";
				break;
			case 2:
				subtype = "DSP";
				break;
			case 3:
				subtype = "Engine/Coprocessor";
				break;
			case 4:
				subtype = "Bus";
				break;
			case 5:
				subtype = "Memory";
				break;
			}
			break;
		case 6:
			major = "Performance Monitor";
			switch (minor) {
			case 0:
				subtype = "other";
				break;
			case 1:
				subtype = "Processor";
				break;
			case 2:
				subtype = "DSP";
				break;
			case 3:
				subtype = "Engine/Coprocessor";
				break;
			case 4:
				subtype = "Bus";
				break;
			case 5:
				subtype = "Memory";
				break;
			}
			break;
		}
		command_print(cmd, "\t\tType is 0x%02" PRIx8 ", %s, %s",
			(uint8_t)(devtype & 0xff),
			major, subtype);
		/* REVISIT also show 0xfc8 DevId */
	}

	return ERROR_OK;
}

 
int vjtag_info_command(struct command_invocation* cmd,
	struct adiv5_ap* ap)
{
	int retval;
	uint32_t dbgbase, apid;
	uint8_t mem_ap;

	/* Now we read ROM table ID registers, ref. ARM IHI 0029B sec  */
	retval = dap_get_debugbase(ap, &dbgbase, &apid);
	if (retval != ERROR_OK)
		return retval;

	command_print(cmd, "AP ID register 0x%8.8" PRIx32, apid);
	if (apid == 0) {
		command_print(cmd, "No AP found at this ap 0x%x", ap->ap_num);
		return ERROR_FAIL;
	}

	switch (apid & (IDR_JEP106 | IDR_TYPE)) {
	case IDR_JEP106_ARM | AP_TYPE_JTAG_AP:
		command_print(cmd, "\tType is JTAG-AP");
		break;
	case IDR_JEP106_ARM | AP_TYPE_AHB3_AP:
		command_print(cmd, "\tType is MEM-AP AHB3");
		break;
	case IDR_JEP106_ARM | AP_TYPE_AHB5_AP:
		command_print(cmd, "\tType is MEM-AP AHB5");
		break;
	case IDR_JEP106_ARM | AP_TYPE_APB_AP:
		command_print(cmd, "\tType is MEM-AP APB");
		break;
	case IDR_JEP106_ARM | AP_TYPE_AXI_AP:
		command_print(cmd, "\tType is MEM-AP AXI");
		break;
	default:
		command_print(cmd, "\tUnknown AP type");
		break;
	}

	/* NOTE: a MEM-AP may have a single CoreSight component that's
	 * not a ROM table ... or have no such components at all.
	 */
	mem_ap = (apid & IDR_CLASS) == AP_CLASS_MEM_AP;
	if (mem_ap) {
		command_print(cmd, "MEM-AP BASE 0x%8.8" PRIx32, dbgbase);

		if (dbgbase == 0xFFFFFFFF || (dbgbase & 0x3) == 0x2) {
			command_print(cmd, "\tNo ROM table present");
		}
		else {
			if (dbgbase & 0x01)
				command_print(cmd, "\tValid ROM table present");
			else
				command_print(cmd, "\tROM table in legacy format");

			vjtag_rom_display(cmd, ap, dbgbase & 0xFFFFF000, 0);
		}
	}

	return ERROR_OK;
}

/* END from arm_adi_v5.c */
/* =================================================================================*/

int vjtag_cleanup_all(void)
{
	struct arm_vjtag_object *obj, *tmp;
	struct adiv5_dap *dap;

	list_for_each_entry_safe(obj, tmp, &all_dap, lh) {
		dap = &obj->dap;
		if (dap->ops && dap->ops->quit)
			dap->ops->quit(dap);

		free(obj->name);
		free(obj);
	}

	return ERROR_OK;
}

enum vjtag_cfg_param {
	CFG_CHAIN_POSITION,
	CFG_IGNORE_SYSPWRUPACK,
};

static const Jim_Nvp nvp_config_opts[] = {
	{ .name = "-chain-position",   .value = CFG_CHAIN_POSITION },
	{ .name = "-ignore-syspwrupack", .value = CFG_IGNORE_SYSPWRUPACK },
	{ .name = NULL, .value = -1 }
};

static int vjtag_configure(Jim_GetOptInfo *goi, struct arm_vjtag_object *dap)
{
	struct jtag_tap *tap = NULL;
	Jim_Nvp *n;
	int e;

	/* parse config or cget options ... */
	while (goi->argc > 0) {
		Jim_SetEmptyResult(goi->interp);

		e = Jim_GetOpt_Nvp(goi, nvp_config_opts, &n);
		if (e != JIM_OK) {
			Jim_GetOpt_NvpUnknown(goi, nvp_config_opts, 0);
			return e;
		}
		switch (n->value) {
		case CFG_CHAIN_POSITION: {
			Jim_Obj *o_t;
			e = Jim_GetOpt_Obj(goi, &o_t);
			if (e != JIM_OK)
				return e;
			tap = jtag_tap_by_jim_obj(goi->interp, o_t);
			if (tap == NULL) {
				Jim_SetResultString(goi->interp, "-chain-position is invalid", -1);
				return JIM_ERR;
			}
			/* loop for more */
			break;
		}
		case CFG_IGNORE_SYSPWRUPACK:
			dap->dap.ignore_syspwrupack = true;
			break;
		default:
			break;
		}
	}

	if (tap == NULL) {
		Jim_SetResultString(goi->interp, "-chain-position required when creating DAP", -1);
		return JIM_ERR;
	}

	vjtag_instance_init(&dap->dap);
	dap->dap.tap = tap;

	return JIM_OK;
}

static int vjtag_create(Jim_GetOptInfo *goi)
{
	struct command_context *cmd_ctx;
	static struct arm_vjtag_object *dap;
	Jim_Obj *new_cmd;
	Jim_Cmd *cmd;
	const char *cp;
	int e;

	cmd_ctx = current_command_context(goi->interp);
	assert(cmd_ctx != NULL);

	if (goi->argc < 3) {
		Jim_WrongNumArgs(goi->interp, 1, goi->argv, "?name? ..options...");
		return JIM_ERR;
	}
	/* COMMAND */
	Jim_GetOpt_Obj(goi, &new_cmd);
	/* does this command exist? */
	cmd = Jim_GetCommand(goi->interp, new_cmd, JIM_ERRMSG);
	if (cmd) {
		cp = Jim_GetString(new_cmd, NULL);
		Jim_SetResultFormatted(goi->interp, "Command: %s Exists", cp);
		return JIM_ERR;
	}

	/* Create it */
	dap = calloc(1, sizeof(struct arm_vjtag_object));
	if (dap == NULL)
		return JIM_ERR;

	e = vjtag_configure(goi, dap);
	if (e != JIM_OK) {
		free(dap);
		return e;
	}

	cp = Jim_GetString(new_cmd, NULL);
	dap->name = strdup(cp);

	struct command_registration vjtag_commands[] = {
		{
			.name = cp,
			.mode = COMMAND_ANY,
			.help = "vJTAG instance command group",
			.usage = "",
			.chain = dap_instance_commands,
		},
		COMMAND_REGISTRATION_DONE
	};

	/* don't expose the instance commands when using hla */
	if (transport_is_hla())
		vjtag_commands[0].chain = NULL;

	e = register_commands(cmd_ctx, NULL, vjtag_commands);
	if (ERROR_OK != e)
		return JIM_ERR;

	struct command *c = command_find_in_context(cmd_ctx, cp);
	assert(c);
	command_set_handler_data(c, dap);

	list_add_tail(&dap->lh, &all_dap);

	return (ERROR_OK == e) ? JIM_OK : JIM_ERR;
}

static int jim_vjtag_create(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	Jim_GetOptInfo goi;
	Jim_GetOpt_Setup(&goi, interp, argc - 1, argv + 1);
	if (goi.argc < 2) {
		Jim_WrongNumArgs(goi.interp, goi.argc, goi.argv,
			"<name> [<vjtag_options> ...]");
		return JIM_ERR;
	}
	return vjtag_create(&goi);
}

static int jim_vjtag_names(Jim_Interp *interp, int argc, Jim_Obj *const *argv)
{
	struct arm_vjtag_object *obj;

	if (argc != 1) {
		Jim_WrongNumArgs(interp, 1, argv, "Too many parameters");
		return JIM_ERR;
	}
	Jim_SetResult(interp, Jim_NewListObj(interp, NULL, 0));
	list_for_each_entry(obj, &all_dap, lh) {
		Jim_ListAppendElement(interp, Jim_GetResult(interp),
			Jim_NewStringObj(interp, obj->name, -1));
	}
	return JIM_OK;
}

COMMAND_HANDLER(handle_vjtag_init)
{
	return vjtag_init_all();
}

COMMAND_HANDLER(handle_vjtag_info_command)
{
	struct target *target = get_current_target(CMD_CTX);
	struct arm *arm = target_to_arm(target);
	struct adiv5_dap *dap = arm->dap;
	uint32_t apsel;

	if (dap == NULL) {
		LOG_ERROR("DAP instance not available. Probably a HLA target...");
		return ERROR_TARGET_RESOURCE_NOT_AVAILABLE;
	}

	switch (CMD_ARGC) {
		case 0:
			apsel = dap->apsel;
			break;
		case 1:
			COMMAND_PARSE_NUMBER(u32, CMD_ARGV[0], apsel);
			if (apsel > DP_APSEL_MAX)
				return ERROR_COMMAND_SYNTAX_ERROR;
			break;
		default:
			return ERROR_COMMAND_SYNTAX_ERROR;
	}

	return vjtag_info_command(CMD, &dap->ap[apsel]);
}

static const struct command_registration vjtag_subcommand_handlers[] = {
	{
		.name = "create",
		.mode = COMMAND_ANY,
		.jim_handler = jim_vjtag_create,
		.usage = "name '-chain-position' name",
		.help = "Creates a new DAP instance",
	},
	{
		.name = "names",
		.mode = COMMAND_ANY,
		.jim_handler = jim_vjtag_names,
		.usage = "",
		.help = "Lists all registered DAP instances by name",
	},
	{
		.name = "init",
		.mode = COMMAND_ANY,
		.handler = handle_vjtag_init,
		.usage = "",
		.help = "Initialize all registered DAP instances"
	},
	{
		.name = "info",
		.handler = handle_vjtag_info_command,
		.mode = COMMAND_EXEC,
		.help = "display ROM table for MEM-AP of current target "
		"(default currently selected AP)",
		.usage = "[ap_num]",
	},
	COMMAND_REGISTRATION_DONE
};

static const struct command_registration vjtag_commands[] = {
	{
		.name = "vjtag",
		.mode = COMMAND_CONFIG,
		.help = "VJTAG commands",
		.chain = vjtag_subcommand_handlers,
		.usage = "",
	},
	COMMAND_REGISTRATION_DONE
};

int vjtag_register_commands(struct command_context *cmd_ctx)
{
	return register_commands(cmd_ctx, NULL, vjtag_commands);
}
