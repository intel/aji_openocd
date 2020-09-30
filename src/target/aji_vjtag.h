#ifndef OPENOCD_TARGET_AJI_VJTAG_H
#define OPENOCD_TARGET_AJI_VJTAG_H

#include "target/arm_adi_v5.h"
struct arm_vjtag_object;
extern int vjtag_info_command(struct command_invocation* cmd,
	struct adiv5_ap* ap);
extern int vjtag_register_commands(struct command_context* cmd_ctx);
extern int vjtag_cleanup_all(void);

#endif