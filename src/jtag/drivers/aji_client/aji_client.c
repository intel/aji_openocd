/***************************************************************************
 *   Copyright (C) 2007-2008 by Øyvind Harboe                              *
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
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifdef INC_CONFIG_H
#include "config.h"
#endif

#include <time.h>

#include <jtag/jtag.h>
#include <target/embeddedice.h>
#include <jtag/interface.h>
#include <helper/jep106.h>

#include "jtag/commands.h"
#include "jtag/jtagcore_overwrite.h"
#include "log.h"
#include "server/server.h"

#include "aji/aji.h"
#include "aji/c_jtag_client_gnuaji.h"
#include "jtagservice.h"

extern void jtag_tap_add(struct jtag_tap* t);


//=================================
// Helpers
//=================================

/*
 * Access functions to lowlevel driver, agnostic of libftdi/libftdxx
 */
static char* hexdump(const uint8_t* buf, const unsigned int size)
{
	unsigned int i;
	char* str = calloc(size * 2 + 1, 1);

	for (i = 0; i < size; i++)
		sprintf(str + 2 * i, "%02x", buf[i]);
	return str;
}





//========================================
// Init/Free
//========================================

/**
 * Initialize
 *
 * Returns ERROR_OK if JTAG Server found.
 * TODO: Write a TCL Command that allows me to specify the jtagserver config file
 */
static int aji_client_init(void)
{
	char* quartus_jtag_client_config = getenv("QUARTUS_JTAG_CLIENT_CONFIG");
	if (quartus_jtag_client_config != NULL) {
		LOG_INFO("Configuration file, set via QUARTUS_JTAG_CLIENT_CONFIG, is '%s'",
			quartus_jtag_client_config
		);
	}
	else {
		LOG_DEBUG("You can choose to pass a jtag client config file QUARTUS_JTAG_CLIENT_CONFIG");
	}

	AJI_ERROR status = AJI_NO_ERROR;
	status = jtagservice_init(JTAGSERVICE_TIMEOUT_MS);
	if (AJI_NO_ERROR != status) {
		LOG_ERROR("Cannot initialize AJI JTAG services Return status is %d (%s)",
			status, c_aji_error_decode(status)
		);
		return ERROR_JTAG_INIT_FAILED;
	}

	status = jtagservice_scan_for_cables();
	if (AJI_NO_ERROR != status) {
		LOG_ERROR("Cannot select JTAG Cable. Return status is %d (%s)",
			status, c_aji_error_decode(status)
		);
		jtagservice_free(JTAGSERVICE_TIMEOUT_MS);
		return ERROR_JTAG_INIT_FAILED;
	}

	status = jtagservice_scan_for_taps();
	if (AJI_NO_ERROR != status) {
		LOG_ERROR("Cannot select TAP devices. Return status is %d (%s)",
			status, c_aji_error_decode(status)
		);
		jtagservice_free(JTAGSERVICE_TIMEOUT_MS);
		return ERROR_JTAG_INIT_FAILED;
	}

	//This driver automanage the tap_state. Hence, this is just
	// to tell the targets which state we think our TAP interface is at.
	tap_set_state(TAP_RESET);

	//overwrites JTAGCORE
	struct jtagcore_overwrite* record = jtagcore_get_overwrite_record();
	record->jtag_examine_chain = jtagservice_jtag_examine_chain;
	record->jtag_validate_ircapture = jtagservice_jtag_validate_ircapture;

	return ERROR_OK;

}

static int aji_client_quit(void)
{
	struct jtagcore_overwrite* record = jtagcore_get_overwrite_record();
	record->jtag_examine_chain = NULL;
	record->jtag_validate_ircapture = NULL;

	jtagservice_free(JTAGSERVICE_TIMEOUT_MS);

	c_jtag_client_gnuaji_free();

	return ERROR_OK;
}



//-----------------
// jtag operations
//-----------------
/**
 * Go to TLR (Test Logic Reset) State
 *
 * \pre @jtagservice_lock() locked the JTAG scan chain by locking
 *      one of the TAP in the JTAG scan chain.
 */
