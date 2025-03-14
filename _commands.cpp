﻿#include "main.h" // ÙŦF-8

void cmd_register(user_t* user, const char* nick, const char* pass)
{
	if (!nick_is_valid(nick, true))
	{
		user->AddChat(0xFFDB0000, "[Error] {ffffff}Nickname is invalid!");
		return;
	}

	if (strlen(pass) < 5)
	{
		user->AddChat(0xFFDB0000, "[Error] {ffffff}Password is too short!");
		return;
	}

	if (db.find_user(nick) != -1 || db.find_blacklist(nick))
	{
		user->AddChat(0xFFDB0000, "[Error] {ffffff}Nickname already in use!");
		return;
	}

	int id = db.add_user(nick, md5(pass));
	if (id != -1)
	{
		userdata_t ud;
		if (db.load_user(id, &ud))
			user->login(&ud);
	}
}

void cmd_auth(user_t* user, const char* nick, const char* pass)
{
	if (!nick_is_valid(nick, false) || strlen(pass) < 5)
	{
		user->AddChat(0xFFDB0000, "[Error] {ffffff}Something was entered incorrectly!");
		return;
	}

	int id = db.find_user(nick);
	if (id == -1)
	{
		user->AddChat(0xFFDB0000, "[Error] {ffffff}Account does not exist!");
		return;
	}

	userdata_t ud;
	if (!db.auth_user(id, md5(pass), &ud))
	{
		user->AddChat(0xFFDB0000, "[Error] {ffffff}Incorrect password!");
		return;
	}

	user->login(&ud);
}

void cmd_setpass(user_t* user, const char* pass)
{
	if (strlen(pass) < 5)
	{
		user->AddChat(0xFFDB0000, "[Error] {FFFFFF}Password is too short!");
		return;
	}

	db.set_password(user->m_id, md5(pass));
	user->AddChat(0xFF9CFF9F, "Password changed to {FFFFFF}\"%s\"{9cff9f}. Use {FFFFFF}F8 {9cff9f}to save.", pass);
}

void cmd_help(user_t* user)
{
	user->AddChat(0xFFF7F488, "Упс! Кто-то забил хер на содержимое этой команды."); //todo: print all commands with their usage
}

void cmd_ping(user_t* user)
{
	user->AddChat(0xFFF7F488, "Pong!");
}

void cmd_time(user_t* user)
{
	struct tm tm_;
	fill_timeinfo(&tm_);
	user->AddChat(0xFFF7F488, "Time: {ffffff}%02d:%02d:%02d", tm_.tm_hour, tm_.tm_min, tm_.tm_sec);
}

void cmd_me(user_t* user, const char* text)
{
	chat.pushf(1, user->m_id, user->m_color, "%s {ff99ff}%s", user->nick(), format_out(text).c_str());
}

void cmd_do(user_t* user, const char* text)
{
	chat.pushf(1, user->m_id, 0xFF4682B4, "%s - | %s", format_out(text).c_str(), user->nick_c());
}

void cmd_todo(user_t* user, const char* text)
{
	char part[2][256];
	if (sscanf(text, "%[^*]*%[^\n]", part[0], part[1]) != 2)
	{
		user->AddChat(0xFFDB0000, "Use: {ffffff}/todo Phrase*Action");
		return;
	}

	chat.pushf(1, user->m_id, 0xFFFFFFFF, "'%s', - said(а) %s{ffffff}, {ff99ff}%s", part[0], user->nick(), part[1]);
}

void cmd_exit(user_t* user)
{
	user->logout();
}

void cmd_clear(user_t* user)
{
	packet_t packet(id_chat_erase);
	packet.write_string("");
	server.Broadcast(&packet);

	chat.clear();
	chat.pushf(0, 0, 0xFFF02E2E, "Chat has been cleared by administrator.");
}

void cmd_update(user_t* user)
{
	user->send_erase("");
	chat.send_history(user);
}

void cmd_erase(user_t* user, const char* text)
{
	int c = chat.clear(text);

	packet_t packet(id_chat_erase);
	packet.write_string(text);
	server.Broadcast(&packet);

	user->AddChat(0xFF00FF00, "[Info] {ffffff}Deleted %d messages.", c);
	_printf("[admin] %s: /erase \"%s\"", user->nick(), text);
}

