/*
 * Copyright (c) 2005 William Pitcock, et al.
 * The rights to this code are as documented under doc/LICENSE.
 *
 * Meow!
 */

#include "atheme-compat.h"

service_t *catserv;

static void catserv_cmd_meow(sourceinfo_t *si, int parc, char *parv[]);
static void catserv_cmd_help(sourceinfo_t *si, int parc, char *parv[]);

command_t catserv_meow = { "MEOW", "Makes the cute little kitty-cat meow!",
				AC_NONE, 0, catserv_cmd_meow, { .path = "" } };
command_t catserv_help = { "HELP", "Displays contextual help information.",
				AC_NONE, 1, catserv_cmd_help, { .path = "help" } };

static void
mod_init(module_t *const restrict m)
{
	catserv = service_add("catserv", NULL);

	service_bind_command(catserv, &catserv_meow);
	service_bind_command(catserv, &catserv_help);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	service_unbind_command(catserv, &catserv_meow);
	service_unbind_command(catserv, &catserv_help);

	service_delete(catserv);
}

static void catserv_cmd_meow(sourceinfo_t *si, int parc, char *parv[])
{
	command_success_nodata(si, "Meow!");
}

static void catserv_cmd_help(sourceinfo_t *si, int parc, char *parv[])
{
	command_help(si, si->service->commands);
}

SIMPLE_DECLARE_MODULE_V1("contrib/ircd_catserv", MODULE_UNLOAD_CAPABILITY_OK)
