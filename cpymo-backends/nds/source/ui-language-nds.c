/* NDS interface language and ROM-resident strings; AGPL-3.0-or-later. */
#include <nds.h>
#include <calico/nds/env.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "cpymo_localization.h"
#include "ui-language-nds.h"

/* magic, firmware language, valid settings, ordered preference ID, effective
   language (0=CHS, 1=EN, 2=CHT, 3=JA), loaded,
   writes, write errors, effective-language changes, refresh pending */
volatile unsigned language_state[12]={0x4e4c4731};
static int preference, firmware_language, effective_language, refresh_pending;
static const char* const language_endonyms[NDS_LANG_COUNT]={NULL,"简体中文","繁體中文","English","日本語"};
static void resolve_language(void){
 int next=preference==0?firmware_language:preference==3?1:preference==2?2:preference==4?3:0;
 if(next!=effective_language){effective_language=next;refresh_pending=1;language_state[8]++;}
 language_state[3]=preference;language_state[4]=effective_language;language_state[9]=refresh_pending;
}
void nds_language_boot(void){
 /* libnds/Calico expose the bootloader's RAM copy; no FAT or NVRAM write.
    Invalid/lost settings and all unsupported languages fall back to Chinese. */
 unsigned language=g_envUserSettings->config.language;
 int valid=g_envUserSettings->version==5&&!PersonalData->settingsLost;
 language_state[1]=language;language_state[2]=valid;
 language_state[10]=g_envUserSettings->version;language_state[11]=PersonalData->settingsLost;
 preference=NDS_LANG_AUTO;firmware_language=valid?(language==EnvLanguage_English?1:language==EnvLanguage_Japanese?3:0):0;
 resolve_language();refresh_pending=0;language_state[9]=0;
}
void nds_language_load(void){
 /* First normal launch is Chinese even on English firmware. IDs follow the
    current five-item menu order; no old-ID remapping. Pre-FAT uses firmware. */
 preference=NDS_LANG_CHINESE;
 FILE* f=fopen("/pymo/nds-language.cfg","rb");
 if(f){char line[32];int value,n=0;
  if(fgets(line,sizeof line,f)&&sscanf(line,"NDSLANG1 %d %n",&value,&n)==1&&n>0&&line[n]==0&&fgetc(f)==EOF&&value>=0&&value<NDS_LANG_COUNT){preference=value;language_state[5]=1;}
  fclose(f);
 }
 resolve_language();refresh_pending=0;language_state[9]=0;
}
int nds_language_get(void){return preference;}
int nds_language_english(void){return effective_language==1;}
int nds_language_effective(void){return effective_language;}
int nds_language_set(int value){
 if(value<NDS_LANG_AUTO||value>=NDS_LANG_COUNT)return 0;
 preference=value;resolve_language();mkdir("/pymo",0777);
 FILE* f=fopen("/pymo/nds-language.cfg","wb");int ok=0;
 if(f){ok=fprintf(f,"NDSLANG1 %d\n",value)>0;if(fclose(f))ok=0;}
 language_state[6]++;if(!ok)language_state[7]++;
 return ok;
}
int nds_language_take_refresh(void){int value=refresh_pending;refresh_pending=0;language_state[9]=0;return value;}
/* Only engine-owned interface literals call this function. Game titles,
   introductions, scripts and resource names are deliberately not passed in. */
