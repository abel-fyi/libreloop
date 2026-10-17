// SPDX-License-Identifier: GPL-3.0-only
#include "file_io.h"
#include "ui_internal.h"

static MidiSession midi_session;
static void session_velocity(void *context,int slot,int c,int pitch,int velocity) {
    (void)context;audio_key_velocity(slot,c,pitch,velocity);
}
static void session_key(void *context,int slot,int c,int pitch,int down) {
    (void)context;audio_key(slot,c,pitch,down);
}
static void session_read(void) {
    midi_session.project=&ui.project;midi_session.take=&ui.midi_take;
    midi_session.key_velocity=session_velocity;midi_session.key=session_key;
    memcpy(midi_session.midi_keys,ui.midi_keys,sizeof ui.midi_keys);
    memcpy(midi_session.midi_sustain,ui.midi_sustain,sizeof ui.midi_sustain);
    memcpy(midi_session.keyboard_notes,ui.keyboard_notes,sizeof ui.keyboard_notes);
    memcpy(midi_session.status,ui.status,sizeof ui.status);
    midi_session.record_mask=ui.record_mask;midi_session.recording_audio=ui.recording_ui.active;
    midi_session.midi_record_target=ui.midi_record_target;midi_session.focused_editor=ui.windows.focused;
    midi_session.piano_channel=ui.piano_channel;midi_session.instrument_channel=ui.instrument_channel;
    midi_session.channel=ui.channel;midi_session.midi_learning=ui.midi_learning;
    midi_session.midi_learn_target=ui.midi_learn_target;midi_session.typing_keys=ui.typing_keys;
    midi_session.browser_focus=ui.browser_focus;
}
static void session_write(void) {
    memcpy(ui.midi_keys,midi_session.midi_keys,sizeof ui.midi_keys);
    memcpy(ui.midi_sustain,midi_session.midi_sustain,sizeof ui.midi_sustain);
    memcpy(ui.keyboard_notes,midi_session.keyboard_notes,sizeof ui.keyboard_notes);
    memcpy(ui.status,midi_session.status,sizeof ui.status);
    ui.midi_learning=midi_session.midi_learning;
}


int midi_target(void) {session_read();return midi_session_target(&midi_session);}
void midi_panic(void) {
    session_read();
    for(int i=0;i<32;i++)if(midi_session.midi_keys[i].used)
        midi_session_release(&midi_session,i,midi_input_time());
    memset(midi_session.midi_sustain,0,sizeof midi_session.midi_sustain);
    session_write();
}

void midi_save_settings(void) {
    AtomicFile out;if(!ui.midi_settings_path[0] || !atomic_file_open(&out,ui.midi_settings_path))return;
    fprintf(out.file,"%u\n%s\n%s\n",ui.record_mask,ui.midi_selected,ui.midi_selected_name);
    fprintf(out.file,"%d %.0f\n",ui.record_count_enabled,ui.record_count_steps);atomic_file_commit(&out);
}

void midi_refresh(void) {
    ui.midi_device_count=midi_input_devices(ui.midi_devices,64);int found=-1;
    for(int i=0;i<ui.midi_device_count;i++)if(!strcmp(ui.midi_devices[i].id,ui.midi_selected)) {
#ifndef __APPLE__
        if(ui.midi_selected_name[0] && strcmp(ui.midi_devices[i].name,ui.midi_selected_name))continue;
#endif
        found=i;break;
    }
#ifndef __APPLE__
    if(found<0 && ui.midi_selected_name[0]) {
        int matches=0;for(int i=0;i<ui.midi_device_count;i++)if(!strcmp(ui.midi_devices[i].name,ui.midi_selected_name)){found=i;matches++;}
        if(matches!=1)found=-1;
    }
#endif
    if(ui.midi_connected && (found<0 || strcmp(ui.midi_selected,ui.midi_devices[found].id))) {
        midi_panic();midi_input_close();ui.midi_connected=0;char error[256];midi_input_open("",error);
        snprintf(ui.status,sizeof ui.status,"MIDI input disconnected; held notes released");
    }
    if(found>=0 && !ui.midi_connected) {
        snprintf(ui.midi_selected,sizeof ui.midi_selected,"%s",ui.midi_devices[found].id);
        char error[256];ui.midi_connected=midi_input_open(ui.midi_selected,error);
        if(!ui.midi_connected)snprintf(ui.status,sizeof ui.status,"%s",error);
    }
}

