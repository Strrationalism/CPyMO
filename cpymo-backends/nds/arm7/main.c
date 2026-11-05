/* CPyMO NDS ARM7 services; AGPL-3.0-or-later.
   Uses Calico 1.2.0 public APIs and preserves the standard service priorities. */
#include <calico.h>
#include <calico/nds/arm7/pmic.h>
#include <calico/nds/arm7/rtc.h>
#include <calico/nds/arm7/sound.h>
#include <calico/nds/arm7/mic.h>
#include <calico/nds/wlmgr.h>
#include <maxmod7.h>

static Mailbox backlight_mailbox;
static u32 backlight_slots[4];
static Thread backlight_thread;
static u64 backlight_stack[128];
static int backlight_worker(void* arg){
 (void)arg;
 for(;;){
  /* PXI mailbox receives the already decoded immediate value. */
  unsigned lights=mailboxRecv(&backlight_mailbox)&12;
  /* Calico PMIC functions require the caller to hold the shared SPI lock.
     Keep read/modify/write together and preserve audio, LED and power bits. */
  spiLock();u8 control=pmicReadRegister(PmicReg_Control);
  bool ok=pmicWriteRegister(PmicReg_Control,(control&~12)|lights);
  u8 actual=pmicReadRegister(PmicReg_Control);spiUnlock();
  pxiReply(PxiChannel_User0,ok?(actual&12):0x100);
 }
 return 0;
}
int main(void){
 envReadNvramSettings();keypadStartExtServer();
 lcdSetIrqMask(DISPSTAT_IE_VBLANK|DISPSTAT_IE_HBLANK|DISPSTAT_IE_VCOUNT,DISPSTAT_IE_VBLANK);
 irqEnable(IRQ_VBLANK);rtcInit();rtcSyncTime();pmInit();blkInit();
 touchInit();touchStartServer(80,28);soundStartServer(12);micStartServer(4);wlmgrStartServer(20);mmInstall(29);
 mailboxPrepare(&backlight_mailbox,backlight_slots,4);
 threadPrepare(&backlight_thread,backlight_worker,0,backlight_stack+128,26);threadStart(&backlight_thread);
 pxiSetMailbox(PxiChannel_User0,&backlight_mailbox);
 while(pmMainLoop())threadWaitForVBlank();
 return 0;
}