void cmd_clearuser(user_t* user, int id)
{
	int c = chat.clear_id(id);

	packet_t packet(id_chat_erase_id);
	packet.write<id_t>(id);
	server.Broadcast(&packet);

	user->AddChat(0xFF00FF00, "[Info] {ffffff}Deleted %d messages.", c);
	_printf("[admin] %s: /clear_user %d -> count: %d", user->nick(), id, c);
}

void cmd_msg(user_t* user, int id, const char* text)
{
	if (id == user->m_id)
	{
		user->AddChat(0xFFDB0000, "[Error] {ffffff}Can't pm yourself lol!");
		return;
	}

	user_t* usr = server.find_user(id);
	if (usr == nullptr)
	{
		user->AddChat(0xFFDB0000, "[Error] {ffffff}User is offline!");
		return;
	}

	server.SendPM(user, usr, format_out(text));
}

void cmd_msg_re(user_t* user, const char* text)
{
	if (user->m_pm_id == -1)
	{
		user->AddChat(0xFFDB0000, "[Error] {ffffff}No one to reply");
		return;
	}

	user_t* usr = server.find_user(user->m_pm_id);
	if (usr == nullptr)
	{
		user->AddChat(0xFFDB0000, "[Error] {ffffff}User is offline!");
		return;
	}

	server.SendPM(user, usr, format_out(text));
}

user_t* find_target(user_t* user, int id)
{
	user_t* target = server.find_user(id);
	if (target == nullptr)
	{
		user->AddChat(0xFFDB0000, "[Error] {ffffff}User not found!");
		return nullptr;
	}

	if (target->m_id != user->m_id && target->m_status >= user->m_status)
	{
		user->AddChat(0xFFDB0000, "[Error] {ffffff}No access!");
		return nullptr;
	}

	return target;
}

void cmd_kick(user_t* user, int id, const char* reason)
{
	user_t* usr = find_target(user, id);
	if (usr == nullptr)
		return;

	chat.pushf(1, user->m_id, 0xFFFF6347, "[A] %s{ff6347}: /kick %s %s", user->nick_c(), usr->nick(), reason);
	server.KickUser(usr);
}

void cmd_skick(user_t* user, int id)
{
	user_t* usr = find_target(user, id);
	if (usr == nullptr)
		return;

	server.KickUser(usr);

	chat.pushf(3, user->m_id, 0xFFADADAD, "[A] %s{adadad}: /skick %s", user->nick_c(), usr->nick());
}

void cmd_ban(user_t* user, int id, const char* reason)
{
	user_t* usr = find_target(user, id);
	if (usr == nullptr)
		return;

	chat.pushf(1, user->m_id, 0xFFFF6347, "[A] %s{ff6347}: /ban %s %s", user->nick_c(), usr->nick(), reason);
	server.MakeBan(usr);
}

void cmd_banip(user_t* user, int id, const char* reason)
{
	user_t* usr = find_target(user, id);
	if (usr == nullptr)
		return;

	chat.pushf(1, user->m_id, 0xFFFF6347, "[A] %s{ff6347}: /banip %s %s", user->nick_c(), usr->nick(), reason);
	server.MakeBanIP(usr);
}

void cmd_destroy(user_t* user, int id)
{
	user_t* usr = find_target(user, id);
	if (usr == nullptr)
		return;

	server.MakeBan(usr);
	cmd_clearuser(user, id);

	chat.pushf(3, user->m_id, 0xFFADADAD, "[A] %s{adadad}: /destroy %s", user->nick_c(), usr->nick());
}

void cmd_setcolor(user_t* user, int id, color_t color)
{
	user_t* usr = find_target(user, id);
	if (usr == nullptr)
		return;

	usr->set_color(color);
	user->AddChat(0xFFDB0000, ">> Color set for %s", usr->nick_c());

	_printf("[admin] %s: /setcolor %d %x", user->nick(), id, color);
}

