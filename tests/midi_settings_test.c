// SPDX-License-Identifier: GPL-3.0-only
#include "../src/ui/frontend/ui_internal.h"
UiState ui;
#include "../src/ui/frontend/app_midi.c"
#define CHECK(x) do{if(!(x)){fprintf(stderr,"%d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void) {
    FILE *f=tmpfile();CHECK(f);fputs("2\n\n\n1 7\n",f);rewind(f);midi_settings_read(f);fclose(f);
    CHECK(ui.record_mask==2 && !ui.midi_selected[0] && !ui.midi_selected_name[0] && ui.record_count_enabled && ui.record_count_steps==7);
    ui.record_new_pattern=1;
    snprintf(ui.midi_settings_path,sizeof ui.midi_settings_path,"midi-settings-test.txt");midi_save_settings();
    ui.record_count_steps=16;ui.record_count_enabled=0;ui.record_new_pattern=0;
    f=fopen(ui.midi_settings_path,"r");CHECK(f);midi_settings_read(f);fclose(f);remove(ui.midi_settings_path);
    CHECK(ui.record_count_steps==7 && ui.record_count_enabled && ui.record_new_pattern==1 && !ui.midi_selected[0]);
    f=tmpfile();CHECK(f);fputs("3\ndevice-id\nDevice Name\n",f);rewind(f);ui.record_count_enabled=0;ui.record_count_steps=16;midi_settings_read(f);fclose(f);
    CHECK(ui.record_mask==3 && !strcmp(ui.midi_selected,"device-id") && !strcmp(ui.midi_selected_name,"Device Name") && ui.record_count_steps==16);
    f=tmpfile();CHECK(f);fputs("2\n\n\n1 nan\n",f);rewind(f);midi_settings_read(f);fclose(f);CHECK(ui.record_count_steps==16);
    return 0;
}
