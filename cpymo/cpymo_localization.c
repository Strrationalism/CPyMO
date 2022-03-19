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
	{ "很慢", "慢", "中", "快", "很快", "瞬间" },

	&F(chs, save_already_save_to),
	&F(chs, save_failed),
	&F(chs, save_are_you_sure_save_to),
	&F(chs, save_title),
	&F(chs, save_auto_title),
	&F(chs, save_are_you_sure_load),
	"确定要加载自动存档吗？",

	&F(chs, date_str),

	"未找到游戏",
	"你需要将游戏放在SD卡中的\"pymogames\"文件夹下，\n并确保路径中只包含英文、数字和下划线。\n"
};


STR_I(cht, save_already_save_to, 32, "已經保存到檔案 %d");
STR_E(cht, save_failed, 32, "保存失敗：%s");
STR_I(cht, save_are_you_sure_save_to, 64, "確定要保存到檔案 %d 嗎？");
STR_I_S(cht, save_title, 32, "檔案 %d      %s");
STR_S(cht, save_auto_title, 32, "自動存檔  %s");
STR_I(cht, save_are_you_sure_load, 64, "確定要加載檔案 %d 嗎？");
STR_I_I(cht, date_str, 32, "%d 月 %d 日");

const cpymo_localization cht = {
	"確定",
	"取消",

	"保存進度",
	"讀取進度",
	"快進",
	"隱藏對話",
	"對話歷史",
	"設定",
	"重啓遊戲",
	"返回遊戲",

	"確定要重新啟動遊戲嗎？",

	"背景音樂音量",
	"音效音量",
	"語音音量",
	"文字速度",
	"文字大小",
	{ "很慢", "慢", "中", "快", "很快", "瞬間" },

	&F(cht, save_already_save_to),
	&F(cht, save_failed),
	&F(cht, save_are_you_sure_save_to),
	&F(cht, save_title),
	&F(cht, save_auto_title),
	&F(cht, save_are_you_sure_load),
	"確定要加載自動存檔嗎？",

	&F(cht, date_str),

	"未找到遊戲",
	"你需要將遊戲放在SD卡中的\"pymogames\"資料夾下，\n並確保路徑中只包含英文、數位和底線。 \n"
};

STR_I(enu, save_already_save_to, 32, "Already save to slot %d.");
STR_E(enu, save_failed, 32, "Save failed: %s");
STR_I(enu, save_are_you_sure_save_to, 64, "Are you sure you want\nto save to slot %d?")
STR_I_S(enu, save_title, 16, "Slot %d   %s");
STR_S(enu, save_auto_title, 16, "Auto Slot  %s");
STR_I(enu, save_are_you_sure_load, 64, "Are you sure you want to\nload from slot %d?");
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

	"Are you sure you want to\nrestart the game?",

	"BGM Volume",
	"SE Volume",
	"Voice Volume",
	"Text Speed",
	"Font Size",
	{ "Very Slow", "Slow", "Normal", "Fast", "Very Fast", "Instant" },

	&F(enu, save_already_save_to),
	&F(enu, save_failed),
	&F(enu, save_are_you_sure_save_to),
	&F(enu, save_title),
	&F(enu, save_auto_title),
	&F(enu, save_are_you_sure_load),
	"Are you sure you want to\nload from auto slot?",

	&F(enu, date_str),

	"No games found",
	"Please make sure folder \"pymogames\" is in SD card root,\nand that you have at least one game in it."
};

#ifndef DEFAULT_LANG
#define DEFAULT_LANG enu
#endif

const cpymo_localization *cpymo_localization_get(struct cpymo_engine *e)
{
	return &DEFAULT_LANG;
}