void cmd_setnick(user_t* user, int id, const char* nick)
{
	user_t* usr = find_target(user, id);
	if (usr == nullptr)
		return;

	if (db.find_user(nick) != -1)
	{
		user->AddChat(0xFFDB0000, "[Error] {ffffff}Nickname already in use!");
		return;
	}
//	if (!nick_is_valid(nick, false))
//	{
//		user->AddChat(0xFFDB0000, "[Error] {ffffff}Nickname is invalid!");
//		return;
//	}

	usr->set_nick(nick);
	user->AddChat(0xFFDB0000, ">> Nickname set for %s", usr->nick_c());

	_printf("[admin] %s: /setnick %d \"%s\"", user->nick(), id, nick);
}

void cmd_setprefix(user_t* user, int id, const char* prefix)
{
	user_t* usr = find_target(user, id);
	if (usr == nullptr)
		return;

	if (!strcmp(prefix, "-"))
	{
		usr->set_prefix("");
		user->AddChat(0xFFDB0000, ">> Prefix reset for %s", usr->nick_c());
	}
	else
	{
		usr->set_prefix(prefix);
		user->AddChat(0xFFDB0000, ">> Prefix set for %s", usr->nick_c());
	}

	_printf("[admin] %s: /setprefix %d \"%s\"", user->nick(), id, prefix);
}

void cmd_setstatus(user_t* user, int id, int status)
{
	user_t* usr = find_target(user, id);
	if (usr == nullptr)
		return;

	if (status < 1 || status >= user->m_status || user->m_id == id)
	{
		user->AddChat(0xFFDB0000, "[Error] {ffffff}Unable to set status!");
		return;
	}

	usr->set_status(status);
	user->AddChat(0xFFDB0000, ">> Status set {ffffff}%d {db0000}for %s", status, usr->nick_c());

	_printf("[admin] %s: /setstatus %d %d", user->nick(), id, status);
}

void cmd_rainbow(user_t* user, int id)
{
	user_t* usr = find_target(user, id);
	if (usr == nullptr)
		return;

	usr->m_rainbow ^= 1;
}

void cmd_hideme(user_t* user)
{
	user->m_hideme ^= 1;
	user->AddChat(0xFFDB0000, ">> Invisible %s.", user->m_hideme ? "Included" : "off");
	user->udn();
	server.on_count();
}

void cmd_chat_vip(user_t* user, const char* text)
{
	chat.pushf(2, user->m_id, 0xFFFCD71C, "[VIP] %s{ffffff}: %s", user->nick_c(), format_out(text, false).c_str());
}

void cmd_chat_admin(user_t* user, const char* text)
{
	chat.pushf(3, user->m_id, 0xFFA6E300, "[A] %s{a6e300}: %s", user->nick_c(), format_out(text, false).c_str());
}

void cmd_chat_all(user_t* user, const char* text)
{
	chat.pushf(0, user->m_id, 0xFFFFE500, "[G] %s{ffe500}: %s", user->nick_c(), format_out(text, false).c_str());
}

void cmd_notify(user_t* user)
{
	user->send_notify();
}

void cmd_nick(user_t* user, const char* nick)
{
	if (!nick_is_valid(nick, true))
	{
		user->AddChat(0xFFDB0000, "[Error] {ffffff}Nickname is invalid!");
		return;
	}

	if (db.find_user(nick) != -1)
	{
		user->AddChat(0xFFDB0000, "[Error] {ffffff}Nickname already in use!");
		return;
	}

	_printf("[nick] %s -> %s", user->nick(), nick);

	user->set_nick(nick);
	user->AddChat(0xFF00FF00, "[Info] {ffffff}Nickname changed: %s", nick);
}

void cmd_online(user_t* _user)
{
	_user->AddChat(0xFFFFFFFF, "%s", server.get_header().c_str());

	server.online_foreach([](user_t* user, const std::string& str, user_t* dst)
	{
		if (user->m_hideme && dst->m_status < 4)
			return;
		dst->AddChat(0xFFFFFFFF, "%s", str.c_str());
	}, _user, true);
}