int aji_client_goto_tlr(void)
{
	/*
	 The code below are working, but disabled because
	 (1) jtagserv.exe automatically managers the internal JTAG State.
	 	 One of the thing it does is to cycle through the TLR state
		 automatically. Making this function redundant
	 (2) In testing, it is found that when another AJI Client is connected
		 to the same JTAG Scan Chain, we will consistently take the
		 "Ignore call to set TAPs to TLR state ..." path meaning
		 we do not move the JTAG state machine to TLR. To have multiple
		 AJI Clients connecting to the same JTAG Scan Chain is the norm
		 for our user.
	 */
	LOG_DEBUG("No need to perform TLR request. The server manages JTAG states");
	tap_set_state(TAP_RESET); //Lie to OpenOCD that TAP_RESET state was reached.
	return ERROR_OK;

	/*
	 The code below are working, See previous comment on why it is redundant
	 */
	/*
	AJI_ERROR status = AJI_NO_ERROR;

	AJI_OPEN_ID open_id =jtagservice_get_in_use_tap_open_id();
	assert(open_id); // i.e., not NULL
	status = c_aji_test_logic_reset(open_id);
	if (AJI_NO_ERROR == status) {
		tap_set_state(TAP_RESET);
	} else if (AJI_CHAIN_IN_USE == status) {
		LOG_WARNING("Ignoring call to set TAPs to TLR state after IR scan."
		" Someone else is using the scan chain."

		);
		tap_set_state(TAP_RESET); //Lie to OpenOCD that TAP_RESET state was reached.
		status = AJI_NO_ERROR;
	} else {
		LOG_ERROR("Unexpected error setting TAPs to TLR state. Return status is %d (%s)",
			status, c_aji_error_decode(status)
		);
	}

	return status == AJI_NO_ERROR ? ERROR_OK : ERROR_FAIL;
	*/
}


/**
 * Build I/O buffers from \c cmd
 *
 * \param cmd The \c scan_command to extract data from
 * \param bit_count On output, the number of data bits required by \c cmd.
 * \param write_buffer On Output, a buffer of at least \c bit_count bits
 *             containing the data to be written to the TAP pointed to by \c cmd.
 *             If NULL, no data write  is needed.
 *             caller to free the memory after use.
 * \param read_buffer On Output, a buffer of at least \c bit_count bits
 *             filled with zero to receive data from the TAP pointed to by \c cmd
 *             If NULL, no data read is needed.
 *             caller to free the memory after use.
 *
 * \return #ERROR_OK success
 * \return #ERROR_FAIL if there is a problem
 */
static int aji_client_build_buffers(
	const struct scan_command *const cmd,
	DWORD *bit_count,
	BYTE **write_buffer,
	BYTE **read_buffer
){
	struct scan_command mock_cmd = {
		.ir_scan = cmd->ir_scan,
		.fields = cmd->tap_fields,
		.num_fields = cmd->num_tap_fields
	};
	*bit_count = jtag_scan_size(&mock_cmd);

	*write_buffer = NULL;
	int req = jtag_build_buffer(&mock_cmd, write_buffer);
	if(req) {
		if(write_buffer == NULL) {
			LOG_ERROR("Insufficient memory for write buffer");
			return ERROR_FAIL;
		}
	} else {
		free(*write_buffer);
		*write_buffer = NULL;
	}

	*read_buffer = NULL;
	req = 0;
	for (int i = 0; i < cmd->num_tap_fields; i++) {
		if (cmd->tap_fields[i].in_value) {
			req += cmd->tap_fields[i].num_bits;
		}
	}
	if(req) {
		*read_buffer = calloc(1, DIV_ROUND_UP(*bit_count, 8));
		if(read_buffer == NULL) {
			LOG_ERROR("Insufficient memory for read buffer");
			return ERROR_FAIL;
		}
	} else {
		*read_buffer = NULL;
	}

	return ERROR_OK;
}