void midi_select(int index) {
    if(recording_active()) return;
    midi_panic(); midi_input_close(); ui.midi_connected=0;
    snprintf(ui.midi_selected,sizeof ui.midi_selected,"%s",index<0?"":ui.midi_devices[index].id);
    snprintf(ui.midi_selected_name,sizeof ui.midi_selected_name,"%s",index<0?"":ui.midi_devices[index].name);
    if(index>=0){char error[256];ui.midi_connected=midi_input_open(ui.midi_selected,error);if(ui.midi_connected)ui.record_mask|=RECORD_NOTES;else snprintf(ui.status,sizeof ui.status,"%s",error);}
    if(index<0){char error[256];midi_input_open("",error);}
    midi_save_settings();
}

static void midi_settings_read(FILE *f) {
    char line[64];unsigned mask;
    if(!fgets(line,sizeof line,f) || sscanf(line,"%u\n",&mask)!=1)return;
    ui.record_mask=mask&7;
    if(fgets(ui.midi_selected,sizeof ui.midi_selected,f))ui.midi_selected[strcspn(ui.midi_selected,"\r\n")]=0;
    if(fgets(ui.midi_selected_name,sizeof ui.midi_selected_name,f))ui.midi_selected_name[strcspn(ui.midi_selected_name,"\r\n")]=0;
    int enabled;float steps;
    if(fgets(line,sizeof line,f) && sscanf(line,"%d%f",&enabled,&steps)==2 && isfinite(steps) && steps>=1 && steps<=4096) {
        ui.record_count_enabled=!!enabled;ui.record_count_steps=roundf(steps);
    }
}

void midi_initialize(void) {
    const char *slash=strrchr(ui.browser.config,'/');if(slash)snprintf(ui.midi_settings_path,sizeof ui.midi_settings_path,"%.*s/midi.txt",(int)(slash-ui.browser.config),ui.browser.config);
    FILE *f=ui.midi_settings_path[0]?platform_file_open(ui.midi_settings_path,"r"):NULL;
    if(f){midi_settings_read(f);fclose(f);}
    midi_input_wake(host_wake);char error[256];midi_input_open("",error);midi_refresh();
}

void midi_release_key(int slot,double time) {session_read();midi_session_release(&midi_session,slot,time);session_write();}
void midi_control(ParameterTarget target,float normalized,double time) {session_read();midi_session_control(&midi_session,target,normalized,time);session_write();}

void midi_poll(void) {
    double now=midi_input_time();
    if(midi_input_changed() || (ui.midi_selected[0] && now-ui.midi_refresh_time>2)){ui.midi_refresh_time=now;midi_refresh();}
    if(ui.midi_learning && host_is_key_pressed(HOST_KEY_ESCAPE)){ui.midi_learning=0;snprintf(ui.status,sizeof ui.status,"MIDI Learn cancelled");}
    int input_error=midi_input_failed();
    if(input_error){for(int i=0;i<32;i++)midi_release_key(i,now);midi_input_flush();
        if(input_error==2){midi_input_close();ui.midi_connected=0;char error[256];midi_input_open("",error);}
        snprintf(ui.status,sizeof ui.status,"MIDI input lost events; held notes released");}
    session_read();
    MidiEvent e;while(midi_input_poll(&e))midi_session_event(&midi_session,e);
    int finish=midi_session_update(&midi_session,now,ui.playing,ui.control_drag);
    session_write();if(finish)recording_finish();

}

void typing_piano(int blocked) {
    session_read();midi_session_typing_at(&midi_session,host_input(),blocked,command_down(),midi_input_time());session_write();
}
void midi_typing_record_begin(double time){session_read();midi_session_typing_begin(&midi_session,time);session_write();}
void midi_typing_record_end(double time){session_read();midi_session_typing_end(&midi_session,time);session_write();}
