/* The auxiliary GPU follows lcdSwap; choose its physical backlight each time. */
static void nds_aux_refresh(void){
 unsigned lights=PM_BACKLIGHT_TOP|PM_BACKLIGHT_BOTTOM;
 if(nds_aux_hidden())lights&=~(screens_swapped?PM_BACKLIGHT_TOP:PM_BACKLIGHT_BOTTOM);
 /* libnds 2.x legacy powerOn/off ignore ARM7 PMIC bits. Use our dedicated
    user PXI service, never a system-reserved channel or direct ARM9 POWCNT. */
 pxiWaitRemote(PxiChannel_User0);
 ui_preferences_state[4]=pxiSendAndReceive(PxiChannel_User0,lights);
 if(nds_playing&&!error_ui_active)launcher_play_panel(NULL);
 nds_preferences_sync();
}
static void nds_aux_poll(void){
 int held=keysHeld();if(!(held&KEY_TOUCH))nds_touch_blocked=false;
 if(keysDown()&KEY_Y){nds_aux_override=!nds_aux_override;ui_preferences_state[8]++;nds_aux_refresh();cpymo_engine_request_redraw(&engine);}
 if((keysDown()&KEY_TOUCH)&&!screens_swapped&&nds_aux_hidden()){
  nds_aux_override=!nds_aux_override;nds_touch_blocked=true;ui_preferences_state[7]++;nds_aux_refresh();cpymo_engine_request_redraw(&engine);
 }
}