static int aji_client_read_buffer(
	BYTE *read_buffer,
	const struct scan_command *cmd
){
	struct scan_command mock_cmd = {
		.ir_scan = cmd->ir_scan,
		.fields = cmd->tap_fields,
		.num_fields = cmd->num_tap_fields
	};
	return jtag_read_buffer(read_buffer, &mock_cmd);
}

/**
 * Handles IR and DR scans
 *
 * \pre @jtagservice_lock() locked the required TAP
 */
static int aji_client_scan(struct scan_command *const cmd)
{
	DWORD bit_count = 0;
	BYTE *write_buffer = NULL,
		 *read_buffer = NULL;
	char *log_buf = NULL;

	aji_client_build_buffers(cmd, &bit_count, &write_buffer, &read_buffer);

	if(write_buffer) {
		log_buf = hexdump(write_buffer, DIV_ROUND_UP(bit_count, 8));
		LOG_DEBUG_IO("%s(scan=%s%s, type=OUT, bits=%d, buf=[%s], end_state=%d)", __func__,
			cmd->tap_is_sld ? "Virtual " : "",
			cmd->ir_scan ? "IRSCAN" : "DRSCAN",
			bit_count, log_buf, cmd->end_state
		);
		free(log_buf);
	} else {
		LOG_DEBUG_IO("%s(scan=%s%s, type=OUT, no write,  end_state=%d)", __func__,
			cmd->tap_is_sld ? "Virtual " : "",
			cmd->ir_scan ? "IRSCAN" : "DRSCAN",
			cmd->end_state
		);
	}


	AJI_ERROR status = AJI_NO_ERROR;
	AJI_OPEN_ID open_id = jtagservice_get_in_use_tap_open_id();
	if(cmd->ir_scan) {
		//LIMITATION: Max IR length is 32 bit has it has to fit into a DWORD
		//  since c_aji_access_ir, a.k.a. aji_access_ir(DWORD)
		//  Cannot be relaxed even if c_aji_access_ir_a(), a.k.a. 
		//  aji_access_ir(BYTE) version, is used because that function also
		//  translate back to c_aji_access_ir()
		//
		//TODO: use c_aji_access_ir_a(), i.e. aji_acess_ir(BYTE) to avoid
		//  having to translate  write_buffer -> instruction and
		//  capture -> read_buffer explicitly

		if(cmd->tap_is_sld) {
			LOG_ERROR("Not yet handling Virtual JTAG yet in %s", __FUNCTION__);
			return ERROR_FAIL;
		}

		DWORD instruction = 0;
		for (DWORD i = 0; i < (bit_count + 7) / 8; i++) {
			instruction |= write_buffer[i] << (i * 8);
		}

		DWORD capture = 0;
		status = c_aji_access_ir(
			open_id, instruction, read_buffer ? &capture : NULL, 0
		);

		if (read_buffer) {
			for (DWORD i = 0; i < (bit_count + 7) / 8; i++) {
				read_buffer[i] = (BYTE)(capture >> (i * 8));
			}
		}
	} else {
		c_aji_access_dr(
				open_id, bit_count, AJI_DR_UNUSED_X,
				0, write_buffer ? bit_count : 0, write_buffer,
				0, read_buffer ? bit_count : 0, read_buffer
		);
	} //end else-if (cmd->ir_scan)

	if(status != AJI_NO_ERROR) {
		LOG_ERROR("Failure to access %s%s register. Return Status is %d (%s)",
			cmd->tap_is_sld ? "Virtual " : "",
			cmd->ir_scan? "IRSCAN" : "DRSCAN",
			status, c_aji_error_decode(status)
		);
		if (write_buffer) 	{ free(write_buffer); }
		if (read_buffer) 	{ free(read_buffer);  }
		return ERROR_FAIL;
	}


	if(read_buffer) {
		log_buf = hexdump(read_buffer, DIV_ROUND_UP(bit_count, 8));
		LOG_DEBUG_IO("%s(scan=%s%s, type=IN, bits=%d, buf=[%s], end_state=%d)", __func__,
			cmd->tap_is_sld ? "Virtual " : "",
			cmd->ir_scan ? "IRSCAN" : "DRSCAN",
			bit_count, log_buf, cmd->end_state
		);
		free(log_buf);
	} else {
		LOG_DEBUG_IO("%s(scan=%s%s, type=IN, no read,  end_state=%d)", __func__,
			cmd->tap_is_sld ? "Virtual " : "",
			cmd->ir_scan ? "IRSCAN" : "DRSCAN",
			cmd->end_state
		);
	}

	if(read_buffer) {
		aji_client_read_buffer(read_buffer, cmd);
	}


	if (write_buffer) 	{ free(write_buffer); }
	if (read_buffer) 	{ free(read_buffer);  }



	if (TAP_IDLE != cmd->end_state) {
		LOG_WARNING("%s%s not yet handle transition to state other than TAP_IDLE(%d)." \
				    " Requested state is %s(%d)", 
					cmd->tap_is_sld ? "Virtual " : "",
					cmd->ir_scan? "IRSCAN" : "DRSCAN",
					TAP_IDLE, tap_state_name(cmd->end_state), cmd->end_state
		);
		assert(0); //deliberately assert() to be able to see where the error is, if it occurs
	}

	//// Let jtagserv manages the JTAG state instead of setting it manually
	/* status = c_aji_run_test_idle(open_id, 2);
	 if (AJI_NO_ERROR != status) {
		LOG_WARNING("Failed to go to TAP_IDLE after IR register write");
	}
	*/

	tap_set_state(TAP_IDLE); //Faking move to TAP_IDLE

	return ERROR_OK;
}

