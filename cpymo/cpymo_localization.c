#include "cpymo_localization.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define F(LOC, FIELD) \
	cpymo_localization_##LOC##_##FIELD

#define ALLOC_STR(BUFFER) \
	*out_str = (char *)malloc(BUFFER); \
	if (*out_str == NULL) return CPYMO_ERR_OUT_OF_MEM;

#define STR_I(LOC, FIELD, BUFFER, FMT) \
	static error_t F(LOC, FIELD)(char **out_str, int id) \
	{ \
		ALLOC_STR(BUFFER); \
		sprintf(*out_str, FMT, id); \
		return CPYMO_ERR_SUCC; \
	} \

#define STR_I_I(LOC, FIELD, BUFFER, FMT) \
	static error_t F(LOC, FIELD)(char **out_str, int i, int ii) \
	{ \
		ALLOC_STR(BUFFER); \
		sprintf(*out_str, FMT, i, ii); \
		return CPYMO_ERR_SUCC; \
	} \

#define STR_E(LOC, FIELD, BUFFER, FMT) \
	static error_t F(LOC, FIELD)(char **out_str, error_t e) \
	{ \
		const char *str = cpymo_error_message(e); \
		ALLOC_STR(BUFFER + strlen(str)); \
		sprintf(*out_str, FMT, str); \
		return CPYMO_ERR_SUCC; \
	}

#define STR_I_S(LOC, FIELD, BUFFER, FMT) \
	static error_t F(LOC, FIELD)(char **out_str, int i, const char *s) \
	{ \
		ALLOC_STR(BUFFER + strlen(s)); \
		sprintf(*out_str, FMT, i, s); \
		return CPYMO_ERR_SUCC; \
	}

#define STR_S(LOC, FIELD, BUFFER, FMT) \
	static error_t F(LOC, FIELD)(char **out_str, const char *s) \
	{ \
		ALLOC_STR(BUFFER + strlen(s)); \
		sprintf(*out_str, FMT, s); \
		return CPYMO_ERR_SUCC; \
	}

STR_I(chs, save_already_save_to, 32, "已经保存到存档 %d");
STR_E(chs, save_failed, 32, "保存失败：%s");
STR_I(chs, save_are_you_sure_save_to, 64, "确定要保存到存档 %d 吗？");
STR_I_S(chs, save_title, 32, "存档 %d      %s");
STR_S(chs, save_auto_title, 32, "自动存档  %s");
STR_I(chs, save_are_you_sure_load, 64, "确定要加载存档 %d 吗？");
STR_I_I(chs, date_str, 32, "%d 月 %d 日");

const cpymo_localization chs = {
	"确定",
	"取消",

	"存档",
	"读档",
	"快进",
	"隐藏对话框",
	"对话历史",
	"设置",
	"重启游戏",
	"返回游戏",

	"确定要重新启动游戏吗？",

	"背景音乐音量",
	"音效音量",
	"语音音量",
	"文字速度",
	"文字大小",

	&F(chs, save_already_save_to),
	&F(chs, save_failed),
	&F(chs, save_are_you_sure_save_to),
	&F(chs, save_title),
	&F(chs, save_auto_title),
	&F(chs, save_are_you_sure_load),
	"确定要加载自动存档吗？",

	&F(chs, date_str)
};

STR_I(enu, save_already_save_to, 32, "Already save to slot %d.");
STR_E(enu, save_failed, 32, "Save failed: %s");
STR_I(enu, save_are_you_sure_save_to, 64, "Are you sure you want to save to slot %d ?")
STR_I_S(enu, save_title, 16, "Slot %d    %s");
STR_S(enu, save_auto_title, 16, "Auto Slot  %s");
STR_I(enu, save_are_you_sure_load, 64, "Are you sure you want to load from slot %d ?");
STR_I_I(enu, date_str, 16, "%d / %d");

const cpymo_localization enu = {
	"OK",
	"Cancel",

	"Save",
	"Load",
	"Skip",
	"Hide Window",
	"History",
	"Settings",
	"Restart Game",
	"Back to Game",

	"Are you sure you want to restart the game?",

	"BGM Volume",
	"SE Volume",
	"Voice Volume",
	"Text Speed",
	"Font Size",

	&F(enu, save_already_save_to),
	&F(enu, save_failed),
	&F(enu, save_are_you_sure_save_to),
	&F(enu, save_title),
	&F(enu, save_auto_title),
	&F(enu, save_are_you_sure_load),
	"Are you sure you want to load from auto slot?",

	&F(enu, date_str)
};

#ifndef DEFAULT_LANG
#define DEFAULT_LANG chs
#endif

const cpymo_localization *cpymo_localization_get(struct cpymo_engine *e)
{
	return &DEFAULT_LANG;
}
