// SPDX-License-Identifier: GPL-3.0-only
#include "ui_internal.h"

int midi_target(void) { return ui.midi_take.active?ui.midi_take.channel:ui.recording_ui.active?ui.midi_record_target:ui.windows.focused==2?ui.piano_channel:ui.windows.focused==4?ui.instrument_channel:ui.channel; }

void midi_panic(void) {
    for(int i=0;i<32;i++)if(ui.midi_keys[i].used) {audio_key_velocity(64+i,ui.midi_keys[i].target,ui.midi_keys[i].pitch,0);if(ui.keyboard_notes[ui.midi_keys[i].target][ui.midi_keys[i].pitch])ui.keyboard_notes[ui.midi_keys[i].target][ui.midi_keys[i].pitch]--;if(ui.midi_take.active && (ui.record_mask&RECORD_NOTES))midi_take_note(&ui.midi_take,&ui.project,ui.midi_keys[i].mchannel,ui.midi_keys[i].pitch,0,midi_input_time());ui.midi_keys[i].used=0;}
    memset(ui.midi_sustain,0,sizeof ui.midi_sustain);
}

void midi_save_settings(void) {
    AtomicFile out;if(!ui.midi_settings_path[0] || !atomic_file_open(&out,ui.midi_settings_path))return;
    fprintf(out.file,"%u\n%s\n%s\n",ui.record_mask,ui.midi_selected,ui.midi_selected_name);atomic_file_commit(&out);
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

void midi_initialize(void) {
    const char *slash=strrchr(ui.browser.config,'/');if(slash)snprintf(ui.midi_settings_path,sizeof ui.midi_settings_path,"%.*s/midi.txt",(int)(slash-ui.browser.config),ui.browser.config);
    FILE *f=ui.midi_settings_path[0]?fopen(ui.midi_settings_path,"r"):NULL;
    if(f){unsigned mask;if(fscanf(f,"%u\n",&mask)==1){ui.record_mask=mask&7;if(fgets(ui.midi_selected,sizeof ui.midi_selected,f))ui.midi_selected[strcspn(ui.midi_selected,"\r\n")]=0;if(fgets(ui.midi_selected_name,sizeof ui.midi_selected_name,f))ui.midi_selected_name[strcspn(ui.midi_selected_name,"\r\n")]=0;}fclose(f);}
    midi_input_wake(glfwPostEmptyEvent);char error[256];midi_input_open("",error);midi_refresh();
}

void midi_release_key(int slot,double time) {
    if(!ui.midi_keys[slot].used)return;
    int target=ui.midi_keys[slot].target,pitch=ui.midi_keys[slot].pitch;
    audio_key_velocity(64+slot,target,pitch,0);if(ui.keyboard_notes[target][pitch])ui.keyboard_notes[target][pitch]--;
    if(ui.midi_take.active && (ui.record_mask&RECORD_NOTES))midi_take_note(&ui.midi_take,&ui.project,ui.midi_keys[slot].mchannel,pitch,0,time);
    ui.midi_keys[slot].used=0;
}

void midi_control(ParameterTarget target,float normalized,double time) {
    float value,lo,hi;if(!parameter_info(&ui.project,target,&value,&lo,&hi))return;
    const ParameterDescriptor *d=parameter_descriptor(target.parameter);
    if(target.parameter==PARAM_MASTER_VOLUME || target.parameter==PARAM_INSERT_VOLUME)normalized=fader_gain(normalized)/hi;
    if(d && d->kind==PARAMETER_LOGARITHMIC && lo>0)normalized=(lo*powf(hi/lo,normalized)-lo)/(hi-lo);
    if(target.parameter>=PARAM_DX7_FIRST && target.parameter<PARAM_DX7_FIRST+126 &&
       (target.parameter-PARAM_DX7_FIRST)%21==18 && ui.project.fm[target.owner].dx7.value[target.parameter-PARAM_DX7_FIRST-1])normalized=roundf(normalized*3)/31;
    if(ui.midi_take.active && (ui.record_mask&RECORD_AUTOMATION))midi_take_control(&ui.midi_take,&ui.project,target,normalized,time);
    parameter_write(&ui.project,target,normalized);
}

void midi_poll(void) {
    double now=midi_input_time();
    if(midi_input_changed() || (ui.midi_selected[0] && now-ui.midi_refresh_time>2)){ui.midi_refresh_time=now;midi_refresh();}
    if(ui.midi_learning && IsKeyPressed(KEY_ESCAPE)){ui.midi_learning=0;snprintf(ui.status,sizeof ui.status,"MIDI Learn cancelled");}
    int input_error=midi_input_failed();
    if(input_error){for(int i=0;i<32;i++)midi_release_key(i,now);midi_input_flush();
        if(input_error==2){midi_input_close();ui.midi_connected=0;char error[256];midi_input_open("",error);}
        snprintf(ui.status,sizeof ui.status,"MIDI input lost events; held notes released");}
    MidiEvent e;
    while(midi_input_poll(&e)) {
        int type=e.status&0xf0,mchannel=e.status&15;
        if(type==0x90 || type==0x80) {
            int slot=-1;for(int i=0;i<32;i++)if(ui.midi_keys[i].used && ui.midi_keys[i].mchannel==mchannel && ui.midi_keys[i].pitch==e.data1){slot=i;break;}
            if(type==0x80 || !e.data2) {if(slot>=0){ui.midi_keys[slot].pressed=0;if(!ui.midi_sustain[mchannel])midi_release_key(slot,e.time);}continue;}
            if(slot>=0)midi_release_key(slot,e.time);
            if(slot<0)for(int i=0;i<32;i++)if(!ui.midi_keys[i].used){slot=i;break;}
            if(slot<0){slot=0;midi_release_key(slot,e.time);}
            int target=midi_target();if(target<0 || target>=ui.project.channel_count)continue;
            ui.midi_keys[slot]=(MidiHeld){1,1,mchannel,e.data1,target,e.data2};
            audio_key_velocity(64+slot,target,e.data1,e.data2);ui.keyboard_notes[target][e.data1]++;
            if(ui.midi_take.active && (ui.record_mask&RECORD_NOTES))midi_take_note(&ui.midi_take,&ui.project,mchannel,e.data1,e.data2,e.time);
        } else if(type==0xb0) {
            if(e.data1>=120){for(int i=0;i<32;i++)if(ui.midi_keys[i].used && ui.midi_keys[i].mchannel==mchannel)midi_release_key(i,e.time);ui.midi_sustain[mchannel]=0;continue;}
            if(e.data1==64){ui.midi_sustain[mchannel]=e.data2>=64;if(!ui.midi_sustain[mchannel])for(int i=0;i<32;i++)if(ui.midi_keys[i].used && ui.midi_keys[i].mchannel==mchannel && !ui.midi_keys[i].pressed)midi_release_key(i,e.time);}
            if(ui.midi_learning){int ok=midi_bind(&ui.project,mchannel,e.data1,ui.midi_learn_target);ui.midi_learning=0;snprintf(ui.status,sizeof ui.status,ok?"MIDI controller %d linked; mapping saved with project":"MIDI link could not be added",e.data1);}
            for(int i=0;i<ui.project.midi_binding_count;i++){MidiBinding b=ui.project.midi_bindings[i];if(b.channel==(unsigned)mchannel && b.controller==e.data1)midi_control(b.target,e.data2/127.f,e.time);}
        } else if(type==0xe0) {
            int target=midi_target();int value=e.data1|(e.data2<<7);
            float pitch=value>=8192?(value-8192)/8191.f:(value-8192)/8192.f;
            midi_control((ParameterTarget){PARAM_CHANNEL_PITCH,target,0},(pitch+1)*.5f,e.time);
        }
    }
    if(ui.midi_take.active) {
        ui.project.bpm=ui.midi_take.bpm;midi_take_update(&ui.midi_take,&ui.project,now);
        if((ui.record_mask&RECORD_AUTOMATION) && ui.control_drag){ParameterTarget target;float value,lo,hi;
            if(parameter_from_pointer(&ui.project,ui.control_drag,&target) && parameter_info(&ui.project,target,&value,&lo,&hi))midi_take_control(&ui.midi_take,&ui.project,target,(value-lo)/(hi-lo),now);}
        if(ui.midi_take.failed || !ui.playing)recording_finish();
    }
}

void typing_piano(int blocked) {
    /* GLFW key tokens describe physical US positions, including Spanish - and +. */
    static const int keys[]={KEY_Z,KEY_S,KEY_X,KEY_D,KEY_C,KEY_V,KEY_G,KEY_B,KEY_H,KEY_N,KEY_J,KEY_M,KEY_COMMA,KEY_L,KEY_PERIOD,KEY_SEMICOLON,KEY_SLASH,
        KEY_Q,KEY_TWO,KEY_W,KEY_THREE,KEY_E,KEY_R,KEY_FIVE,KEY_T,KEY_SIX,KEY_Y,KEY_SEVEN,KEY_U,KEY_I,KEY_NINE,KEY_O,KEY_ZERO,KEY_P,KEY_LEFT_BRACKET,KEY_EQUAL,KEY_RIGHT_BRACKET};
    static unsigned char held[sizeof keys/sizeof *keys];
    int enabled=ui.typing_keys && !blocked && !ui.browser_focus && IsWindowFocused() && !command_down() && !IsKeyDown(KEY_LEFT_ALT) && !IsKeyDown(KEY_RIGHT_ALT);
    for(unsigned i=0;i<sizeof keys/sizeof *keys;i++) {
        int pitch=i<17?48+i:60+i-17;
        if(held[i] && (!enabled || !IsKeyDown(keys[i]))) {
            if(ui.keyboard_notes[held[i]-1][pitch]) ui.keyboard_notes[held[i]-1][pitch]--;
            audio_key(i,ui.channel,pitch,0); held[i]=0;
        }
        if(enabled && !held[i] && IsKeyPressed(keys[i])) {
            audio_key(i,ui.channel,pitch,1); held[i]=ui.channel+1; ui.keyboard_notes[ui.channel][pitch]++;
        }
    }
}