/**
 * Find the next tap used in a jtag_command sequence
 *
 * \param cmds The list of commands to search
 * \param next_tap On output,the next tap if found. 
 *                 if not, no modification to the input value, 
 */
static void find_next_active_tap(
				const struct jtag_command *cmds, 
					  struct jtag_tap **next_tap ) 
{
	for(const struct jtag_command *candidate = cmds; 
								   candidate != NULL; 
								   candidate = candidate->next) {
		if(candidate->type == JTAG_SCAN) {
			(*next_tap) = candidate->cmd.scan->tap;
			return;
		}
	}
}

int aji_client_execute_queue(void)
{
	struct jtag_command *cmd;
	int ret = ERROR_OK;

LOG_INFO("###########################################>>> execute queue start, queue=%p", jtag_command_queue);
	if(jtag_command_queue == NULL) {
LOG_INFO("###########################################>>> execute queue returned. Nothing to do, queue=%p", jtag_command_queue);
		return ERROR_OK;
	}

	struct jtag_tap *tap = NULL;
	for (cmd = jtag_command_queue; ret == ERROR_OK && cmd != NULL;
	     cmd = cmd->next) {

		find_next_active_tap(cmd, &tap);
LOG_INFO("Active tap = %s (%p)", jtag_tap_name(tap), tap);
struct jtag_tap *pt;
pt = jtag_tap_next_enabled(NULL);
for (; pt != NULL && pt != tap; pt = jtag_tap_next_enabled(pt)) {}
if(pt) { LOG_INFO("Found tap = %s",  jtag_tap_name(tap)); }
else  { LOG_INFO("NOTFOUND tap = %s", jtag_tap_name(tap)); }
		jtagservice_lock(tap);

		switch (cmd->type) {
		case JTAG_RESET:
			LOG_INFO("Ignoring command JTAG_RESET(trst=%d, srst=%d)\n", 
					 cmd->cmd.reset->trst, 
					 cmd->cmd.reset->srst
			);
			//aji_client_reset(cmd->cmd.reset->trst, cmd->cmd.reset->srst);
			break;
		case JTAG_RUNTEST:
			LOG_ERROR("===> Not yet coded JTAG_RUNTEST(num_cycles=%d, end_state=0x%x)\n",
					  cmd->cmd.runtest->num_cycles, 
					  cmd->cmd.runtest->end_state
			);
			//aji_client_runtest(
			//	cmd->cmd.runtest->num_cycles,
			//	cmd->cmd.runtest->end_state
			//);
			assert(0); //deliberately assert() to be able to see where the error is, if it occurs
			break;
		case JTAG_STABLECLOCKS:
			LOG_ERROR("===> Not yet coded JTAG_STABLECLOCKS(num_cycles=%d)\n", 
					  cmd->cmd.stableclocks->num_cycles
			);
			//aji_client_stableclocks(cmd->cmd.stableclocks->num_cycles);
			break;
		case JTAG_TLR_RESET:
			LOG_INFO("===> JTAG_TLR_RESET(end_state=0x%x,skip_initial_bits=%d)\n", 
					 cmd->cmd.statemove->end_state, 
					 0
			);
			aji_client_goto_tlr();
			break;
		case JTAG_PATHMOVE:
			LOG_INFO(
				"===> Ignoring JTAG_PATHMOVE(numstate=%d, first_state=0x%x, end_state=0x%x)\n",
				cmd->cmd.pathmove->num_states, 
				cmd->cmd.pathmove->path[0], 
				cmd->cmd.pathmove->path[cmd->cmd.pathmove->num_states-1]
			);
			//aji_client_path_move(cmd->cmd.pathmove);
			assert(0); //deliberately assert() to be able to see where the error is, if it occurs
			break;
		case JTAG_TMS:
			LOG_ERROR("===> Not yet coded JTAG_TMS(num_bits=%d)\n",
					  cmd->cmd.tms->num_bits
			);
			//aji_client_tms(cmd->cmd.tms);
			break;
		case JTAG_SLEEP:
			LOG_ERROR("===> Not yet coded JTAG_SLEEP(time=%d us)\n", cmd->cmd.sleep->us);
			//aji_client_usleep(cmd->cmd.sleep->us);
			assert(0); //deliberately assert() to be able to see where the error is, if it occurs
			break;
		case JTAG_SCAN:
			LOG_INFO(
				"===> JTAG_SCAN(register=%s, num_fields=%d, end_state=0x%x"
                " tap=%p (%s), "
				" num_tap_fields=%d, tap_fields=%p (>%p) )\n",
				cmd->cmd.scan->ir_scan? "IR" : "DR", 
				cmd->cmd.scan->num_fields, 
				cmd->cmd.scan->end_state,
				cmd->cmd.scan->tap,
				jtag_tap_name(cmd->cmd.scan->tap),
				cmd->cmd.scan->num_tap_fields,
				cmd->cmd.scan->tap_fields,
				cmd->cmd.scan->fields
			);
			ret = aji_client_scan(cmd->cmd.scan);
			break;
		default:
			LOG_ERROR("===> BUG: unknown JTAG command type 0x%X",
				  cmd->type);
			ret = ERROR_FAIL;
			break;
		}
	}
	jtagservice_unlock();
LOG_INFO("###########################################>>> execute queue end");

	return ret;
}


