/*
 * Copyright (c) 2005 William Pitcock, et al.
 * Rights to this code are as documented in doc/LICENSE.
 *
 * This file contains code for the CService FREGISTER function.
 */

#include "atheme-compat.h"
#include "../chanserv/chanserv.h"

static void
cs_cmd_fregister(sourceinfo_t *si, int parc, char *parv[])
{
	channel_t *c;
	chanuser_t *cu;
	mychan_t *mc;
	char *name = parv[0];
	char str[21];
	hook_channel_register_check_t hdatac;
	hook_channel_req_t hdata;
	unsigned int fl;
	bool joined_channel = false;

	/* This command is not useful on registered channels, ignore it if
	 * it is a fantasy command so users can program bots to react on
	 * it without interference from ChanServ.
	 */
	if (si->c != NULL)
		return;

	if (!name)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "FREGISTER");
		command_fail(si, fault_needmoreparams, _("To forcibly register a channel: FREGISTER <#channel>"));
		return;
	}

	if (*name != '#')
	{
		command_fail(si, fault_badparams, STR_INVALID_PARAMS, "FREGISTER");
		command_fail(si, fault_badparams, _("Syntax: FREGISTER <#channel>"));
		return;
	}

	/* make sure they're logged in */
	if (!si->smu)
	{
		command_fail(si, fault_noprivs, STR_NOT_LOGGED_IN);
		return;
	}

	if (si->smu->flags & MU_WAITAUTH)
	{
		command_fail(si, fault_notverified, STR_EMAIL_NOT_VERIFIED);
		return;
	}

	/* make sure it isn't already registered */
	if ((mc = mychan_find(name)))
	{
		if (!use_channel_private || !(mc->flags & MC_PRIVATE))
			command_fail(si, fault_alreadyexists, _("\2%s\2 is already registered to \2%s\2."), mc->name, mychan_founder_names(mc));
		else
			command_fail(si, fault_alreadyexists, _("\2%s\2 is already registered."), mc->name);
		return;
	}

	/* make sure the channel exists, otherwise create it by joining */
	if (!(c = channel_find(name)))
	{
		join(name, chansvs.nick);
		if (!(c = channel_find(name)))
		{
			command_fail(si, fault_internalerror, _("Unable to create channel \2%s\2."), name);
  			return;
		}
  		joined_channel = true;
	}

	hdatac.si = si;
	hdatac.name = name;
	hdatac.chan = c;
	hdatac.approved = 0;
	hook_call_channel_can_register(&hdatac);
	if (hdatac.approved != 0)
		return;

	logcommand(si, CMDLOG_REGISTER | CMDLOG_ADMIN, "FREGISTER: \2%s\2", name);

	mc = mychan_add(name);
	mc->registered = CURRTIME;
	mc->used = CURRTIME;
	mc->mlock_on |= (CMODE_NOEXT | CMODE_TOPIC);
	if (c->limit == 0)
		mc->mlock_off |= CMODE_LIMIT;
	if (c->key == NULL)
		mc->mlock_off |= CMODE_KEY;
	mc->flags |= config_options.defcflags;

	chanacs_add(mc, entity(si->smu), custom_founder_check(), CURRTIME, entity(si->smu));

	if (c->ts > 0)
	{
		snprintf(str, sizeof str, "%lu", (unsigned long)c->ts);
		metadata_add(mc, "private:channelts", str);
	}

	if (chansvs.deftemplates != NULL && *chansvs.deftemplates != '\0')
		metadata_add(mc, "private:templates",
				chansvs.deftemplates);

	command_success_nodata(si, _("\2%s\2 is now registered to \2%s\2."), mc->name, entity(si->smu)->name);
	
	if (joined_channel && config_options.leave_chans)
	{
		part(name, chansvs.nick);
	}

	hdata.si = si;
	hdata.mc = mc;
	hook_call_channel_register(&hdata);
	/* Allow the hook to override this. */
	fl = chanacs_source_flags(mc, si);
	cu = chanuser_find(mc->chan, si->su);
	if (cu == NULL)
		;
	else if (ircd->uses_owner && fl & CA_USEOWNER && fl & CA_AUTOOP &&
			!(cu->modes & CSTATUS_OWNER))
	{
		modestack_mode_param(si->service->nick, mc->chan, MTYPE_ADD,
				ircd->owner_mchar[1], CLIENT_NAME(si->su));
		cu->modes |= CSTATUS_OWNER;
	}
	else if (ircd->uses_protect && fl & CA_USEPROTECT && fl & CA_AUTOOP &&
			!(cu->modes & CSTATUS_PROTECT))
	{
		modestack_mode_param(si->service->nick, mc->chan, MTYPE_ADD,
				ircd->protect_mchar[1], CLIENT_NAME(si->su));
		cu->modes |= CSTATUS_PROTECT;
	}
}

static command_t cs_fregister = {
	.name           = "FREGISTER",
	.desc           = N_("Forcibly registers a channel."),
	.access         = PRIV_CHAN_ADMIN,
	.maxparc        = 3,
	.cmd            = &cs_cmd_fregister,
	.help           = { .path = "contrib/cs_fregister" },
};

static void
mod_init(module_t *const restrict m)
{
        service_named_bind_command("chanserv", &cs_fregister);
}

static void
mod_deinit(const module_unload_intent_t intent)
{
	service_named_unbind_command("chanserv", &cs_fregister);
}

SIMPLE_DECLARE_MODULE_V1("contrib/cs_fregister", MODULE_UNLOAD_CAPABILITY_OK)