static const struct {const char* zh;const char* en;} strings[]={
 {"正在读取游戏列表...","Reading game list..."},
 {"游戏资料库","Game library"},{"选择游戏","Select a game"},{"关于与许可","About and licenses"},
 {"A / DOWN / 轻触：继续","A / DOWN / Touch: Next"},{"X：菜单 · B：返回","X: Menu / B: Back"},
 {"B：按住隐藏文字","Hold B: Hide text"},{"R：按设置快进","R: Skip (per settings)"},
 {"L：切换显示模式","L: Cycle display mode"},{"Y：隐藏／唤回辅助屏","Y: Toggle info screen"},
 {"START：快速保存到存档1","START: Save to slot 1"},{"SELECT：快速读取存档1","SELECT: Load slot 1"},
 {"显示、字号、音量：X 菜单","Display, font, audio: X"},
 {"已快速保存到存档1","Saved to slot 1"},{"已快速读取存档1","Loaded slot 1"},
 {"快速保存失败，请检查TF卡","Save failed; check TF card"},{"快速读取失败，请检查存档1","Load failed; check slot 1"},
 {"设置保存失败","Could not save settings"},
 {"快速保存到存档1失败","Could not save to slot 1"},{"读取存档1失败或存档不存在","Slot 1 missing or unreadable"},
 {"NDS 平台移植：千岚之夏","NDS port: 千岚之夏"},
 {"引擎与后端：AGPL v3","Engine/backend: AGPL v3"},{"转换工具：MIT","Converter: MIT"},
 {"许可详情请见附带文档","See included license notices"},
 {"PyMO：chen_xin_ming","PyMO: chen_xin_ming"},
 {"已转换，可以启动","Converted: ready to start"},{"缓存无效，请重新转换","Invalid cache: reconvert"},
 {"游戏配置无效","Invalid game configuration"},{"需要先使用转换脚本","Convert this game first"},
 {"没有找到游戏","No games found"},{"请先转换游戏，再放入","Convert games, then place in"},
 {"CPyMO：Seng Jik 及贡献者","CPyMO: Seng Jik and contributors"},
 {"游戏与汉化署名照原样保留","Game/translation credits retained"},
 {"新游戏请先使用转换助手","Use the converter for new games"},
 {"按 B 返回游戏列表","B: Back to game list"},
 {"A 启动 · 上下选择 · SELECT 刷新","A: Start / Up,Down / SELECT: Scan"},
 {"轻触两次启动 · B 关于与许可","Tap twice: Start / B: About"},
 {"请在电脑上先转换或修复缓存","Convert or repair on your PC"},
 {"无法访问存储卡","Storage unavailable"},
 {"启动失败，请检查转换结果","Game could not start"},
 {"运行失败，请检查游戏资源","Game could not continue"},
 {"错误码：%d","Error code: %d"},{"处理建议","What to try"},
 {"关机后重新插好TF卡。\n检查读卡与R4内核文件，再重试。\n此错误码不代表TF卡已损坏。","Power off and reinsert the TF card.\nCheck the card reader and R4 files.\nThis code does not prove card damage."},
 {"请检查转换结果和游戏资源。\n原始错误码已保留，便于反馈。","Check the conversion and game files.\nInclude the original code when reporting."},
 {"请关机后重新启动","Power off, then restart"},{"按 B 返回游戏资料库","B: Return to game library"},
 {"界面语言","UI language"},{"自动","Auto"},{"已读变灰","Dim read text"},
 {"立绘特效(全局)","Sprite effects (global)"},{"开","On"},{"关","Off"},{"大","Large"},{"小","Small"},
 {"X：界面语言","X: UI language"},{"上下选择 · A 确定 · B 返回","Up/Down: Select / A: Set / B: Back"},
 {"选择游戏 · X 语言","Select game / X: Language"},
 {"轻触两次启动 · B 关于 · X 语言","Tap twice / B: About / X: Lang"},
 {"语言设置已保存","Language preference saved"},
 {"SELECT: 语言 / B: 返回","SELECT: Language / B: Back"},
 /* Runtime profile: native mode detection, not a console-model guess. */
 {"DSi 模式 · 16 MiB","DSi mode · 16 MiB"},
 {"DS 模式 · 4 MiB","DS mode · 4 MiB"},
 /* Classic-lower: the stylus now targets game coordinates, not virtual A. */
 {"A：继续 · 触屏操控","A: Next / Touch: Game"},
 /* 1.2 clean dual-screen guide: controller lettering and explicit mode names. */
 {"A / DOWN：继续","A / DOWN: Continue"},
 {"UP：历史对话","UP: History"},
 {"R：快进 · L：切换模式","R: Skip / L: Cycle modes"},
 {"上屏游戏 / 下屏游戏 / 增强双屏","Game top / Game bottom / Dual screen"},
 {"START：快速保存  SELECT：快速读取","START: Save / SELECT: Load"},
 {"SELECT：快速读取","SELECT: Quick load"},
 {"退出到游戏库","Exit to library"},
 {"确定退出到游戏库？\n未保存的进度将丢失。","Exit to library?\nUnsaved progress will be lost."},
 {"主题设置","Theme settings"},
 {"默认","Default"},
 {"主题已保存","Theme saved"},
 {"游戏列表已刷新","Game list refreshed"},
 {"A / START 启动 · 轻触两次启动","A/START: Start · Tap twice"},
 {"上下选择 · B 关于 · X 语言 · Y 主题","UP/DOWN · B Info · X Lang · Y Color"},
 {"UP / DOWN：选择项目","UP / DOWN: Choose"},
 {"A：确认","A: Confirm"},
 {"B：返回","B: Back"},
 {"标题菜单","Title menu"},
 {"当前显示：上屏游戏","Display: Upper game"},
 {"当前显示：下屏游戏","Display: Lower game"},
 {"当前显示：增强双屏","Display: Dual screen"},
 {"方向键选择 · A 确定 · B 返回","D-pad: Pick · A: Set · B: Back"},
 {"R：快进 · L：切换显示模式","R: Skip · L: Cycle display"},
 {"L：切换显示模式","L: Cycle display mode"},
 {"NDS 移植：千岚之夏（Blueteemo）","NDS port: 千岚之夏 (Blueteemo)"}
};
#include "ui-language-extra.h"
int nds_language_choice_get(void){return preference;}
int nds_language_choice_set(int i){return i>=0&&i<NDS_LANG_COUNT?nds_language_set(i):0;}
const char* nds_language_choice_name(int i){if(i<0||i>=NDS_LANG_COUNT)return "?";return i?language_endonyms[i]:nds_ui("自动");}
const char* nds_ui(const char* zh){
 if(!effective_language||!zh)return zh;
 for(unsigned i=0;i<sizeof strings/sizeof *strings;i++)if(!strcmp(zh,strings[i].zh))return effective_language==1?strings[i].en:extra_ui[effective_language-2][i];
 return zh;
}
extern const cpymo_localization chs,cht,enu;
const cpymo_localization* __wrap_cpymo_localization_get(struct cpymo_engine* e){
 (void)e;return effective_language==1?&enu:effective_language==2?&cht:effective_language==3?&jpn:&chs;
}