static const struct command_registration aji_client_subcommand_handlers[] = {
	{
		/*
		 * This is a duplication of "hardware" top level command
		 * Prefer the "hardware" top level command over this one,
		 * i.e., "aji_client hardware ..."
		 * The reason this exists is because there are implementation that
		 * relies on "aji_client hardware ..."
		 */
		.name = "hardware",
		.jim_handler = &jim_hardware_newhardware,
		.mode = COMMAND_CONFIG,
		.help = "select the hardware",
		.usage = "<name> <type>[<port>]",
	},
	COMMAND_REGISTRATION_DONE
};

static const struct command_registration aji_client_command_handlers[] = {
	{
		.name = "aji_client",
		.mode = COMMAND_CONFIG,
		.help = "Perform aji_client management",
		.usage = "",
		.chain = aji_client_subcommand_handlers,
	},
	COMMAND_REGISTRATION_DONE
};


static struct jtag_interface aji_client_interface = {
	.execute_queue = aji_client_execute_queue,
};

struct adapter_driver aji_client_adapter_driver = {
	.name = "aji_client",
	.transports = jtag_only,
	.commands = aji_client_command_handlers,

	.init = aji_client_init,
	.quit = aji_client_quit,
	.speed = NULL,
	.khz = NULL,
	.speed_div = NULL,
	.power_dropout = NULL,
	.srst_asserted = NULL,

	.jtag_ops = &aji_client_interface,
};