void init_commands()
{
	cmds.add({ "reg", "register" },		new cmd_t{ 0, (void*)cmd_register,			"ss",	"<nick> <pass>"	});
	cmds.add({ "auth", "login" },		new cmd_t{ 0, (void*)cmd_auth,				"ss",	"<nick> <pass>"	});
	cmds.add({ "setpass", "passwd" },	new cmd_t{ 1, (void*)cmd_setpass,			"s",	"<pass>"		});
	cmds.add({ "ping" },				new cmd_t{ 1, (void*)cmd_ping,				"",		""				});
	cmds.add({ "time" },				new cmd_t{ 1, (void*)cmd_time,				"",		""				});
	cmds.add({ "help" },				new cmd_t{ 1, (void*)cmd_help,				"",		""				});
	cmds.add({ "msg", "pm", "sms" },	new cmd_t{ 1, (void*)cmd_msg,				"d*",	"<id> <text>"	});
	cmds.add({ "re", "r" },				new cmd_t{ 1, (void*)cmd_msg_re,			"*",	"<text>"		});
	cmds.add({ "me" },					new cmd_t{ 1, (void*)cmd_me,				"*",	"<text>"		});
	cmds.add({ "do" },					new cmd_t{ 1, (void*)cmd_do,				"*",	"<text>"		});
	cmds.add({ "todo" },				new cmd_t{ 1, (void*)cmd_todo,				"*",	"<text>*<text>"	});
	cmds.add({ "exit", "quit" },		new cmd_t{ 2, (void*)cmd_exit,				"",		""				});
	cmds.add({ "update" },				new cmd_t{ 2, (void*)cmd_update,			"",		""				});
	cmds.add({ "vr", "vip" },			new cmd_t{ 2, (void*)cmd_chat_vip,			"*",	"<text>"		});
	cmds.add({ "a" },					new cmd_t{ 3, (void*)cmd_chat_admin,		"*",	"<text>"		});
	cmds.add({ "g", "ao" },				new cmd_t{ 3, (void*)cmd_chat_all,			"*",	"<text>"		});
	cmds.add({ "erase" },				new cmd_t{ 3, (void*)cmd_erase,				"*",	"<text>"		});
	cmds.add({ "kick" },				new cmd_t{ 3, (void*)cmd_kick,				"d*",	"<id> <reason>" });
	cmds.add({ "skick" },				new cmd_t{ 4, (void*)cmd_skick,				"d",	"<id>"			});
	cmds.add({ "ban" },					new cmd_t{ 3, (void*)cmd_ban,				"d*",	"<id> <reason>" });
	cmds.add({ "banip" },				new cmd_t{ 4, (void*)cmd_banip,				"d*",	"<id> <reason>"	});
	cmds.add({ "cc", "clear_chat" },	new cmd_t{ 4, (void*)cmd_clear,				"",		""				});
	cmds.add({ "cu", "clear_user" },	new cmd_t{ 3, (void*)cmd_clearuser,			"d",	"<id>"			});
	cmds.add({ "destroy" },				new cmd_t{ 4, (void*)cmd_destroy,			"d",	"<id>"			});
	cmds.add({ "setcolor" },			new cmd_t{ 4, (void*)cmd_setcolor,			"dx",	"<id> <color>"	});
	cmds.add({ "setprefix" },			new cmd_t{ 4, (void*)cmd_setprefix,			"d*",	"<id> <prefix>"	});
	cmds.add({ "setnick" },				new cmd_t{ 5, (void*)cmd_setnick,			"ds",	"<id> <nick>"	});
	cmds.add({ "setstatus" },			new cmd_t{ 3, (void*)cmd_setstatus,			"dd",	"<id> <status>"	});
	cmds.add({ "rainbow", "makegay" },	new cmd_t{ 5, (void*)cmd_rainbow,			"d",	"<id>"			});
	cmds.add({ "hideme" },				new cmd_t{ 4, (void*)cmd_hideme,			"",		""				});
	cmds.add({ "notify" },				new cmd_t{ 1, (void*)cmd_notify,			"",		""				});
	cmds.add({ "nick" },				new cmd_t{ 1, (void*)cmd_nick,				"*",	"<nick>"		});
	cmds.add({ "online" },				new cmd_t{ 1, (void*)cmd_online,			"",		""				});

	_printf("[info] init_commands(): %d commands loaded.", cmds.get_count());
}
