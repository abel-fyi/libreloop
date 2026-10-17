# Exact interaction constants and branches

Companion to behavior-spec.md. Lines are copied verbatim from the frozen GPL-3.0-only source as documentary excerpts; they are not compiled rewrite code. Each excerpt retains original line numbers. This includes declarations, numeric expressions, input gates, hit-tests, timing and gesture assignments. Full frozen functions remain the authority where the excerpt omits intervening logic. No constants may be normalized or re-derived.

## app_midi.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### midi_target

```c
   4: int midi_target(void) { return ui.midi_take.active?ui.midi_take.channel:ui.recording_ui.active?ui.midi_record_target:ui.windows.focused==2?ui.piano_channel:ui.windows.focused==4?ui.instrument_channel:ui.channel; }
```

### midi_panic

```c
   7:     for(int i=0;i<32;i++)if(ui.midi_keys[i].used) {audio_key_velocity(64+i,ui.midi_keys[i].target,ui.midi_keys[i].pitch,0);if(ui.keyboard_notes[ui.midi_keys[i].target][ui.midi_keys[i].pitch])ui.keyboard_notes[ui.midi_keys[i].target][ui.midi_keys[i].pitch]--;if(ui.midi_take.active && (ui.record_mask&RECORD_NOTES))midi_take_note(&ui.midi_take,&ui.project,ui.midi_keys[i].mchannel,ui.midi_keys[i].pitch,0,midi_input_time());ui.midi_keys[i].used=0;}
   8:     memset(ui.midi_sustain,0,sizeof ui.midi_sustain);
```

### midi_save_settings

```c
  12:     AtomicFile out;if(!ui.midi_settings_path[0] || !atomic_file_open(&out,ui.midi_settings_path))return;
```

### midi_refresh

```c
  17:     ui.midi_device_count=midi_input_devices(ui.midi_devices,64);int found=-1;
  18:     for(int i=0;i<ui.midi_device_count;i++)if(!strcmp(ui.midi_devices[i].id,ui.midi_selected)) {
  20:         if(ui.midi_selected_name[0] && strcmp(ui.midi_devices[i].name,ui.midi_selected_name))continue;
  25:     if(found<0 && ui.midi_selected_name[0]) {
  26:         int matches=0;for(int i=0;i<ui.midi_device_count;i++)if(!strcmp(ui.midi_devices[i].name,ui.midi_selected_name)){found=i;matches++;}
  27:         if(matches!=1)found=-1;
  30:     if(ui.midi_connected && (found<0 || strcmp(ui.midi_selected,ui.midi_devices[found].id))) {
  31:         midi_panic();midi_input_close();ui.midi_connected=0;char error[256];midi_input_open("",error);
  34:     if(found>=0 && !ui.midi_connected) {
  36:         char error[256];ui.midi_connected=midi_input_open(ui.midi_selected,error);
```

### midi_select

```c
  43:     midi_panic(); midi_input_close(); ui.midi_connected=0;
  44:     snprintf(ui.midi_selected,sizeof ui.midi_selected,"%s",index<0?"":ui.midi_devices[index].id);
  45:     snprintf(ui.midi_selected_name,sizeof ui.midi_selected_name,"%s",index<0?"":ui.midi_devices[index].name);
  46:     if(index>=0){char error[256];ui.midi_connected=midi_input_open(ui.midi_selected,error);if(ui.midi_connected)ui.record_mask|=RECORD_NOTES;else snprintf(ui.status,sizeof ui.status,"%s",error);}
  47:     if(index<0){char error[256];midi_input_open("",error);}
```

### midi_initialize

```c
  53:     FILE *f=ui.midi_settings_path[0]?fopen(ui.midi_settings_path,"r"):NULL;
  54:     if(f){unsigned mask;if(fscanf(f,"%u\n",&mask)==1){ui.record_mask=mask&7;if(fgets(ui.midi_selected,sizeof ui.midi_selected,f))ui.midi_selected[strcspn(ui.midi_selected,"\r\n")]=0;if(fgets(ui.midi_selected_name,sizeof ui.midi_selected_name,f))ui.midi_selected_name[strcspn(ui.midi_selected_name,"\r\n")]=0;}fclose(f);}
  55:     midi_input_wake(glfwPostEmptyEvent);char error[256];midi_input_open("",error);midi_refresh();
```

### midi_release_key

```c
  61:     audio_key_velocity(64+slot,target,pitch,0);if(ui.keyboard_notes[target][pitch])ui.keyboard_notes[target][pitch]--;
  62:     if(ui.midi_take.active && (ui.record_mask&RECORD_NOTES))midi_take_note(&ui.midi_take,&ui.project,ui.midi_keys[slot].mchannel,pitch,0,time);
  63:     ui.midi_keys[slot].used=0;
```

### midi_control

```c
  70:     if(d && d->kind==PARAMETER_LOGARITHMIC && lo>0)normalized=(lo*powf(hi/lo,normalized)-lo)/(hi-lo);
  71:     if(target.parameter>=PARAM_DX7_FIRST && target.parameter<PARAM_DX7_FIRST+126 &&
  72:        (target.parameter-PARAM_DX7_FIRST)%21==18 && ui.project.fm[target.owner].dx7.value[target.parameter-PARAM_DX7_FIRST-1])normalized=roundf(normalized*3)/31;
```

### midi_poll

```c
  79:     if(midi_input_changed() || (ui.midi_selected[0] && now-ui.midi_refresh_time>2)){ui.midi_refresh_time=now;midi_refresh();}
  80:     if(ui.midi_learning && IsKeyPressed(KEY_ESCAPE)){ui.midi_learning=0;snprintf(ui.status,sizeof ui.status,"MIDI Learn cancelled");}
  82:     if(input_error){for(int i=0;i<32;i++)midi_release_key(i,now);midi_input_flush();
  83:         if(input_error==2){midi_input_close();ui.midi_connected=0;char error[256];midi_input_open("",error);}
  87:         int type=e.status&0xf0,mchannel=e.status&15;
  88:         if(type==0x90 || type==0x80) {
  89:             int slot=-1;for(int i=0;i<32;i++)if(ui.midi_keys[i].used && ui.midi_keys[i].mchannel==mchannel && ui.midi_keys[i].pitch==e.data1){slot=i;break;}
  90:             if(type==0x80 || !e.data2) {if(slot>=0){ui.midi_keys[slot].pressed=0;if(!ui.midi_sustain[mchannel])midi_release_key(slot,e.time);}continue;}
  91:             if(slot>=0)midi_release_key(slot,e.time);
  92:             if(slot<0)for(int i=0;i<32;i++)if(!ui.midi_keys[i].used){slot=i;break;}
  93:             if(slot<0){slot=0;midi_release_key(slot,e.time);}
  94:             int target=midi_target();if(target<0 || target>=ui.project.channel_count)continue;
  95:             ui.midi_keys[slot]=(MidiHeld){1,1,mchannel,e.data1,target,e.data2};
  96:             audio_key_velocity(64+slot,target,e.data1,e.data2);ui.keyboard_notes[target][e.data1]++;
  97:             if(ui.midi_take.active && (ui.record_mask&RECORD_NOTES))midi_take_note(&ui.midi_take,&ui.project,mchannel,e.data1,e.data2,e.time);
  98:         } else if(type==0xb0) {
  99:             if(e.data1>=120){for(int i=0;i<32;i++)if(ui.midi_keys[i].used && ui.midi_keys[i].mchannel==mchannel)midi_release_key(i,e.time);ui.midi_sustain[mchannel]=0;continue;}
 100:             if(e.data1==64){ui.midi_sustain[mchannel]=e.data2>=64;if(!ui.midi_sustain[mchannel])for(int i=0;i<32;i++)if(ui.midi_keys[i].used && ui.midi_keys[i].mchannel==mchannel && !ui.midi_keys[i].pressed)midi_release_key(i,e.time);}
 101:             if(ui.midi_learning){int ok=midi_bind(&ui.project,mchannel,e.data1,ui.midi_learn_target);ui.midi_learning=0;snprintf(ui.status,sizeof ui.status,ok?"MIDI controller %d linked; mapping saved with project":"MIDI link could not be added",e.data1);}
 102:             for(int i=0;i<ui.project.midi_binding_count;i++){MidiBinding b=ui.project.midi_bindings[i];if(b.channel==(unsigned)mchannel && b.controller==e.data1)midi_control(b.target,e.data2/127.f,e.time);}
 103:         } else if(type==0xe0) {
 104:             int target=midi_target();int value=e.data1|(e.data2<<7);
 105:             float pitch=value>=8192?(value-8192)/8191.f:(value-8192)/8192.f;
 106:             midi_control((ParameterTarget){PARAM_CHANNEL_PITCH,target,0},(pitch+1)*.5f,e.time);
 111:         if((ui.record_mask&RECORD_AUTOMATION) && ui.control_drag){ParameterTarget target;float value,lo,hi;
 112:             if(parameter_from_pointer(&ui.project,ui.control_drag,&target) && parameter_info(&ui.project,target,&value,&lo,&hi))midi_take_control(&ui.midi_take,&ui.project,target,(value-lo)/(hi-lo),now);}
```

### typing_piano

```c
 122:     int enabled=ui.typing_keys && !blocked && !ui.browser_focus && IsWindowFocused() && !command_down() && !IsKeyDown(KEY_LEFT_ALT) && !IsKeyDown(KEY_RIGHT_ALT);
 123:     for(unsigned i=0;i<sizeof keys/sizeof *keys;i++) {
 124:         int pitch=i<17?48+i:60+i-17;
 125:         if(held[i] && (!enabled || !IsKeyDown(keys[i]))) {
 126:             if(ui.keyboard_notes[held[i]-1][pitch]) ui.keyboard_notes[held[i]-1][pitch]--;
 127:             audio_key(i,ui.channel,pitch,0); held[i]=0;
 129:         if(enabled && !held[i] && IsKeyPressed(keys[i])) {
 130:             audio_key(i,ui.channel,pitch,1); held[i]=ui.channel+1; ui.keyboard_notes[ui.channel][pitch]++;
```

## app_project.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### channel_missing

```c
   9:     return c>=0 && c<ui.project.channel_count && !ui.originals[c].frames && ui.project.paths[c][0] &&
```

### sample_duration

```c
  18:     if(original.frames || !p->paths[c][0] || !strcmp(p->paths[c],SAMPLE_EMPTY)) p->audio_seconds[c]=processed.frames/(float)RATE;
```

### sample_copy

```c
  22:     Sample copy={0}; sample_clone(source,&copy); return copy;
```

### sampler_worker

```c
  28:     atomic_store(&ui.sampler_job.done,1); glfwPostEmptyEvent(); return NULL;
```

### sampler_update

```c
  36:                 Sample old=ui.samples[c]; audio_sample(c,ui.sampler_job.result); ui.samples[c]=ui.sampler_job.result; ui.sampler_job.result=(Sample){0}; sample_free(old);
  41:         sample_free(ui.sampler_job.input); sample_free(ui.sampler_job.result); ui.sampler_job.busy=0;
```

### sampler_flush

```c
  47:         pthread_join(ui.sampler_job.thread,NULL); sample_free(ui.sampler_job.input); sample_free(ui.sampler_job.result); ui.sampler_job.busy=0;
  49:     for(int c=0;c<ui.project.channel_count;c++) if(ui.sampler_generation[c]!=ui.sample_generation[c] || !sampler_processing_equal(ui.sampler_applied[c],ui.project.sampler[c])) {
  50:         Sample next; if(!sample_process(ui.originals[c],ui.project.sampler[c],&next)) return 0;
  55:     return 1;
```

### sampler_queue

```c
  61:     if(ui.sampler_job.busy || ui.control_drag || ui.arrangement.gesture==STRETCH_CLIP || ui.pattern_popup==5) return;
  62:     for(int c=0;c<ui.project.channel_count;c++) if(ui.sampler_generation[c]!=ui.sample_generation[c] || !sampler_processing_equal(ui.sampler_applied[c],ui.project.sampler[c])) {
  65:         ui.sampler_job.input=copy; ui.sampler_job.result=(Sample){0}; ui.sampler_job.channel=c;
  66:         ui.sampler_job.settings=ui.project.sampler[c]; ui.sampler_job.epoch=ui.sample_epoch; ui.sampler_job.generation=ui.sample_generation[c]; atomic_store(&ui.sampler_job.done,0);
  68:         ui.sampler_job.busy=1; return;
```

### import_sample

```c
  77:     if(!realpath(path,full) || strlen(full)>=sizeof ui.project.paths[c] || !sample_load(full,&s)) { snprintf(ui.status,sizeof ui.status,"Cannot load sample: unreadable, unsupported, too large or insufficient memory."); return 0; }
  79:     if(!sample_process(s,ui.project.sampler[c],&cooked)) { sample_free(s); snprintf(ui.status,sizeof ui.status,"Sample processing failed; channel unchanged"); return 0; }
  81:         for(int a=ui.project.automation_count-1;a>=0;a--) if(ui.project.automations[a].target.owner==(unsigned)c && ((ui.project.automations[a].target.parameter>=PARAM_FM_RATIO && ui.project.automations[a].target.parameter<=PARAM_FM_LAST) || (ui.project.automations[a].target.parameter>=PARAM_DX7_FIRST && ui.project.automations[a].target.parameter<=PARAM_DX7_LAST))) automation_delete(&ui.project,a);
  88:     memset(ui.project.channel_names[c],0,PATTERN_NAME); snprintf(ui.project.channel_names[c],PATTERN_NAME,"%s",GetFileNameWithoutExt(full));
  89:     snprintf(ui.status,sizeof ui.status,"Loaded %.100s into %s",GetFileName(path),ui.project.channel_names[c]); return 1;
```

### audition_entry

```c
  94:     if(entry<0 || entry>=ui.browser.items) return;
  96:     if(!IsFileExtension(path,".wav;.flac;.mp3") || DirectoryExists(path)) return;
  98:     if(!sample_load(path,&next)) { snprintf(ui.status,sizeof ui.status,"Cannot preview %.120s",GetFileName(path)); return; }
 102:     snprintf(ui.status,sizeof ui.status,"Preview: %.140s",GetFileName(path));
```

### rack_reveal_last

```c
 106:     Editor *e=&ui.windows.editors[0];
 108:     e->rect.h=fminf(GetScreenHeight()/ui_scale()-66,RACK_TOP+48+count*28);
 109:     ui.rack_scroll=fmaxf(0,count-(int)((e->rect.h-RACK_TOP-48)/28));
```

### add_instrument

```c
 117:     ui.project.channel_pitch[c]=0; ui.project.pitch_range[c]=2; ui.project.channel_audio[c]=0; ui.project.audio_seconds[c]=0; ui.rack_filter=2; ui.project.channel_count++; instrument_descriptor(INSTRUMENT_SAMPLER)->defaults(&ui.project.sampler[c]); ui.project.volume[c]=1; ui.project.pan[c]=0; ui.project.mute[c]=0; ui.project.route[c]=0;
 119:     memset(ui.project.channel_names[c],0,PATTERN_NAME); snprintf(ui.project.channel_names[c],PATTERN_NAME,"%s",instrument_descriptor(type)->name);
 120:     for(int pat=0;pat<PATTERNS;pat++) {
 121:         memset(ui.project.notes[pat][c],0,sizeof ui.project.notes[pat][c]); memset(ui.note_selected[pat][c],0,NOTES); ui.piano_channels[pat][c]=0;
 123:     Sample old=ui.samples[c],raw=ui.originals[c]; ui.originals[c]=ui.samples[c]=(Sample){0}; audio_channels(&ui.project,ui.samples); sample_free(old); sample_free(raw);
 125:     ui.channel=ui.instrument_channel=c; ui.browser_focus=0; rack_reveal_last(); windows_focus(&ui.windows,4);
```

### replace_instrument

```c
 134:     ui.automation_selected=ui.automation_node=-1; ui.control_drag=NULL; ui.menu_value=NULL;
 137:     ui.channel=ui.instrument_channel=c; ui.browser_focus=0; windows_focus(&ui.windows,4);
```

### pattern_instruments

```c
 142:     int count=0;
 143:     if(pat<0 || pat>=ui.project.pattern_count) return 0;
 144:     for(int c=0;c<ui.project.channel_count;c++) for(int n=0;n<NOTES;n++) if(ui.project.notes[pat][c][n].velocity) { rows[count++]=c; break; }
 145:     if(!count && ui.channel>=0 && ui.channel<ui.project.channel_count) rows[count++]=ui.channel;
```

### drop_sample

```c
 151:     if(target==1) {
 152:         Rect r=ui.windows.editors[1].rect;
 153:         float gx=r.x+212,gy=r.y+PLAYLIST_GRID_TOP,gridw=r.w-236;
 154:         int list=ui.mouse.x>=r.x+4 && ui.mouse.x<r.x+116 && ui.mouse.y>=gy && ui.mouse.y<r.y+r.h-66;
 160:             if(hit>=0 && AUDIO_SOURCE(ui.project.clips[lane][hit]-1)) {
 161:                 int c=ui.project.clips[lane][hit]-PATTERNS-1;
 163:                     ui.channel=ui.instrument_channel=c; ui.browser_focus=0;
 164:                     ui.arrangement.source_pattern=PATTERNS+c; ui.arrangement.source_steps=ui.project.audio_seconds[c]; ui.arrangement.source_offset=0;
 165:                     memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected); ui.arrangement.selected[lane][hit]=1;
 166:                     audio_channels(&ui.project,ui.samples); windows_focus(&ui.windows,1);
 167:                     snprintf(ui.status,sizeof ui.status,"Replaced sample with %.100s",ui.project.channel_names[c]);
 177:         int slot=-1;
 180:             Project next=ui.project; next.channel_count++; next.channel_audio[c]=1; next.audio_seconds[c]=.001f;
 181:             slot=arrangement_place(&next,lane,start,PATTERNS+c,.001f*ui.project.bpm/15);
 182:             if(slot<0) { snprintf(ui.status,sizeof ui.status,"No space here: choose an empty part of the track."); return; }
 186:             float length=ui.samples[c].frames/(float)RATE*ui.project.bpm/15;
 188:             if(slot<0) { /* Discard the import without adding a channel. */
 189:                 Sample empty={0}; audio_sample(c,empty); sample_free(ui.samples[c]); sample_free(ui.originals[c]); ui.samples[c]=ui.originals[c]=empty;
 190:                 ui.project.paths[c][0]=0; snprintf(ui.status,sizeof ui.status,"Audio overlaps another clip: choose an empty part of the track."); return;
 193:         uint32_t colors[CHANNELS]; int count=0;
 194:         for(int i=0;i<ui.project.channel_count;i++) if(ui.project.channel_audio[i]) colors[count++]=ui.project.channel_colors[i];
 196:         ui.project.channel_audio[c]=1; ui.project.channel_count++;
 197:         ui.channel=ui.instrument_channel=c; ui.rack_filter=1; ui.rack_scroll=0;
 198:         ui.arrangement.source_pattern=PATTERNS+c; ui.arrangement.source_steps=ui.project.audio_seconds[c]; ui.arrangement.source_offset=0;
 199:         if(!list) { memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected); ui.arrangement.selected[lane][slot]=1; }
 200:         ui.picker_tab=1; ui.picker_scroll=count;
 201:         ui.browser_focus=0; audio_channels(&ui.project,ui.samples); windows_focus(&ui.windows,1);
 202:         snprintf(ui.status,sizeof ui.status,list?"Imported %.100s into the Audio list; drag it to the Playlist when ready.":"Placed %.100s; available in the Audio list and Rack Audio group.",ui.project.channel_names[c]); return;
 204:     if(target==4 && ui.mouse.y>=ui.windows.editors[4].rect.y+TITLE) {
 206:             ui.channel=ui.instrument_channel; ui.browser_focus=0; audio_channels(&ui.project,ui.samples); windows_focus(&ui.windows,4);
 210:     Rect r=ui.windows.editors[0].rect;
 211:     if(windows_hit(&ui.windows,ui.mouse.x,ui.mouse.y)!=0 || ui.mouse.y<r.y+TITLE) {
 214:     int c=rack_channel_at(ui.rack_scroll+(ui.mouse.y-r.y-RACK_TOP)/28);
 215:     int visible=fmaxf(1,(r.h-RACK_TOP-48)/28);
 216:     int replace=ui.mouse.y>=r.y+RACK_TOP && ui.mouse.y<r.y+RACK_TOP+visible*28 && c>=0 && c<ui.project.channel_count;
 221:             ui.project.channel_audio[c]=0; ui.project.channel_count++; ui.rack_filter=2;
 224:         ui.channel=c; ui.browser_focus=0; windows_focus(&ui.windows,0); audio_channels(&ui.project,ui.samples);
```

### delete_channel

```c
 231:     ui.automation_selected=ui.automation_node=-1; ui.picker_drag=-1;
 236:         ui.samples[i]=ui.samples[i+1]; ui.originals[i]=ui.originals[i+1]; ui.sampler_applied[i]=ui.sampler_applied[i+1];
 237:         ui.sample_generation[i]=ui.sample_generation[i+1]; ui.sampler_generation[i]=ui.sampler_generation[i+1];
 239:     ui.originals[ui.project.channel_count]=(Sample){0};
 240:     ui.samples[ui.project.channel_count]=(Sample){0};
 241:     for(int pat=0;pat<PATTERNS;pat++) {
 242:         memmove(ui.note_selected[pat][c],ui.note_selected[pat][c+1],(ui.project.channel_count-c)*NOTES); memset(ui.note_selected[pat][ui.project.channel_count],0,NOTES);
 243:         memmove(&ui.piano_channels[pat][c],&ui.piano_channels[pat][c+1],ui.project.channel_count-c);
 244:         ui.piano_channels[pat][ui.project.channel_count]=0;
 246:     ui.channel=fmaxf(0,fminf(ui.channel-(ui.channel>c),ui.project.channel_count-1));
 247:     ui.piano_channel=fmaxf(0,fminf(ui.piano_channel-(ui.piano_channel>c),ui.project.channel_count-1));
 248:     ui.instrument_channel=fmaxf(0,fminf(ui.instrument_channel-(ui.instrument_channel>c),ui.project.channel_count-1));
 249:     ui.arrangement.gesture=IDLE; ui.arrangement.source_pattern=-1; memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected);
 250:     audio_channels(&ui.project,ui.samples); sample_free(old); sample_free(raw); ui.reset=1;
```

### install_project

```c
 254:     recording_finish();midi_panic();ui.midi_learning=0;
 255:     ui.sample_epoch++; audio_stop(); ui.fm_graph_drag=-1;
 256:     sample_free(ui.audition); free(ui.audition_wave.tree); ui.audition_wave=(Waveform){0}; ui.audition=(Sample){0}; ui.audition_path[0]=0; ui.browser_progress=-1;
 257:     memset(ui.keyboard_notes,0,sizeof ui.keyboard_notes); ui.piano_note_length=2; if(ui.edit_clipboard.kind==COPY_CLIPS) ui.edit_clipboard.kind=COPY_EMPTY;
 258:     ui.picker_tab=ui.picker_scroll=0; ui.picker_drag=-1; ui.automation_selected=ui.automation_node=ui.automation_lane=ui.automation_clip=-1; ui.sample_drag[0]=0;
 259:     ui.rack_hdrag=ui.rack_vdrag=ui.playlist_pan=ui.playlist_vpan=ui.piano_scroll_drag=ui.piano_vdrag=ui.mixer_pan=0;
 260:     ui.piano_scroll_remainder=0; ui.track_zoom=ui.piano_zoom=1; ui.row_zoom_drag=-1; memset(ui.track_heights,0,sizeof ui.track_heights);
 261:     ui.navigation_active=0; ui.navigation_drag=-1; ui.track_resize=-1; ui.selected_track=-1; ui.ruler_loop_drag=0; ui.stop_armed=0; ui.ruler_last_id=-1;
 262:     ui.visual_step=0;
 263:     ui.playing=0; ui.pattern=(int)fminf(ui.pattern,next.pattern_count-1); audio_update(&next,0,ui.song,ui.pattern,1,ui.output_volume,0,0,0);
 264:     for(int c=0;c<CHANNELS;c++) sample_duration(&next,c,processed[c],fresh[c]);
 266:     for(int c=0;c<CHANNELS;c++) {
 271:     ui.project.insert_count=INSERTS; ui.rack_scroll=0; ui.rack_filter=0; ui.channel=fmaxf(0,fminf(ui.channel,ui.project.channel_count-1));
 273:     for(int i=0;i<PATTERNS;i++) { ui.piano_span[i]=fmaxf(STEPS,next.pattern_steps[i]); ui.piano_pan[i]=0; ui.piano_range[i]=0; }
 274:     memset(ui.note_selected,0,sizeof ui.note_selected); ui.piano_gesture=PIANO_IDLE; ui.playlist_start=0; memset(ui.piano_start,0,sizeof ui.piano_start); memset(ui.song_loop,0,sizeof ui.song_loop); memset(ui.pattern_loop,0,sizeof ui.pattern_loop); ui.marker_drag=-1;
 275:     memset(ui.piano_channels,0,sizeof ui.piano_channels); ui.mixer_selected=ui.project.route[ui.channel]; ui.mixer_scroll=0;
 276:     ui.piano_key_drag=0; ui.piano_key=-1;
 277:     ui.note_drag=NULL; ui.velocity_drag=-1; ui.control_drag=NULL; ui.route_drag=-1; ui.rack_paint=-1; ui.cable_drag=-1;
 278:     memset(&ui.arrangement,0,sizeof ui.arrangement); ui.arrangement.source_pattern=-1; ui.arrangement.source_steps=STEPS; ui.arrangement.zoom=1; ui.playlist_pan=0; ui.playlist_vpan=0; ui.track_scroll=0; memset(ui.rack_view,0,sizeof ui.rack_view); memset(ui.rack_range,0,sizeof ui.rack_range);
 281:     snprintf(ui.status,sizeof ui.status,"Loaded %.150s",filename); ui.reset=1;
```

### new_project

```c
 285:     Project next; Sample fresh[CHANNELS]={0},processed[CHANNELS]={0};
 288:     for(int c=0;c<CHANNELS;c++) {
 289:         if((demo && c<4 && !fresh[c].data) || !sample_process(fresh[c],next.sampler[c],&processed[c])) {
 290:             for(int i=0;i<CHANNELS;i++) { sample_free(fresh[i]); sample_free(processed[i]); }
 294:     ui.pattern=ui.channel=0; ui.song=demo; install_project(next,fresh,processed,demo?"demo" PROJECT_FILE_SUFFIX:"project" PROJECT_FILE_SUFFIX); ui.document.saved_on_disk=0;
```

### load_project

```c
 299:     Project next; Sample fresh[CHANNELS]={0},processed[CHANNELS]={0}; int missing=0;
 301:         snprintf(ui.status,sizeof ui.status,"Project unchanged: invalid project, sample path too long or processing failed: %.100s",path); return 0;
 306:     return 1;
```

### replace_project

```c
 311:     if(project_document_request(&ui.document,&ui.project,action)) { open_popup(12); return; }
 312:     if(action==4) load_project(ui.document.replacement_path); else if(action==3) project_file_action(4); else if(action==REPLACE_QUIT) ui.document.quit=1; else new_project(action==2);
```

### request_load_project

```c
 317:     snprintf(ui.document.replacement_path,sizeof ui.document.replacement_path,"%s",path); replace_project(4);
```

### replacement_saved

```c
 322:     int action=ui.document.pending_action; ui.document.pending_action=0;
 323:     if(action==4) load_project(ui.document.replacement_path); else if(action==3) project_file_action(4); else if(action==REPLACE_QUIT) ui.document.quit=1; else if(action) new_project(action==2);
```

### history_stamps

```c
 326: void history_stamps(uint64_t stamps[CHANNELS]) {
 327:     for(int c=0;c<CHANNELS;c++) stamps[c]=((uint64_t)ui.sample_epoch<<32)|ui.sample_generation[c];
```

### history_checkpoint

```c
 331:     uint64_t stamps[CHANNELS]; history_stamps(stamps);
 333:         snprintf(ui.status,sizeof ui.status,"Could not retain undo history: insufficient memory."); return 0;
 335:     return 1;
```

### undo_redo

```c
 339:     if(recording_active() || captured()) { snprintf(ui.status,sizeof ui.status,"Finish the current edit or recording before undo/redo."); return; }
 341:     uint8_t keep[CHANNELS]={0}; uint64_t current_stamps[CHANNELS]; history_stamps(current_stamps);
 342:     Project next; Sample retained[CHANNELS],fresh[CHANNELS]={0},processed[CHANNELS]={0};
 343:     if(!history_peek(&ui.edit_history,direction,&next,retained)) { snprintf(ui.status,sizeof ui.status,direction<0?"Nothing to undo.":"Nothing to redo."); return; }
 344:     for(int c=0;c<CHANNELS;c++) {
 346:             keep[c]=1; fresh[c]=ui.originals[c]; processed[c]=ui.samples[c]; sample_duration(&next,c,ui.samples[c],ui.originals[c]); continue;
 352:     if(ui.sampler_job.busy) { pthread_join(ui.sampler_job.thread,NULL); sample_free(ui.sampler_job.input); sample_free(ui.sampler_job.result); ui.sampler_job.busy=0; }
 354:     for(int c=0;c<CHANNELS;c++) {
 360:     uint64_t stamps[CHANNELS]; history_stamps(stamps); history_rebind(&ui.edit_history,ui.originals,stamps);
 361:     ui.pattern=fmaxf(0,fminf(ui.pattern,ui.project.pattern_count-1)); ui.channel=fmaxf(0,fminf(ui.channel,ui.project.channel_count-1));
 362:     ui.piano_channel=ui.instrument_channel=ui.channel; ui.automation_selected=ui.automation_node=-1; ui.picker_drag=-1;
 363:     ui.note_drag=NULL; ui.control_drag=NULL; ui.menu_value=NULL; ui.context_kind=0;
 364:     memset(ui.note_selected,0,sizeof ui.note_selected); memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected);
 365:     ui.arrangement.source_pattern=-1; ui.arrangement.source_steps=STEPS; ui.arrangement.source_offset=0;
 366:     ui.rack_scroll=0; ui.picker_scroll=0; ui.piano_gesture=PIANO_IDLE;
 367:     snprintf(ui.status,sizeof ui.status,direction<0?"Undone.":"Redone."); return;
 369:     for(int c=0;c<CHANNELS;c++) if(!keep[c]) { sample_free(fresh[c]); sample_free(processed[c]); }
```

### preset_commit

```c
 377:         if(owner<0 || owner>ui.project.insert_count || slot<0 || slot>=EFFECT_SLOTS || ui.project.effect_type[owner][slot]!=(ui.preset_kind==PRESET_EQ?EFFECT_EQ:EFFECT_CHORUS)) return 0;
 380:         if(owner<0 || owner>=ui.project.channel_count || (ui.preset_kind==PRESET_FM)!=(ui.project.instrument[owner]==INSTRUMENT_FM)) return 0;
 385:         int ok=preset_save(chosen,&preset); snprintf(ui.status,sizeof ui.status,ok?"Preset saved: %.180s":"Cannot save preset: %.180s",chosen); return ok;
 387:     if(!preset_load(chosen,&preset) || preset.kind!=ui.preset_kind) { snprintf(ui.status,sizeof ui.status,"Invalid preset or wrong device type"); return 0; }
 389:     else if(ui.preset_kind==PRESET_EQ) { ui.project.eq[owner][slot]=preset.eq; ui.project.effect_mix[owner][slot]=preset.mix; ui.project.effect_bypass[owner][slot]=0; }
 390:     else if(ui.preset_kind==PRESET_CHORUS) { ui.project.chorus[owner][slot]=preset.chorus; ui.project.effect_mix[owner][slot]=preset.mix; ui.project.effect_bypass[owner][slot]=0; }
 392:         Sample raw={0},processed={0}; int ok;
 393:         if(!sampler_flush()) return 0;
 394:         if(!strcmp(preset.sample_path,SAMPLE_EMPTY)) ok=1;
 395:         else if(preset.sample_path[0]) ok=sample_load(preset.sample_path,&raw);
 398:             sample_free(raw); sample_free(processed); snprintf(ui.status,sizeof ui.status,"Preset sample missing or processing failed; channel unchanged"); return 0;
 403:         if(preset.sample_path[0]) snprintf(ui.project.paths[owner],sizeof ui.project.paths[owner],"%s",preset.sample_path);
 407:     snprintf(ui.status,sizeof ui.status,"Loaded preset: %.150s",GetFileNameWithoutExt(chosen)); return 1;
```

### project_file_commit

```c
 411:     if(ui.file_action==8 || ui.file_action==9) return preset_commit(chosen,ui.file_action==8);
 413:     if(ui.file_action==4) return load_project(chosen);
 414:     if(ui.file_action==7) {
 415:         if(ui.relink_channel<0 || ui.relink_channel>=ui.project.channel_count) return 0;
 421:     if(ui.file_action==5) {
 424:         snprintf(ui.status,sizeof ui.status,ok?"Exported %.220s":"Export failed: %.220s",chosen); return ok;
 426:     ok=sampler_flush() && project_save_assets(chosen,&ui.project,ui.originals,ui.file_action==6);
 428:     snprintf(ui.status,sizeof ui.status,ok?"Saved %.220s":"Save failed: %.220s",chosen); return ok;
```

## app_recording.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### recording_finish

```c
   6:         int failed=ui.midi_take.failed;audio_record_mode(0);midi_take_finish(&ui.midi_take,&ui.project,midi_input_time());midi_panic();
   7:         ui.pattern=fmaxf(0,fminf(ui.pattern,ui.project.pattern_count-1));
  12:     for(int i=0;i<ui.recording_ui.count;i++) {
  15:         recording_writer_free(take->writer,0); take->writer=NULL;
  19:             if(take->clip>=0) ui.project.clip_steps[take->lane][take->clip]=0;
  20:             snprintf(ui.project.paths[c],sizeof ui.project.paths[c],"%s",take->path); ok=0;
  22:             if(take->clip>=0) ui.project.clips[take->lane][take->clip]=0;
  23:             ui.project.audio_seconds[c]=0;
  27:             if(take->clip>=0) ui.project.clip_steps[take->lane][take->clip]=0;
  30:                 processed=take->sample; take->sample=(Sample){0}; ok=0;
  37:         free(take->wave.tree); take->wave=(Waveform){0}; take->sample=(Sample){0};
  40:     ui.recording_ui.count=0;
```

### recording_start_audio

```c
  49:     int buses[CHANNELS],count=0;
  50:     for(int id=0;id<=ui.project.insert_count;id++) if(ui.record_armed[id]) {
  56:     if(MakeDirectory("recordings")!=0) { snprintf(ui.status,sizeof ui.status,"Cannot create recordings/ in the working directory."); return; }
  58:     float start=ui.playing && ui.song?audio_visual_position()/(RATE*60.0/ui.project.bpm/4):ui.playlist_start*STEPS;
  61:     char error[256]={0};
  63:     for(int i=0;i<count;i++) audio_sample(ui.recording_ui.takes[i].channel,(Sample){0});
  66:         audio_update(&ui.project,ui.playing,ui.song,ui.pattern,1,ui.output_volume,playback_start(),playback_loop()[0],playback_loop()[1]);
  71:         audio_update(&ui.project,ui.playing,ui.song,ui.pattern,1,ui.output_volume,playback_start(),playback_loop()[0],playback_loop()[1]);
  74:     for(int i=0;i<count;i++) {
  76:         ui.samples[c]=ui.originals[c]=(Sample){0}; ui.sampler_applied[c]=next.sampler[c];
  78:     ui.project=next; ui.recording_ui.active=1; ui.recording_ui.bpm=ui.project.bpm; ui.recording_ui.start=start;
  79:     ui.playing=ui.song=1; ui.reset=0; ui.playlist_start=start/STEPS; memset(ui.song_loop,0,sizeof ui.song_loop);
  80:     memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected);
  81:     for(int i=0;i<count;i++) { RecordingTake *take=&ui.recording_ui.takes[i]; ui.arrangement.selected[take->lane][take->clip]=1; }
  82:     ui.picker_tab=1; ui.picker_scroll=0; ui.channel=ui.instrument_channel=ui.recording_ui.takes[0].channel;
  83:     ui.arrangement.source_pattern=-1; ui.arrangement.source_offset=0;
  84:     windows_focus(&ui.windows,1); ui.browser_focus=0;
```

### recording_start

```c
  96:     if((ui.record_mask&RECORD_NOTES) && !ui.midi_connected){snprintf(ui.status,sizeof ui.status,"Choose a MIDI input in View > MIDI / Recording");open_popup(13);return;}
  97:     int target=midi_target();ui.midi_record_target=target;float origin=ui.playing && ui.song?audio_visual_position()/(RATE*15/ui.project.bpm):ui.playlist_start*STEPS;
 102:     int armed=0;for(int i=0;i<=ui.project.insert_count;i++)armed+=ui.record_armed[i]!=0;
 106:         audio_record_mode(1);double now=midi_input_time();
 107:         if(ui.recording_ui.active)ui.midi_take.time=now-fmax(0,audio_visual_position()/(RATE*15/ui.project.bpm)-origin)*15/ui.project.bpm;
 109:         if(!ui.playing || !ui.song){ui.playing=ui.song=1;ui.reset=1;}
 110:         else if(!ui.recording_ui.active)ui.reset=0;
 111:         if(!ui.recording_ui.active){audio_update(&ui.project,1,1,ui.pattern,ui.reset,ui.output_volume,origin,0,0);ui.reset=0;}
 112:         if(ui.midi_take.pattern>=0){ui.pattern=ui.midi_take.pattern;ui.piano_channel=target;ui.piano_channels[ui.pattern][target]=1;}
 113:         for(int i=0;i<32;i++)if(ui.midi_keys[i].used && ui.midi_keys[i].target==target && (ui.record_mask&RECORD_NOTES))midi_take_note(&ui.midi_take,&ui.project,ui.midi_keys[i].mchannel,ui.midi_keys[i].pitch,ui.midi_keys[i].velocity,ui.midi_take.time);
 114:         memset(ui.song_loop,0,sizeof ui.song_loop);windows_focus(&ui.windows,2);ui.browser_focus=0;
```

### recording_source

```c
 123:     if(ui.recording_ui.active) for(int i=0;i<ui.recording_ui.count;i++) if(source==PATTERNS+ui.recording_ui.takes[i].channel) return 1;
 124:     return 0;
```

## arrangement.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### quantum

```c
   6: static float quantum(const Arrangement *a) { return a->snap>0?a->snap:.01f; }
```

### snapped

```c
   7: static float snapped(const Arrangement *a,float x) { return a->snap>0?snap_floor(x*STEPS+.00001f,a->snap)/STEPS:x; }
```

### arrangement_hit

```c
  12:     if(lane<0 || lane>=LANES || bar<0) return -1;
  13:     for(int b=CLIPS-1;b>=0;b--) if(p->clips[lane][b] && bar>=p->clip_starts[lane][b] && bar<p->clip_starts[lane][b]+clip_length(p,lane,b)/STEPS) return b;
  14:     return -1;
```

### arrangement_place

```c
  17:     if(l<0 || l>=LANES || pattern<0 || pattern>=SOURCES || x<0 || !isfinite(x) || !isfinite(length) || length<=0) return -1;
  18:     for(int i=0;i<CLIPS;i++) if(p->clips[l][i] && x*STEPS<p->clip_starts[l][i]*STEPS+clip_length(p,l,i)-.0001f && p->clip_starts[l][i]*STEPS<x*STEPS+length-.0001f) return -1;
  19:     int b=x<CLIPS?(int)x:0;
  20:     if(p->clips[l][b]) { for(b=0;b<CLIPS && p->clips[l][b];b++) {} if(b==CLIPS) return -1; }
  21:     p->clips[l][b]=pattern+1; p->clip_steps[l][b]=AUDIO_SOURCE(pattern)?(fabsf(length-clip_source_steps(p,pattern))<.0001f?0:length*15/audio_source_bpm(p,pattern-PATTERNS)):length; p->clip_starts[l][b]=x; p->clip_offsets[l][b]=0; return b;
```

### source_steps

```c
  23: static float source_steps(const Arrangement *a,const Project *p) { return AUDIO_SOURCE(a->source_pattern)?a->source_steps*audio_source_bpm(p,a->source_pattern-PATTERNS)/15:a->source_steps; }
```

### place_copy

```c
  26:     if(slot>=0) {
  28:         if(AUDIO_SOURCE(source)) p->clip_steps[lane][slot]=fabsf(length-(clip_source_steps(p,source)-clip_offset_steps(p,lane,slot)))<.0001f?0:length*15/audio_source_bpm(p,source-PATTERNS);
```

### paint

```c
  35:     if(slot>=0) a->selected[l][slot]=1;
```

### arrangement_select_press

```c
  40:     if(!additive) memset(a->selected,0,sizeof a->selected);
  42:     a->gesture=x<0?TRACK_SELECT:BOX_SELECT;
```

### arrangement_press

```c
  49:     if(l<0 || l>=LANES) return;
  50:     if(x<0) { if(!additive) memset(a->selected,0,sizeof a->selected); a->gesture=TRACK_SELECT; arrangement_drag(a,p,x,y); return; }
  51:     if(right) { a->gesture=ERASE_CLIPS; arrangement_drag(a,p,x,y); return; }
  53:         if(hit>=0) { a->selected[l][hit]^=1; a->source_pattern=p->clips[l][hit]-1; a->source_steps=AUDIO_SOURCE(a->source_pattern)?clip_length(p,l,hit)*15/audio_source_bpm(p,a->source_pattern-PATTERNS):clip_length(p,l,hit); a->source_offset=p->clip_offsets[l][hit]; }
  54:         else a->gesture=BOX_SELECT;
  58:         if(hit<0) return;
  61:         if(left<=.0001f || right_length<=.0001f) return;
  62:         int slot=0; while(slot<CLIPS && p->clips[l][slot]) slot++;
  64:         int source=p->clips[l][hit]-1;
  65:         float seconds=AUDIO_SOURCE(source)?15/audio_source_bpm(p,source-PATTERNS):1;
  68:         p->clip_steps[l][hit]=left*(AUDIO_SOURCE(source)?seconds:1);
  69:         p->clip_steps[l][slot]=right_length*(AUDIO_SOURCE(source)?seconds:1);
  70:         memset(a->selected,0,sizeof a->selected); a->selected[l][hit]=a->selected[l][slot]=1;
  73:     if(a->tool==STRETCH && (hit<0 || !edge || !AUDIO_SOURCE(p->clips[l][hit]-1))) return;
  74:     if(a->source_pattern<PATTERNS && a->source_pattern!=pattern) { a->source_pattern=pattern; a->source_steps=p->pattern_steps[pattern]; a->source_offset=0; }
  75:     if(hit>=0) {
  76:         a->source_pattern=p->clips[l][hit]-1; a->source_steps=AUDIO_SOURCE(a->source_pattern)?clip_length(p,l,hit)*15/audio_source_bpm(p,a->source_pattern-PATTERNS):clip_length(p,l,hit);
  80:             a->gesture=a->tool==STRETCH?STRETCH_CLIP:SIZE_CLIP;
  81:             if(a->gesture==STRETCH_CLIP) {
  87:             if(a->gesture==STRETCH_CLIP || !a->selected[l][hit]) {
  88:                 memset(a->selected,0,sizeof a->selected); a->selected[l][hit]=1;
  90:             if(a->gesture==SIZE_CLIP) {
  95:                 for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) a->lengths[i][j]=clip_length(p,i,j);
 100:     if(a->tool==SELECT && hit<0) { memset(a->selected,0,sizeof a->selected); a->gesture=BOX_SELECT; return; }
 101:     int group=0; for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) group+=a->selected[i][j]!=0;
 102:     if(a->tool==BRUSH && !(hit>=0 && a->selected[l][hit] && group>1)) {
 103:         a->paint_origin=hit>=0?p->clip_starts[l][hit]:snapped(a,x);
 104:         memset(a->selected,0,sizeof a->selected);
 105:         if(hit>=0) a->selected[l][hit]=1;
 106:         a->gesture=PAINT_CLIPS; if(hit<0) paint(a,p,l,snapped(a,x)); return;
 108:     if(hit<0) {
 109:         hit=paint(a,p,l,snapped(a,x)); if(hit<0) return; a->bar=hit;
 110:         memset(a->selected,0,sizeof a->selected);
 112:     if(!a->selected[l][hit]) { memset(a->selected,0,sizeof a->selected); a->selected[l][hit]=1; }
 114:     for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) a->lengths[i][j]=clip_length(p,i,j);
 115:     memcpy(a->starts,p->clip_starts,sizeof a->starts); memcpy(a->offsets,p->clip_offsets,sizeof a->offsets); memcpy(a->moved,a->selected,sizeof a->moved); a->gesture=MOVE_CLIPS;
```

### arrangement_drag

```c
 117: void arrangement_drag(Arrangement *a,Project *p,float x,float y) {
 118:     int l=clamp((int)floorf(y),0,LANES-1);
 120:     if(a->gesture==STRETCH_CLIP) {
 123:         if(a->edge<0) delta=fmaxf(delta,-a->size_start*STEPS);
 124:         float n=fmaxf(.01f,a->size_length+(a->edge<0?-delta:delta));
 125:         float time=fmaxf(.25f,fminf(4,a->size_time*n/a->size_length));
 128:         for(int l=0;l<LANES;l++) for(int j=0;j<CLIPS;j++) if(p->clips[l][j]==a->source_pattern+1) {
 132:         p->clip_starts[lane][b]=a->edge<0?fmaxf(0,a->size_start+(a->size_length-a->size_length*ratio)/STEPS):a->size_start;
 133:     } else if(a->gesture==SIZE_CLIP) {
 134:         float q=quantum(a),delta=a->edge<0?snap_round((x-a->x)*STEPS,a->snap):
 136:         int unchanged=fabsf(x-a->x)<.00001f;
 138:         for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) if(a->selected[i][j] && a->before[i][j]) {
 139:             int source=a->before[i][j]-1;
 140:             float offset=AUDIO_SOURCE(source)?a->offsets[i][j]*audio_source_bpm(p,source-PATTERNS)/15/channel_speed(p,source-PATTERNS):a->offsets[i][j];
 142:             if(a->edge<0) {
 151:         delta=unchanged?0:fmaxf(low,fminf(high,delta));
 152:         if(a->edge<0 && delta<0) {
 153:             for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) if(a->selected[i][j] && a->before[i][j]>AUTOMATION_SOURCE) {
 154:                 int source=a->before[i][j]-1;
 156:                 if(shift<=0) continue;
 158:                 for(int n=0;n<curve->count;n++) curve->points[n].step+=shift;
 160:                 for(int lane=0;lane<LANES;lane++) for(int slot=0;slot<CLIPS;slot++) if(p->clips[lane][slot]==source+1) {
 165:         for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) if(a->selected[i][j] && a->before[i][j]) {
 166:             int source=a->before[i][j]-1;
 167:             float conversion=AUDIO_SOURCE(source)?15/audio_source_bpm(p,source-PATTERNS)*channel_speed(p,source-PATTERNS):1;
 168:             float offset=a->offsets[i][j]/conversion,length=a->lengths[i][j]+(a->edge<0?-delta:delta);
 169:             p->clip_starts[i][j]=a->starts[i][j]+(a->edge<0?delta/STEPS:0);
 170:             p->clip_offsets[i][j]=a->offsets[i][j]+(a->edge<0?delta*conversion:0);
 173:                 if(a->edge<0) offset+=delta;
 174:                 float available=fmaxf(0,clip_source_steps(p,source)-offset);
 175:                 p->clip_steps[i][j]=AUDIO_SOURCE(source)?(fabsf(length-available)<.0001f?0:length*15/audio_source_bpm(p,source-PATTERNS)):length;
 180:         a->source_steps=AUDIO_SOURCE(a->source_pattern)?length*15/audio_source_bpm(p,a->source_pattern-PATTERNS):length;
 181:     } else if(a->gesture==MOVE_CLIPS) {
 182:         float dx=a->snap>0?snapped(a,x)-snapped(a,a->x):x-a->x; int dy=l-a->lane;
 183:         float first=INFINITY; int top=LANES,bottom=-1;
 184:         for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) if(a->selected[i][j]) {
 187:         if(bottom<0) return;
 188:         dx=fmaxf(-first,dx); dy=clamp(dy,-top,LANES-1-bottom);
 190:         uint8_t moved[LANES][CLIPS]={0};
 191:         for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) if(a->selected[i][j]) { next.clips[i][j]=0; next.clip_steps[i][j]=0; }
 192:         for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) if(a->selected[i][j]) {
 193:             int slot=place_copy(&next,i+dy,a->starts[i][j]+dx,a->before[i][j]-1,a->lengths[i][j]?a->lengths[i][j]:STEPS,a->offsets[i][j]);
 194:             if(slot<0) return;
 195:             moved[i+dy][slot]=1;
 198:     } else if(a->gesture==BOX_SELECT || a->gesture==TRACK_SELECT) {
 200:         for(int i=0;i<LANES;i++) for(int j=0;j<CLIPS;j++) a->selected[i][j]=(a->additive && a->selection_before[i][j]) || (p->clips[i][j] && (a->gesture==TRACK_SELECT?i<=bottom:i<bottom) && i+1>top && (a->gesture==TRACK_SELECT || (p->clip_starts[i][j]<right && p->clip_starts[i][j]+clip_length(p,i,j)/STEPS>left)));
 201:     } else if(a->gesture==PAINT_CLIPS || a->gesture==ERASE_CLIPS) {
 202:         if(x<0 || y<0 || y>=LANES) { a->last_lane=-1; return; }
 204:         if(a->gesture==ERASE_CLIPS) {
 205:             for(int j=0;j<CLIPS;j++) if(p->clips[l][j] && p->clip_starts[l][j]<=right && p->clip_starts[l][j]+clip_length(p,l,j)/STEPS>left) { p->clips[l][j]=0; p->clip_steps[l][j]=0; a->selected[l][j]=0; }
 208:             if(stride<=0) stride=1;
 209:             float first=ceilf((left-origin)/stride-.00001f);
 210:             for(int k=0;k<CLIPS && origin+(first+k)*stride<=right+.00001f;k++) paint(a,p,l,origin+(first+k)*stride);
```

### arrangement_release

```c
 216:     if(a->gesture==MOVE_CLIPS) memcpy(a->selected,a->moved,sizeof a->selected);
 217:     a->gesture=IDLE;
```

### arrangement_zoom

```c
 220:     float span=BARS/(a->zoom>0?a->zoom:1);
 221:     timeline_zoom(&span,&a->view_start,wheel,cursor,1); a->zoom=BARS/span;
 222:     a->range=fmaxf(a->range,a->view_start+span*2);
```

## arrangement.h

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
   8:     int tool,gesture,lane,bar,source_pattern,last_lane,edge;
  11:     uint8_t selected[LANES][CLIPS],before[LANES][CLIPS],selection_before[LANES][CLIPS];
  15:     uint8_t moved[LANES][CLIPS];
  24: void arrangement_drag(Arrangement *a,Project *p,float bar,float lane);
```

## audio.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
  18: #define AUDIO_COMMANDS 256
  24:     uint64_t serial;
  31:     uint64_t serial;
  38:     VoiceView voices[128];
  39:     uint8_t instrument[CHANNELS];
  40:     uint8_t pattern_notes[PATTERNS][CHANNELS][NOTES];
  45: static atomic_uint_fast64_t acknowledged;
  55: static double preview_position,preview_speed=1,preview_frame,preview_time,preview_rate=1;
  56: static float preview_gain=.7f;
  59: static int preview_channel=-1;
```

### audio_record_mode

```c
  65: static float output_volume=1;
  66: static atomic_uint_fast64_t position;
  67: static _Atomic float meter_peak[INSERTS+1][2];
  71: static float clicks[2][1200];
  72: static unsigned click_position=1200;
  77: #define ANALYZER_FRAMES 8192u
  78: typedef struct { float pcm[ANALYZER_FRAMES][2]; atomic_uint read,write; } AnalyzerRing;
  79: static AnalyzerRing analyzer[2];
```

### analyzer_push

```c
  86:     ring->pcm[w%ANALYZER_FRAMES][0]=isfinite(l)?l:0; ring->pcm[w%ANALYZER_FRAMES][1]=isfinite(r)?r:0;
  87:     atomic_store_explicit(&ring->write,w+1,memory_order_release);
```

### audio_spectrum_read

```c
  91:     if(stream<0 || stream>1) return 0;
  94:     for(unsigned i=0;i<n;i++) { pcm[i*2]=ring->pcm[(r+i)%ANALYZER_FRAMES][0]; pcm[i*2+1]=ring->pcm[(r+i)%ANALYZER_FRAMES][1]; }
  97: #define RECORD_RING_FRAMES (RATE*4u)
 100:     atomic_uint_fast64_t read,write;
```

### ring_push

```c
 110:     uint64_t w=atomic_load_explicit(&ring->write,memory_order_relaxed);
 112:         atomic_store(&ring->overflow,1); return 0;
 115:     ring->pcm[at*2]=isfinite(left)?left:0; ring->pcm[at*2+1]=isfinite(right)?right:0;
 116:     atomic_store_explicit(&ring->write,w+1,memory_order_release); return 1;
```

### ring_pop

```c
 118: static int ring_pop(RecordRing *ring,float stereo[2]) {
 119:     uint64_t r=atomic_load_explicit(&ring->read,memory_order_relaxed);
 120:     if(r==atomic_load_explicit(&ring->write,memory_order_acquire)) return 0;
 122:     stereo[0]=ring->pcm[at*2]; stereo[1]=ring->pcm[at*2+1];
 123:     atomic_store_explicit(&ring->read,r+1,memory_order_release); return 1;
```

### capture_callback

```c
 125: static void capture_callback(ma_device *d,void *out,const void *in,ma_uint32 frames) {
 127:     for(unsigned i=0;i<frames;i++) ring_push(ring,pcm?pcm[i*2]:0,pcm?pcm[i*2+1]:0);
```

### record_input

```c
 129: static void record_input(void *context,float buses[INSERTS+1][2]) {
 130:     (void)context; float input[CHANNELS][2]={{0}};
 131:     for(int i=0;i<recording.devices_count;i++) ring_pop(&recording.inputs[i],input[i]);
 132:     for(int i=0;i<recording.count;i++) if(recording.sources[i]>=0) {
 134:         buses[id][0]+=input[source][0]; buses[id][1]+=input[source][1];
```

### record_output

```c
 139:     if(bus==callback_bus) { analyzer_push(1,left,right); callback_taps++; }
 140:     for(int i=0;i<recording.count;i++) if(recording.buses[i]==bus) ring_push(&recording.takes[i],left,right);
```

### monotonic_time

```c
 144:     return t.tv_sec+t.tv_nsec/1e9;
```

### callback

```c
 146: static void callback(ma_device *d, void *out, const void *in, ma_uint32 frames) {
 148:     memset(out,0,frames*2*sizeof(float));
 154:     callback_taps=0;
 156:     MixerIO taps=recording.active?recording.io:(MixerIO){0}; taps.output=record_output; taps.monitor_only=!(recording.active || atomic_load(&midi_recording));
 157:     float peaks[INSERTS+1][2]={{0}};
 159:         visual_frame=llround(player.frame*(player.clock_bpm?player.clock_bpm/project.bpm:1));
 160:         render_mixer_io(&player,&live,&project,samples,out,frames,1,peaks,&taps);
 162:         double step_frames=RATE*60.0/project.bpm/4;
 163:         visual_start=(player.loop_end?player.loop_start:player.song?0:player.start_step)*step_frames;
 165:         if(visual_start>=end*step_frames) visual_start=0;
 168:     else render_mixer_io(&live,NULL,&project,samples,out,frames,0,peaks,&taps);
 169:     for(int l=0;l<LANES;l++) {
 171:         if(playing && player.song && player.lane_trigger[l]) atomic_store_explicit(&track_trigger[l],1,memory_order_relaxed);
 173:     for(int c=0;c<CHANNELS;c++) {
 175:         int previewing=preview_channel==c && preview_remaining && preview_position<preview_end && preview_gain>0;
 177:         if(rendered->channel_trigger[c]) atomic_store_explicit(&channel_trigger[c],1,memory_order_relaxed);
 181:     double pitch_speed=pow(2,project.master_pitch/12.0);
 183:     for(unsigned i=0;i<frames && preview_position<preview_end && preview_remaining;i++,preview_position+=pitch_speed*preview_speed,preview_remaining--) {
 185:         for(unsigned side=0;side<2;side++) {
 186:             float a=sample_at(preview,n,side),b=sample_at(preview,n+1,side);
 188:             buffer[i*2+side]+=x;
 192:         double beat_frames=RATE*60.0/project.bpm;
 193:         for(unsigned i=0;i<frames;i++) {
 197:             if(phase<1) { click_position=0; click_accent=fmod(beat,4)==0; }
 198:             if(click_position<1200) {
 200:                 buffer[i*2]+=x;
 201:                 buffer[i*2+1]+=x;
 204:     } else click_position=1200;
 205:     for(unsigned i=0;i<frames*2;i++) {
 206:         peaks[0][i%2]=fmaxf(peaks[0][i%2],fabsf(buffer[i]));
 208:         peaks[0][i%2]=fmaxf(peaks[0][i%2],fabsf(x));
 209:         buffer[i]=isfinite(x)?fmaxf(-1,fminf(1,x)):0;
 211:     for(unsigned i=0;i<frames;i++) analyzer_push(0,buffer[i*2],buffer[i*2+1]);
 212:     for(unsigned i=callback_taps;callback_bus>=0 && i<frames;i++) analyzer_push(1,0,0);
 213:     int active=0;
 214:     for(int id=0;id<=INSERTS;id++) for(int side=0;side<2;side++) {
 216:         active|=peak>.00001f;
```

### publish_view

```c
 227:     memset(view.pattern_notes,0,sizeof view.pattern_notes);
 228:     if(playing) for(int i=0;i<128;i++) {
 230:         if(v->gain>0 && v->note_id>0 && v->note_id<=NOTES && v->pattern>=0 && v->pattern<PATTERNS && v->channel>=0 && v->channel<CHANNELS && player.channel_active[v->channel] && (v->lane<0 || (v->lane<LANES && player.lane_active[v->lane])))
 231:             view.pattern_notes[v->pattern][v->channel][v->note_id-1]=1;
 236:     for(int i=0;i<128;i++) {
 238:         view.voices[i]=(VoiceView){.channel=v->channel,.gain=v->gain,.sampler={v->instrument==INSTRUMENT_FM?0:v->sampler.position,v->instrument==INSTRUMENT_FM?1:v->sampler.speed}};
 240:     double pitch_speed=pow(2,project.master_pitch/12.0);
 241:     for(int c=0;c<CHANNELS;c++) {
 243:         view.speeds[c]=pitch_speed*channel_speed(&project,c)*(project.sampler[c].fit_bpm?project.bpm/project.sampler[c].fit_bpm:1);
```

### consume_commands

```c
 256:             memcmp(project.audio_seconds,next->audio_seconds,sizeof project.audio_seconds)) player.audio_resync=1;
 260:     pending.dirty=0;
 261:     uint64_t serial=0;
 269:                 click_position=1200; player_reset(&player); player.effects=effects; player_seek(&player,&project,c->start);
 270:                 atomic_store(&position,player.frame); visual_frame=player.frame; visual_frames=0;
 273:             player.loop_start=isfinite(c->loop_start)?fmaxf(0,c->loop_start):0;
 274:             player.loop_end=isfinite(c->loop_end) && c->loop_end>player.loop_start?c->loop_end:0;
 275:             if(playing && player.loop_end && player.frame*(player.clock_bpm?player.clock_bpm/project.bpm:1)>=(uint64_t)llround(player.loop_end*RATE*60.0/project.bpm/4)) {
 276:                 player.frame=(uint64_t)llround(player.loop_start*RATE*60.0/project.bpm/4); player.clock_bpm=project.bpm;
 277:                 player.last_step=-1; memset(player.voices,0,sizeof player.voices);
 281:             for(int channel=0;channel<CHANNELS;channel++) {
 284:                     for(int v=0;v<128;v++) {
 285:                         if(player.voices[v].channel==channel) player.voices[v].gain=0;
 286:                         if(live.voices[v].channel==channel) live.voices[v].gain=0;
 288:                     samples[channel]=next; player.audio_resync=1;
 291:             preview=(Sample){0}; preview_end=preview_remaining=0; preview_channel=-1;
 294:             preview_channel=-1;
 295:             preview=c->sample; preview_position=preview_frame=0; preview_time=monotonic_time();
 296:             preview_speed=preview_rate=1; preview_gain=.7f; preview_end=preview.frames; preview_remaining=5*RATE;
 299:             if(c->channel>=0 && c->channel<project.channel_count) {
 301:                     Voice *voice=&live.voices[127]; uint32_t phase[6];
 302:                     int retain=voice->instrument==INSTRUMENT_FM && voice->channel==c->channel && voice->fm.engine==1 && project.fm[c->channel].engine==1 && !project.fm[c->channel].dx7.value[136];
 303:                     if(retain) memcpy(phase,voice->fm.dx7.phase,sizeof phase);
 304:                     *voice=(Voice){.channel=c->channel,.instrument=INSTRUMENT_FM,.remaining=(c->note.length?c->note.length:1)*RATE*15/project.bpm,.gain=c->note.velocity/127.f,.lane=-1};
 305:                     instrument_start(&voice->device,voice->instrument,(InstrumentSettings){&project.sampler[c->channel],&project.fm[c->channel]},(InstrumentNote){.frequency=440*pow(2,(c->note.pitch-69)/12.0),.velocity=c->note.velocity/127.f});
 306:                     if(retain) memcpy(voice->fm.dx7.phase,phase,sizeof phase);
 307:                     atomic_store(&channel_trigger[c->channel],1); break;
 309:                 preview=samples[c->channel]; preview_position=preview_frame=0; preview_time=monotonic_time();
 310:                 preview_remaining=UINT_MAX; preview_speed=pow(2,((int)c->note.pitch-60)/12.0)*channel_speed(&project,c->channel);
 311:                 preview_rate=preview_speed*pow(2,project.master_pitch/12.0);
 312:                 preview_gain=c->note.velocity/127.f*project.volume[c->channel]*project.master;
 313:                 preview_end=fmin(preview.frames,preview_speed*RATE*60.0/project.bpm/4*(c->note.length?c->note.length:1));
 315:                 if(preview_end && preview_gain>0) atomic_store(&channel_trigger[c->channel],1);
 319:             if(c->down && c->channel>=0 && c->channel<project.channel_count) {
 320:                 if((samples[c->channel].frames || project.instrument[c->channel]==INSTRUMENT_FM) && project.volume[c->channel]>0 && !(project.mute[c->channel]&1)) atomic_store(&channel_trigger[c->channel],1);
 321:                 uint32_t phase[6]; Voice *old=&live.voices[c->slot];
 322:                 int retain=old->instrument==INSTRUMENT_FM && old->channel==c->channel && old->fm.engine==1 && project.fm[c->channel].engine==1 && !project.fm[c->channel].dx7.value[136];
 323:                 if(retain) memcpy(phase,old->fm.dx7.phase,sizeof phase);
 324:                 live.voices[c->slot]=(Voice){.channel=c->channel,.sampler={.speed=pow(2,(c->pitch-60)/12.0)},.remaining=-1,.gain=c->note.velocity/127.f,.lane=-1};
 327:                     instrument_start(&live.voices[c->slot].device,INSTRUMENT_FM,(InstrumentSettings){&project.sampler[c->channel],&project.fm[c->channel]},(InstrumentNote){.frequency=440*pow(2,(c->pitch-69)/12.0),.velocity=c->note.velocity/127.f});
 328:                     if(retain) memcpy(live.voices[c->slot].fm.dx7.phase,phase,sizeof phase);
 333:                 else voice->remaining=RATE*.005;
 338:             for(int i=0;i<128;i++) active|=live.voices[i].gain!=0;
 340:             playing=0; preview=(Sample){0}; preview_end=preview_remaining=0; preview_channel=-1; click_position=1200;
 341:             memset(player.voices,0,sizeof player.voices); memset(live.voices,0,sizeof live.voices); effects_reset(effects);
```

### wait_control

```c
 355:     struct timespec pause={0,100000}; nanosleep(&pause,NULL);
```

### consume_stopped

```c
 359:     if(!ready) { consume_commands(); publish_view(); return 1; }
 360:     if(ma_device_get_state(&device)!=ma_device_state_stopped) return 0;
```

### submit

```c
 368: static uint64_t submit(AudioCommand command) {
```

### wait_acknowledged

```c
 378: static void wait_acknowledged(uint64_t serial) {
```

### audio_start

```c
 386:     for(int accent=0;accent<2;accent++) for(int i=0;i<1200;i++)
 387:         clicks[accent][i]=.25f*sinf(6.2831853f*(accent?1800:1200)*i/RATE)*expf(-i/180.f);
 388:     memset(&pending,0,sizeof pending); memset(&view,0,sizeof view);
 389:     atomic_store(&acknowledged,0); atomic_store(&position,0);
 392:     effects=effects_create(INSERTS+1); if(!effects) return 0;
 393:     project_snapshot_update(&pending.snapshot,p,1,NULL); playback=pending.snapshot.plan;
 395:     player.effects=live.effects=effects; playing=0; preview=(Sample){0}; preview_end=preview_remaining=0; preview_channel=-1;
 396:     visual_frames=live_frames=0; output_volume=1; publish_view();
 398:     config.playback.format=ma_format_f32; config.playback.channels=2;
 400:     if(ma_device_init(NULL,&config,&device)!=MA_SUCCESS) { effects_free(effects); effects=NULL; player.effects=live.effects=NULL; return 0; }
 401:     if(ma_device_start(&device)!=MA_SUCCESS) { ma_device_uninit(&device); effects_free(effects); effects=NULL; player.effects=live.effects=NULL; return 0; }
 402:     ready=1; return 1;
```

### audio_devices

```c
 404: int audio_devices(int capture,char names[][128],int capacity) {
 405:     if(!ready || capacity<=0) return 0;
 406:     ma_device_info *outputs,*inputs; ma_uint32 output_count,input_count;
 407:     if(ma_context_get_devices(device.pContext,&outputs,&output_count,&inputs,&input_count)!=MA_SUCCESS) return 0;
 410:     for(int i=0;i<n;i++) snprintf(names[i],128,"%s",devices[i].name);
```

### audio_record_end

```c
 416:     ma_device_stop(&device); recording.active=0;
 417:     for(int i=0;i<recording.devices_count;i++) ma_device_uninit(&recording.devices[i]);
 418:     if(ma_device_start(&device)!=MA_SUCCESS) ready=0;
```

### record_release

```c
 421:     for(int i=0;i<CHANNELS;i++) { free(recording.inputs[i].pcm); free(recording.takes[i].pcm); }
 422:     memset(&recording,0,sizeof recording);
```

### audio_record_start

```c
 424: int audio_record_start(const Project *p,const int buses[],int count,float start_step,float output,char error[256]) {
 425:     if(!ready || count<1 || count>CHANNELS || recording.active) { snprintf(error,256,"Audio unavailable or invalid recording tracks."); return 0; }
 427:     ma_device_info *outputs,*inputs; ma_uint32 output_count,input_count;
 429:     char names[CHANNELS][128]={{0}};
 430:     for(int i=0;i<count;i++) {
 431:         int bus=buses[i]; if(bus<0 || bus>p->insert_count) goto failed;
 432:         recording.buses[i]=bus; recording.sources[i]=-1; recording.io.active[bus]=1;
 433:         recording.takes[i].pcm=calloc(RECORD_RING_FRAMES*2,sizeof(float));
 435:         const char *name=p->audio_io[bus][0]; if(!*name) continue;
 436:         int source=0; while(source<recording.devices_count && strcmp(names[source],name)) source++;
 441:             for(unsigned n=0;n<input_count;n++) if(!strcmp(inputs[n].name,name)) { id=&inputs[n].id; break; }
 442:             if(!id) { snprintf(error,256,"Input device unavailable: %.120s",name); goto failed; }
 444:         RecordRing *ring=&recording.inputs[source]; ring->pcm=calloc(RECORD_RING_FRAMES*2,sizeof(float));
 447:         config.capture.pDeviceID=id; config.capture.format=ma_format_f32; config.capture.channels=2;
 450:             snprintf(error,256,"Cannot open input %.120s. Check microphone permission and device availability.",name); goto failed;
 452:         recording.devices_count++; snprintf(names[source],128,"%s",name);
 455:     for(int i=0;i<recording.devices_count;i++) if(ma_device_start(&recording.devices[i])!=MA_SUCCESS) goto failed;
 457:     recording.active=1;
 458:     audio_update(p,1,1,player.pattern,1,output,start_step,0,0);
 459:     if(ma_device_start(&device)==MA_SUCCESS) return 1;
 460:     recording.active=0;
 462:     if(!*error) snprintf(error,256,"Could not start recording: device or memory unavailable.");
 463:     for(int i=0;i<recording.devices_count;i++) ma_device_uninit(&recording.devices[i]);
 464:     record_release(); if(ma_device_start(&device)!=MA_SUCCESS) ready=0; return 0;
```

### audio_record_read

```c
 467:     if(take<0 || take>=recording.count) return 0;
 468:     unsigned n=0; while(n<frames && ring_pop(&recording.takes[take],stereo+n*2)) n++;
```

### audio_record_failed

```c
 473:         if(ma_device_get_state(&device)!=ma_device_state_started) return 1;
 474:         for(int i=0;i<recording.devices_count;i++) if(ma_device_get_state(&recording.devices[i])!=ma_device_state_started) return 1;
 476:     for(int i=0;i<recording.count;i++) if(atomic_load(&recording.takes[i].overflow)) return 1;
 477:     for(int i=0;i<recording.devices_count;i++) if(atomic_load(&recording.inputs[i].overflow)) return 1;
 478:     return 0;
```

### audio_sample

```c
 488:     if(c<0 || c>=CHANNELS) return;
 490:     uint64_t serial=submit((AudioCommand){.kind=SAMPLE,.channel=c,.sample=s});
```

### audio_channels

```c
 496:     uint64_t serial=submit(command);
```

### audio_preview

```c
 501:     uint64_t serial=submit((AudioCommand){.kind=PREVIEW,.sample=s});
```

### audio_key

```c
 507: void audio_key(int slot,int channel,int pitch,int down) { audio_key_velocity(slot,channel,pitch,down?100:0); }
```

### audio_key_velocity

```c
 509:     if(slot<0 || slot>=128 || pitch<0 || pitch>127) return;
 511:     submit((AudioCommand){.kind=KEY,.slot=slot,.channel=channel,.pitch=pitch,.down=velocity>0,.note={.velocity=velocity>0?(velocity>127?127:velocity):0}});
```

### audio_stop

```c
 515:     pthread_mutex_lock(&mutex); uint64_t serial=submit((AudioCommand){.kind=STOP}); pthread_mutex_unlock(&mutex);
```

### audio_preview_position

```c
 520:     double progress=-1;
 522:         double frame=fmax(0,fmin(view.preview_position,view.preview_frame+(monotonic_time()-view.preview_time)*RATE*view.preview_rate));
 523:         if(frame<view.preview_end) progress=fmin(1,frame/sample.frames);
```

### audio_key_position

```c
 528:     if(slot<0 || slot>=128 || channel<0 || channel>=CHANNELS) return -1;
 530:     VoiceView v=view.voices[slot]; double progress=-1;
 531:     int queued=0;
 535:         if(c->kind==STOP || c->kind==SAMPLE || c->kind==CHANNEL_SAMPLES) v.gain=0;
 537:             if(c->down && c->channel>=0 && c->channel<pending.snapshot.project.channel_count) {
 538:                 v=(VoiceView){.channel=c->channel,.gain=1}; queued=1;
 542:     if(v.gain && v.channel==channel && view.instrument[channel]==INSTRUMENT_FM) progress=0;
 544:         double behind=fmax(0,view.live_frames-(monotonic_time()-view.live_time)*RATE);
 545:         progress=queued?0:fmin(1,fmax(0,v.sampler.position-behind*v.sampler.speed*view.speeds[channel])/view.sample_frames[channel]);
```

### audio_position

```c
 549: uint64_t audio_position(void) { return atomic_load(&position); }
```

### audio_meters

```c
 550: void audio_meters(float peaks[INSERTS+1][2]) {
 551:     for(int id=0;id<=INSERTS;id++) for(int side=0;side<2;side++)
 552:         peaks[id][side]=atomic_exchange_explicit(&meter_peak[id][side],0,memory_order_relaxed);
```

### audio_channel_activity

```c
 554: void audio_channel_activity(uint8_t active[CHANNELS],uint8_t triggered[CHANNELS]) {
 555:     for(int c=0;c<CHANNELS;c++) {
 557:         triggered[c]=atomic_exchange_explicit(&channel_trigger[c],0,memory_order_relaxed);
```

### audio_pattern_activity

```c
 560: void audio_pattern_activity(int pattern,uint8_t notes[CHANNELS][NOTES]) {
 562:     if(pattern>=0 && pattern<PATTERNS) memcpy(notes,view.pattern_notes[pattern],CHANNELS*NOTES);
 563:     else memset(notes,0,CHANNELS*NOTES);
```

### audio_track_activity

```c
 566: void audio_track_activity(uint8_t active[LANES],uint8_t triggered[LANES]) {
 567:     for(int l=0;l<LANES;l++) {
 569:         triggered[l]=atomic_exchange_explicit(&track_trigger[l],0,memory_order_relaxed);
```

### audio_visual_position

```c
 578:         frame+=fmin(view.visual_frames,fmax(0,(monotonic_time()-view.visual_time)*RATE));
```

### audio_close

```c
 587:     ready=0;
```

### sample_load

```c
 592:     if(sample_map_wav(path,s)) return 1;
 594:     ma_decoder_config config=ma_decoder_config_init(ma_format_f32,0,RATE);
 595:     if(ma_decoder_init_file(path,&config,&decoder)!=MA_SUCCESS) return 0;
 597:     ma_uint64 frames=0,read=0;
 598:     int ok=ma_decoder_get_length_in_pcm_frames(&decoder,&frames)==MA_SUCCESS && frames>0 && frames<=SAMPLE_MAX_FRAMES && (channels==1 || channels==2) && frames<=SIZE_MAX/(channels*sizeof(float));
 602:     if(!ok) { free(data); return 0; }
 603:     *s=(Sample){.data=data,.frames=(unsigned)frames,.channels=channels}; return 1;
```

## audio_timing.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### audio_timeline_init

```c
   9:     map->project=p; map->channel=channel; map->count=0;
  10:     unsigned relevant=0;
  11:     for(int a=0;a<p->automation_count;a++) {
  13:         if(t.parameter==PARAM_MASTER_PITCH || (t.owner==(unsigned)channel && (t.parameter==PARAM_CHANNEL_PITCH || t.parameter==PARAM_PITCH_RANGE))) relevant|=1u<<a;
  17:     for(int l=0;l<LANES;l++) if(!(p->lane_mute[l]&1) && (!solo || (p->lane_mute[l]&2))) for(int b=0;b<CLIPS;b++) {
  18:         int a=p->clips[l][b]-AUTOMATION_SOURCE-1;
  19:         if(a>=0 && a<p->automation_count && (relevant&(1u<<a))) map->clips[map->count++]=l*CLIPS+b;
```

### segment

```c
  24:     double values[3]={p->channel_pitch[c],p->pitch_range[c],p->master_pitch},slopes[3]={0},next=INFINITY,held_end[3]={-INFINITY,-INFINITY,-INFINITY};
  25:     int active[3]={0};
  26:     for(unsigned n=0;n<map->count;n++) {
  28:         const Automation *a=&p->automations[p->clips[l][b]-AUTOMATION_SOURCE-1];
  33:         int target=a->target.parameter==PARAM_CHANNEL_PITCH?0:a->target.parameter==PARAM_PITCH_RANGE?1:2;
  36:                 double lo=target==0?-1:target==1?1:-12,range=target==0?2:target==1?47:24;
  37:                 values[target]=lo+range*automation_value(a,end-start+p->clip_offsets[l][b]); slopes[target]=0; held_end[target]=end;
  41:         active[target]=1;
  42:         double local=position-start+p->clip_offsets[l][b],value=a->points[a->count-1].value,gradient=0;
  43:         if(local<a->points[0].step) { value=a->points[0].value; next=fmin(next,start+a->points[0].step-p->clip_offsets[l][b]); }
  44:         else for(int i=1;i<a->count;i++) if(local<a->points[i].step) {
  45:             AutomationPoint x=a->points[i-1],y=a->points[i];
  49:         double lo=target==0?-1:target==1?1:-12,range=target==0?2:target==1?47:24;
  53:     double range=floor(values[1]+.5+ (slopes[1]<0?-1e-9:slopes[1]>0?1e-9:0));
  54:     if(slopes[1]) {
  55:         double boundary=range+(slopes[1]>0?.5:-.5),delta=(boundary-values[1])/slopes[1];
  56:         if(delta>1e-10) next=fmin(next,position+delta);
  58:     *speed=exp2((values[0]*range+values[2])/12);
  59:     *slope=(slopes[0]*range+slopes[2])*log(2)/12;
```

### integral

```c
  63:     return fabs(slope)<1e-12?speed*duration:speed*expm1(slope*duration)/slope;
```

### audio_timeline_source

```c
  66:     double consumed=0;
```

### audio_timeline_duration

```c
  76:     while(source_steps>0) {
  81:         if(source_steps<=consumed) return start-origin+(fabs(slope)<1e-12?source_steps/speed:log1p(source_steps*slope/speed)/slope);
```

### audio_clip_steps

```c
  87:     int c=p->clips[lane][clip]-PATTERNS-1;
  88:     if(p->clip_steps[lane][clip]) return p->clip_steps[lane][clip]*audio_source_bpm(p,c)/15;
  89:     double remaining=fmax(0,p->audio_seconds[c]-p->clip_offsets[lane][clip])*audio_source_bpm(p,c)/15;
```

### audio_clip_position

```c
  94:     int c=p->clips[lane][clip]-PATTERNS-1;
  97:     return (p->clip_offsets[lane][clip]+audio_timeline_source(&map,start,fmax(start,step))*15/audio_source_bpm(p,c))*RATE;
```

## automation.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### automation_value

```c
   7:     if(!a->count) return 0;
   8:     if(step<a->points[0].step) return a->points[0].value;
   9:     for(int i=1;i<a->count;i++) if(step<a->points[i].step) {
  10:         AutomationPoint x=a->points[i-1],y=a->points[i];
  13:     return a->points[a->count-1].value;
```

### automation_evaluate

```c
  16:     int active=0,solo=solo_any(p->lane_mute,LANES); float held_end=-INFINITY;
  17:     for(int l=0;l<LANES;l++) if(!(p->lane_mute[l]&1) && (!solo || (p->lane_mute[l]&2))) for(int b=0;b<CLIPS;b++) {
  18:         int a=p->clips[l][b]-AUTOMATION_SOURCE-1;
  19:         if(a<0 || a>=p->automation_count) continue;
  24:         if(step>=start && step<end) { *normalized=automation_value(curve,step-start+p->clip_offsets[l][b]); active=2; }
  25:         else if(step>=end && active!=2 && end>=held_end) { *normalized=automation_value(curve,end-start+p->clip_offsets[l][b]); held_end=end; active=1; }
```

### automation_point

```c
  30:     if(!isfinite(step) || !isfinite(value) || step<0 || step>a->steps || value<0 || value>1) return -1;
  31:     int i=0; while(i<a->count && a->points[i].step<step) i++;
  32:     if(i<a->count && fabsf(a->points[i].step-step)<.00001f) { a->points[i].value=value; return i; }
  33:     if(a->count>=AUTOMATION_POINTS) return -1;
  34:     memmove(&a->points[i+1],&a->points[i],(a->count-i)*sizeof a->points[0]); a->count++;
```

### automation_move_point

```c
  38:     if(index<0 || index>=p->automation_count || !isfinite(step) || fabsf(step)>1e14f || !isfinite(value) || value<0 || value>1) return -1;
  40:     if(point<0 || point>=a->count) return -1;
  41:     if(point>0) step=fmaxf(a->points[point-1].step,step);
  42:     if(point<a->count-1) step=fminf(a->points[point+1].step,step);
  43:     if(step<0) {
  45:         for(int n=0;n<a->count;n++) a->points[n].step+=shift;
  48:         for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(p->clips[l][b]==AUTOMATION_SOURCE+index+1) p->clip_offsets[l][b]+=shift;
  49:         step=0;
```

### automation_create

```c
  56:     if(p->automation_count<0 || p->automation_count>=AUTOMATIONS || !isfinite(steps) || steps<=0 || !parameter_info(p,target,&v,&lo,&hi)) return -1;
  57:     int index=p->automation_count++; Automation *a=&p->automations[index]; memset(a,0,sizeof *a);
  59:     for(int color=0;color<PATTERNS;color++) {
  60:         int used=0; for(int i=0;i<index;i++) if(p->automations[i].color==pattern_palette[color]) used=1;
  63:     snprintf(a->name,sizeof a->name,"%s",name); float value=fmaxf(0,fminf(1,(v-lo)/(hi-lo)));
  64:     automation_point(a,0,value); automation_point(a,steps,value); return index;
```

### automation_delete

```c
  67:     if(index<0 || index>=p->automation_count) return 0;
  68:     for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) {
  69:         int s=p->clips[l][b]-1;
  70:         if(s==AUTOMATION_SOURCE+index) { p->clips[l][b]=0; p->clip_steps[l][b]=0; }
  73:     memmove(&p->automations[index],&p->automations[index+1],(--p->automation_count-index)*sizeof p->automations[0]);
  74:     memset(&p->automations[p->automation_count],0,sizeof p->automations[0]); return 1;
```

### automation_valid

```c
  77:     if(p->automation_count<0 || p->automation_count>AUTOMATIONS) return 0;
  78:     for(int i=0;i<p->automation_count;i++) {
  80:         if((a->target.parameter<PARAM_PLUGIN && a->target.slot) || (((a->target.parameter>=PARAM_MASTER_VOLUME && a->target.parameter<=PARAM_MASTER_PITCH) || a->target.parameter==PARAM_MASTER_MUTE || a->target.parameter==PARAM_SWING) && a->target.owner) || !a->target.parameter || (!parameter_info(p,a->target,&v,&lo,&hi) && a->target.parameter<PARAM_PLUGIN) || a->target.slot>65535 || a->target.owner>65535 || a->color>0xffffff || !a->name[0] || strnlen(a->name,sizeof a->name)==sizeof a->name || strchr(a->name,'\n') || strchr(a->name,'\r') || !isfinite(a->steps) || a->steps<=0 || a->steps>1e15f || a->count<1 || a->count>AUTOMATION_POINTS) return 0;
  81:         for(int n=0;n<a->count;n++) if(!isfinite(a->points[n].step) || a->points[n].step<0 || a->points[n].step>a->steps || (n && a->points[n].step<a->points[n-1].step) || !isfinite(a->points[n].value) || a->points[n].value<0 || a->points[n].value>1) return 0;
  83:     return 1;
```

## browser.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### browser_select

```c
  12:     if(entry<0 || entry>=b->items) return;
  13:     b->selected=entry; b->follow=1;
```

### browser_toggle

```c
  18:     if(entry<0 || entry>=b->items || !b->nodes[entry].dir) return 0;
  21:         int end=entry+1;
  23:         int removed=end-entry-1;
  26:         memmove(b->nodes+entry+1,b->nodes+end,(b->items-end)*sizeof *b->nodes);
  27:         b->items-=removed; b->nodes[entry].open=0; return 1;
  29:     if(depth>=64 || !DirectoryExists(b->nodes[entry].path)) return 0;
  31:     BrowserNode *children=calloc(files.count?files.count:1,sizeof *children); int n=0,ok=children!=NULL;
  32:     for(unsigned i=0;ok && i<files.count;i++) {
  34:         if(!dir && !IsFileExtension(files.paths[i],".wav;.flac;.mp3;" PROJECT_FILE_SUFFIX)) continue;
  35:         char *path=strdup(files.paths[i]); if(!path) { ok=0; break; }
  36:         children[n++]=(BrowserNode){path,depth+1,dir,0};
  39:     if(b->items+n>10000) ok=0;
  41:     if(!nodes) { for(int i=0;i<n;i++) free(children[i].path); free(children); return 0; }
  43:     memmove(nodes+entry+1+n,nodes+entry+1,(b->items-entry-1)*sizeof *nodes);
  44:     memcpy(nodes+entry+1,children,n*sizeof *children); free(children);
  45:     b->items+=n; nodes[entry].open=1;
  46:     if(b->selected>entry) { b->selected+=n; b->follow=1; }
  47:     return 1;
```

### browser_left

```c
  50:     int i=b->selected; if(i<0 || i>=b->items) return;
  53:     while(--i>=0) if(b->nodes[i].depth<depth) { browser_select(b,i); break; }
```

### browser_right

```c
  56:     int i=b->selected; if(i<0 || i>=b->items || !b->nodes[i].dir) return;
  58:     else if(i+1<b->items && b->nodes[i+1].depth>b->nodes[i].depth) browser_select(b,i+1);
```

### root_add

```c
  61:     char *copy=strdup(path); if(!copy) return -1;
  62:     BrowserNode *nodes=realloc(b->nodes,(b->items+1)*sizeof *nodes);
  63:     if(!nodes) { free(copy); return -1; }
  64:     b->nodes=nodes; int entry=b->items++; nodes[entry]=(BrowserNode){copy,0,1,0}; return entry;
```

### browser_init

```c
  67:     memset(b,0,sizeof *b); b->selected=-1;
  71:     else base[0]=0;
  72:     if(base[0] && snprintf(dir,sizeof dir,"%s/libreloop",base)<(int)sizeof dir) {
  74:         if(snprintf(b->config,sizeof b->config,"%s/folders.txt",dir)>=(int)sizeof b->config) b->config[0]=0;
  77:     root_add(b,b->samples); browser_select(b,0); browser_toggle(b,0);
  78:     FILE *f=b->config[0]?fopen(b->config,"r"):NULL;
  79:     if(!f && base[0] && snprintf(dir,sizeof dir,"%s/homebeat/folders.txt",base)<(int)sizeof dir) f=fopen(dir,"r");
  83:             char *newline=strchr(line,'\n'); if(!newline) continue; *newline=0;
  84:             if(*line && strcmp(line,b->samples) && root_add(b,line)>=0) snprintf(b->folders[b->count++],PATH_MAX,"%s",line);
```

### browser_add

```c
  91:     if(!realpath(path,full) || !DirectoryExists(full) || strchr(full,'\n')) return 0;
  92:     for(int i=0;i<b->items;i++) if(b->nodes[i].depth==0 && !strcmp(full,b->nodes[i].path)) {
  93:         browser_select(b,i); if(!b->nodes[i].open) browser_toggle(b,i); return 1;
  95:     if(b->count==FOLDERS || !b->config[0]) return 0;
  96:     FILE *f=fopen(b->config,"w"); if(!f) return 0;
  97:     for(int i=0;i<b->count;i++) fprintf(f,"%s\n",b->folders[i]);
  98:     fprintf(f,"%s\n",full); int ok=!ferror(f); if(fclose(f)) ok=0;
  99:     if(!ok) return 0;
 100:     int entry=root_add(b,full); if(entry<0) return 0;
 102:     browser_select(b,entry); browser_toggle(b,entry); return 1;
```

### browser_close

```c
 105:     for(int i=0;i<b->items;i++) free(b->nodes[i].path);
 106:     free(b->nodes); b->nodes=NULL; b->items=0;
```

## edit_clipboard.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### clipboard_copy_notes

```c
   5: int clipboard_copy_notes(EditClipboard *c,const Project *p,int pat,int ch,const uint8_t selected[NOTES]) {
   6:     if(pat<0 || pat>=p->pattern_count || ch<0 || ch>=p->channel_count) return 0;
   7:     int count=0; float start=INFINITY;
   8:     for(int i=0;i<NOTES;i++) if(selected[i] && p->notes[pat][ch][i].velocity) { count++; start=fminf(start,p->notes[pat][ch][i].start); }
   9:     if(!count) return 0;
  10:     c->kind=COPY_NOTES; c->count=0;
  11:     for(int i=0;i<NOTES;i++) if(selected[i] && p->notes[pat][ch][i].velocity) { Note n=p->notes[pat][ch][i]; n.start-=start; c->notes[c->count++]=n; }
```

### clipboard_copy_clips

```c
  14: int clipboard_copy_clips(EditClipboard *c,const Project *p,const uint8_t selected[LANES][CLIPS]) {
  15:     int count=0,lane=LANES; float start=INFINITY;
  16:     for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(selected[l][b] && p->clips[l][b]) { count++; if(l<lane) lane=l; start=fminf(start,p->clip_starts[l][b]); }
  17:     if(!count) return 0;
  18:     c->kind=COPY_CLIPS; c->count=0; c->lane=lane; c->channels=p->channel_count; c->patterns=p->pattern_count; c->automations=p->automation_count;
  19:     for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(selected[l][b] && p->clips[l][b])
```

### clipboard_paste_notes

```c
  23: int clipboard_paste_notes(const EditClipboard *c,Project *p,int pat,int ch,float start,uint8_t selected[NOTES]) {
  24:     if(c->kind!=COPY_NOTES || !c->count) return 0;
  25:     if(pat<0 || pat>=p->pattern_count || ch<0 || ch>=p->channel_count || !isfinite(start) || start<0) return -4;
  26:     int free=0; float end=p->pattern_steps[pat];
  27:     for(int i=0;i<NOTES;i++) free+=!p->notes[pat][ch][i].velocity;
  28:     if(free<c->count) return -2;
  29:     for(int i=0;i<c->count;i++) {
  31:         if(!isfinite(at) || at>1e12f) return -4;
  32:         float finish=at+(n->length?n->length:1); if(!isfinite(finish) || finish<=at) return -4;
  35:     p->pattern_steps[pat]=ceilf(end/STEPS)*STEPS; memset(selected,0,NOTES);
  36:     for(int i=0,slot=0;i<c->count;i++) {
  38:         Note n=c->notes[i]; n.start+=start; p->notes[pat][ch][slot]=n; selected[slot++]=1;
```

### clipboard_paste_clips

```c
  42: int clipboard_paste_clips(const EditClipboard *c,Project *p,int lane,float bar,uint8_t selected[LANES][CLIPS]) {
  43:     if(c->kind!=COPY_CLIPS || !c->count) return 0;
  44:     if(c->channels!=p->channel_count || c->patterns!=p->pattern_count || c->automations!=p->automation_count) return -1;
  45:     int needed[LANES]={0},free[LANES]={0};
  46:     for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) free[l]+=!p->clips[l][b];
  47:     for(int i=0;i<c->count;i++) {
  48:         CopiedClip n=c->clips[i]; int l=lane+n.lane,source=n.source-1; float at=bar+n.start;
  49:         if(l<0 || l>=LANES || at<0 || !isfinite(at) || at>1e12f) return -4;
  50:         if(source<PATTERNS ? source>=p->pattern_count : source<AUTOMATION_SOURCE ? source-PATTERNS>=p->channel_count || !p->channel_audio[source-PATTERNS] : source-AUTOMATION_SOURCE>=p->automation_count) return -1;
  51:         if(++needed[l]>free[l]) return -2;
  53:         int empty=0; while(empty<CLIPS && p->clips[l][empty]) empty++;
  54:         uint8_t saved=p->clips[l][empty]; float saved_length=p->clip_steps[l][empty],saved_offset=p->clip_offsets[l][empty],saved_start=p->clip_starts[l][empty];
  58:         for(int b=0;b<CLIPS;b++) if(p->clips[l][b] && at<p->clip_starts[l][b]+clip_length(p,l,b)/STEPS-.00001f && p->clip_starts[l][b]<at+length-.00001f) return -3;
  60:     memset(selected,0,LANES*CLIPS);
  61:     for(int i=0;i<c->count;i++) {
  62:         CopiedClip n=c->clips[i]; int l=lane+n.lane,b=0; while(p->clips[l][b]) b++;
  63:         p->clips[l][b]=n.source; p->clip_starts[l][b]=bar+n.start; p->clip_steps[l][b]=n.length; p->clip_offsets[l][b]=n.offset; selected[l][b]=1;
```

## engine.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
   9: // Twelve vivid rainbow hues, paired at OKLCH lightness 0.68 and 0.57.
  10: // Chroma 0.11 keeps source colors soft so selected controls stand out.
  11: // Regenerate with: python3 tools/audit_palette.py --generate-clips
  12: const uint32_t pattern_palette[COLOR_COUNT]={
  13:     0xa388d2,0xbf7fb9,0xcf7a95,0xd37d6f,0xca874e,0xb3953f,0x90a250,0x63ab74,0x32ae9b,0x25aabe,0x51a1d5,0x7e94dc,
  14:     0x8267ae,0x9b5e96,0xab5a75,0xaf5c50,0xa6662b,0x917415,0x6f812d,0x418954,0x008b7a,0x008797,0x2c7fb1,0x5e73b8,
```

### next_source_color

```c
  16: uint32_t next_source_color(const uint32_t *colors,int count) {
  17:     for(int color=0;color<COLOR_COUNT;color++) {
  18:         int used=0;
  19:         for(int i=0;i<count;i++) if(colors[i]==pattern_palette[color]) used=1;
```

### grid_interval

```c
  26:     return grid.lines>0?grid.lines:grid.labels;
```

### snap_floor

```c
  28: float snap_floor(float value,float interval) { return interval>0?floorf(value/interval)*interval:value; }
```

### snap_round

```c
  29: float snap_round(float value,float interval) { return interval>0?roundf(value/interval)*interval:value; }
```

### timeline_zoom

```c
  32:     float next=*span*expf(-wheel*logf(1.08f));
  33:     if(!isfinite(next) || next<=0 || !isfinite(1/next)) return;
  34:     *start=fmaxf(0,*start+anchor*(*span-next)); *span=next;
```

### timeline_grid_layout

```c
  37:     TimelineGrid grid={0,STEPS,0};
  38:     if(!isfinite(pixels) || pixels<=0) return grid;
  40:     while(grid.labels*pixels+.001f<30 && grid.labels<1e30f) grid.labels*=2;
  41:     if(bar+.001f>=12) {
  42:         grid.lines=.25f;
  43:         while(grid.lines*pixels+.001f>=32) grid.lines*=.5f;
  44:         while(grid.lines*pixels+.001f<16 && grid.lines<STEPS) grid.lines*=2;
  47:     grid.band_alpha=.09f*fminf(1,fmaxf(0,(bar-2)/2));
```

### timeline_thumb

```c
  50: float timeline_thumb(float width,float span,float range) { return fminf(width,fmaxf(24,width*span/range)); }
```

### gain_db

```c
  51: float gain_db(float gain) { return gain>0?20*log10f(gain):-INFINITY; }
```

### fader_position

```c
  54:     if(gain<=0) return 0;
  56:     return fmaxf(0,fminf(1,gain<=1?.75f*(db+60)/60:.75f+.25f*db/gain_db(MIXER_GAIN_MAX)));
```

### fader_gain

```c
  59:     if(position<=0) return 0;
  60:     position=fminf(1,position);
  61:     float db=position<=.75f?60*(position/.75f-1):(position-.75f)/.25f*gain_db(MIXER_GAIN_MAX);
  62:     return fminf(MIXER_GAIN_MAX,powf(10,db/20));
```

### project_default

```c
  65:     memset(p, 0, sizeof *p); p->bpm=120; p->master=1; p->master_width=1; p->pattern_count=1; p->channel_count=4; snprintf(p->audio_io[0][1],128,"@default");
  66:     for(int pat=0;pat<PATTERNS;pat++) { p->pattern_colors[pat]=pattern_palette[pat]; p->pattern_steps[pat]=STEPS; snprintf(p->pattern_names[pat],PATTERN_NAME,"Pattern %d",pat+1); }
  67:     for(int id=0;id<=INSERTS;id++) for(int slot=0;slot<10;slot++) { p->effect_mix[id][slot]=1; device_descriptor(DEVICE_CHORUS)->defaults(&p->chorus[id][slot]); device_descriptor(DEVICE_EQ)->defaults(&p->eq[id][slot]); }
  68:     for(int l=0;l<LANES;l++) snprintf(p->track_names[l],PATTERN_NAME,"Track %d",l+1);
  70:     for(int c=0;c<CHANNELS;c++) { device_descriptor(DEVICE_FM)->defaults(&p->fm[c]); p->pitch_range[c]=2; p->volume[c]=1; p->sampler[c].time=p->sampler[c].length=1; if(c<4) snprintf(p->channel_names[c],PATTERN_NAME,"%s",names[c]); else snprintf(p->channel_names[c],PATTERN_NAME,"Channel %d",c+1); }
  71:     for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) p->clip_starts[l][b]=b;
  72:     for(int i=0;i<INSERTS;i++) snprintf(p->insert_names[i],PATTERN_NAME,"Insert %d",i+1);
  74:     for(int i=0;i<INSERTS;i++) p->insert_volume[i]=p->insert_width[i]=1;
  75:     for(int c=0;c<4;c++) p->route[c]=c+1;
  76:     for(int pat=0;pat<1;pat++) for(int i=0;i<STEPS;i++) {
  77:         if(i%4==0) p->notes[pat][0][i]=(Note){60,100,i,0};
  78:         if(i%8==4) p->notes[pat][1][i]=(Note){60,100,i,0};
  79:         if(i%2==0) p->notes[pat][2][i]=(Note){60,70,i,0};
  80:         if(pat && i==15) p->notes[pat][1][i]=(Note){60,75,i,0};
```

### project_new

```c
  85:     project_default(p); memset(p->notes,0,sizeof p->notes); p->channel_count=1;
  86:     memset(p->route,0,sizeof p->route);
  87:     for(int c=0;c<CHANNELS;c++) {
  89:         snprintf(p->channel_names[c],PATTERN_NAME,"Channel %d",c+1);
  91:     snprintf(p->channel_names[0],PATTERN_NAME,"Sampler");
```

### project_demo

```c
  94:     project_default(p); p->bpm=110; p->pattern_count=2; p->pattern_steps[1]=32;
  95:     snprintf(p->pattern_names[0],PATTERN_NAME,"Intro"); snprintf(p->pattern_names[1],PATTERN_NAME,"Groove");
  96:     snprintf(p->channel_names[3],PATTERN_NAME,"Bass"); snprintf(p->track_names[0],PATTERN_NAME,"Demo groove");
  97:     for(int c=0;c<CHANNELS;c++) {
  98:         if(c<4) p->volume[c]=c==2?.18f:.35f;
 101:     for(int c=0;c<3;c++) for(int n=0;n<16;n++) if(p->notes[0][c][n].velocity) {
 102:         p->notes[1][c][n]=p->notes[0][c][n]; p->notes[1][c][n+16]=p->notes[0][c][n]; p->notes[1][c][n+16].start+=16;
 104:     const int pitches[]={60,60,67,63,65,65,67,58};
 105:     for(int n=0;n<8;n++) p->notes[1][3][n]=(Note){pitches[n],85,n*4,3};
 106:     for(int b=0;b<8;b+=b<2?1:2) { p->clips[0][b]=b<2?1:2; p->clip_steps[0][b]=b<2?16:32; }
```

### channel_replace_instrument

```c
 109:     if(c<0 || c>=p->channel_count || !instrument_descriptor(type) ||
 110:        (type==INSTRUMENT_FM && p->channel_audio[c])) return 0;
 111:     if(p->instrument[c]==type) return 1;
 112:     if(type!=INSTRUMENT_FM) for(int a=p->automation_count-1;a>=0;a--) {
 114:         if(target.owner==(unsigned)c && ((target.parameter>=PARAM_FM_RATIO && target.parameter<=PARAM_FM_LAST) || (target.parameter>=PARAM_DX7_FIRST && target.parameter<=PARAM_DX7_LAST))) automation_delete(p,a);
 116:     p->instrument[c]=type; return 1;
```

### channel_delete

```c
 119:     if(c<0 || c>=p->channel_count) return 0;
 120:     for(int i=c;i<p->channel_count-1;i++) {
 121:         p->instrument[i]=p->instrument[i+1]; p->fm[i]=p->fm[i+1]; p->sampler[i]=p->sampler[i+1]; p->channel_pitch[i]=p->channel_pitch[i+1]; p->pitch_range[i]=p->pitch_range[i+1];
 122:         p->channel_colors[i]=p->channel_colors[i+1]; p->channel_audio[i]=p->channel_audio[i+1]; p->audio_seconds[i]=p->audio_seconds[i+1];
 123:         p->volume[i]=p->volume[i+1]; p->pan[i]=p->pan[i+1]; p->mute[i]=p->mute[i+1]; p->route[i]=p->route[i+1];
 124:         memcpy(p->paths[i],p->paths[i+1],sizeof p->paths[i]);
 125:         memmove(p->channel_names[i],p->channel_names[i+1],PATTERN_NAME);
 126:         size_t length=strlen(p->channel_names[i]); memset(p->channel_names[i]+length,0,PATTERN_NAME-length);
 127:         for(int pat=0;pat<PATTERNS;pat++) memcpy(p->notes[pat][i],p->notes[pat][i+1],sizeof p->notes[pat][i]);
 129:     for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) {
 131:         if(id==PATTERNS+c+1) { p->clips[l][b]=0; p->clip_steps[l][b]=0; }
 132:         else if(id>PATTERNS+c+1 && id<=AUTOMATION_SOURCE) p->clips[l][b]--;
 134:     for(int i=p->automation_count-1;i>=0;i--) {
 136:         if(t->parameter==PARAM_CHANNEL_VOLUME || t->parameter==PARAM_CHANNEL_PAN || t->parameter==PARAM_CHANNEL_PITCH || t->parameter==PARAM_CHANNEL_MUTE || t->parameter==PARAM_PITCH_RANGE || ((t->parameter>=PARAM_FM_RATIO && t->parameter<=PARAM_FM_LAST) || (t->parameter>=PARAM_DX7_FIRST && t->parameter<=PARAM_DX7_LAST))) {
 141:     for(int i=p->midi_binding_count-1;i>=0;i--) {
 144:            (t->parameter>=PARAM_FM_RATIO && t->parameter<=PARAM_FM_LAST) || (t->parameter>=PARAM_DX7_FIRST && t->parameter<=PARAM_DX7_LAST)) {
 145:             if(t->owner==(unsigned)c) {memmove(&p->midi_bindings[i],&p->midi_bindings[i+1],(--p->midi_binding_count-i)*sizeof p->midi_bindings[0]);memset(&p->midi_bindings[p->midi_binding_count],0,sizeof p->midi_bindings[0]);}
 149:     int last=--p->channel_count; p->instrument[last]=INSTRUMENT_SAMPLER; p->fm[last]=fm_default(); p->channel_colors[last]=0; p->channel_pitch[last]=0; p->pitch_range[last]=2; p->channel_audio[last]=0; p->audio_seconds[last]=0; p->sampler[last]=(Sampler){.time=1,.length=1}; p->volume[last]=1; p->pan[last]=0; p->mute[last]=p->route[last]=0;
 150:     memset(p->paths[last],0,sizeof p->paths[last]); memset(p->channel_names[last],0,PATTERN_NAME); snprintf(p->channel_names[last],PATTERN_NAME,"Channel %d",last+1);
 151:     for(int pat=0;pat<PATTERNS;pat++) memset(p->notes[pat][last],0,sizeof p->notes[pat][last]);
 152:     return 1;
```

### pattern_delete

```c
 155:     if(pattern<0 || pattern>=p->pattern_count) return 0;
 156:     for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) {
 157:         if(p->clips[l][b]==pattern+1) { p->clips[l][b]=0; p->clip_steps[l][b]=0; }
 158:         else if(p->clips[l][b]>pattern+1 && p->clips[l][b]<=PATTERNS) p->clips[l][b]--;
 160:     for(int i=pattern;i<p->pattern_count-1;i++) {
 161:         memcpy(p->notes[i],p->notes[i+1],sizeof p->notes[i]);
 162:         memcpy(p->pattern_names[i],p->pattern_names[i+1],PATTERN_NAME);
 163:         p->pattern_steps[i]=p->pattern_steps[i+1]; p->pattern_colors[i]=p->pattern_colors[i+1];
 165:     int last=p->pattern_count>1?--p->pattern_count:0;
 166:     memset(p->notes[last],0,sizeof p->notes[last]); p->pattern_steps[last]=STEPS; p->pattern_colors[last]=pattern_palette[last];
 167:     snprintf(p->pattern_names[last],PATTERN_NAME,"Pattern %d",last+1);
 168:     return 1;
```

### insert_reset

```c
 171:     if(id<1 || id>p->insert_count) return 0;
 172:     p->insert_volume[id-1]=p->insert_width[id-1]=1; p->insert_pan[id-1]=0; p->insert_mute[id-1]=0; p->insert_output[id-1]=0;
 173:     memset(p->audio_io[id],0,sizeof p->audio_io[id]);
 174:     for(int slot=0;slot<EFFECT_SLOTS;slot++) { p->effect_type[id][slot]=EFFECT_EMPTY; p->effect_mix[id][slot]=1; device_descriptor(DEVICE_CHORUS)->defaults(&p->chorus[id][slot]); device_descriptor(DEVICE_EQ)->defaults(&p->eq[id][slot]); p->effect_bypass[id][slot]=0; }
 175:     for(int a=p->automation_count-1;a>=0;a--) {
 179:     return 1;
```

### insert_connect

```c
 182:     if(source<1 || source>p->insert_count || destination<0 || (destination>p->insert_count && destination!=255)) return 0;
 184:     for(int hops=0;next && next!=255;hops++) {
 185:         if(next==source || next>p->insert_count || hops>=INSERTS) return 0;
 186:         next=p->insert_output[next-1];
 188:     p->insert_output[source-1]=destination; return 1;
```

### note_at

```c
 191:     if(pat<0 || pat>=PATTERNS || channel<0 || channel>=CHANNELS) return NULL;
 192:     for(int i=0;i<NOTES;i++) { Note *n=&p->notes[pat][channel][i]; if(n->velocity && fabsf(n->start-start)<.00001f && n->pitch==pitch) return n; }
```

### note_hit

```c
 196:     for(int i=NOTES-1;i>=0;i--) {
 198:         if(n.velocity && n.pitch==pitch && n.start<=step && n.start+(n.length?n.length:1)>step) return i;
 200:     return -1;
```

### notes_velocity

```c
 203:     if(!isfinite(from) || !isfinite(to) || !isfinite(from_value) || !isfinite(to_value) || !isfinite(radius) || radius<0) return;
 205:     for(int i=0;i<NOTES;i++) {
 209:         float t=fabsf(to-from)>.000001f?fmaxf(0,fminf(1,(n->start-from)/(to-from))):1;
 210:         n->velocity=(uint8_t)roundf(fmaxf(1,fminf(127,from_value+(to_value-from_value)*t)));
```

### note_add

```c
 214:     if(pat<0 || pat>=PATTERNS || channel<0 || channel>=CHANNELS || !isfinite(start) || !isfinite(length) || start<0 || start>=p->pattern_steps[pat] || pitch<0 || pitch>127 || length<0 || length>p->pattern_steps[pat]-start) return NULL;
 215:     for(int i=0;i<NOTES;i++) if(!p->notes[pat][channel][i].velocity) {
 216:         Note *n=&p->notes[pat][channel][i]; *n=(Note){pitch,100,start,length}; return n;
```

### note_move

```c
 221:     if(pat<0 || pat>=PATTERNS || channel<0 || channel>=p->channel_count || !note || !note->velocity || limit>p->pattern_steps[pat]) return 0;
 222:     float length=note->length?note->length:1;
 223:     if(!isfinite(start) || !isfinite(limit) || start<0 || start+length>limit || pitch<0 || pitch>127) return 0;
 224:     note->start=start; note->pitch=pitch; return 1;
```

### notes_move

```c
 226: int notes_move(Project *p,int pat,int channel,const Note before[NOTES],const uint8_t selected[NOTES],float dx,int dy,float limit) {
 227:     if(pat<0 || pat>=PATTERNS || channel<0 || channel>=p->channel_count || !isfinite(dx) || !isfinite(limit) || limit>p->pattern_steps[pat]) return 0;
 228:     for(int i=0;i<NOTES;i++) if(selected[i]) {
 230:         if(!n.velocity || start<0 || start+(n.length?n.length:1)>limit || pitch<0 || pitch>127) return 0;
 232:     for(int i=0;i<NOTES;i++) if(selected[i]) { p->notes[pat][channel][i]=before[i]; p->notes[pat][channel][i].start+=dx; p->notes[pat][channel][i].pitch+=dy; }
 233:     return 1;
```

### notes_resize

```c
 235: float notes_resize(Project *p,int pat,int channel,const Note before[NOTES],const uint8_t selected[NOTES],float delta,float minimum) {
 236:     if(pat<0 || pat>=PATTERNS || channel<0 || channel>=p->channel_count || !isfinite(delta) || !isfinite(minimum) || minimum<=0) return 0;
 237:     float lower=-INFINITY,end=0;
 238:     for(int i=0;i<NOTES;i++) if(selected[i] && before[i].velocity) {
 239:         float length=before[i].length?before[i].length:1;
 243:     for(int i=0;i<NOTES;i++) if(selected[i] && before[i].velocity) {
 245:         if(delta!=0) p->notes[pat][channel][i].length=(before[i].length?before[i].length:1)+delta;
 247:         end=fmaxf(end,before[i].start+(length?length:1));
```

### samples_default

```c
 252:     uint32_t seed=1; memset(s,0,CHANNELS*sizeof *s);
 253:     for(int c=0;c<4;c++) {
 254:         s[c].frames=RATE/2; s[c].data=calloc(s[c].frames,sizeof(float));
 255:         if(!s[c].data) { s[c].frames=0; continue; }
 256:         double phase=0;
 257:         for(unsigned i=0;i<s[c].frames;i++) {
 259:             seed=seed*1664525u+1013904223u;
 260:             double noise=(double)(seed>>8)/8388608.0-1;
 261:             phase+=6.28318530718*(c==0?48+110*exp(-t*35):c==3?130.8128:180)/RATE;
 262:             double x=c==0?sin(phase)*exp(-t*12):c==1?(noise*.8+sin(phase)*.2)*exp(-t*22):c==2?noise*exp(-t*65):sin(phase)*exp(-t*8);
 263:             s[c].data[i]=(float)(x*fmin(t*1000,1));
```

### player_reset

```c
 267: void player_reset(Player *p) { memset(p,0,sizeof *p); p->last_step=-1; }
```

### player_seek

```c
 269:     p->start_step=isfinite(step)?fmaxf(0,step):0;
 270:     p->frame=(uint64_t)llround(p->start_step*RATE*60.0/pr->bpm/4); p->last_step=-1; p->clock_bpm=pr->bpm;
 271:     memset(p->voices,0,sizeof p->voices);
```

### channel_speed

```c
 273: float channel_speed(const Project *p,int channel) { return p->channel_pitch[channel]?powf(2,p->channel_pitch[channel]*p->pitch_range[channel]/12):1; }
```

### clip_source_steps

```c
 275:     if(source<0 || source>=SOURCES) return 0;
 276:     if(source>=AUTOMATION_SOURCE) return source-AUTOMATION_SOURCE<p->automation_count?p->automations[source-AUTOMATION_SOURCE].steps:0;
 277:     return source<PATTERNS?p->pattern_steps[source]:p->audio_seconds[source-PATTERNS]*audio_source_bpm(p,source-PATTERNS)/15/channel_speed(p,source-PATTERNS);
```

### clip_offset_steps

```c
 280:     int source=p->clips[lane][clip]-1;
 282:     return AUDIO_SOURCE(source)?offset*audio_source_bpm(p,source-PATTERNS)/15/channel_speed(p,source-PATTERNS):offset;
```

### clip_length

```c
 285:     float length=p->clip_steps[lane][bar]; int source=p->clips[lane][bar]-1;
```

### song_steps

```c
 291:     for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(p->clips[l][b] && p->clip_starts[l][b]*STEPS+clip_length(p,l,b)>end) end=p->clip_starts[l][b]*STEPS+clip_length(p,l,b);
```

### solo_any

```c
 294: int solo_any(const uint8_t *states,int count) {
 295:     for(int i=0;i<count;i++) if(states[i]&2) return 1;
 296:     return 0;
```

### solo_toggle

```c
 298: void solo_toggle(uint8_t *states,int count,int selected) {
 299:     if(selected<0 || selected>=count) return;
 300:     if(states[selected]&2) states[selected]&=~2;
 301:     else states[selected]=(states[selected]&~1)|2;
```

### trigger

```c
 303: static void trigger(Player *p,const Project *pr,int pat,int64_t tick,float remaining,int lane,int entering,float swing,const float *channel_speeds,double master_speed,const uint8_t *channel_mutes,const FMSettings *fm_settings) {
 304:     for(int c=0;c<pr->channel_count;c++) for(int i=0;i<NOTES;i++) {
 306:         if(!n.velocity || (channel_mutes[c]&1)) continue;
 308:         if(swing>0) {
 310:             double pair=floor(start/2)*2,phase=start-pair,delay=swing*.5;
 311:             start=pair+(phase<1?phase*(1+delay):1+delay+(phase-1)*(1-delay));
 313:         int64_t onset=llround(start*96.0);
 314:         double elapsed=(tick-onset)/96.0;
 315:         if(onset!=tick && !(entering && n.length && elapsed>0 && elapsed<n.length)) continue;
 316:         int v=0; while(v<127 && p->voices[v].gain!=0) v++;
 317:         p->channel_trigger[c]=1;
 318:         if(lane>=0 && pr->volume[c]>0) p->lane_trigger[lane]=1;
 319:         double speed=pow(2,((int)n.pitch-60)/12.0),frames_per_step=RATE*60.0/pr->bpm/4;
 320:         uint32_t phase[6]; int retain_phase=p->voices[v].instrument==INSTRUMENT_FM && p->voices[v].channel==c && p->voices[v].fm.engine==1 && pr->instrument[c]==INSTRUMENT_FM && fm_settings[c].engine==1 && !fm_settings[c].dx7.value[136];
 321:         if(retain_phase) memcpy(phase,p->voices[v].fm.dx7.phase,sizeof phase);
 322:         p->voices[v]=(Voice){.channel=c,.sampler={.position=elapsed*frames_per_step*speed*channel_speeds[c]*master_speed*(pr->sampler[c].fit_bpm?pr->bpm/pr->sampler[c].fit_bpm:1),.speed=speed},.remaining=n.length?fmin(n.length-elapsed,remaining)*frames_per_step:-1,.gain=n.velocity/127.f,.lane=lane,.pattern=pat,.note_id=i+1};
 325:             voice->remaining=fmin(n.length?n.length-elapsed:1,remaining)*frames_per_step;
 326:             instrument_start(&voice->device,voice->instrument,(InstrumentSettings){&pr->sampler[c],&fm_settings[c]},(InstrumentNote){.frequency=261.6255653005986*speed,.velocity=n.velocity/127.f});
 327:             if(retain_phase) memcpy(voice->fm.dx7.phase,phase,sizeof phase);
 330:             instrument_start(&voice->device,INSTRUMENT_SAMPLER,(InstrumentSettings){&pr->sampler[c],&fm_settings[c]},(InstrumentNote){.position=voice->sampler.position,.speed=speed,.velocity=n.velocity/127.f});
```

### active_voices

```c
 334: static int active_voices(Player *p,Player *live,Voice *active[256]) {
 335:     int count=0;
 336:     for(int v=0;v<128;v++) if(p->voices[v].gain) active[count++]=&p->voices[v];
 337:     if(live) for(int v=0;v<128;v++) if(live->voices[v].gain) active[count++]=&live->voices[v];
```

### render_audio

```c
 340: static void render_audio(Player *p,Player *live,const Project *pr,const Sample s[CHANNELS],float *out,unsigned frames,int sequence,float peaks[INSERTS+1][2],const MixerIO *io) {
 341:     memset(p->channel_active,0,sizeof p->channel_active); memset(p->channel_trigger,0,sizeof p->channel_trigger);
 342:     memset(p->lane_active,0,sizeof p->lane_active); memset(p->lane_trigger,0,sizeof p->lane_trigger);
 347:         for(int v=0;v<128;v++) if(p->voices[v].gain && p->voices[v].remaining>=0) p->voices[v].remaining*=ratio;
 351:     p->audio_resync=0;
 352:     if(resync || tempo_changed) for(int v=0;v<128;v++) if(p->voices[v].audio_clip && (resync || !pr->sampler[p->voices[v].channel].fit_bpm)) p->voices[v].gain=0;
 353:     Voice *active[256]; int count=active_voices(p,live,active);
 355:     int loop=isfinite(p->loop_start) && isfinite(p->loop_end) && p->loop_start>=0 && p->loop_end>p->loop_start;
 356:     float begin=loop?p->loop_start:p->song?0:p->start_step;
 365:     int order[INSERTS+1],buses_count=plan->buses_count;
 366:     uint8_t input_audible[INSERTS+1],effect_audible[INSERTS+1],channel_audible[CHANNELS];
 372:     if(end<=begin) begin=0;
 373:     double stepframes=RATE*60.0/pr->bpm/4/96,pitch_speed=pow(2,pr->master_pitch/12.0);
 374:     uint64_t begin_frame=(uint64_t)llround(begin*stepframes*96),end_frame=(uint64_t)llround(end*stepframes*96);
 376:     uint8_t channel_mutes[CHANNELS]; memcpy(channel_mutes,pr->mute,sizeof channel_mutes);
 379:     for(int l=0;l<LANES;l++) lane_enabled[l]=!(pr->lane_mute[l]&1) && (!lane_solo || (pr->lane_mute[l]&2));
 380:     for(int c=0;c<pr->channel_count;c++) {
 382:         float gain=(pr->mute[c]&1) || (channel_solo && !(pr->mute[c]&2)) || !channel_audible[c]?0:pr->volume[c];
 383:         gain_left[c]=gain*fminf(1,1-pr->pan[c]); gain_right[c]=gain*fminf(1,1+pr->pan[c]);
 385:     float bus_left[INSERTS+1],bus_right[INSERTS+1],bus_width[INSERTS+1];
 386:     bus_left[0]=bus_right[0]=pr->master_mute?0:pr->master; bus_width[0]=pr->master_width;
 387:     for(int id=1;id<=pr->insert_count;id++) {
 388:         int i=id-1; float gain=(pr->insert_mute[i]&1)?0:pr->insert_volume[i],pan=pr->insert_pan[i];
 389:         bus_left[id]=gain*fminf(1,1-pan); bus_right[id]=gain*fminf(1,1+pan); bus_width[id]=pr->insert_width[i];
 394:     int automation_clips_count=0;
 395:     float channel_controls[CHANNELS][5],insert_controls[INSERTS][4],master_controls[5]={pr->master,pr->master_width,pr->master_pitch,pr->master_mute,pr->swing};
 396:     ChorusSettings effect_updates[INSERTS+1][EFFECT_SLOTS];
 397:     EQSettings eq_updates[INSERTS+1][EFFECT_SLOTS];
 399:     float mix_updates[INSERTS+1][EFFECT_SLOTS];
 404:     float *bindings[AUTOMATIONS]={0},baseline[AUTOMATIONS],low[AUTOMATIONS],high[AUTOMATIONS];
 405:     uint8_t channel_changed[CHANNELS]={0},insert_changed[INSERTS]={0}; int master_changed=0,master_pitch_changed=0;
 406:     uint8_t channel_pitch_changed[CHANNELS]={0};
 418:         for(int c=0;c<pr->channel_count;c++) { channel_controls[c][0]=pr->volume[c]; channel_controls[c][1]=pr->pan[c]; channel_controls[c][2]=pr->channel_pitch[c]; channel_controls[c][3]=!!(pr->mute[c]&1); channel_controls[c][4]=pr->pitch_range[c]; }
 419:         for(int i=0;i<pr->insert_count;i++) { insert_controls[i][0]=pr->insert_volume[i]; insert_controls[i][1]=pr->insert_pan[i]; insert_controls[i][2]=pr->insert_width[i]; insert_controls[i][3]=!!(pr->insert_mute[i]&1); }
 420:         for(int a=0;a<pr->automation_count;a++) {
 423:             if((id>=PARAM_FM_RATIO && id<=PARAM_FM_LAST) || (id>=PARAM_DX7_FIRST && id<=PARAM_DX7_LAST)) {
 428:                 EQBand *b=&eq_updates[c][t.slot].bands[(id-PARAM_EQ_FIRST)/3];
 429:                 bindings[a]=(id-PARAM_EQ_FIRST)%3==0?&b->frequency:(id-PARAM_EQ_FIRST)%3==1?&b->gain:&b->q;
 434:             else if(id<=PARAM_CHANNEL_PITCH || id==PARAM_CHANNEL_MUTE || id==PARAM_PITCH_RANGE) { bindings[a]=&channel_controls[c][id==PARAM_CHANNEL_MUTE?3:id==PARAM_PITCH_RANGE?4:id-PARAM_CHANNEL_VOLUME]; channel_changed[c]=1; if(id==PARAM_CHANNEL_PITCH || id==PARAM_PITCH_RANGE) channel_pitch_changed[c]=1; }
 435:             else if(id<=PARAM_INSERT_WIDTH || id==PARAM_INSERT_MUTE) { bindings[a]=&insert_controls[c][id==PARAM_INSERT_MUTE?3:id-PARAM_INSERT_VOLUME]; insert_changed[c]=1; }
 436:             else { bindings[a]=&master_controls[id==PARAM_MASTER_MUTE?3:id==PARAM_SWING?4:id-PARAM_MASTER_VOLUME]; master_changed=1; if(id==PARAM_MASTER_PITCH) master_pitch_changed=1; }
 438:         for(int l=0;l<LANES;l++) if(lane_enabled[l]) for(int b=0;b<CLIPS;b++) if(pr->clips[l][b]>AUTOMATION_SOURCE) {
 439:             int a=pr->clips[l][b]-AUTOMATION_SOURCE-1;
 446:     float buses[INSERTS+1][2];
 447:     for(unsigned f=0;f<frames;f++,p->frame++) {
 449:         int64_t step=(int64_t)(p->frame/stepframes);
 450:         if(sequence && (!io || io->monitor_only) && p->frame>=end_frame) { p->frame=begin_frame; step=(int64_t)(p->frame/stepframes); p->last_step=-1; if(loop) memset(p->voices,0,sizeof p->voices); }
 452:             double position=p->frame/(stepframes*96);
 453:             for(int a=0;a<pr->automation_count;a++) if(bindings[a]) *bindings[a]=baseline[a];
 454:             double held_end[AUTOMATIONS]; for(int a=0;a<pr->automation_count;a++) held_end[a]=-INFINITY;
 455:             for(int n=0;n<automation_clips_count;n++) if(position>=automation_clips[n].end) {
 459:                 for(int i=0;i<pr->automation_count;i++) if(bindings[i]==bindings[a]) held_end[i]=automation_clips[n].end;
 462:             for(int n=0;n<automation_clips_count;n++) if(position>=automation_clips[n].start && position<automation_clips[n].end) {
 466:             for(int a=0;a<pr->automation_count;a++) if(bindings[a]) {
 470:             for(int c=0;c<pr->channel_count;c++) if(channel_changed[c]) {
 471:                 float gain=channel_controls[c][0],pan=channel_controls[c][1]; int audible=channel_audible[c];
 472:                 channel_mutes[c]=(pr->mute[c]&~1)|(channel_controls[c][3]>=.5f);
 473:                 if(channel_controls[c][3]>=.5f || (channel_solo && !(pr->mute[c]&2)) || !audible) gain=0;
 474:                 gain_left[c]=gain*fminf(1,1-pan); gain_right[c]=gain*fminf(1,1+pan);
 475:                 if(channel_pitch_changed[c]) speed[c]=pow(2,channel_controls[c][2]*roundf(channel_controls[c][4])/12);
 477:             for(int i=0;i<pr->insert_count;i++) if(insert_changed[i]) {
 478:                 float gain=insert_controls[i][3]>=.5f?0:insert_controls[i][0],pan=insert_controls[i][1];
 479:                 bus_left[i+1]=gain*fminf(1,1-pan); bus_right[i+1]=gain*fminf(1,1+pan); bus_width[i+1]=insert_controls[i][2];
 481:             if(master_changed) { bus_left[0]=bus_right[0]=master_controls[3]>=.5f?0:master_controls[0]; bus_width[0]=master_controls[1]; if(master_pitch_changed) pitch_speed=pow(2,master_controls[2]/12); }
 484:             int tick=step!=p->last_step,entered=p->last_step<0; p->last_step=step;
 486:                 for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) {
 488:                     int64_t local=step-(int64_t)llround(pr->clip_starts[l][b]*STEPS*96.0); float length=lengths[l][b];
 490:                         int c=pat-PATTERNS-1;
 491:                         double song_position=p->frame/(stepframes*96),clip_start=pr->clip_starts[l][b]*STEPS;
 493:                         int inside=reconcile?(song_position>=clip_start && song_position<clip_start+length):(local>=0 && local<length*96);
 494:                         if(inside && (reconcile || (tick && (local==0 || entered)))) {
 495:                             int v=0; while(v<127 && p->voices[v].gain) v++;
 496:                             double position=fmax(0,p->frame-pr->clip_starts[l][b]*STEPS*stepframes*96);
 497:                             if(lane_enabled[l] && (gain_left[c]>0 || gain_right[c]>0)) p->lane_trigger[l]=1;
 498:                             p->channel_trigger[c]=1;
 499:                             p->voices[v]=(Voice){.channel=c,.sampler={.position=audio_clip_position(pr,l,b,p->frame/(stepframes*96)),.speed=1},.remaining=fmin(length*stepframes*96-position,end_frame-p->frame),.gain=1,.lane=l,.audio_clip=1};
 501:                     } else if(tick && local>=0 && local<length*96 && local+pr->clip_offsets[l][b]*96<pr->pattern_steps[pat-1]*96) trigger(p,pr,pat-1,local+(int64_t)llround(pr->clip_offsets[l][b]*96),fminf(length-local/96.f,end-step/96.f),l,pr->clip_offsets[l][b]>0 && (local==0 || entered),master_controls[4],speed,pitch_speed,channel_mutes,fm_controls);
 503:             } else trigger(p,pr,p->pattern,step,end-step/96.f,-1,0,master_controls[4],speed,pitch_speed,channel_mutes,fm_controls);
 504:             count=active_voices(p,live,active); resync=tempo_changed=0;
 506:         for(int at=0;at<buses_count;at++) buses[order[at]][0]=buses[order[at]][1]=0;
 509:             for(int id=0;id<=pr->insert_count;id++) if(!input_audible[id]) buses[id][0]=buses[id][1]=0;
 511:         for(int v=0;v<count;v++) {
 514:             if(c>=pr->channel_count || voice->instrument!=pr->instrument[c]) { voice->gain=0; continue; }
 516:             if(!synth && voice->sampler.position>=s[c].frames) { voice->gain=0; continue; }
 517:             float gain=voice->instrument==INSTRUMENT_FM && voice->fm.engine==1?1:voice->gain;
 519:                 if(voice->remaining>=0 && --voice->remaining<=0) { instrument_release(&voice->device,voice->instrument,(InstrumentSettings){&pr->sampler[c],&fm_controls[c]}); voice->remaining=-1; }
 523:                     gain*=fmin(1,(s[c].frames-voice->sampler.position)/(rate*voice->sampler.speed*pitch_speed*speed[c]*RATE*.005));
 525:                 if(voice->remaining>=0) {
 526:                     gain*=fminf(1,voice->remaining/(RATE*.005f));
 527:                     if(--voice->remaining<=0) voice->gain=0;
 530:             if(sequence && p->song && voice->lane>=0 && voice->lane<LANES && !lane_enabled[voice->lane]) gain=0;
 531:             if(gain>0 && !pr->master_mute && (gain_left[c]>0 || gain_right[c]>0)) p->channel_active[c]=1;
 533:             float stereo[2];
 534:             double rate=pr->sampler[c].fit_bpm?pr->bpm/pr->sampler[c].fit_bpm:1;
 539:                 if(!instrument_active(&voice->device,INSTRUMENT_FM,s[c])) voice->gain=0;
 541:             for(unsigned side=0;side<2;side++) buses[id][side]+=stereo[side]*gain*(side?gain_right[c]:gain_left[c]);
 543:         for(int at=0;at<buses_count;at++) {
 544:             int id=order[at]; float stereo[2]={buses[id][0],buses[id][1]};
 546:             float l=effect_audible[id]?stereo[0]*bus_left[id]:0,r=effect_audible[id]?stereo[1]*bus_right[id]:0;
 547:             if(bus_width[id]!=1) { float mid=(l+r)*.5f,side=(l-r)*.5f*bus_width[id]; l=mid+side; r=mid-side; }
 550:                 int dest=pr->insert_output[id-1]; if(dest!=255) { buses[dest][0]+=l; buses[dest][1]+=r; }
 552:                 if(sequence) { out[f*2]=l; out[f*2+1]=r; }
 553:                 else { out[f*2]+=l; out[f*2+1]+=r; }
 556:             if(peaks) { peaks[id][0]=fmaxf(peaks[id][0],fabsf(l)); peaks[id][1]=fmaxf(peaks[id][1],fabsf(r)); }
 559:     for(int v=0;v<128;v++) {
 561:         if(voice->gain && l>=0 && l<LANES && c<pr->channel_count && instrument_active(&voice->device,voice->instrument,s[c]) && lane_enabled[l] && (gain_left[c]>0 || gain_right[c]>0)) p->lane_active[l]=1;
 563:     for(int c=0;c<pr->channel_count;c++) if(pr->master_mute || !(gain_left[c]>0 || gain_right[c]>0) || (pr->instrument[c]!=INSTRUMENT_FM && !s[c].frames)) p->channel_trigger[c]=0;
 564:     for(int l=0;l<LANES;l++) if(!lane_enabled[l] || pr->master_mute) p->lane_active[l]=p->lane_trigger[l]=0;
```

### render

```c
 567:     render_audio(p,NULL,pr,s,out,frames,1,NULL,NULL);
```

### render_live

```c
 570:     render_audio(p,NULL,pr,s,out,frames,0,NULL,NULL);
```

### render_mixer

```c
 572: void render_mixer(Player *p,Player *live,const Project *pr,const Sample s[CHANNELS],float *out,unsigned frames,int sequence,float peaks[INSERTS+1][2]) {
```

### render_mixer_io

```c
 575: void render_mixer_io(Player *p,Player *live,const Project *pr,const Sample s[CHANNELS],float *out,unsigned frames,int sequence,float peaks[INSERTS+1][2],const MixerIO *io) {
```

## file_chooser.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### file_chooser_locations

```c
  14:     memset(recent,0,sizeof recent); locations_path[0]=0;
  16:     if(!slash || snprintf(locations_path,sizeof locations_path,"%.*s/chooser-folders.txt",(int)(slash-browser_config),browser_config)>=(int)sizeof locations_path) { locations_path[0]=0; return; }
  18:     char line[PATH_MAX+32];
  21:         *newline=0; long purpose=strtol(line,&end,10);
  22:         if(end!=line && *end=='\t' && end[1]=='/' && purpose>=0 && purpose<FILE_PURPOSES && strlen(end+1)<PATH_MAX)
  23:             snprintf(recent[purpose],PATH_MAX,"%s",end+1);
```

### remember_folder

```c
  28:     if(purpose<0 || purpose>=FILE_PURPOSES || !folder[0] || strlen(folder)>=PATH_MAX || !strcmp(recent[purpose],folder)) return;
  31:     if(!locations_path[0] || !atomic_file_open(&out,locations_path)) return;
  32:     for(int i=0;i<FILE_PURPOSES;i++) if(recent[i][0] && !strchr(recent[i],'\n') && !strchr(recent[i],'\r')) fprintf(out.file,"%d\t%s\n",i,recent[i]);
```

### file_chooser_remember

```c
  39:     if(!slash) return; if(slash==parent) slash[1]=0; else *slash=0;
  40:     remember_folder(c->purpose,parent); c->remember=0;
```

### free_entries

```c
  43:     for(int i=0;i<count;i++) free(entries[i].name);
```

### file_chooser_close

```c
  46: void file_chooser_close(FileChooser *c) { if(c->remember) remember_folder(c->purpose,c->directory); free_entries(c->entries,c->count); *c=(FileChooser){.selected=-1}; }
```

### matches

```c
  52:     const char *dot=strrchr(name,'.'); if(!dot) return 0;
  53:     size_t length=strlen(dot+1);
  56:         if(n==length && !strncasecmp(dot+1,at,n)) return 1;
  58:         at=end+1;
  60:     return 0;
```

### file_chooser_folder

```c
  64:     if(!realpath(path,resolved)) { snprintf(c->error,sizeof c->error,"Cannot open folder: %s",strerror(errno)); return 0; }
  66:     if(!dir) { snprintf(c->error,sizeof c->error,"Cannot read folder: %s",strerror(errno)); return 0; }
  67:     FileEntry *entries=NULL; int count=0,capacity=0,failed=0; struct dirent *entry;
  69:         errno=0; entry=readdir(dir);
  70:         if(!entry) { if(errno) failed=1; break; }
  71:         if(!strcmp(entry->d_name,".") || !strcmp(entry->d_name,"..") || (!c->hidden && entry->d_name[0]=='.')) continue;
  76:         if(count==10000) { failed=1; break; }
  78:             int next=capacity?capacity*2:64; FileEntry *grown=realloc(entries,next*sizeof *grown);
  79:             if(!grown) { failed=1; break; } entries=grown; capacity=next;
  81:         char *name=strdup(entry->d_name); if(!name) { failed=1; break; }
  85:     if(failed) { free_entries(entries,count); snprintf(c->error,sizeof c->error,"Could not list folder (memory, read error or too many files)."); return 0; }
  86:     if(count>1) qsort(entries,count,sizeof *entries,compare);
  87:     free_entries(c->entries,c->count); c->entries=entries; c->count=count; c->selected=-1; c->scroll=0;
  88:     snprintf(c->directory,sizeof c->directory,"%s",resolved); c->error[0]=0; return 1;
```

### file_chooser_parent

```c
  92:     char *slash=strrchr(path,'/'); if(slash==path) slash[1]=0; else if(slash) *slash=0;
```

### file_chooser_begin

```c
  98:     if(strlen(initial)>=sizeof path) { snprintf(c->error,sizeof c->error,"Path is too long."); return 0; }
 100:     if(slash) { snprintf(c->name,sizeof c->name,"%s",slash+1); if(slash==path) slash[1]=0; else *slash=0; }
 101:     else { snprintf(c->name,sizeof c->name,"%s",initial); if(!getcwd(path,sizeof path)) return 0; }
 103:         if(!getcwd(path,sizeof path) || !file_chooser_folder(c,path)) return 0;
 105:     if(!save) c->name[0]=0;
 106:     return 1;
```

### file_chooser_path

```c
 109:     if(!c->name[0]) { snprintf(c->error,sizeof c->error,"Choose a file or enter a filename."); return -1; }
 110:     int n=c->name[0]=='/'?snprintf(path,capacity,"%s",c->name):snprintf(path,capacity,"%s/%s",c->directory,c->name);
 111:     if(n<0 || (size_t)n>=capacity) { snprintf(c->error,sizeof c->error,"Path is too long."); return -1; }
 113:     if(!stat(path,&info) && S_ISDIR(info.st_mode)) return file_chooser_folder(c,path)?0:-1;
 117:         if(n<0 || (size_t)n>=capacity-length) { snprintf(c->error,sizeof c->error,"Path is too long."); return -1; }
 120:     if(strlen(path)>=sizeof parent) { snprintf(c->error,sizeof c->error,"Path is too long."); return -1; }
 122:     if(!slash) return -1;
 123:     if(slash==parent) slash[1]=0; else *slash=0;
 124:     if(!realpath(parent,resolved) || access(resolved,c->save?W_OK:R_OK)) { snprintf(c->error,sizeof c->error,"Folder is unavailable or access is denied."); return -1; }
 125:     const char *name=strrchr(path,'/')+1; char final[PATH_MAX];
 126:     if(snprintf(final,sizeof final,"%s%s%s",resolved,!strcmp(resolved,"/")?"":"/",name)>=(int)sizeof final || strlen(final)>=capacity) { snprintf(c->error,sizeof c->error,"Path is too long."); return -1; }
 127:     memcpy(path,final,strlen(final)+1);
 129:         if(!S_ISREG(info.st_mode)) { snprintf(c->error,sizeof c->error,"Choose a regular file."); return -1; }
 130:         if(!c->save && (!matches(path,c->extension) || access(path,R_OK))) { snprintf(c->error,sizeof c->error,"Choose a readable .%s file.",c->extension); return -1; }
 131:         return c->save?2:1;
 133:     if(c->save && errno==ENOENT) return 1;
 134:     snprintf(c->error,sizeof c->error,"File is unavailable: %s",strerror(errno)); return -1;
```

### file_chooser_begin_recent

```c
 139:     if(!file_chooser_begin(c,initial,extension,save)) return 0;
 140:     if(purpose>=0 && purpose<FILE_PURPOSES) {
 141:         if(recent[purpose][0]) { file_chooser_folder(c,recent[purpose]); c->error[0]=0; }
 142:         c->purpose=purpose; c->remember=1;
 144:     return 1;
```

## history.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
   6: typedef struct { Sample sample; const float *identity; uint64_t stamp; unsigned refs; } HistorySample;
```

### discard

```c
  10:     for(int c=0;c<CHANNELS;c++) if(e->sources[c] && !--e->sources[c]->refs) {
```

### history_clear

```c
  16: void history_clear(History *h) { for(int i=0;i<h->count;i++) discard(h,h->entries[i]); *h=(History){0}; }
```

### canonical

```c
  19:     for(int c=0;c<CHANNELS;c++) if(sources[c].frames || !p->paths[c][0] || !strcmp(p->paths[c],SAMPLE_EMPTY)) copy.audio_seconds[c]=0;
```

### history_capture

```c
  23: int history_capture(History *h,const Project *p,const Sample sources[CHANNELS],const uint64_t stamps[CHANNELS]) {
  28:     for(int c=0;c<CHANNELS && same;c++) {
  32:     if(same) return 1;
  33:     HistoryEntry *e=calloc(1,sizeof *e); if(!e) return 0;
  35:     for(int c=0;c<CHANNELS;c++) if(sources[c].frames) {
  38:         HistorySample *s=calloc(1,sizeof *s); if(!s) { discard(h,e); return 0; }
  39:         if(!sample_clone(sources[c],&s->sample)) { free(s); discard(h,e); return 0; }
  40:         s->identity=sources[c].data; s->stamp=stamps[c]; s->refs=1; e->sources[c]=s;
  43:     while(h->count && h->count-1>h->cursor) discard(h,h->entries[--h->count]);
  45:         discard(h,h->entries[0]); memmove(h->entries,h->entries+1,(--h->count)*sizeof h->entries[0]);
  47:     h->entries[h->count++]=e; h->cursor=h->count-1;
  48:     while(h->count>1 && h->bytes>HISTORY_BYTES) {
  49:         discard(h,h->entries[0]); memmove(h->entries,h->entries+1,(--h->count)*sizeof h->entries[0]); h->cursor--;
  51:     return 1;
```

### history_peek

```c
  55:     if((direction!=-1 && direction!=1) || index<0 || index>=h->count) return 0;
  57:     for(int c=0;c<CHANNELS;c++) sources[c]=h->entries[index]->sources[c]?h->entries[index]->sources[c]->sample:(Sample){0};
  58:     return 1;
```

### history_step

```c
  60: void history_step(History *h,int direction) { if((direction==-1 || direction==1) && h->cursor+direction>=0 && h->cursor+direction<h->count) h->cursor+=direction; }
```

### history_rebind

```c
  62: void history_rebind(History *h,const Sample sources[CHANNELS],const uint64_t stamps[CHANNELS]) {
  64:     for(int c=0;c<CHANNELS;c++) {
```

### history_source_matches

```c
  70: int history_source_matches(const History *h,int direction,int channel,Sample source,uint64_t stamp) {
  72:     if(index<0 || index>=h->count || channel<0 || channel>=CHANNELS) return 0;
```

## history.h

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
   5: #define HISTORY_LIMIT 64
   6: #define HISTORY_BYTES ((size_t)256*1024*1024)
  10: int history_capture(History *h,const Project *p,const Sample sources[CHANNELS],const uint64_t stamps[CHANNELS]);
  12: int history_source_matches(const History *h,int direction,int channel,Sample source,uint64_t stamp);
  13: void history_rebind(History *h,const Sample sources[CHANNELS],const uint64_t stamps[CHANNELS]);
```

## macos_input.m

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
   5: #include <GLFW/glfw3.h>
   6: #include <GLFW/glfw3native.h>
```

### macos_navigation_init

```c
  17:                 float factor=1+event.magnification;
  18:                 if(isfinite(factor) && factor>0) pending.zoom+=logf(factor)/logf(1.08f);
  20:                 pending.x+=event.scrollingDeltaX; pending.y+=event.scrollingDeltaY; pending.precise=1;
```

### macos_navigation_poll

```c
  26:     NavigationInput input=pending; pending=(NavigationInput){0}; return input;
```

### macos_navigation_close

```c
  30:     pending=(NavigationInput){0};
```

## main.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### main

```c
   5:     int smoke=0;
   6:     for(int i=1;i<argc;i++) {
   7:         if(!strcmp(argv[i],"--smoke")) smoke=1;
   8:         else if(!strcmp(argv[i],"--version")) { printf("LibreLoop %s\n",LIBRELOOP_VERSION); return 0; }
  12:         fprintf(stderr,"Smoke check failed: bundled samples, font or logo are missing.\n"); return 1;
  19:             || MakeDirectory(directory)!=0 || !ChangeDirectory(directory)) {
  20:             fprintf(stderr,"Cannot open a writable LibreLoop working directory.\n"); return 1;
  25:     for(int c=0;c<CHANNELS;c++) ui.sampler_applied[c]=ui.project.sampler[c];
  30:     /* On Linux, raylib 5.5's high-DPI flag rescales mouse/scissors separately
  31:        from our 2D cameras. Keep window coordinates consistent; rasterize fonts
  33:     SetConfigFlags(flags); InitWindow(1200,675,"LibreLoop " LIBRELOOP_VERSION " - pattern workstation");
  39:     SetWindowMinSize(900,506); SetTargetFPS(60); SetExitKey(KEY_NULL);
  50:     browser_init(&ui.browser,ui.samples_path); midi_initialize(); theme_init(); file_chooser_locations(ui.browser.config); if(smoke) new_project(1); int frames=0,initialized=0; double last_activity=GetTime();
  60:         Vector2 wheel=GetMouseWheelMoveV();
  61:         ui.navigation_input=(NavigationInput){wheel.x,wheel.y,0,0};
  64:         if(native.precise) ui.navigation_input=native;
  65:         else ui.navigation_input.zoom=native.zoom;
  67:         Vector2 movement=GetMouseDelta();
  68:         if(movement.x || movement.y || ui.navigation_input.x || ui.navigation_input.y || ui.navigation_input.zoom || IsWindowResized()) last_activity=GetTime();
  69:         for(int key=32;key<=KEY_KB_MENU;key++) if(IsKeyPressed(key) || IsKeyReleased(key) || IsKeyPressedRepeat(key)) last_activity=GetTime();
  70:         for(int b=0;b<8;b++) if(ui.pending_clicks[b] || IsMouseButtonReleased(b)) last_activity=GetTime();
  71:         for(int b=0;b<8;b++) { ui.frame_clicks[b]=ui.pending_clicks[b]>0; if(ui.frame_clicks[b]) ui.pending_clicks[b]--; }
  76:         float raster_scale=scale*(window_width>0 && framebuffer_width>0?framebuffer_width/(float)window_width:1);
  80:         float sidebar=ui.browser_hidden?24:ui.browser_width;
  81:         Rect desktop={sidebar+6,42,width-sidebar-10,height-66};
  82:         if(!initialized) { windows_init(&ui.windows,width,height); initialized=1; }
  83:         ui.mouse=GetMousePosition(); ui.mouse.x/=scale; ui.mouse.y/=scale; ui.reset=0; ui.popup_opened=0; ui.context_opened=0;
  86:         if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) arrangement_release(&ui.arrangement);
  87:         if(ui.arrangement.gesture) {
  88:             Rect r=ui.windows.editors[1].rect;
  89:             arrangement_drag(&ui.arrangement,&ui.project,ui.arrangement.view_start+(ui.mouse.x-r.x-212)/((r.w-236)/BARS*ui.arrangement.zoom),track_at(ui.track_scroll+ui.mouse.y-r.y-PLAYLIST_GRID_TOP));
  90:             int held=ui.arrangement.gesture==ERASE_CLIPS?(IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_LEFT)):IsMouseButtonDown(MOUSE_BUTTON_LEFT);
  93:         if(ui.fm_graph_drag>=0) {
  94:             if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || ui.pattern_popup || ui.context_kind || !IsWindowFocused() ||
  95:                ui.instrument_channel!=ui.fm_graph_channel || ui.project.instrument[ui.fm_graph_channel]!=INSTRUMENT_FM || !ui.windows.editors[4].visible) ui.fm_graph_drag=-1;
  98:         if(ui.row_zoom_drag>=0) {
  99:             if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || ui.pattern_popup || ui.context_kind || !IsWindowFocused()) ui.row_zoom_drag=-1;
 103:             ui.arrangement.view_start=fmaxf(0,ui.playlist_pan_start+(ui.mouse.x-ui.playlist_pan_x)*ui.playlist_pan_range);
 104:             if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.playlist_pan=0;
 107:             Rect r=ui.windows.editors[1].rect; float area=r.h-PLAYLIST_GRID_TOP-PLAYLIST_BOTTOM+14;
 108:             float total=track_position(LANES),travel=area-timeline_thumb(area,area-14,total),maximum=fmaxf(0,total-(area-14));
 109:             ui.track_scroll=fmaxf(0,fminf(maximum,ui.playlist_vpan_start+(ui.mouse.y-ui.playlist_vpan_y)/fmaxf(1,travel)*maximum));
 110:             if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.playlist_vpan=0;
 112:         if(ui.track_resize>=0) {
 113:             ui.track_heights[ui.track_resize]=roundf(fmaxf(32,fminf(320,ui.track_resize_height+ui.mouse.y-ui.track_resize_y)))/ui.track_zoom;
 115:             if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) ui.track_resize=-1;
 118:             Rect r=ui.windows.editors[3].rect; float area=r.w-MIXER_PANEL-MIXER_LEFT-8; int visible=fmaxf(1,area/51-1);
 119:             ui.mixer_scroll=fmaxf(0,fminf(INSERTS-visible,(ui.mouse.x-r.x-MIXER_LEFT-area*visible/INSERTS/2)/area*INSERTS));
 120:             if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.mixer_pan=0;
 122:         if(ui.cable_drag>0) {
 123:             if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.cable_drag=-1;
 124:             else if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
 126:                 if(destination>=0) snprintf(ui.status,sizeof ui.status,insert_connect(&ui.project,ui.cable_drag,destination)?"Mixer cable connected.":"Connection blocked: it would create a feedback loop.");
 127:                 ui.cable_drag=-1;
 130:         if(ui.rack_paint>=0) {
 131:             if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) ui.rack_paint=-1;
 133:                 Rect r=ui.windows.editors[0].rect; paint_rack(r.w,ui.mouse.x-r.x,ui.mouse.y-r.y);
 134:                 if(!IsMouseButtonDown(ui.rack_paint)) ui.rack_paint=-1;
 137:         int modal=ui.pattern_popup!=0 || ui.context_kind!=0,dragging=ui.sample_drag[0]!=0;
 138:         if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || modal) { ui.route_drag=-1; ui.control_drag=NULL; ui.note_drag=NULL; }
 139:         if(!modal && !captured() && shortcut_down() && IsKeyPressed(KEY_Z)) undo_redo(IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)?1:-1);
 141:         if(!modal && !captured() && shortcut_down() && IsKeyPressed(KEY_Y)) undo_redo(1);
 143:         if(!modal && !captured() && !dragging && shortcut_down() && IsKeyPressed(KEY_S)) project_file_action(IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT)?3:2);
 144:         if(!modal && !captured() && !dragging && shortcut_down() && IsKeyPressed(KEY_O)) replace_project(3);
 145:         if(!modal && !captured() && !dragging && !ui.browser_focus && shortcut_down()) {
 146:             if(ui.windows.focused==2 && ui.piano_tool==PENCIL && (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_K))) piano_octave(1);
 147:             else if(ui.windows.focused==2 && ui.piano_tool==PENCIL && (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_J))) piano_octave(-1);
 148:             else if(IsKeyPressed(KEY_X)) edit_selection(0);
 149:             else if(IsKeyPressed(KEY_C)) edit_selection(1);
 150:             else if(IsKeyPressed(KEY_V)) edit_selection(2);
 151:             else if(IsKeyPressed(KEY_A)) edit_selection(3);
 153:         if(!modal && !captured() && !dragging && !ui.browser_focus && !shortcut_down() &&
 154:            !IsKeyDown(KEY_LEFT_ALT) && !IsKeyDown(KEY_RIGHT_ALT) &&
 155:            (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))) {
 156:             if(IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) piano_shift(-1,0,"left one step");
 157:             else if(IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) piano_shift(1,0,"right one step");
 158:             else if(IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) piano_shift(0,1,"up one semitone");
 159:             else if(IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) piano_shift(0,-1,"down one semitone");
 161:         if(!modal && IsKeyPressed(KEY_SPACE)) transport_toggle();
 163:             ui.rack_view[ui.pattern]=fmaxf(0,ui.rack_hstart+(ui.mouse.x-ui.rack_hx)*ui.rack_hscale);
 164:             if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || modal) ui.rack_hdrag=0;
 167:             ui.rack_scroll=fmaxf(0,ui.rack_vstart+(ui.mouse.y-ui.rack_vy)*ui.rack_vscale);
 168:             if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || modal) ui.rack_vdrag=0;
 170:         if(ui.piano_key_drag) {
 171:             Rect r=ui.windows.editors[2].rect;
 172:             piano_key_update(ui.mouse.x-r.x,ui.mouse.y-r.y,modal || !IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || !IsWindowFocused() || !ui.windows.editors[2].visible);
 174:         if(ui.marker_drag>=0) {
 175:             int id=ui.marker_drag; Rect r=ui.windows.editors[id].rect;
 176:             float gx=id==0?200:id==1?212:60,width=r.w-(id==0?226:id==1?236:84);
 177:             float span=id==0?width/RACK_STEP_WIDTH:id==1?BARS/ui.arrangement.zoom*STEPS:ui.piano_span[ui.pattern];
 178:             float start=id==0?ui.rack_view[ui.pattern]:id==1?ui.arrangement.view_start*STEPS:ui.piano_pan[ui.pattern];
 179:             float q=id==0?1:grid_interval(width/span);
 180:             int button=ui.ruler_loop_drag?MOUSE_BUTTON_RIGHT:MOUSE_BUTTON_LEFT;
 181:             if(modal || IsMouseButtonPressed(button)) ui.marker_drag=-1;
 183:                 float at=fmaxf(0,snap_round(start+(ui.mouse.x-r.x-gx)/width*span,q));
 184:                 if(ui.ruler_loop_drag) {
 185:                     float *range=id==1?ui.song_loop:ui.pattern_loop[ui.pattern];
 186:                     range[0]=fminf(ui.ruler_anchor,at); range[1]=fmaxf(ui.ruler_anchor,at);
 187:                     if(range[1]>range[0]) { if(id==1) ui.playlist_start=range[0]/STEPS; else ui.piano_start[ui.pattern]=range[0]; }
 188:                 } else if(id==1) ui.playlist_start=at/STEPS; else ui.piano_start[ui.pattern]=at;
 189:                 if(!ui.playing) ui.reset=1;
 190:                 if(!IsMouseButtonDown(button)) ui.marker_drag=-1;
 193:         if(!modal && !dragging && !captured() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && ui.mouse.x>=sidebar && ui.mouse.x<sidebar+6 && ui.mouse.y>=42 && ui.mouse.y<height-22) ui.browser_resize=1;
 195:             ui.browser_hidden=ui.mouse.x<120;
 196:             if(!ui.browser_hidden) ui.browser_width=fminf(fminf(600,width-460),ui.mouse.x);
 197:             sidebar=ui.browser_hidden?24:ui.browser_width;
 198:             desktop=(Rect){sidebar+6,42,width-sidebar-10,height-66};
 199:             if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) ui.browser_resize=0;
 202:             Editor *list=&ui.windows.editors[1];
 203:             if(fabsf(list->rect.x-ui.last_sidebar-6)<1 && fabsf(list->rect.w-(width-ui.last_sidebar-10))<1) list->rect=desktop;
 207:             if(fabsf(ui.mouse.x-ui.sample_origin.x)+fabsf(ui.mouse.y-ui.sample_origin.y)>5) ui.sample_moved=1;
 208:             if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.sample_drag[0]=0;
 209:             else if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
 210:                 if(ui.sample_moved && !modal) drop_sample(ui.sample_drag);
 211:                 ui.sample_drag[0]=0;
 212:             } else if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ESCAPE)) ui.sample_drag[0]=0;
 214:         dragging=ui.sample_drag[0]!=0;
 215:         if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.windows.grab=-1;
 216:         if(!modal && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.browser_focus=ui.mouse.x<sidebar && ui.mouse.y>=42 && ui.mouse.y<height-22;
 217:         if(ui.control_drag) {
 218:             float value=ui.control_fader?fader_gain(fmaxf(0,fminf(1,(ui.control_bottom-ui.mouse.y)/ui.control_range))):*ui.control_drag-GetMouseDelta().y/scale*(ui.control_reverse?-1:1)*(ui.control_drag==&ui.project.bpm?.25f:(ui.control_high-ui.control_low)/160);
 219:             if(ui.control_logarithmic) value=*ui.control_drag*powf(ui.control_high/ui.control_low,-GetMouseDelta().y/scale/160);
 220:             if(ui.control_integer) { ui.control_raw=fmaxf(ui.control_low,fminf(ui.control_high,ui.control_raw-GetMouseDelta().y/scale*(ui.control_high-ui.control_low)/160)); value=roundf(ui.control_raw); }
 221:             *ui.control_drag=fmaxf(ui.control_low,fminf(ui.control_high,value));
 223:         if(ui.piano_scroll_drag) {
 224:             ui.piano_pan[ui.pattern]=fmaxf(0,ui.piano_scroll_start+(ui.mouse.x-ui.piano_scroll_x)*ui.piano_scroll_range);
 225:             if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.piano_scroll_drag=0;
 228:             Rect r=ui.windows.editors[2].rect; float area=r.h-PIANO_GRID_PADDING+14;
 229:             ui.piano_top=127-(int)fmaxf(0,fminf(127-piano_min_top(),ui.piano_vscroll_start+(ui.mouse.y-ui.piano_vscroll_y)/area*128));
 230:             if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.piano_vdrag=0;
 232:         if(ui.piano_gesture) {
 233:             int right=ui.piano_gesture==PIANO_ERASE;
 234:             if(modal || IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) ui.piano_gesture=PIANO_IDLE;
 236:                 Rect r=ui.windows.editors[2].rect; float cw=(r.w-84)/ui.piano_span[ui.pattern],rh=(r.h-PIANO_GRID_PADDING)/piano_visible_rows();
 237:                 piano_gesture_update(ui.piano_pan[ui.pattern]+(ui.mouse.x-r.x-60)/cw,ui.piano_top-(ui.mouse.y-r.y-PIANO_GRID_TOP)/rh,grid_interval(cw));
 238:                 if(!IsMouseButtonDown(right?MOUSE_BUTTON_RIGHT:MOUSE_BUTTON_LEFT)) ui.piano_gesture=PIANO_IDLE;
 241:         if(ui.note_drag) {
 242:             Rect r=ui.windows.editors[2].rect; float cw=(r.w-84)/ui.piano_span[ui.pattern],rh=(r.h-PIANO_GRID_PADDING)/piano_visible_rows();
 243:             piano_note_drag_update(ui.piano_pan[ui.pattern]+(ui.mouse.x-r.x-60)/cw,(ui.mouse.y-r.y-PIANO_GRID_TOP)/rh,grid_interval(cw));
 245:         if(ui.velocity_drag>=0) {
 246:             int button=ui.velocity_drag?MOUSE_BUTTON_RIGHT:MOUSE_BUTTON_LEFT;
 248:                IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) ui.velocity_drag=-1;
 250:                 Rect r=ui.windows.editors[2].rect; float cw=(r.w-84)/ui.piano_span[ui.pattern];
 251:                 Vector2 point={ui.piano_pan[ui.pattern]+(ui.mouse.x-r.x-60)/cw,
 252:                     fmaxf(1,fminf(127,(r.y+r.h-22-ui.mouse.y)/.48f))};
 253:                 Vector2 from=ui.velocity_drag?ui.velocity_from:ui.velocity_now;
 254:                 notes_velocity(ui.project.notes[ui.pattern][ui.piano_channel],ui.velocity_drag?ui.velocity_before:NULL,
 255:                     from.x,point.x,from.y,point.y,3/cw);
 257:                 if(!IsMouseButtonDown(button)) ui.velocity_drag=-1;
 260:         if(ui.route_drag>=0) {
 261:             ui.project.route[ui.route_drag]=(int)fmaxf(0,fminf(ui.project.insert_count,ui.route_start+(int)((ui.route_y-ui.mouse.y)/3)));
 262:             ui.mixer_selected=ui.project.route[ui.route_drag];
 263:             int visible=fmaxf(1,(ui.windows.editors[3].rect.w-MIXER_PANEL-MIXER_LEFT-8)/51-1);
 265:             else if(ui.mixer_selected>0 && ui.mixer_selected<=ui.mixer_scroll) ui.mixer_scroll=ui.mixer_selected-1;
 266:             snprintf(ui.status,sizeof ui.status,"%s -> %s",ui.project.channel_names[ui.route_drag],mixer_name(ui.mixer_selected));
 268:         Rect filter_rect=ui.windows.editors[0].rect;
 269:         if(!modal && !captured() && !dragging && windows_hit(&ui.windows,ui.mouse.x,ui.mouse.y)==0 && ui.mouse.x>=filter_rect.x+124 && ui.mouse.x<filter_rect.x+180 && ui.mouse.y>=filter_rect.y+2 && ui.mouse.y<filter_rect.y+TITLE-2 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
 270:             windows_focus(&ui.windows,0); open_context(6,0,(Vector2){filter_rect.x+124,filter_rect.y+TITLE}); modal=1;
 272:         if(!modal && !captured() && !dragging) windows_update(&ui.windows,desktop,ui.mouse.x,ui.mouse.y,IsMouseButtonPressed(MOUSE_BUTTON_LEFT),IsMouseButtonDown(MOUSE_BUTTON_LEFT),GetTime());
 273:         else { if(ui.browser_resize) windows_update(&ui.windows,desktop,ui.mouse.x,ui.mouse.y,0,0,GetTime()); ui.windows.owner=-1; ui.windows.grab=-1; }
 274:         int focused=EDITORS-1;
 275:         while(focused>0 && !ui.windows.editors[ui.windows.order[focused]].visible) focused--;
 276:         if(!modal && !dragging && !captured() && !ui.browser_focus && !ui.midi_take.active && ui.windows.order[focused]==1 && ui.windows.editors[1].visible && delete_pressed()) {
 277:             for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(ui.arrangement.selected[l][b]) { ui.project.clips[l][b]=0; ui.project.clip_steps[l][b]=0; }
 278:             memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected);
 280:         if(!modal && !dragging && !captured() && !ui.browser_focus && !ui.midi_take.active && ui.windows.order[focused]==2 && ui.windows.editors[2].visible && delete_pressed()) {
 281:             for(int i=0;i<NOTES;i++) if(ui.note_selected[ui.pattern][ui.piano_channel][i]) ui.project.notes[ui.pattern][ui.piano_channel][i].velocity=0;
 282:             memset(ui.note_selected[ui.pattern][ui.piano_channel],0,NOTES);
 286:             for(unsigned i=0;i<dropped.count;i++) {
 296:         if(ui.windows.editors[1].visible) previews_update(raster_scale);
 297:         ui.knob_context=-1; ui.browser_progress=-1;
 298:         BeginDrawing(); ClearBackground(bg); BeginMode2D((Camera2D){.zoom=scale});
 299:         ui.input_enabled=!modal && !dragging && !captured() && ui.windows.grab<0 && ui.mouse.y<42;
 300:         ui_surface((Rectangle){0,0,width,40},panel);
 301:         ui_surface((Rectangle){8,8,176,22},cell);
 302:         if(button("FILE",8,8,44,22,ui.pattern_popup==1)) open_popup(1);
 303:         if(hover(8,8,44,22)) snprintf(ui.status,sizeof ui.status,"File: New, Demo, Save, Open, Export or Collect samples");
 304:         if(button("EDIT",52,8,44,22,ui.pattern_popup==10)) open_popup(10);
 305:         if(hover(52,8,44,22)) snprintf(ui.status,sizeof ui.status,"Edit: Undo, Redo, Cut, Copy, Paste and Select all");
 306:         if(button("VIEW",96,8,44,22,ui.pattern_popup==7)) open_popup(7);
 307:         if(hover(96,8,44,22)) snprintf(ui.status,sizeof ui.status,"View: MIDI recording settings");
 308:         if(button("HELP",140,8,44,22,ui.pattern_popup==9)) open_popup(9);
 309:         label(fit_text(TextFormat("%s%s",GetFileName(ui.document.path),project_dirty()?" *":""),176,11),8,29,11,muted);
 310:         const int gap=4,pitch_x=184+gap+KNOB_RADIUS,volume_x=pitch_x+KNOB_RADIUS*2+gap;
 311:         const int mode_x=volume_x+KNOB_RADIUS+gap,play_x=mode_x+52+gap,stop_x=play_x+24+gap,record_x=stop_x+24+gap;
 312:         const int metro_x=record_x+24+3*22+gap,tempo_x=metro_x+22+gap;
 313:         const int position_x=tempo_x+56+12;
 315:         const int audio_x=position_x+88+12;
 316:         const int analyzer_width=(int)fminf(108,fmaxf(0,width-audio_x-208));
 317:         const int editors_x=audio_x+analyzer_width+gap+40+12;
 318:         const int follow_x=editors_x+4*24,keys_x=follow_x+24;
 319:         label("Pitch",pitch_x-text_width("Pitch",8)/2,30,8,muted);
 320:         label("Vol",volume_x-text_width("Vol",8)/2,30,8,muted);
 321:         knob_style(pitch_x,19,&ui.project.master_pitch,-12,12,0,"Master pitch (semitones)",KNOB_CENTER);
 322:         if(hover(pitch_x-11,8,22,22) || ui.control_drag==&ui.project.master_pitch) snprintf(ui.status,sizeof ui.status,"Master pitch: %+.2f semitones (sample speed; right-click to enter)",ui.project.master_pitch);
 323:         knob_style(volume_x,19,&ui.output_volume,0,VOLUME_KNOB_MAX,1,"LibreLoop output volume",KNOB_VOLUME);
 324:         if(hover(volume_x-11,8,22,22) || ui.control_drag==&ui.output_volume) snprintf(ui.status,sizeof ui.status,"LibreLoop output volume: listening level; Master and WAV export unchanged");
 325:         if(button("",mode_x,8,52,22,0) && !recording_active()) { ui.song=!ui.song; ui.reset=1; }
 326:         DrawRectangle(mode_x,8,52,11,ui.song?cell:ui_theme.patt);
 327:         DrawRectangle(mode_x,19,52,11,ui.song?accent:cell);
 328:         label("PATT",mode_x+(52-text_width("PATT",10))/2,8,10,ui.song?muted:ui_theme.selected_text);
 329:         label("SONG",mode_x+(52-text_width("SONG",10))/2,19,10,ui.song?ui_theme.selected_text:muted);
 330:         if(hover(mode_x,8,52,22)) snprintf(ui.status,sizeof ui.status,"Playback mode: click to switch Pattern (orange) / Song (green)");
 331:         int transport_active=ui.playing || audio_preview_position(ui.audition)>=0 || audio_key_position(127,ui.instrument_channel)>=0;
 332:         if(button(transport_active?"||":">",play_x,8,24,22,transport_active)) transport_toggle();
 333:         if(hover(play_x,8,24,22)) snprintf(ui.status,sizeof ui.status,"Play from ruler / stop all audio, including previews (Space)");
 334:         if(button("[]",stop_x,8,24,22,0)) transport_stop();
 335:         if(hover(stop_x,8,24,22)) snprintf(ui.status,sizeof ui.status,"Stop all audio; press again while stopped to return the start marker to the beginning");
 336:         if(button("",record_x,8,24,22,recording_active())) recording_start();
 337:         circle(record_x+12,19,7,ui_theme.meter_high);
 338:         if(hover(record_x,8,24,22) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){midi_refresh();open_popup(13);}
 341:         int record_input=ui.input_enabled;if(recording_active())ui.input_enabled=0;
 342:         for(int i=0;i<3;i++){int xx=record_x+24+i*22;
 343:             if(button("",xx,8,22,22,ui.record_mask&(1u<<i))){ui.record_mask^=1u<<i;midi_save_settings();}
 344:             icon(record_icons[i],xx+11,19,17,ui.record_mask&(1u<<i)?ui_theme.selected_text:muted);
 345:             if(hover(xx,8,22,22))snprintf(ui.status,sizeof ui.status,"Record %s: %s",record_names[i],ui.record_mask&(1u<<i)?"enabled":"disabled");
 347:         if(hover(record_x,8,24,22)) snprintf(ui.status,sizeof ui.status,recording_active()?"Recording: click to finish the takes":"Record enabled types; right-click for MIDI inputs and recording settings");
 348:         if(button("",metro_x,8,22,22,ui.metronome)) { ui.metronome=!ui.metronome; audio_metronome(ui.metronome); }
 349:         icon(ICON_METRO,metro_x+11,19,20,ui.metronome?ui_theme.selected_text:ink);
 350:         if(hover(metro_x,8,22,22)) snprintf(ui.status,sizeof ui.status,"Metronome: %s | beat clicks during playback; first beat accented; excluded from WAV export",ui.metronome?"On":"Off");
 351:         if(drag_button(TextFormat("%.2f",ui.project.bpm),tempo_x,8,56,22,ui.control_drag==&ui.project.bpm) && !recording_active()) capture_control(&ui.project.bpm,30,300,0);
 352:         if(hover(tempo_x,8,56,22) && !recording_active()) {
 353:             ui.project.bpm=fmaxf(30,fminf(300,ui.project.bpm+GetMouseWheelMove()));
 355:             if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(&ui.project.bpm,30,300,120,"Tempo (BPM)");
 357:         uint64_t step=(uint64_t)(audio_position()/(RATE*60.0/ui.project.bpm/4));
 358:         ui_surface((Rectangle){position_x,8,88,22},bg);
 359:         label(TextFormat("%03llu:%02llu",(unsigned long long)(step/16+1),(unsigned long long)(step%16+1)),position_x+10,10,18,ui_theme.signal);
 360:         if(hover(position_x,8,88,22)) snprintf(ui.status,sizeof ui.status,"Playback position: bar and sixteenth-note step");
 361:         for(int i=0;i<4;i++) { int id=i==1?2:i==2?1:i; editor_button(id,editors_x+i*24); }
 362:         if(button("",follow_x,8,22,22,ui.follow_playhead)) { ui.follow_playhead=!ui.follow_playhead; if(!ui.follow_playhead) { ui.arrangement.view_start=fmaxf(0,ui.arrangement.view_start); for(int p=0;p<PATTERNS;p++) ui.piano_pan[p]=fmaxf(0,ui.piano_pan[p]); } }
 363:         icon(ICON_FOLLOW,follow_x+11,19,20,ui.follow_playhead?ui_theme.selected_text:ink);
 364:         if(hover(follow_x,8,22,22)) snprintf(ui.status,sizeof ui.status,"Follow playhead: center playback in the Playlist (Song) or Piano Roll (PAT)");
 365:         if(button("",keys_x,8,22,22,ui.typing_keys)) ui.typing_keys=!ui.typing_keys;
 366:         icon(ICON_KEYS,keys_x+11,19,20,ui.typing_keys?ui_theme.selected_text:ink);
 367:         if(hover(keys_x,8,22,22)) snprintf(ui.status,sizeof ui.status,"Typing piano: Z row C3, Q row C4; S/D/G/H/J and 2/3/5/6/7 sharps; disabled in Browser/text fields");
 369:         ui.visual_step=ui.playing && !ui.reset?audio_visual_position()/(RATE*60.0/ui.project.bpm/4):playback_start(); automation_display_update();
 370:         int browser_hover=!ui.browser_hidden && ui.mouse.x<sidebar && ui.mouse.y>=42 && ui.mouse.y<height-22;
 372:         if(analyzer_width>0) {
 373:             DrawRectangle(audio_x,8,analyzer_width,22,bg);
 374:             spectrum_draw(&ui.master_spectrum,(Rectangle){audio_x+2,9,analyzer_width-4,20},0);
 375:             int mx=audio_x+analyzer_width+gap; DrawRectangle(mx,8,40,22,bg);
 376:             for(int side=0;side<2;side++) {
 377:                 float level=fmaxf(0,fminf(1,(20*log10f(fmaxf(.001f,ui.meter_level[0][side]))+60)/60));
 378:                 DrawRectangle(mx+2,10+side*10,36*level,7,accent);
 379:                 if(ui.meter_hold[0][side]>=1) DrawRectangle(mx+36,10+side*10,2,7,ui_theme.meter_high);
 381:             if(hover(audio_x,8,analyzer_width+gap+40,22)) snprintf(ui.status,sizeof ui.status,audio_ok?"Master frequency spectrum and stereo level · red indicates clipping":"Audio device unavailable");
 383:         ui.input_enabled=!modal && !ui.pattern_popup && !ui.context_kind && !dragging && !captured() && ui.windows.grab<0 && ui.mouse.x<sidebar+6 && ui.mouse.y>=42;
 384:         DrawRectangle(0,42,sidebar,height-66,ui_theme.browser);
 386:             if(hover(0,42,sidebar,height-66)) snprintf(ui.status,sizeof ui.status,"Browser: click to preview; drag to load | Up/Down or j/k: select | Left/Right or h/l: fold");
 387:             label("Browser",10,50,14,ink);
 388:             if(button("+ Add folder",8,76,sidebar-16,23,0)) { snprintf(ui.folder_text,sizeof ui.folder_text,"%s",ui.browser.path); ui.rename_select_all=1; open_popup(3); }
 389:             int fy=108,rows=fmaxf(1,(height-fy-98)/23);
 390:             if(!modal && !ui.pattern_popup && !ui.context_kind && (ui.browser_focus || browser_hover) && !dragging && !captured() && !command_down()) {
 391:                 if(IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_K)) audition_entry((int)fmaxf(0,ui.browser.selected-1));
 392:                 if(IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_J)) audition_entry((int)fminf(ui.browser.items-1,ui.browser.selected+1));
 393:                 if(IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_H)) browser_left(&ui.browser);
 394:                 if(IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_L)) browser_right(&ui.browser);
 396:             if(ui.browser.follow && ui.browser.selected>=0) {
 397:                 ui.browser.follow=0;
 399:                 if(ui.browser.selected>=ui.browser.scroll+rows) ui.browser.scroll=ui.browser.selected-rows+1;
 401:             if(hover(0,fy,sidebar,rows*23)) ui.browser.scroll-=(int)GetMouseWheelMove()*3;
 402:             ui.browser.scroll=(int)fmaxf(0,fminf(fmaxf(0,ui.browser.items-rows),ui.browser.scroll));
 403:             BeginScissorMode(0,fy*scale,(sidebar-4)*scale,rows*23*scale);
 406:                 int y=fy+(entry-ui.browser.scroll)*23,indent=12+node->depth*14;
 407:                 int clicked=button_color("",8,y,sidebar-16,21,ui.browser.selected==entry,ui_theme.browser);
 410:                     if(node->open) DrawTriangle((Vector2){indent,y+8},(Vector2){indent+4,y+13},(Vector2){indent+8,y+8},color);
 411:                     else DrawTriangle((Vector2){indent,y+6},(Vector2){indent,y+14},(Vector2){indent+6,y+10},color);
 414:                 label(fit_text(node->depth==0 && !strcmp(path,ui.browser.samples)?"LibreLoop samples":name,fmaxf(0,sidebar-indent-24),13),indent+14,y+4,13,color);
 415:                 if(hover(8,y,sidebar-16,21)) snprintf(ui.status,sizeof ui.status,"%.80s: %s | j/k: select; h/l: fold",path,node->dir?"click to expand/collapse":IsFileExtension(path,PROJECT_FILE_SUFFIX)?"click to open project":"click to preview; drag to Rack or Playlist");
 418:                     if(node->dir) { if(!browser_toggle(&ui.browser,entry)) snprintf(ui.status,sizeof ui.status,"Folder unavailable: %.180s",path); break; }
 421:                         audition_entry(entry); snprintf(ui.sample_drag,sizeof ui.sample_drag,"%s",path);
 422:                         ui.sample_origin=ui.mouse; ui.sample_moved=0;
 423:                         ui.sample_offset=(Vector2){ui.mouse.x-8,ui.mouse.y-y}; ui.sample_drag_width=sidebar-16; ui.sample_text_offset=indent+6;
 428:             if(ui.browser.selected>=0 && ui.browser.selected<ui.browser.items && ui.audition.data && !strcmp(ui.browser.nodes[ui.browser.selected].path,ui.audition_path)) {
 429:                 draw_waveform((WaveDisplay){.sample=ui.audition,.wave=&ui.audition_wave,.origin=8,.width=sidebar-16},
 430:                     (Rectangle){8,height-98,sidebar-16,68},8,sidebar-8,ui_theme.waveform);
 431:                 if(hover(8,height-98,sidebar-16,68)) {
 432:                     snprintf(ui.status,sizeof ui.status,"Click waveform to replay %.100s | Up/Down or j/k: select; Left/Right or h/l: fold",GetFileName(ui.audition_path));
 433:                     if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.browser_focus=1; audio_preview(ui.audition); }
 436:                 if(ui.browser_progress>=0) {
 437:                     float x=8+ui.browser_progress*(sidebar-16);
 438:                     DrawLineEx((Vector2){x,height-98},(Vector2){x,height-30},1.5f,ink);
 442:         int resize_hover=hover(sidebar,42,6,height-66);
 443:         DrawRectangle(sidebar,42,6,height-66,ui.browser_resize || resize_hover?accent:cell);
 445:         EndMode2D();
 448:         for(int i=0;i<EDITORS;i++) { ui.input_enabled=!modal && !ui.pattern_popup && !ui.context_kind && !dragging && !ui.sample_drag[0] && !captured(); draw_editor(order[i],scale); }
 450:         BeginMode2D((Camera2D){.zoom=scale}); DrawRectangle(0,height-22,width,22,panel); label(fit_text(ui.status,width-16,11),8,height-17,11,ink);
 451:         draw_browser_drag();
 452:         draw_picker_drag();
 453:         draw_popup(); draw_context(); EndMode2D();
 455:         if(!captured() && !recording_active()) history_checkpoint();
 456:         if(ui.pattern_popup!=11 && ui.file_picker.entries) file_chooser_close(&ui.file_picker);
 458:         if(recording_active()) { ui.project.bpm=ui.recording_ui.active?ui.recording_ui.bpm:ui.midi_take.bpm; ui.song=ui.playing=1; ui.reset=0; }
 459:         flush_cursor(); audio_update(&ui.project,ui.playing,ui.song,ui.pattern,ui.reset,ui.output_volume,playback_start(),recording_active()?0:playback_loop()[0],recording_active()?0:playback_loop()[1]);
 460:         int animate=audio_active() || ui.mixer_decay_active || ui.spectrum_active || (ui.windows.editors[0].visible && ui.channel_decay_active) || ui.browser_progress>=0 || (ui.windows.editors[3].visible && (audio_active() || ui.mixer_decay_active)) || ui.playing || captured() || ui.windows.grab>=0 || ui.popup_drag || ui.sample_drag[0] ||
 461:             (ui.windows.editors[4].visible && audio_key_position(127,ui.instrument_channel)>=0);
 462:         if(!smoke && !animate && GetTime()-last_activity>.15) EnableEventWaiting(); else DisableEventWaiting();
 464:         if(smoke && ++frames%10==0) {
 465:             int stage=frames/10;
 466:             TakeScreenshot(stage==1?"libreloop-smoke.png":TextFormat("libreloop-view-%d.png",stage-1));
 467:             if(stage==4) break;
 468:             if(stage==1) windows_focus(&ui.windows,1);
 469:             if(stage==2) windows_focus(&ui.windows,2);
 470:             if(stage==3) windows_focus(&ui.windows,3);
 475:     for(int pat=0;pat<PATTERNS;pat++) if(ui.previews[pat].image.id) UnloadRenderTexture(ui.previews[pat].image);
 481:     for(int c=0;c<CHANNELS;c++) { free(ui.audio_waves[c].tree); free(ui.sampler_views[c].wave.tree); sample_free(ui.samples[c]); sample_free(ui.originals[c]); }
 482:     return 0;
```

## midi.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### midi_parse

```c
   7:     for(unsigned i=0;i<count;i++) {
   8:         unsigned char b=data[i]; if(b>=0xf8) continue;
   9:         if(b&0x80) {
  10:             p->count=0;
  11:             if(b>=0xf0) { p->status=0; p->sysex=b==0xf0; continue; }
  12:             p->status=b; p->sysex=0; continue;
  16:         unsigned needed=(p->status&0xf0)==0xc0 || (p->status&0xf0)==0xd0?1:2;
  17:         if(p->count==needed) { emit(context,(MidiEvent){p->status,p->data[0],needed==2?p->data[1]:0,time}); p->count=0; }
```

### midi_bind

```c
  21:     float v,lo,hi;if(channel>15 || controller>119 || !parameter_info(p,target,&v,&lo,&hi))return 0;
  22:     int i=0;for(;i<p->midi_binding_count;i++)if(p->midi_bindings[i].channel==channel && p->midi_bindings[i].controller==controller)break;
  23:     if(i>=MIDI_BINDINGS)return 0;if(i==p->midi_binding_count)p->midi_binding_count++;
  24:     p->midi_bindings[i]=(MidiBinding){channel,controller,target};return 1;
```

### midi_unbind

```c
  27:     for(int i=p->midi_binding_count-1;i>=0;i--){ParameterTarget x=p->midi_bindings[i].target;
  28:         if(x.parameter==target.parameter && x.owner==target.owner && x.slot==target.slot){memmove(&p->midi_bindings[i],&p->midi_bindings[i+1],(--p->midi_binding_count-i)*sizeof p->midi_bindings[0]);memset(&p->midi_bindings[p->midi_binding_count],0,sizeof p->midi_bindings[0]);}}
```

### position

```c
  30: static float position(const MidiTake *t,double time) { return fmax(0,(time-t->time)*t->bpm/15); }
```

### free_lane

```c
  32:     for(int l=0;l<LANES;l++) { int used=0;for(int j=0;j<CLIPS;j++)used|=p->clips[l][j]!=0;if(!used)return l; } return -1;
```

### midi_take_begin

```c
  35:     if(channel<0 || channel>=p->channel_count || !isfinite(origin) || origin<0 || !isfinite(time))return 0;
  36:     int lane=notes?free_lane(p):-1;
  37:     if(notes && (p->pattern_count>=PATTERNS || lane<0))return 0;
  38:     memset(t,0,sizeof *t); memset(t->held,0xff,sizeof t->held);
  39:     t->active=1;t->pattern=-1;t->channel=channel;t->lane=lane;t->clip=0;t->origin=origin;t->time=time;t->bpm=p->bpm;
  41:         t->lane_mute[0]=p->lane_mute[lane];p->lane_mute[lane]=1;
  45:         t->previous_steps=p->pattern_steps[t->pattern];t->previous_start=p->clip_starts[lane][0];
  46:         t->previous_cap=p->clip_steps[lane][0];t->previous_offset=p->clip_offsets[lane][0];
  48:         memset(p->notes[t->pattern],0,sizeof p->notes[t->pattern]);
  49:         snprintf(p->pattern_names[t->pattern],PATTERN_NAME,"MIDI take %d",t->pattern+1);
  50:         p->clips[lane][0]=t->pattern+1;p->clip_starts[lane][0]=origin/STEPS;p->clip_steps[lane][0]=.01f;p->clip_offsets[lane][0]=0;
  52:     return 1;
```

### midi_take_note

```c
  55:     if(!t->active || t->pattern<0 || channel<0 || channel>15 || pitch<0 || pitch>127 || !isfinite(time))return 0;
  57:     if(*slot>=0) { Note *n=&p->notes[t->pattern][t->channel][*slot];n->length=fmaxf(.01f,step-n->start);*slot=-1; }
  58:     if(velocity>0) {
  59:         p->pattern_steps[t->pattern]=fmaxf(p->pattern_steps[t->pattern],ceilf((step+.01f)/STEPS)*STEPS);
  60:         Note *n=note_add(p,t->pattern,t->channel,step,pitch,.01f);
  61:         if(!n) {t->failed=1;return 0;}n->velocity=fminf(127,velocity);*slot=n-p->notes[t->pattern][t->channel];
  63:     return 1;
```

### midi_take_control

```c
  66:     float original,lo,hi;if(!t->active || !parameter_info(p,target,&original,&lo,&hi) || !isfinite(time) || !isfinite(value))return 0;
  67:     int entry=0;for(;entry<t->count;entry++) {ParameterTarget x=p->automations[t->curves[entry]].target;if(x.parameter==target.parameter && x.owner==target.owner && x.slot==target.slot)break;}
  69:         int lane=free_lane(p);if(lane<0 || t->count>=AUTOMATIONS){t->failed=1;return 0;}
  70:         int a=automation_create(p,target,parameter_descriptor(target.parameter)->name,STEPS);if(a<0){t->failed=1;return 0;}
  71:         t->lane_mute[entry+1]=p->lane_mute[lane];p->lane_mute[lane]=1;
  72:         t->curves[entry]=a;t->curve_lane[entry]=lane;t->curve_clip[entry]=0;t->count++;
  73:         p->automations[a].count=0;p->clips[lane][0]=AUTOMATION_SOURCE+a+1;p->clip_starts[lane][0]=t->origin/STEPS;p->clip_steps[lane][0]=.01f;p->clip_offsets[lane][0]=0;
  74:         float initial=fmaxf(0,fminf(1,(original-lo)/(hi-lo)));
  75:         automation_point(&p->automations[a],0,initial);
  76:         float first=position(t,time);p->automations[a].steps=fmaxf(STEPS,ceilf((first+.01f)/STEPS)*STEPS);
  77:         if(first>.01f)automation_point(&p->automations[a],first-.01f,initial);
  79:     Automation *a=&p->automations[t->curves[entry]];float step=position(t,time);a->steps=fmaxf(a->steps,ceilf((step+.01f)/STEPS)*STEPS);
  80:     value=fmaxf(0,fminf(1,value));
  81:     if(a->count)step=fmaxf(step,a->points[a->count-1].step);
  82:     if(a->count>=2 && fabsf(a->points[a->count-1].value-value)<.000001f && fabsf(a->points[a->count-2].value-value)<.000001f) {
  83:         a->points[a->count-1].step=step;return 1;
  88:         int remove=1;float smallest=INFINITY;
  89:         for(int i=1;i<a->count-1;i++){AutomationPoint x=a->points[i-1],y=a->points[i],z=a->points[i+1];
  90:             float span=z.step-x.step,estimate=span>0?x.value+(z.value-x.value)*(y.step-x.step)/span:y.value;
  92:         memmove(&a->points[remove],&a->points[remove+1],(--a->count-remove)*sizeof a->points[0]);
  94:     if(automation_point(a,step,value)<0)return 0;
  95:     return 1;
```

### midi_take_update

```c
  98:     if(!t->active || !isfinite(time))return;float step=position(t,time),length=fmaxf(.01f,step);
  99:     if(t->pattern>=0) {
 100:         for(int c=0;c<16;c++)for(int k=0;k<128;k++)if(t->held[c][k]>=0){Note *n=&p->notes[t->pattern][t->channel][t->held[c][k]];n->length=fmaxf(.01f,step-n->start);}
 101:         p->pattern_steps[t->pattern]=fmaxf(p->pattern_steps[t->pattern],ceilf((step+.01f)/STEPS)*STEPS);
 104:     for(int i=0;i<t->count;i++)p->clip_steps[t->curve_lane[i]][t->curve_clip[i]]=length;
```

### midi_take_finish

```c
 107:     midi_take_update(t,p,time);t->active=0;
 108:     if(t->pattern>=0)p->lane_mute[t->lane]=t->lane_mute[0];
 109:     for(int i=0;i<t->count;i++)p->lane_mute[t->curve_lane[i]]=t->lane_mute[i+1];
 110:     if(t->pattern>=0){int used=0;for(int i=0;i<NOTES;i++)used|=p->notes[t->pattern][t->channel][i].velocity;if(!used && t->pattern==p->pattern_count-1){p->clips[t->lane][t->clip]=0;p->pattern_count--;
 115:     memset(t->held,0xff,sizeof t->held);
```

## navigation.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### navigation_motion

```c
   5:     if(zoom_modifier) { motion.zoom+=input.y*(input.precise?.05f:1); return motion; }
   6:     float distance=input.precise?1/scale:48;
```

## recording.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### recording_refresh

```c
  14:     int ok=1;
  15:     for(int i=0;i<session->count;i++) {
  17:         if(recording_writer_failed(take->writer)) ok=0;
  18:         if(take->clip<0 || project->clips[take->lane][take->clip]!=PATTERNS+take->channel+1) {
  19:             take->clip=-1;
  20:             for(int l=0;l<LANES && take->clip<0;l++) for(int b=0;b<CLIPS;b++) if(project->clips[l][b]==PATTERNS+take->channel+1) { take->lane=l; take->clip=b; break; }
  25:             if(!sample_map_recording(take->path,frames,&mapped)) { ok=0; continue; }
  27:             if(!waveform_append(&take->wave,take->sample,old)) ok=0;
  29:             if(take->clip>=0) project->clip_steps[take->lane][take->clip]=project->audio_seconds[take->channel];
```

### recording_prepare

```c
  35:     if(session->active || session->count || !buses || next->channel_count<0 || next->channel_count>CHANNELS ||
  36:        count<1 || count>CHANNELS-next->channel_count || !isfinite(start) || start<0) return 0;
  37:     for(int i=0;i<count;i++) if(buses[i]<0 || buses[i]>next->insert_count) return 0;
  38:     session->count=0;
  39:     for(int i=0;i<count;i++) {
  40:         int lane=0;
  41:         for(;lane<LANES;lane++) { int occupied=0; for(int b=0;b<CLIPS;b++) occupied|=next->clips[lane][b]!=0; if(!occupied) break; }
  43:         RecordingTake *take=&session->takes[i]; memset(take,0,sizeof *take);
  45:         session->count=i+1;
  47:         if(strlen(recording_writer_path(take->writer))>=sizeof next->paths[0]) goto failed;
  49:         int c=take->channel; next->instrument[c]=INSTRUMENT_SAMPLER; next->fm[c]=fm_default(); next->channel_audio[c]=1; next->audio_seconds[c]=.001f;
  50:         next->volume[c]=1; next->pan[c]=next->channel_pitch[c]=0; next->pitch_range[c]=2; next->mute[c]=0;
  51:         next->route[c]=0; next->sampler[c]=(Sampler){.time=1,.length=1};
  53:         snprintf(next->channel_names[c],PATTERN_NAME,"%s take",buses[i]?next->insert_names[buses[i]-1]:"Master");
  54:         unsigned colors[CHANNELS]; int ncolors=0;
  55:         for(int j=0;j<c;j++) if(next->channel_audio[j]) colors[ncolors++]=next->channel_colors[j];
  57:         take->clip=arrangement_place(next,lane,start/STEPS,PATTERNS+c,.001f*next->bpm/15);
  58:         if(take->clip<0) goto failed;
  59:         next->clip_steps[lane][take->clip]=0;
  61:     return 1;
  63:     recording_cancel(session); return 0;
```

### recording_start_workers

```c
  66:     for(int i=0;i<session->count;i++) if(!recording_writer_start(session->takes[i].writer,recording_read,NULL,i)) return 0;
  67:     return 1;
```

### recording_finish_writers

```c
  71:     for(int i=0;i<session->count;i++) if(!recording_writer_finish(session->takes[i].writer)) ok=0;
  72:     if(!recording_refresh(session,project)) ok=0;
  73:     session->active=0; return ok;
```

### recording_cancel

```c
  76:     for(int i=0;i<session->count;i++) {
  77:         RecordingTake *take=&session->takes[i]; recording_writer_free(take->writer,1);
  78:         sample_free(take->sample); free(take->wave.tree); memset(take,0,sizeof *take);
  80:     session->count=0;
```

## recording_writer.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### le

```c
  20: static void le(unsigned char *out,uint32_t value,int bytes) { for(int i=0;i<bytes;i++) out[i]=(value>>(8*i))&255; }
```

### header

```c
  22:     unsigned char data[44]={0};
  23:     memcpy(data,"RIFF",4); le(data+4,36+frames*8,4); memcpy(data+8,"WAVEfmt ",8);
  24:     le(data+16,16,4); le(data+20,3,2); le(data+22,2,2); le(data+24,RATE,4);
  25:     le(data+28,RATE*8,4); le(data+32,8,2); le(data+34,32,2); memcpy(data+36,"data",4); le(data+40,frames*8,4);
  26:     size_t at=0;
  29:         if(n<0 && errno==EINTR) continue;
  30:         if(n<=0) return 0;
  33:     return 1;
```

### recording_writer_open

```c
  36:     RecordingWriter *w=calloc(1,sizeof *w); if(!w) return NULL;
  38:     w->fd=mkstemps(w->path,4);
  39:     if(w->fd<0) { free(w); return NULL; }
  40:     atomic_init(&w->frames,0); atomic_init(&w->stop,0); atomic_init(&w->failed,0);
  41:     if(!header(w,0) || lseek(w->fd,44,SEEK_SET)!=44) { close(w->fd); unlink(w->path); free(w); return NULL; }
```

### worker

```c
  45:     RecordingWriter *w=context; float pcm[4096*2]; unsigned char bytes[4096*8];
  46:     unsigned checkpoint=0;
  48:         unsigned n=w->read(w->context,w->take,pcm,4096);
  52:             struct timespec pause={0,5000000}; nanosleep(&pause,NULL); continue;
  54:         if(n>4096 || n>SAMPLE_MAX_FRAMES-old) { atomic_store(&w->failed,1); break; }
  55:         for(unsigned i=0;i<n*2;i++) {
  56:             float value=isfinite(pcm[i])?pcm[i]:0; uint32_t bits; memcpy(&bits,&value,4); le(bytes+i*4,bits,4);
  58:         size_t at=0,total=(size_t)n*8;
  61:             if(written<0 && errno==EINTR) continue;
  62:             if(written<=0) { atomic_store(&w->failed,1); break; } at+=(size_t)written;
  66:         if(old+n-checkpoint>=RATE) { if(!header(w,old+n)) { atomic_store(&w->failed,1); break; } checkpoint=old+n; }
  69:     if(ftruncate(w->fd,44+(off_t)frames*8) || !header(w,frames) || fsync(w->fd)) atomic_store(&w->failed,1);
```

### recording_writer_start

```c
  74:     if(w->started || !read || w->fd<0) return 0;
  76:     if(pthread_create(&w->thread,NULL,worker,w)) return 0;
  77:     w->started=1; return 1;
```

### recording_writer_finish

```c
  82:     if(w->started) { atomic_store(&w->stop,1); pthread_join(w->thread,NULL); w->started=0; }
  83:     if(w->fd>=0) { if(close(w->fd)) atomic_store(&w->failed,1); w->fd=-1; }
```

## sample_storage.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### sample_free

```c
  14:     if(atomic_fetch_sub(&sample.storage->refs,1)==1) {
```

### sample_clone

```c
  19:     if(source.frames && !source.data) return 0;
  20:     if(source.storage) { atomic_fetch_add(&source.storage->refs,1); *result=source; return 1; }
  24:         copy.data=malloc(bytes); if(!copy.data) return 0;
  27:     *result=copy; return 1;
```

### sample_map_recording

```c
  30:     if(!frames) { *result=(Sample){0}; return 1; }
  31:     /* Our writer emits a 44-byte, little-endian, float32 stereo WAV. */
  32:     uint32_t endian=1; if(*(unsigned char *)&endian!=1 || sizeof(float)!=4) return 0;
  33:     if(frames>SAMPLE_MAX_FRAMES) return 0;
  34:     size_t bytes=44+(size_t)frames*8;
  35:     int fd=open(path,O_RDONLY); if(fd<0) return 0;
  37:     if(fstat(fd,&info) || info.st_size<(off_t)bytes) { close(fd); return 0; }
  38:     void *mapping=mmap(NULL,bytes,PROT_READ,MAP_SHARED,fd,0); close(fd);
  39:     if(mapping==MAP_FAILED) return 0;
  41:     if(memcmp(header,"RIFF",4) || memcmp(header+8,"WAVEfmt ",8) ||
  42:        header[20]!=3 || header[21]!=0 || header[22]!=2 || header[23]!=0 ||
  43:        header[24]!=(RATE&255) || header[25]!=((RATE>>8)&255) || header[26]!=0 || header[27]!=0 ||
  44:        header[32]!=8 || header[33]!=0 || header[34]!=32 || header[35]!=0 || memcmp(header+36,"data",4)) {
  45:         munmap(mapping,bytes); return 0;
  48:     if(!storage) { munmap(mapping,bytes); return 0; }
  49:     storage->mapping=mapping; storage->bytes=bytes; atomic_init(&storage->refs,1);
  50:     *result=(Sample){.data=(float *)((char *)mapping+44),.frames=frames,.channels=2,.storage=storage};
  51:     return 1;
```

### sample_map_wav

```c
  55:     FILE *file=fopen(path,"rb"); if(!file) return 0;
  56:     unsigned char header[44]; size_t n=fread(header,1,sizeof header,file); fclose(file);
  57:     if(n!=sizeof header) return 0;
  58:     uint32_t bytes=(uint32_t)header[40] | (uint32_t)header[41]<<8 | (uint32_t)header[42]<<16 | (uint32_t)header[43]<<24;
  59:     if(!bytes || bytes%8) return 0;
  60:     return sample_map_recording(path,bytes/8,result);
```

## text_fonts.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
  10: static TextFont fonts[23];
```

### text_fonts_close

```c
  18:     for(int i=10;i<=22;i++) { unload(fonts[i].font); free(fonts[i].codepoints); }
  19:     memset(fonts,0,sizeof fonts); density=0;
```

### text_fonts_update

```c
  24:     for(int i=10;i<=22;i++) { unload(fonts[i].font); fonts[i].font=(Font){0}; fonts[i].dirty=1; }
```

### add

```c
  28:     if(codepoint<32 || codepoint>0x10ffff) return 1;
  29:     for(int i=0;i<atlas->count;i++) if(atlas->codepoints[i]==codepoint) return 1;
  31:         int next=atlas->capacity?atlas->capacity*2:256;
  32:         int *grown=realloc(atlas->codepoints,(size_t)next*sizeof *grown); if(!grown) return 0;
  35:     atlas->codepoints[atlas->count++]=codepoint; atlas->dirty=1; return 1;
```

### text_font

```c
  38:     if(size<10 || size>22 || density<=0) return GetFontDefault();
  40:     if(!atlas->count) for(int i=32;i<256;i++) if(!add(atlas,i)) break;
  41:     for(int i=0;text && text[i];) {
  42:         int bytes=1,codepoint=GetCodepointNext(text+i,&bytes); if(bytes<1) bytes=1;
  49:         atlas->font=LoadFontEx(font_path,(int)fmaxf(1,roundf(size*density)),atlas->codepoints,atlas->count);
  51:         SetTextureFilter(atlas->font.texture,TEXTURE_FILTER_BILINEAR); atlas->dirty=0;
```

### advance

```c
  62:     return font.glyphs[glyph].advanceX!=0?font.glyphs[glyph].advanceX:font.recs[glyph].width;
```

### text_font_width

```c
  71:     Font font=text_font(text,size); float position=0,left=0,right=0,maximum=0;
  72:     for(int i=0;text && text[i];) {
  73:         int bytes,codepoint=GetCodepointNext(text+i,&bytes); if(bytes<1) bytes=1;
  74:         if(codepoint=='\n') position=left=right=0;
  78:     return density>0?maximum/density:maximum;
```

### text_fit

```c
  82:     if(!text || width<=0) { out[0]=0; return; }
  83:     Font font=text_font(text,size); float position=0,left=0,right=0;
  84:     size_t used=0;
  85:     for(size_t i=0;text[i];) {
  86:         int bytes,codepoint=GetCodepointNext(text+i,&bytes); if(bytes<1) bytes=1;
  88:         if(codepoint=='\n') position=left=right=0;
  91:             if(ceilf(density>0?pixels/density:pixels)>width) break;
  95:     out[used]=0;
```

### text_button_size

```c
  98:     int preferred=(int)fmaxf(10,fminf(15,height-4));
  99:     for(int size=preferred;size>=10;size--) if(ceilf(text_font_width(text,size))<=width) return size;
```

## ui_instruments.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### sampler_toggle

```c
   5:     int over=hover(x,y,width,22);
   8:     if(over) ui_surface((Rectangle){x,y,width,22},ui_theme.hover);
   9:     circle(x+9,y+11,7,over?foreground:ui_theme.border); circle(x+9,y+11,5,cell);
  10:     if(active) circle(x+9,y+11,3,accent);
  11:     label(name,x+24,y+4,13,foreground);
  12:     return over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
```

### sampler_switch

```c
  15: void sampler_switch(int x,int y,int width,const char *name,uint8_t *flags,int bit) {
```

### fm_graph_update_at

```c
  20:     if(ui.fm_graph_channel<0 || ui.fm_graph_channel>=ui.project.channel_count) return;
  22:     if(ui.fm_graph_drag>=4) {
  23:         int stage=ui.fm_graph_drag-4; float slot=(ui.fm_graph_area.width-16)/4;
  24:         float position=fmaxf(0,fminf(1,(x-ui.fm_graph_area.x-8-slot*(stage+.1f))/(slot*.8f)));
  25:         s->dx7.value[ui.fm_dx_drag_base+stage]=roundf((1-position)*99);
  26:         s->dx7.value[ui.fm_dx_drag_base+4+stage]=roundf(fmaxf(0,fminf(99,(1-(y-ui.fm_graph_area.y-6)/fmaxf(1,ui.fm_graph_area.height-12))*99)));
  29:     if(ui.fm_graph_drag==3) {
  30:         float position=fmaxf(0,fminf(1,(x-ui.fm_graph_area.x-8)/(ui.fm_graph_area.width-16)));
  31:         s->lfo_rate=.1f*powf(120,position);
  32:         float amount=fmaxf(0,fminf(1,(ui.fm_graph_area.y+ui.fm_graph_area.height-22-y)/(ui.fm_graph_area.height-34)));
  33:         if(ui.fm_motion_target) s->tremolo=amount; else s->vibrato=amount*100;
  36:     int field=ui.fm_graph_drag==0?0:ui.fm_graph_drag==1?1:3;
  38:     float slot=(ui.fm_graph_area.width-24)/3;
  39:     float position=fmaxf(0,fminf(1,(x-ui.fm_graph_area.x-8-slot*ui.fm_graph_drag)/(.9f*slot)));
  40:     float value=expm1f(position*log1pf(info->high*100))/100;
  42:     if(ui.fm_graph_drag==1) {
  43:         float sustain=fmaxf(0,fminf(1,(ui.fm_graph_area.y+ui.fm_graph_area.height-22-y)/(ui.fm_graph_area.height-34)));
  44:         *(float *)fm_parameter_pointer(s,ui.fm_envelope_ids[ui.fm_tab][2])=sustain;
```

### fm_graph_update

```c
  49:     Rect r=ui.windows.editors[4].rect; fm_graph_update_at(ui.mouse.x-r.x,ui.mouse.y-r.y);
```

### fm_control

```c
  56:         (info->low>0 && (!strcmp(caption,"Attack") || !strcmp(caption,"Decay") || !strcmp(caption,"Release")));
  57:     int fixed_coarse=id>=PARAM_DX7_FIRST && id<PARAM_DX7_FIRST+126 &&
  58:         (id-PARAM_DX7_FIRST)%21==18 && s->dx7.value[id-PARAM_DX7_FIRST-1];
  60:         fixed_coarse?KNOB_FIXED_COARSE:logarithmic?KNOB_LOGARITHMIC:info->low<0?KNOB_CENTER:KNOB_NORMAL);
  61:     if(ui.control_drag==value && logarithmic) ui.control_logarithmic=1;
  62:     label(caption,x-text_width(caption,11)/2,y+17,11,muted);
  65:     if(id>=PARAM_DX7_FIRST && id<=PARAM_DX7_LAST) {
  66:         int index=id-PARAM_DX7_FIRST,field=index%21;
  67:         if(index>=DX7_NATIVE_PARAMETERS) shown=TextFormat("%.1f%%",display);
  68:         else if(index<126 && field==18) {
  70:             double fine=s->dx7.value[base+19]/100;
  71:             if(s->dx7.value[base+17]) {
  72:                 double frequency=pow(10,((int)display&3)+fine);
  73:                 shown=frequency>=1000?TextFormat("%.2f kHz",frequency/1000):
  74:                     TextFormat(frequency<10?"%.2f Hz":"%.0f Hz",frequency);
  75:             } else shown=TextFormat("%.2fx",fmaxf(.5f,display)*(1+fine));
  77:         else if(index<126 && field==8) { static const char *notes[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"}; int key=(int)display+17; shown=TextFormat("%s%d",notes[key%12],key/12-1); }
  78:         else if(index<126 && (field==11 || field==12)) { static const char *curves[]={"-Lin","-Exp","+Exp","+Lin"}; shown=curves[(int)display]; }
  79:         else if(index==136 || index==141) shown=display>=.5f?"On":"Off";
  80:         else if(index==142) { static const char *waves[]={"Triangle","Saw down","Saw up","Square","Sine","S&H"}; shown=waves[(int)display]; }
  81:         else if(index<126 && field==20) shown=TextFormat("%+.0f",display-7);
  82:         else if(index==134) shown=TextFormat("%.0f",display+1);
  83:         else if(index==144) shown=TextFormat("%+.0f st",display-24);
  84:         else shown=TextFormat("%.0f",display);
  86:     else if(id==PARAM_FM_FILTER_CUTOFF) shown=TextFormat(display>=1000?"%.2g kHz":"%.0f Hz",display>=1000?display/1000:display);
  87:     else if(id==PARAM_FM_CARRIER_RATIO || id==PARAM_FM_ATTACK_RATIO) shown=TextFormat("%.2gx",display);
  88:     else if(id==PARAM_FM_BODY_PITCH) shown=TextFormat("%+.1f st",display);
  89:     else if(id==PARAM_FM_CARRIER_DETUNE || id==PARAM_FM_BODY_DETUNE || id==PARAM_FM_ATTACK_DETUNE) shown=TextFormat("%+.0f ct",display);
  92:         shown=display<1?TextFormat("%.0f ms",display*1000):TextFormat("%.2g s",display);
  93:     else shown=TextFormat("%.2g",display);
  94:     label(shown,x-text_width(shown,10)/2,y+31,10,ink);
  95:     if(id>=PARAM_DX7_FIRST+DX7_NATIVE_PARAMETERS && id<=PARAM_DX7_LAST &&
  96:        (hover(x-16,y-16,32,32) || ui.control_drag==value))
  97:         snprintf(ui.status,sizeof ui.status,"Key tracking: 0%% fixed, 50%% half an octave per octave, 100%% follows notes. Anchor: middle C (C4).");
```

### fm_envelope_graph

```c
 102:     float slot=(plot.width-24)/3,base=plot.y+plot.height-22,range=plot.height-34;
 103:     float times[3]={*fm_parameter_pointer(s,ui.fm_envelope_ids[ui.fm_tab][0]),*fm_parameter_pointer(s,ui.fm_envelope_ids[ui.fm_tab][1]),*fm_parameter_pointer(s,ui.fm_envelope_ids[ui.fm_tab][3])};
 104:     int fields[]={0,1,3}; Vector2 handles[3];
 105:     float sustain=*fm_parameter_pointer(s,ui.fm_envelope_ids[ui.fm_tab][2]);
 106:     for(int i=0;i<3;i++) {
 108:         float t=log1pf(times[i]*100)/log1pf(info->high*100);
 109:         handles[i]=(Vector2){plot.x+8+slot*i+.9f*slot*t,i==0?base-range:i==1?base-sustain*range:base};
 110:         DrawLineEx((Vector2){plot.x+8+slot*i,plot.y+8},(Vector2){plot.x+8+slot*i,base},1,ui_theme.grid_minor);
 112:     smooth_line((Vector2){plot.x+8,base},handles[0],1.5f,accent);
 113:     Vector2 previous=handles[0];
 114:     for(int i=1;i<=40;i++) {
 115:         float t=i/40.f,level=ui.fm_tab?sustain+(1-sustain)*expf(-4.6051702f*t):1-(1-sustain)*t;
 116:         Vector2 point={handles[0].x+(handles[1].x-handles[0].x)*t,base-level*range};
 117:         smooth_line(previous,point,1.5f,accent); previous=point;
 119:     smooth_line(previous,handles[1],1.5f,accent);
 120:     Vector2 release={plot.x+8+slot*2,handles[1].y};
 121:     smooth_line(handles[1],release,1.5f,accent); smooth_line(release,handles[2],1.5f,accent);
 123:     for(int i=0;i<3;i++) {
 124:         circle(handles[i].x,handles[i].y,4,ink); circle(handles[i].x,handles[i].y,2,accent);
 125:         label(captions[i],plot.x+8+slot*i,base+7,10,muted);
 126:         if(hover(handles[i].x-7,handles[i].y-7,14,14)) {
 128:             if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.fm_graph_drag=i; ui.fm_graph_channel=ui.instrument_channel; ui.fm_graph_area=plot; ui.input_enabled=0; }
 131:     if(hover(plot.x,plot.y,plot.width,plot.height)) snprintf(ui.status,sizeof ui.status,"Drag envelope points: left/right changes time; the middle point also changes sustain. Knobs offer exact entry and automation.");
```

### fm_legacy_editor

```c
 136:     label(fit_text(ui.project.channel_names[c],width-298,14),12,30,14,ink); channel_controls(c,width);
 137:     label("Custom three-operator FM",12,56,11,muted);
 138:     const char *tabs[]={"Sound","Body","Attack","Motion"}; float tabw=(width-24)/4;
 139:     for(int i=0;i<4;i++) if(button(tabs[i],12+i*tabw,78,tabw-3,26,ui.fm_tab==i)) ui.fm_tab=i;
 140:     float column=(width-32)/4;
 141:     const unsigned head[4][4]={
 142:         {PARAM_FM_CARRIER_RATIO,PARAM_FM_CARRIER_DETUNE,PARAM_FM_VELOCITY,0},
 147:     const char *captions[4][4]={{"Pitch","Fine","Velocity",""},{"Pitch","Fine","Harmonic","FM amount"},{"Pitch","Fine","FM amount","Velocity"},{"Speed","Pitch motion","Volume motion","Fade in"}};
 149:     label(descriptions[ui.fm_tab],20,113,12,muted);
 150:     for(int i=0;i<4;i++) if(head[ui.fm_tab][i]) fm_control(s,head[ui.fm_tab][i],captions[ui.fm_tab][i],20+column*(i+.5f),139);
 151:     if(ui.fm_tab==0) {
 152:         int x=20+column*3;
 153:         if(button(s->routing?"Stacked":"Body + Attack",x,131,column-8,24,0)) open_context(20,c,(Vector2){ui.mouse.x+ui.windows.editors[4].rect.x,ui.mouse.y+ui.windows.editors[4].rect.y});
 154:         if(hover(x,131,column-8,24) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(&s->routing,0,1,0,"FM routing");
 155:         label("Routing",x+8,159,11,muted);
 157:     if(ui.fm_tab<3) {
 159:         label("Envelope",20,184,12,muted);
 160:         for(int i=0;i<4;i++) fm_control(s,ui.fm_envelope_ids[ui.fm_tab][i],names[i],20+column*(i+.5f),207);
 161:         fm_envelope_graph(s,(Rectangle){12,252,width-24,height-294});
 164:         if(button(shapes[(int)s->lfo_shape],12,184,112,24,0)) open_context(19,c,(Vector2){ui.mouse.x+ui.windows.editors[4].rect.x,ui.mouse.y+ui.windows.editors[4].rect.y});
 165:         if(hover(12,184,112,24) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(&s->lfo_shape,0,3,0,"LFO shape");
 166:         if(button("Pitch",132,184,72,24,!ui.fm_motion_target)) ui.fm_motion_target=0;
 167:         if(button("Volume",208,184,80,24,ui.fm_motion_target)) ui.fm_motion_target=1;
 168:         Rectangle plot={12,216,width-24,height-258}; DrawRectangleRec(plot,bg);
 169:         float base=plot.y+plot.height-22,range=plot.height-34,mid=plot.y+(plot.height-22)/2;
 170:         float amount=ui.fm_motion_target?s->tremolo:s->vibrato/100;
 171:         Vector2 previous={plot.x+8,mid};
 172:         for(int i=0;i<(int)plot.width-16;i++) {
 173:             float t=i/(plot.width-16)*s->lfo_rate*.5f;
 174:             Vector2 point={plot.x+8+i,mid-fm_lfo_value(t,(int)s->lfo_shape)*range*.42f};
 175:             if(i) smooth_line(previous,point,1.5f,Fade(accent,.35f+amount*.65f));
 178:         Vector2 handle={plot.x+8+logf(s->lfo_rate/.1f)/logf(120)*(plot.width-16),base-amount*range};
 179:         circle(handle.x,handle.y,5,ink); circle(handle.x,handle.y,3,accent);
 180:         label("Drag: speed / amount",plot.x+8,base+7,10,muted);
 181:         if(hover(plot.x,plot.y,plot.width,plot.height)) {
 184:             if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.fm_graph_drag=3; ui.fm_graph_channel=c; ui.fm_graph_area=plot; fm_graph_update_at(ui.mouse.x,ui.mouse.y); ui.input_enabled=0; }
 187:     if(button("Factory",12,height-34,98,24,0)) open_context(21,c,(Vector2){ui.mouse.x+ui.windows.editors[4].rect.x,ui.mouse.y+ui.windows.editors[4].rect.y});
 188:     if(button("Load",114,height-34,64,24,0)) preset_action(PRESET_FM,c,0,0);
 189:     if(button("Save",182,height-34,64,24,0)) preset_action(PRESET_FM,c,0,1);
 190:     double active=audio_key_position(127,c);
 191:     if(button(active>=0?"Release C4":"Play C4",width-112,height-34,100,24,active>=0)) { ui.channel=c; audio_key(127,c,60,active<0); }
```

### fm_sound_name

```c
 195:     for(int i=0;i<FM_FACTORY_COUNT;i++) { FMSettings factory=fm_factory(i); if(!memcmp(s,&factory,sizeof factory)) return fm_factory_name(i); }
```

### fm_footer

```c
 200:     if(button("Factory",12,height-34,98,24,0)) open_context(21,c,(Vector2){ui.mouse.x+ui.windows.editors[4].rect.x,ui.mouse.y+ui.windows.editors[4].rect.y});
 201:     if(button("Load",114,height-34,64,24,0)) preset_action(PRESET_FM,c,0,0);
 202:     if(button("Save",182,height-34,64,24,0)) preset_action(PRESET_FM,c,0,1);
 203:     int active=audio_key_position(127,c)>=0;
 204:     if(button(active?"Release C4":"Play C4",width-112,height-34,100,24,active)) { ui.channel=c; audio_key(127,c,60,!active); }
```

### fm_controls_row

```c
 208:     float column=(width-32)/count;
 209:     for(int i=0;i<count;i++) fm_control(s,ids[i],names[i],20+column*(i+.5f),y);
```

### dx7_envelope_graph

```c
 212: void dx7_envelope_graph(FMSettings *s,int base,Rectangle plot) {
 213:     DrawRectangleRec(plot,bg); float width=(plot.width-16)/4,height=fmaxf(1,plot.height-12);
 214:     Vector2 previous={plot.x+8,plot.y+height};
 215:     for(int i=0;i<4;i++) {
 216:         float rate=s->dx7.value[base+i],level=s->dx7.value[base+4+i];
 217:         Vector2 point={plot.x+8+width*(i+.1f+.8f*(1-rate/99)),plot.y+6+height*(1-level/99)};
 218:         smooth_line(previous,point,1.5f,accent); circle(point.x,point.y,3,ink); previous=point;
 219:         if(hover(point.x-7,point.y-7,14,14)) {
 220:             SetMouseCursor(MOUSE_CURSOR_POINTING_HAND); snprintf(ui.status,sizeof ui.status,"Drag stage %d: left/right changes rate, up/down changes level",i+1);
 221:             if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.fm_graph_drag=4+i; ui.fm_graph_channel=ui.instrument_channel; ui.fm_graph_area=plot; ui.fm_dx_drag_base=base; ui.input_enabled=0; }
```

### fm_dx_editor

```c
 228:     label(fit_text(ui.project.channel_names[c],width-298,14),12,30,14,ink); channel_controls(c,width);
 229:     label(fit_text(fm_sound_name(s),width-160,11),12,56,11,muted);
 230:     float tab=(width-24)/7;
 231:     for(int i=0;i<7;i++) if(button(i==6?"Global":TextFormat("Op %d",i+1),12+i*tab,78,tab-3,26,ui.fm_dx_operator==i)) ui.fm_dx_operator=i;
 232:     if(ui.fm_dx_operator<6) {
 233:         int base=(5-ui.fm_dx_operator)*21;
 234:         label(TextFormat("Operator %d",ui.fm_dx_operator+1),20,113,12,muted);
 235:         unsigned head[]={PARAM_DX7_FIRST+base+16,PARAM_DX7_FIRST+base+18,PARAM_DX7_FIRST+base+19,PARAM_DX7_FIRST+base+20,PARAM_DX7_FIRST+base+15,PARAM_DX7_FIRST+DX7_NATIVE_PARAMETERS+base/21};
 236:         const char *head_names[]={"Output","Coarse","Fine","Detune","Velocity","Tracking"}; fm_controls_row(s,head,head_names,6,width,136);
 237:         if(button("Envelope",12,184,100,24,!ui.fm_dx_page)) ui.fm_dx_page=0;
 238:         if(button("Keyboard",116,184,100,24,ui.fm_dx_page)) ui.fm_dx_page=1;
 239:         float *mode=&s->dx7.value[base+17];
 240:         if(button(*mode?"Hz at C4":"Ratio",width-112,184,100,24,0)) {
 241:             *mode=1-*mode; s->dx7.value[DX7_NATIVE_PARAMETERS+base/21]=*mode?0:100;
 242:             if(*mode) s->dx7.value[base+18]=(int)s->dx7.value[base+18]&3;
 244:         if(hover(width-112,184,100,24) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(mode,0,1,0,"Operator frequency mode");
 246:             unsigned rates[4],levels[4]; for(int i=0;i<4;i++) { rates[i]=PARAM_DX7_FIRST+base+i; levels[i]=PARAM_DX7_FIRST+base+4+i; }
 247:             const char *rnames[]={"Rate 1","Rate 2","Rate 3","Release rate"},*lnames[]={"Level 1","Level 2","Sustain level","Release level"};
 248:             fm_controls_row(s,rates,rnames,4,width,226); fm_controls_row(s,levels,lnames,4,width,292);
 249:             dx7_envelope_graph(s,base,(Rectangle){12,336,width-24,height-378});
 251:             const int fields[]={8,9,10,14,11,12,13,15}; const char *names[]={"Breakpoint","Left depth","Right depth","Amp motion","Left curve","Right curve","Key rate","Velocity"};
 252:             for(int row=0;row<2;row++) { unsigned ids[4]; for(int i=0;i<4;i++) ids[i]=PARAM_DX7_FIRST+base+fields[row*4+i]; fm_controls_row(s,ids,names+row*4,4,width,226+row*66); }
 253:             label("Curves: 0 -linear, 1 -exponential, 2 +exponential, 3 +linear",20,344,11,muted);
 256:         unsigned head[]={PARAM_DX7_FIRST+134,PARAM_DX7_FIRST+135,PARAM_DX7_FIRST+144,PARAM_DX7_FIRST+136};
 257:         const char *names[]={"Algorithm","Feedback","Transpose","Osc sync"}; label("Routing and tuning",20,113,12,muted); fm_controls_row(s,head,names,4,width,136);
 258:         if(button("Motion",12,184,100,24,!ui.fm_dx_page)) ui.fm_dx_page=0;
 259:         if(button("Pitch envelope",116,184,140,24,ui.fm_dx_page)) ui.fm_dx_page=1;
 261:             unsigned a[]={PARAM_DX7_FIRST+137,PARAM_DX7_FIRST+138,PARAM_DX7_FIRST+139,PARAM_DX7_FIRST+140},b[]={PARAM_DX7_FIRST+142,PARAM_DX7_FIRST+141,PARAM_DX7_FIRST+143};
 263:             fm_controls_row(s,a,an,4,width,226); fm_controls_row(s,b,bn,3,width,292);
 264:             label("Waves: triangle, saw down, saw up, square, sine, sample & hold",20,344,11,muted);
 266:             unsigned rates[4],levels[4]; for(int i=0;i<4;i++) { rates[i]=PARAM_DX7_FIRST+126+i; levels[i]=PARAM_DX7_FIRST+130+i; }
 267:             const char *rn[]={"Rate 1","Rate 2","Rate 3","Release rate"},*ln[]={"Level 1","Level 2","Sustain level","Release level"};
 268:             fm_controls_row(s,rates,rn,4,width,226); fm_controls_row(s,levels,ln,4,width,292);
 269:             dx7_envelope_graph(s,126,(Rectangle){12,336,width-24,height-378});
```

### fm_analog_editor

```c
 277:     label(fit_text(ui.project.channel_names[c],width-298,14),12,30,14,ink); channel_controls(c,width);
 278:     label(fit_text(fm_sound_name(s),width-160,11),12,56,11,muted);
 279:     const char *tabs[]={"Sound","Amplitude","Filter","Motion"}; float tab=(width-24)/4;
 280:     for(int i=0;i<4;i++) if(button(tabs[i],12+i*tab,78,tab-3,26,ui.fm_analog_tab==i)) ui.fm_analog_tab=i;
 281:     if(ui.fm_analog_tab==0) {
 283:         const char *an[]={"Pitch","Detune","Osc 2","Sub","Noise"},*bn[]={"Pulse width","PWM","Chorus","Velocity"};
 284:         label("Oscillators",20,113,12,muted); fm_controls_row(s,a,an,5,width,136);
 286:         if(button(waves[(int)s->analog.wave],12,184,112,24,0)) open_context(23,c,(Vector2){ui.mouse.x+ui.windows.editors[4].rect.x,ui.mouse.y+ui.windows.editors[4].rect.y});
 287:         if(hover(12,184,112,24) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(&s->analog.wave,0,3,0,"Oscillator waveform");
 288:         fm_controls_row(s,b,bn,4,width,226); label("PWM moves the Pulse shape. Chorus spreads the sound in stereo.",20,296,11,muted);
 289:     } else if(ui.fm_analog_tab==1 || ui.fm_analog_tab==2) {
 290:         ui.fm_tab=ui.fm_analog_tab==1?0:1;
 291:         if(ui.fm_analog_tab==2) {
 293:             label("Filter",20,113,12,muted); fm_controls_row(s,a,names,4,width,136);
 294:         } else label("Volume envelope",20,113,12,muted);
 295:         unsigned ids[4]; for(int i=0;i<4;i++) ids[i]=ui.fm_envelope_ids[ui.fm_tab][i]; const char *names[]={"Attack","Decay","Sustain","Release"};
 296:         fm_controls_row(s,ids,names,4,width,207); fm_envelope_graph(s,(Rectangle){12,252,width-24,height-294});
 299:         label("Per-note modulation",20,113,12,muted); fm_controls_row(s,ids,names,4,width,136);
 301:         if(button(waves[(int)s->lfo_shape],12,184,112,24,0)) open_context(19,c,(Vector2){ui.mouse.x+ui.windows.editors[4].rect.x,ui.mouse.y+ui.windows.editors[4].rect.y});
 302:         if(hover(12,184,112,24) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(&s->lfo_shape,0,3,0,"LFO shape");
 303:         label("Pitch motion adds vibrato; volume motion adds tremolo. PWM follows this LFO.",20,228,11,muted);
```

### fm_editor

```c
 310:     if(s->engine==1) fm_dx_editor(width,height); else if(s->engine==2) fm_analog_editor(width,height); else fm_legacy_editor(width,height);
 312:     if(button(engines[(int)s->engine],width-132,52,120,22,0)) open_context(22,ui.instrument_channel,(Vector2){ui.mouse.x+ui.windows.editors[4].rect.x,ui.mouse.y+ui.windows.editors[4].rect.y});
 313:     if(hover(width-132,52,120,22)) snprintf(ui.status,sizeof ui.status,"Choose the synthesis engine; factory presets select their own engine");
```

### sampler

```c
 319:     label(fit_text(channel_caption(c),width-298,14),12,30,14,ink);
 321:     label(fit_text(!ui.originals[c].frames?"Drop a sample from the Browser":ui.project.paths[c][0]?GetFileName(ui.project.paths[c]):"Built-in sample",width-298,12),12,54,12,muted);
 322:     float right=width/2+6,groupw=width/2-18;
 323:     label("Sample effects",20,82,12,muted);
 324:     sampler_switch(20,100,groupw-16,"Normalize",&settings->flags,SAMPLE_NORMALIZE);
 325:     sampler_switch(20,128,groupw-16,"Reverse",&settings->flags,SAMPLE_REVERSE);
 326:     sampler_switch(20,156,groupw-16,"Reverse polarity",&settings->flags,SAMPLE_POLARITY);
 327:     if(hover(20,100,groupw-16,22)) snprintf(ui.status,sizeof ui.status,"Normalize: bring the processed sample peak to full scale");
 328:     if(hover(20,128,groupw-16,22)) snprintf(ui.status,sizeof ui.status,"Reverse: play the cropped sample backwards");
 329:     if(hover(20,156,groupw-16,22)) snprintf(ui.status,sizeof ui.status,"Polarity: flip the waveform vertically");
 330:     label("Sample processing",right+8,82,12,muted);
 331:     knob_style(right+28,108,&settings->pitch,-12,12,0,"Process pitch (semitones)",KNOB_CENTER);
 332:     knob_style(right+84,108,&settings->time,.25f,4,1,"Time multiplier",KNOB_CENTER);
 333:     label("Pitch shift",right+4,128,11,muted); label("Time",right+71,128,11,muted);
 334:     if(button(settings->stretch?"Stretch":"Resample",right+116,98,groupw-128,22,0)) open_context(13,c,(Vector2){ui.mouse.x+ui.windows.editors[4].rect.x,ui.mouse.y+ui.windows.editors[4].rect.y});
 335:     DrawTriangle((Vector2){right+groupw-23,107},(Vector2){right+groupw-19,112},(Vector2){right+groupw-15,107},muted);
 336:     label("Mode",right+116,128,11,muted);
 337:     if(sampler_toggle(right+8,146,groupw-16,"Fit to tempo",settings->fit_bpm>0)) {
 338:         settings->fit_bpm=settings->fit_bpm?0:ui.project.bpm;
 340:     if(hover(right+8,146,groupw-16,22)) snprintf(ui.status,sizeof ui.status,"Fit to tempo: keep audio fixed on the grid when BPM changes; uses Resample/Stretch mode");
 341:     if(hover(right+116,98,groupw-128,22)) snprintf(ui.status,sizeof ui.status,"Time mode: Resample changes speed/pitch; Stretch preserves pitch; click to choose");
 342:     label("Presets",20,190,11,muted);
 343:     if(button("Load",20,210,(groupw-20)/2,24,0)) preset_action(PRESET_SAMPLER,c,0,0);
 344:     if(button("Save",24+(groupw-20)/2,210,(groupw-20)/2,24,0)) preset_action(PRESET_SAMPLER,c,0,1);
 345:     label("Sample region",right+8,182,12,muted);
 346:     knob(right+28,212,&settings->start,0,1,0,"Start");
 347:     knob(right+84,212,&settings->length,0,1,1,"Length");
 348:     label("Start",right+14,230,11,muted); label("Length",right+67,230,11,muted);
 349:     knob(right+156,212,&settings->trim,0,1,0,"Trim quiet beginning and end (threshold)");
 350:     label("Trim",right+143,230,11,muted);
 359:     float area=height-276;
 360:     Rectangle display={12,252,width-24,area};
 363:         .origin=12,.width=width-24},display,12,width-12,ui_theme.waveform);
 364:     double progress=audio_key_position(127,c);
 365:     if(hover(12,252,width-24,area)) {
 366:         snprintf(ui.status,sizeof ui.status,"Sample waveform: click to %s; drop a Browser sample here to load it",progress>=0?"stop playback":"play from the beginning");
 367:         if(ui.samples[c].frames && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.channel=c; audio_key(127,c,60,progress<0); }
 369:     if(progress>=0 && !live_preview) {
 370:         float x=12+progress*(width-24);
 371:         DrawLineEx((Vector2){x,252},(Vector2){x,252+area},1.5f,ink);
 373:     label("0 s",16,254,10,muted);
 374:     label(TextFormat("%.2f s%s",duration,pending?" *":""),width-78,254,10,muted);
 375:     if(live_preview && hover(12,252,width-24,area)) snprintf(ui.status,sizeof ui.status,"Live envelope preview; processed pitch/stretch detail appears when ready; audio changes after release");
 376:     if(ui.sample_drag[0] && ui.sample_moved) DrawRectangleLines(12,252,width-24,area,ui_theme.signal);
```

## ui_internal.h

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
  29: #include <GLFW/glfw3.h>
  41: enum { RECORD_AUDIO=1,RECORD_NOTES=2,RECORD_AUTOMATION=4 };
  43: enum { PLAYLIST_GRID_TOP=96, PLAYLIST_BOTTOM=18, PIANO_GRID_TOP=86, PIANO_GRID_PADDING=180, EDITOR_SCROLLBAR=16, EDITOR_CORNER=16 };
  63: #define IsMouseButtonPressed mouse_pressed
  71: #define RACK_STEP_WIDTH 16
  72: #define RACK_TOP (TITLE+14)
  73: #define MIXER_LEFT 62
  74: #define MIXER_PANEL 160
  75: #define KNOB_RADIUS 10
  76: #define SWING_RADIUS 8
  81:     unsigned pending_clicks[8],frame_clicks[8];
  92:     MidiDevice midi_devices[64];
  94:     char midi_selected[128],midi_selected_name[128],midi_settings_path[PATH_MAX];
  97:     MidiHeld midi_keys[32];
  98:     unsigned char midi_sustain[16];
 105:     Vector2 mouse;
 114:     int picker_tab,picker_scroll,picker_drag;
 120:     char menu_name[96];
 121:     Vector2 picker_origin,picker_offset;
 130:     int row_zoom_drag;
 135:     int marker_drag,ruler_loop_drag,stop_armed;
 138:     Vector2 ruler_last_position;
 139:     float song_loop[2],pattern_loop[PATTERNS][2],ruler_anchor;
 144:     const char *rack_filters[3];
 146:     Vector2 context_position;
 147:     int pattern_popup,popup_opened,rename_pattern,rename_select_all,popup_drag;
 148:     Vector2 popup_position[14],popup_offset;
 149:     char rename_text[PATTERN_NAME],folder_text[PATH_MAX],number_text[64];
 152:     char device_names[64][128];
 155:     int route_drag,route_start,mixer_selected,mixer_scroll,mixer_pan,cable_drag;
 163:     char sample_drag[PATH_MAX];
 164:     Vector2 sample_origin,sample_offset;
 165:     float sample_drag_width,sample_text_offset;
 166:     float *control_drag,control_low,control_high,control_bottom,control_range;
 169:     Note *note_drag;
 170:     int moving_note,piano_scroll_drag,piano_vdrag;
 173:     int piano_key_drag,piano_key,piano_key_channel;
 176:     uint8_t note_selected[PATTERNS][CHANNELS][NOTES],note_selection_before[NOTES];
 177:     uint8_t keyboard_notes[CHANNELS][128],playing_notes[CHANNELS][NOTES],playing_keys[CHANNELS][128];
 178:     int piano_tool,piano_gesture,piano_additive;
 179:     Vector2 piano_from,piano_now;
 180:     Vector2 note_grab;
 184:     int velocity_drag,velocity_pattern,velocity_channel;
 185:     Vector2 velocity_from,velocity_now;
 189:     int navigation_drag,navigation_active;
 190:     Vector2 navigation_last;
 191:     NavigationInput navigation_input;
 193:     int eq_drag;
 194:     int fm_dx_operator,fm_dx_page,fm_analog_tab,fm_dx_drag_base;
 195:     int fm_tab,fm_motion_target,fm_graph_drag,fm_graph_channel;
 197:     char status[256];
 202:     int file_action,file_field,file_confirm,file_scroll_drag,file_last_row;
 206:     Vector2 text_origin;
 207:     Texture2D circle_texture;
 208:     Texture2D icons;
 210:     Texture2D knob_arcs;
 212:     Texture2D cable_texture;
 215:         Vector2 pointer;
 216:         Rectangle bounds[128];
 219:     uint8_t automation_visible[AUTOMATIONS];
 225:     uint8_t channel_active_ui[CHANNELS];
 228:         RenderTexture2D image;
 233:     uint8_t track_active_ui[LANES];
 234:     float meter_level[INSERTS+1][2],meter_hold[INSERTS+1][2];
 235:     double meter_until[INSERTS+1][2],meter_time;
 236:     uint8_t record_armed[INSERTS+1];
 241:     const unsigned fm_envelope_ids[3][4];
 245: int shortcut_down(void);
 246: int command_down(void);
 270: int captured(void);
 290: void cable(Vector2 points[4],Color color);
 291: void smooth_line(Vector2 start,Vector2 end,float width,Color color);
 292: int hover(float x,float y,float w,float h);
 300: int color_picker(int x,int y,int width,uint32_t selected);
 304: void mute_light(float x,float y,uint8_t *states,int count,int selected,const char *name);
 306: void open_context(int kind,int target,Vector2 position);
 327: Color source_rgb(uint32_t rgb);
 358: void history_stamps(uint64_t stamps[CHANNELS]);
 372: Color pattern_rgb(uint32_t rgb);
 381: void draw_picker_drag(void);
 382: void draw_browser_drag(void);
 386: void ruler_press(int id,int button,float at,double time,Vector2 position);
 399: void piano_gesture_update(float step,float pitch,float q);
 400: void piano_note_drag_update(float step,float row,float q);
 406: int mixer_input_at(Vector2 point);
 435: void sampler_switch(int x,int y,int width,const char *name,uint8_t *flags,int bit);
 444: void dx7_envelope_graph(FMSettings *s,int base,Rectangle plot);
```

## ui_menus.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### open_popup

```c
   5:     if(kind==2) ui.rename_automation=-1;
   6:     if(kind==4) ui.help_scroll=0;
   7:     ui.pattern_popup=kind; ui.popup_opened=1; ui.popup_drag=0;
   8:     ui.popup_position[kind]=kind==7?(Vector2){96,34}:(Vector2){-1,-1};
```

### mixer_name

```c
  11: const char *mixer_name(int id) { return id?ui.project.insert_names[id-1]:"Master"; }
```

### begin_rename

```c
  14:     ui.rename_track=ui.rename_mixer=ui.rename_channel=-1;
  16:     ui.rename_select_all=1; open_popup(2);
```

### begin_number

```c
  20:     ui.number_integer=0; ui.number_target=value; ui.number_low=low; ui.number_high=high; ui.number_default=initial; ui.number_name=name;
  21:     snprintf(ui.number_text,sizeof ui.number_text,"%.6g",*value); ui.rename_select_all=1; open_popup(5);
```

### control_menu

```c
  28:     Vector2 position=ui.mouse;
  29:     if(ui.knob_context>=0) { position.x+=ui.windows.editors[ui.knob_context].rect.x; position.y+=ui.windows.editors[ui.knob_context].rect.y; }
  30:     open_context(automated?9:15,0,position);
```

### create_automation

```c
  34:     float start=ui.song_loop[1]>ui.song_loop[0]?ui.song_loop[0]/STEPS:floorf((ui.playing && ui.song?ui.visual_step:ui.playlist_start)/STEPS);
  35:     float steps=ui.song_loop[1]>ui.song_loop[0]?ui.song_loop[1]-ui.song_loop[0]:fmaxf(STEPS,ui.project.pattern_steps[ui.pattern]);
  38:     const char *prefix=((id>=PARAM_FM_RATIO && id<=PARAM_FM_LAST) || (id>=PARAM_DX7_FIRST && id<=PARAM_DX7_LAST))?ui.project.channel_names[owner]:id>=PARAM_CHORUS_RATE && id<=PARAM_EFFECT_MIX?(owner?ui.project.insert_names[owner-1]:"Master"):id<=PARAM_CHANNEL_PITCH || id==PARAM_CHANNEL_MUTE || id==PARAM_PITCH_RANGE?ui.project.channel_names[owner]:id<=PARAM_INSERT_WIDTH || id==PARAM_INSERT_MUTE?ui.project.insert_names[owner]:"Master";
  40:     if(!strncmp(parameter,"Insert ",7)) parameter+=7;
  41:     else if(!strncmp(parameter,"Channel ",8)) parameter+=8;
  42:     else if(!strncmp(parameter,"Master ",7)) parameter+=7;
  43:     snprintf(name,sizeof name,"%.20s %.23s",prefix,parameter);
  45:     if(a<0) { snprintf(ui.status,sizeof ui.status,"Cannot create automation: %d clip limit.",AUTOMATIONS); return; }
  46:     int lane,slot=-1;
  47:     for(lane=0;lane<LANES;lane++) if((slot=arrangement_place(&ui.project,lane,start,AUTOMATION_SOURCE+a,steps))>=0) break;
  48:     if(slot<0) { automation_delete(&ui.project,a); snprintf(ui.status,sizeof ui.status,"No free Playlist space for automation."); return; }
  49:     ui.automation_selected=a; ui.picker_tab=2; ui.picker_scroll=a; ui.picker_drag=-1;
  50:     ui.arrangement.source_pattern=AUTOMATION_SOURCE+a; ui.arrangement.source_steps=steps; ui.arrangement.source_offset=0;
  51:     memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected); ui.arrangement.selected[lane][slot]=1;
  52:     ui.arrangement.view_start=fmaxf(0,start); ui.track_scroll=track_position(lane); ui.browser_focus=0; windows_focus(&ui.windows,1);
```

### text_input

```c
  57:     if(shortcut_down() && IsKeyPressed(KEY_A)) ui.rename_select_all=1;
  58:     if(shortcut_down() && IsKeyPressed(KEY_V)) {
  61:             if(ui.rename_select_all) { text[0]=0; ui.rename_select_all=0; }
  64:                 if(ch<32 || ch==127 || n+bytes>=capacity) break;
  65:                 memcpy(text+n,paste,bytes); text[n+bytes]=0; paste+=bytes;
  69:     if(IsKeyPressed(KEY_BACKSPACE)) { if(ui.rename_select_all) text[0]=0; else backspace(text); ui.rename_select_all=0; }
  70:     if(IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) ui.rename_select_all=0;
  72:     while((ch=GetCharPressed())>0) {
  73:         if(ch<32 || ch==127 || command_down()) continue;
  74:         if(ui.rename_select_all) { text[0]=0; ui.rename_select_all=0; }
  75:         int bytes; const char *utf8=CodepointToUTF8(ch,&bytes); size_t n=strlen(text);
  76:         if(n+bytes<capacity) { memcpy(text+n,utf8,bytes); text[n+bytes]=0; }
```

### device_button

```c
  84:     if(button_color(fit_text(TextFormat("%s: %s",io?"Out":"In",device_caption(selected)),width-30,13),x,y,width,22,0,ui_theme.browser) && !recording_active()) {
  85:         ui.device_target=ui.mixer_selected; ui.device_io=io; ui.device_scroll=0;
  86:         ui.device_count=audio_devices(!io,ui.device_names,64);
  87:         open_popup(8);
  88:         Rect r=ui.windows.editors[3].rect;
  89:         ui.popup_position[8]=(Vector2){r.x+x,r.y+y+24};
  91:     if(hover(x,y,width,22)) snprintf(ui.status,sizeof ui.status,io?"Output device choice is saved; external-output routing is not implemented yet":"Input device for recording this mixer track; None records its internal audio");
  92:     DrawTriangle((Vector2){x+width-16,y+8},(Vector2){x+width-12,y+14},(Vector2){x+width-8,y+8},muted);
```

### help_group

```c
  96:     label(title,x,*y,13,accent); *y+=26;
```

### help_binding

```c
 100:     label(keys,x,*y,11,ink); label(action,x+126,*y,11,muted); *y+=23;
```

### draw_popup_content

```c
 104:     if(!ui.pattern_popup) { ui.popup_drag=0; return; }
 105:     ui.input_enabled=1;
 106:     if(ui.pattern_popup==10) {
 107:         int count=ui.windows.focused==2 && ui.piano_tool==PENCIL?8:6;
 108:         int x=52,y=34,w=210,h=8+count*26;
 109:         if(IsKeyPressed(KEY_ESCAPE) || (!ui.popup_opened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hover(x,y,w,h))) { ui.pattern_popup=0; return; }
 112:         for(int i=0;i<count;i++) if(button(names[i],x+4,y+4+i*26,w-8,26,0) && !ui.popup_opened) { ui.pattern_popup=0; if(i<2) undo_redo(i?1:-1); else if(i<6) edit_selection(i-2); else piano_octave(i==6?1:-1); }
 115:     if(ui.pattern_popup==1 || ui.pattern_popup==9) {
 116:         int file=ui.pattern_popup==1,x=file?8:140,y=34,w=210,h=file?190:34;
 117:         if(IsKeyPressed(KEY_ESCAPE) || (!ui.popup_opened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hover(x,y,w,h))) { ui.pattern_popup=0; return; }
 120:         for(int i=0;i<(file?7:1);i++) if(button(file?items[i]:"Keybindings",x+4,y+4+i*26,w-8,24,0) && !ui.popup_opened) {
 122:             ui.pattern_popup=0;
 123:             if(!file) open_popup(4);
 124:             else if(i==0) replace_project(1);
 125:             else if(i==1) replace_project(2);
 126:             else if(i==4) replace_project(3);
 132:     if(ui.pattern_popup==8) {
 133:         int visible=fminf(10,ui.device_count+2),w=300,h=visible*24+8;
 135:         int x=fmaxf(0,fminf(GetScreenWidth()/scale-w,ui.popup_position[8].x)),y=fmaxf(42,fminf(GetScreenHeight()/scale-24-h,ui.popup_position[8].y));
 136:         if(IsKeyPressed(KEY_ESCAPE) || (!ui.popup_opened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hover(x,y,w,h))) { ui.pattern_popup=0; return; }
 138:         DrawRectangleLinesEx((Rectangle){x,y,w,h},1,ui_theme.border);
 139:         if(hover(x,y,w,h)) ui.device_scroll=fmaxf(0,fminf(ui.device_count+2-visible,ui.device_scroll-GetMouseWheelMove()));
 140:         for(int row=0;row<visible;row++) {
 141:             int i=row+ui.device_scroll; const char *name=i==0?"":i==1?"@default":ui.device_names[i-2];
 143:             if(button_color(fit_text(device_caption(name),w-32,13),x+4,y+4+row*24,w-8,22,!strcmp(name,selected),ui_theme.browser) && !ui.popup_opened) {
 144:                 snprintf(selected,128,"%s",name); ui.pattern_popup=0;
 150:     if(ui.pattern_popup==7) {
 151:         int x=96,y=34,w=304,h=34;
 152:         if(IsKeyPressed(KEY_ESCAPE) || (!ui.popup_opened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hover(x,y,w,h))) { ui.pattern_popup=0; return; }
 154:         if(button("MIDI / Recording",x+4,y+4,w-8,25,0) && !ui.popup_opened){midi_refresh();open_popup(13);return;}
 157:     if(ui.pattern_popup==12) {
 158:         float scale=ui_scale(); int w=400,h=152,x=(GetScreenWidth()/scale-w)/2,y=(GetScreenHeight()/scale-h)/2;
 159:         ui_frame((Rectangle){x,y,w,h}); label("Save your changes?",x+16,y+16,16,ink);
 160:         label(fit_text(GetFileName(ui.document.path),w-32,13),x+16,y+48,13,muted);
 161:         if(button("Save",x+16,y+100,100,28,1)) { ui.pattern_popup=0; project_file_action(2); return; }
 162:         if(button("Discard",x+124,y+100,120,28,0)) { int action=ui.document.pending_action; ui.document.pending_action=0; ui.pattern_popup=0; if(action==4) load_project(ui.document.replacement_path); else if(action==3) project_file_action(4); else if(action==REPLACE_QUIT) ui.document.quit=1; else new_project(action==2); return; }
 163:         if(button("Cancel",x+252,y+100,100,28,0) || IsKeyPressed(KEY_ESCAPE)) { ui.document.pending_action=0; ui.pattern_popup=0; }
 166:     int picker=ui.pattern_popup==11,folder=ui.pattern_popup==3,help=ui.pattern_popup==4,number=ui.pattern_popup==5,w=ui.pattern_popup==13?460:picker?700:ui.pattern_popup==6?320:help?660:number?360:folder?620:260,h=ui.pattern_popup==13?390:picker?(ui.file_confirm?180:500):help?645:116;
 168:     if(help || picker || ui.pattern_popup==13) h=fminf(h,GetScreenHeight()/scale-56);
 169:     Vector2 *position=&ui.popup_position[ui.pattern_popup];
 170:     if(position->x<0 || position->y<0) *position=(Vector2){(GetScreenWidth()/scale-w)/2,(GetScreenHeight()/scale-h)/2};
 171:     if(ui.popup_drag) {
 173:         if(!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.popup_drag=0;
 175:     position->x=fmaxf(0,fminf(GetScreenWidth()/scale-w,position->x));
 176:     position->y=fmaxf(0,fminf(GetScreenHeight()/scale-h,position->y));
 180:     if(button("x",x+w-24,y+1,20,16,0)) { if(ui.pattern_popup==11) ui.document.pending_action=0; ui.pattern_popup=0; ui.popup_drag=0; return; }
 181:     if(hover(x,y,w-26,TITLE)) snprintf(ui.status,sizeof ui.status,"Drag this title bar to move the dialog");
 182:     if(hover(x,y,w-26,TITLE) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.popup_drag=1; ui.popup_offset=(Vector2){ui.mouse.x-x,ui.mouse.y-y}; }
 183:     if(IsKeyPressed(KEY_ESCAPE)) { if(ui.pattern_popup==11) ui.document.pending_action=0; ui.pattern_popup=0; ui.popup_drag=0; return; }
 184:     if(!ui.popup_drag && !ui.popup_opened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hover(x,y,w,h)) { if(ui.pattern_popup==11) ui.document.pending_action=0; ui.pattern_popup=0; return; }
 185:     ui.input_enabled=!ui.popup_drag;
 187:     if(ui.pattern_popup==13) {
 188:         label("MIDI / Recording",x+8,y+3,13,ink);
 189:         int enabled=ui.input_enabled;if(recording_active())ui.input_enabled=0;
 191:         for(int i=0;i<3;i++)if(button(modes[i],x+12+i*145,y+30,140,28,ui.record_mask&(1u<<i))){ui.record_mask^=1u<<i;midi_save_settings();}
 192:         int target=midi_target();label(fit_text(TextFormat("Keyboard plays: %s",target>=0 && target<ui.project.channel_count?ui.project.channel_names[target]:"Select an instrument"),w-24,12),x+12,y+68,12,ink);
 193:         label("Select a channel in the Rack or Piano Roll to play it.",x+12,y+88,11,muted);
 194:         if(button("Refresh inputs",x+12,y+112,140,24,0))midi_refresh();
 195:         if(button("Clear MIDI links",x+160,y+112,140,24,0)){ui.project.midi_binding_count=0;memset(ui.project.midi_bindings,0,sizeof ui.project.midi_bindings);ui.midi_learning=0;}
 196:         label(ui.midi_connected?"Input connected":"Select an input (USB or virtual MIDI)",x+12,y+145,11,ui.midi_connected?accent:muted);
 197:         int visible=fmaxf(1,(h-180)/24);if(hover(x+12,y+168,w-24,h-180))ui.midi_scroll=fmaxf(0,fminf(fmaxf(0,ui.midi_device_count+1-visible),ui.midi_scroll-GetMouseWheelMove()));
 198:         for(int row=0;row<visible && row+ui.midi_scroll<=ui.midi_device_count;row++) {
 199:             int i=row+ui.midi_scroll-1;const char *name=i<0?"None":ui.midi_devices[i].name;
 200:             if(button(fit_text(name,w-40,12),x+12,y+168+row*24,w-24,22,i<0?!ui.midi_selected[0]:!strcmp(ui.midi_selected,ui.midi_devices[i].id)))midi_select(i);
 204:     if(ui.pattern_popup==6) {
 205:         label("Add instrument",x+8,y+3,13,ink);
 206:         if(button(instrument_descriptor(INSTRUMENT_SAMPLER)->name,x+12,y+30,w-24,30,0)) { add_instrument(INSTRUMENT_SAMPLER); ui.pattern_popup=0; }
 207:         if(button(instrument_descriptor(INSTRUMENT_FM)->name,x+12,y+66,w-24,30,0)) { add_instrument(INSTRUMENT_FM); ui.pattern_popup=0; }
 211:         label("Help - Keybindings",x+8,y+3,13,ink);
 212:         int maximum=767-h;
 213:         if(hover(x,y+TITLE,w,h-TITLE)) ui.help_scroll-=GetMouseWheelMove()*36;
 214:         ui.help_scroll=fmaxf(0,fminf(maximum,ui.help_scroll));
 215:         BeginScissorMode(x*scale,(y+TITLE)*scale,w*scale,(h-TITLE-30)*scale);
 216:         int left=x+18,right=x+348,ly=y+34-ui.help_scroll,ry=ly;
 228:         ly+=12; help_group(left,&ly,"Transport & keyboard audition");
 230:         help_binding(left,&ly,"Z row + S/D/G/H/J","Play notes from C3");
 231:         help_binding(left,&ly,"Q row + number keys","Play notes from C4");
 232:         ly+=12; help_group(left,&ly,"Editor windows");
 235:         ly+=12; help_group(left,&ly,"Playlist tools");
 245:         ly+=12; help_group(left,&ly,"Piano Roll velocity lane");
 248:         ly+=12; help_group(left,&ly,"MIDI & recording");
 275:         ry+=12; help_group(right,&ry,"Automation");
 279:         ry+=12; help_group(right,&ry,"Text fields & dialogs");
 287:         ry+=12; help_group(right,&ry,"Browser (hover or focus)");
 292:         label(maximum>0?"Wheel to scroll keybindings":"Navigation follows the pointer. Audition requires Keys enabled.",x+18,y+h-24,11,muted);
 296:     label(number?ui.number_name:folder?"Add Browser folder":(ui.rename_automation>=0?"Rename automation":ui.rename_channel>=0?TextFormat("Rename channel %d",ui.rename_channel+1):ui.rename_mixer>=0?TextFormat("Rename insert %d",ui.rename_mixer+1):ui.rename_track>=0?TextFormat("Rename track %d",ui.rename_track+1):TextFormat("Rename pattern %d",ui.rename_pattern+1)),x+8,y+3,13,ink);
 297:     DrawRectangle(x+8,y+32,w-16,28,bg); DrawRectangleLines(x+8,y+32,w-16,28,cell);
 298:     if(ui.rename_select_all) DrawRectangle(x+12,y+38,fminf(w-26,text_width(text,13)),16,cell);
 299:     if(hover(x+8,y+32,w-16,28)) snprintf(ui.status,sizeof ui.status,number?"Enter an exact value using a decimal dot or comma":folder?"Enter the folder path to add as a Browser root":"Enter a name");
 300:     if(hover(x+8,y+32,w-16,28) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.rename_select_all=0;
 302:     label(fit_text(text,w-26,13),x+12,y+40,13,ink);
 303:     label(number?TextFormat("%.6g to %.6g; decimal dot or comma",ui.number_low,ui.number_high):"Choose Apply or Cancel below",x+8,y+66,11,muted);
 304:     int apply=button("Apply",x+8,y+84,76,24,1) || IsKeyPressed(KEY_ENTER);
 305:     if(button("Cancel",x+92,y+84,76,24,0)) ui.pattern_popup=0;
 306:     if(number && button("Reset",x+176,y+84,76,24,0)) { *ui.number_target=ui.number_default; ui.pattern_popup=0; }
 309:             char value[64],*end; snprintf(value,sizeof value,"%s",text);
 312:             if(!parsed_any || *end || !isfinite(parsed) || parsed<ui.number_low || parsed>ui.number_high || (ui.number_integer && parsed!=roundf(parsed))) { snprintf(ui.status,sizeof ui.status,"Enter a number from %.6g to %.6g.",ui.number_low,ui.number_high); return; }
 318:             size_t n=strlen(text); while(n && text[n-1]==' ') text[--n]=0;
 321:             snprintf(ui.rename_automation>=0?ui.project.automations[ui.rename_automation].name:ui.rename_channel>=0?ui.project.channel_names[ui.rename_channel]:ui.rename_mixer>=0?ui.project.insert_names[ui.rename_mixer]:ui.rename_track>=0?ui.project.track_names[ui.rename_track]:ui.project.pattern_names[ui.rename_pattern],PATTERN_NAME,"%s",name);
 323:         ui.pattern_popup=0;
```

### draw_popup

```c
 329:     int navigation=kind==1 || kind==6 || kind==7 || kind==8 || kind==9 || kind==10 || kind==12;
 330:     if(navigation) menu_keys_begin(kind,ui.popup_opened,kind==12?2:0,kind==8?&ui.device_scroll:NULL,ui.device_count+2);
 333:     if(!ui.pattern_popup && !ui.context_kind) ui.menu_keys.id=0;
```

### file_picker_sync

```c
 338:     ui.file_last_row=-1; ui.file_last_click=-1; ui.file_scroll_drag=0;
```

### preset_action

```c
 350:     ui.file_action=save?8:9; ui.file_field=save?2:0; ui.file_confirm=0;
 351:     file_picker_sync(); ui.rename_select_all=save; open_popup(11);
```

### project_file_action

```c
 360:     if(action==2 && ui.document.saved_on_disk) {
 361:         int ok=sampler_flush() && project_save_assets(ui.document.path,&ui.project,ui.originals,0);
 362:         snprintf(ui.status,sizeof ui.status,ok?"Saved %.220s":"Save failed: %.220s",ui.document.path);
 363:         if(ok) replacement_saved(); else if(ui.document.pending_action) open_popup(12);
 366:     const char *current=action==7?ui.project.paths[ui.relink_channel]:action==5?(ui.document.export_path[0]?ui.document.export_path:"song.wav"):ui.document.path;
 368:     int length=current[0]=='/'?snprintf(initial,sizeof initial,"%s",current):snprintf(initial,sizeof initial,"%s/%s",GetWorkingDirectory(),current);
 370:     if(!file_chooser_begin_recent(&ui.file_picker,initial,action==7?"wav;flac;mp3":action==5?"wav":PROJECT_FILE_EXTENSION,action!=4 && action!=7,action==4?FILE_OPEN:action==5?FILE_EXPORT:action==7?FILE_SAMPLE:FILE_SAVE)) { snprintf(ui.status,sizeof ui.status,"%s",ui.file_picker.error); return; }
 371:     ui.file_action=action; ui.file_field=action==4 || action==7?0:2; ui.file_confirm=0;
 372:     file_picker_sync(); ui.rename_select_all=ui.file_field==2; open_popup(11);
```

### file_picker_commit

```c
 376:     if(!project_file_commit(ui.file_pending)) return 0;
 377:     file_chooser_remember(&ui.file_picker,ui.file_pending); return 1;
```

### file_picker_submit

```c
 382:     if(result<0) return;
 383:     if(result==0) { ui.file_picker.name[0]=0; file_picker_sync(); return; }
 384:     if(result==2) { ui.file_confirm=1; ui.file_field=0; ui.rename_select_all=0; return; }
 385:     if(file_picker_commit()) { ui.pattern_popup=0; if(ui.file_action==2 || ui.file_action==3 || ui.file_action==6) replacement_saved(); }
 386:     else snprintf(ui.file_picker.error,sizeof ui.file_picker.error,"%.159s",ui.status);
```

### draw_file_picker

```c
 391:     label(ui.file_action==8?"Save preset":ui.file_action==9?"Load preset":ui.file_action==4?"Open project":ui.file_action==5?"Export WAV":ui.file_action==6?"Collect samples and save":ui.file_action==7?"Relink sample":ui.file_action==3?"Save project as":"Save project",x+8,y+3,13,ink);
 393:         label("Replace this existing file?",x+16,y+56,15,ink);
 394:         label(fit_text(ui.file_pending,w-32,13),x+16,y+86,13,muted);
 395:         if(button("Replace",x+16,y+126,100,26,1) || IsKeyPressed(KEY_ENTER)) {
 396:             if(file_picker_commit()) { ui.pattern_popup=0; if(ui.file_action==2 || ui.file_action==3 || ui.file_action==6) replacement_saved(); }
 397:             else { ui.file_confirm=0; snprintf(c->error,sizeof c->error,"%.159s",ui.status); }
 399:         if(button("Back",x+124,y+126,100,26,0)) ui.file_confirm=0;
 402:     if(button("Up",x+12,y+30,44,26,0) && file_chooser_parent(c)) file_picker_sync();
 404:     if(button("Home",x+60,y+30,54,26,0) && home && file_chooser_folder(c,home)) file_picker_sync();
 405:     if(button("Hidden",x+w-84,y+30,72,26,c->hidden)) {
 408:     Rectangle path_box={x+122,y+30,w-214,26};
 409:     DrawRectangleRec(path_box,bg); DrawRectangleLinesEx(path_box,1,ui.file_field==1?accent:cell);
 410:     if(hover(path_box.x,path_box.y,path_box.width,path_box.height) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.file_field=1; ui.rename_select_all=1; }
 411:     int path_submitted=ui.file_field==1 && IsKeyPressed(KEY_ENTER);
 412:     if(ui.file_field==1) {
 414:         if(IsKeyPressed(KEY_ENTER) && file_chooser_folder(c,ui.file_directory)) { file_picker_sync(); ui.file_field=0; ui.rename_select_all=0; }
 416:     if(ui.file_field==1 && ui.rename_select_all) DrawRectangle(path_box.x+4,path_box.y+5,fminf(path_box.width-8,text_width(ui.file_directory,12)),16,cell);
 417:     label(fit_text(ui.file_directory,path_box.width-8,12),path_box.x+4,path_box.y+6,12,ink);
 418:     int list_y=y+66,list_h=h-174,rows=list_h/24,list_w=w-38;
 419:     if(hover(x+12,list_y,w-24,list_h)) c->scroll-=GetMouseWheelMove()*3;
 420:     if(ui.file_field!=1 && ui.file_field!=2) {
 421:         if(IsKeyPressed(KEY_UP)) c->selected=c->selected<0?0:(int)fmaxf(0,c->selected-1);
 422:         if(IsKeyPressed(KEY_DOWN)) c->selected=(int)fminf(c->count-1,c->selected+1);
 423:         if(c->selected>=0 && c->selected<c->count) {
 425:             if(c->selected>=c->scroll+rows) c->scroll=c->selected-rows+1;
 429:     int maximum=(int)fmaxf(0,c->count-rows); c->scroll=(int)fmaxf(0,fminf(maximum,c->scroll));
 430:     DrawRectangle(x+12,list_y,w-24,list_h,bg);
 431:     BeginScissorMode((x+12)*scale,list_y*scale,list_w*scale,list_h*scale);
 432:     int activate=-1;
 434:         FileEntry *entry=&c->entries[row]; int ry=list_y+(row-c->scroll)*24;
 435:         if(button("",x+12,ry,list_w,24,c->selected==row)) {
 436:             double now=GetTime(); activate=ui.file_last_row==row && now-ui.file_last_click<.35?row:-1;
 437:             c->selected=row; ui.file_field=0; ui.rename_select_all=0; c->error[0]=0;
 441:         if(entry->directory) { DrawRectangleLines(x+20,ry+8,12,9,c->selected==row?ui_theme.selected_text:muted); DrawRectangle(x+20,ry+5,6,3,c->selected==row?ui_theme.selected_text:muted); }
 442:         label(fit_text(entry->name,list_w-40,13),x+40,ry+5,13,c->selected==row?ui_theme.selected_text:ink);
 445:     float thumb=fminf(list_h,fmaxf(24,list_h*rows/(float)fmaxf(1,c->count))),travel=list_h-thumb;
 446:     float thumb_y=list_y+(maximum?c->scroll/(float)maximum*travel:0);
 447:     if(hover(x+w-22,list_y,10,list_h) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.file_scroll_drag=1;
 448:     if(ui.file_scroll_drag) {
 449:         if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) c->scroll=(int)fmaxf(0,fminf(maximum,(ui.mouse.y-list_y-thumb/2)/fmaxf(1,travel)*maximum));
 450:         else ui.file_scroll_drag=0;
 452:     ui_surface((Rectangle){x+w-22,thumb_y,8,thumb},ui.file_scroll_drag?accent:muted);
 453:     if(!c->count) label("No matching files in this folder",x+24,list_y+12,13,muted);
 454:     int enter=IsKeyPressed(KEY_ENTER) && ui.file_field!=1 && !path_submitted;
 455:     if(activate>=0 || (enter && ui.file_field==0 && c->selected>=0 && c->selected<c->count && c->entries[c->selected].directory)) {
 456:         int selected=activate>=0?activate:c->selected;
 461:         enter=0;
 463:     label(ui.file_action==8 || ui.file_action==9?"Device presets (*.llpreset)":ui.file_action==7?"Audio samples (*.wav, *.flac, *.mp3)":ui.file_action==5?"WAV audio (*.wav)":"LibreLoop projects (*" PROJECT_FILE_SUFFIX ")",x+12,y+h-102,11,muted);
 464:     label("Filename",x+12,y+h-76,12,muted);
 465:     Rectangle name_box={x+82,y+h-82,w-94,26};
 466:     DrawRectangleRec(name_box,bg); DrawRectangleLinesEx(name_box,1,ui.file_field==2?accent:cell);
 467:     if(hover(name_box.x,name_box.y,name_box.width,name_box.height) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.file_field=2; ui.rename_select_all=1; }
 468:     if(ui.file_field==2) text_input(c->name,sizeof c->name);
 469:     if(ui.file_field==2 && ui.rename_select_all) DrawRectangle(name_box.x+4,name_box.y+5,fminf(name_box.width-8,text_width(c->name,13)),16,cell);
 470:     label(fit_text(c->name,name_box.width-8,13),name_box.x+4,name_box.y+6,13,ink);
 471:     label(fit_text(c->error,w-210,11),x+12,y+h-30,11,ui_theme.meter_high);
 472:     if(button("Cancel",x+w-188,y+h-38,80,26,0)) { ui.document.pending_action=0; ui.pattern_popup=0; snprintf(ui.status,sizeof ui.status,"Cancelled."); }
 473:     if(button(ui.file_action==4 || ui.file_action==9?"Open":ui.file_action==7?"Relink":ui.file_action==5?"Export":"Save",x+w-100,y+h-38,88,26,1) || enter) file_picker_submit();
```

### delete_pattern

```c
 480:     for(int i=target;i<old_count-1;i++) {
 481:         ui.rack_view[i]=ui.rack_view[i+1]; ui.rack_range[i]=ui.rack_range[i+1]; ui.piano_start[i]=ui.piano_start[i+1];
 482:         ui.piano_span[i]=ui.piano_span[i+1]; ui.piano_pan[i]=ui.piano_pan[i+1]; ui.piano_range[i]=ui.piano_range[i+1];
 483:         memcpy(ui.pattern_loop[i],ui.pattern_loop[i+1],sizeof ui.pattern_loop[i]);
 484:         memcpy(ui.piano_channels[i],ui.piano_channels[i+1],sizeof ui.piano_channels[i]);
 485:         memcpy(ui.note_selected[i],ui.note_selected[i+1],sizeof ui.note_selected[i]);
 487:     int last=old_count>1?ui.project.pattern_count:0;
 488:     ui.rack_view[last]=ui.rack_range[last]=ui.piano_start[last]=ui.piano_pan[last]=ui.piano_range[last]=0; ui.piano_span[last]=STEPS;
 489:     memset(ui.pattern_loop[last],0,sizeof ui.pattern_loop[last]); memset(ui.piano_channels[last],0,sizeof ui.piano_channels[last]); memset(ui.note_selected[last],0,sizeof ui.note_selected[last]);
 490:     ui.pattern=fminf(target,ui.project.pattern_count-1); ui.playing=0; ui.reset=1; ui.note_drag=NULL; ui.piano_gesture=ui.moving_note=0;
 491:     ui.arrangement.gesture=IDLE; ui.arrangement.source_pattern=-1;
 492:     memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected);
 493:     snprintf(ui.status,sizeof ui.status,"Deleted pattern and its Playlist clips%s",old_count==1?"; kept one blank pattern":"");
```

### draw_context_content

```c
 499:     int instrument_rows[CHANNELS],instrument_count=ui.context_kind==17?pattern_instruments(ui.context_target,instrument_rows):0;
 500:     int w=ui.context_kind==14 || ui.context_kind==16?160:(ui.context_kind==5 || ui.context_kind==8 || ui.context_kind==12)?304:228,h=ui.context_kind==9?133:ui.context_kind==21?8+25*FM_FACTORY_COUNT:ui.context_kind==22?83:ui.context_kind==23?108:ui.context_kind==19?108:ui.context_kind==20?58:ui.context_kind==17?8+25*(int)fminf(8,instrument_count):ui.context_kind==18?58:ui.context_kind==4?108:ui.context_kind==16?32:ui.context_kind==15?58:ui.context_kind==14?8+EQ_SHAPES*25:ui.context_kind==13?58:ui.context_kind==2?158:ui.context_kind==7?133:(ui.context_kind==5 || ui.context_kind==8 || ui.context_kind==12)?76:ui.context_kind==3 && !ui.context_target?32:82,x=fmaxf(0,fminf(GetScreenWidth()/scale-w,ui.context_position.x)),y=fmaxf(0,fminf(GetScreenHeight()/scale-h,ui.context_position.y));
 501:     ui.input_enabled=1;
 502:     if(IsKeyPressed(KEY_ESCAPE) || (!ui.context_opened && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !hover(x,y,w,h))) { ui.context_kind=0; return; }
 505:     if(kind==21 || kind==22 || kind==23) {
 506:         if(target<0 || target>=ui.project.channel_count || ui.project.instrument[target]!=INSTRUMENT_FM) { ui.context_kind=0; return; }
 508:         int count=kind==21?FM_FACTORY_COUNT:kind==22?3:4;
 509:         for(int i=0;i<count;i++) if(button(kind==21?fm_factory_name(i):kind==22?engines[i]:waves[i],x+4,y+4+i*25,w-8,24,0)) {
 510:             if(kind==21) ui.project.fm[target]=fm_factory(i);
 511:             else if(kind==22) ui.project.fm[target].engine=i;
 513:             ui.fm_graph_drag=-1; ui.context_kind=0; return;
 515:     } else if(kind==19 || kind==20) {
 516:         if(target<0 || target>=ui.project.channel_count || ui.project.instrument[target]!=INSTRUMENT_FM) { ui.context_kind=0; return; }
 518:         float *value=kind==19?&ui.project.fm[target].lfo_shape:&ui.project.fm[target].routing;
 519:         for(int i=0;i<(kind==19?4:2);i++) if(button(kind==19?shapes[i]:routing[i],x+4,y+4+i*25,w-8,24,*value==i)) { *value=i; ui.context_kind=0; return; }
 520:     } else if(kind==17) {
 521:         int visible=(int)fminf(8,instrument_count);
 522:         if(hover(x,y,w,h)) ui.instrument_replace_scroll=(int)fmaxf(0,fminf(instrument_count-visible,ui.instrument_replace_scroll-GetMouseWheelMove()));
 523:         for(int row=0;row<visible;row++) {
 525:             if(button(fit_text(TextFormat("Replace %s...",ui.project.channel_names[c]),w-16,13),x+4,y+4+row*25,w-8,24,0)) {
 526:                 open_context(18,c,ui.context_position); return;
 529:         if(hover(x,y,w,h)) snprintf(ui.status,sizeof ui.status,"Choose the pattern's instrument to replace; the change applies across all patterns. Scroll for more instruments.");
 530:     } else if(kind==18) {
 531:         if(target<0 || target>=ui.project.channel_count) { ui.context_kind=0; return; }
 534:             if(type==INSTRUMENT_FM && ui.project.channel_audio[target]) ui.input_enabled=0;
 535:             if(button(instrument_descriptor(type)->name,x+4,y+4+type*25,w-8,24,ui.project.instrument[target]==type)) {
 536:                 ui.context_kind=0; replace_instrument(target,type); return;
 539:             if(hover(x+4,y+4+type*25,w-8,24)) snprintf(ui.status,sizeof ui.status,
 542:     } else if(kind==16) {
 543:         if(ui.eq_bus>ui.project.insert_count || ui.project.effect_type[ui.eq_bus][ui.eq_slot]!=EFFECT_EQ || target<0 || target>=EQ_BANDS) { ui.context_kind=0; return; }
 544:         if(button("Reset",x+4,y+4,w-8,24,0)) { ui.project.eq[ui.eq_bus][ui.eq_slot].bands[target]=equalizer_default().bands[target]; ui.context_kind=0; }
 545:         if(hover(x+4,y+4,w-8,24)) snprintf(ui.status,sizeof ui.status,"Reset band %d frequency, gain, Q and shape to defaults",target+1);
 546:     } else if(kind==14) {
 547:         if(ui.eq_bus>ui.project.insert_count || ui.project.effect_type[ui.eq_bus][ui.eq_slot]!=EFFECT_EQ || target<0 || target>=EQ_BANDS) { ui.context_kind=0; return; }
 549:         for(int shape=0;shape<EQ_SHAPES;shape++) if(button(equalizer_shape_name(shape),x+4,y+4+shape*25,w-8,24,band->shape==(unsigned)shape)) { band->shape=shape; ui.context_kind=0; }
 550:     } else if(kind==13) {
 551:         for(int mode=0;mode<2;mode++) if(button(mode?"Stretch":"Resample",x+4,y+4+mode*25,w-8,24,ui.project.sampler[target].stretch==mode)) { ui.project.sampler[target].stretch=mode; ui.context_kind=0; }
 552:     } else if(kind==9 || kind==15) {
 553:         if(kind==9) {
 554:             if(button("MIDI Learn",x+4,y+79,w-8,24,0)){ui.midi_learn_target=ui.automation_target;ui.midi_learning=1;ui.context_kind=0;snprintf(ui.status,sizeof ui.status,"Move a MIDI CC knob or fader to link %s; Escape cancels",ui.menu_name);}
 555:             if(button("Clear MIDI link",x+4,y+104,w-8,24,0)){midi_unbind(&ui.project,ui.automation_target);ui.context_kind=0;}
 557:         if(button("Reset",x+4,y+4,w-8,24,0)) { *ui.menu_value=ui.menu_initial; ui.context_kind=0; }
 558:         if(hover(x+4,y+4,w-8,24)) snprintf(ui.status,sizeof ui.status,"Reset %s to %.6g",ui.menu_name,ui.menu_initial);
 559:         if(button("Enter value",x+4,y+29,w-8,24,0)) {
 561:             const ParameterDescriptor *descriptor=kind==9?parameter_descriptor(ui.automation_target.parameter):NULL;
 562:             ui.number_integer=descriptor && descriptor->kind==PARAMETER_INTEGER; ui.context_kind=0;
 564:         if(kind==9 && button("Create automation",x+4,y+54,w-8,24,0)) { create_automation(); ui.context_kind=0; }
 565:     } else if(kind==10) {
 566:         if(button("Create automation",x+4,y+4,w-8,24,0)) { create_automation(); ui.context_kind=0; }
 567:         if(button(ui.automation_target.parameter==PARAM_MASTER_MUTE?"Clear all solos":"Toggle solo",x+4,y+29,w-8,24,0)) {
 570:             else { for(int c=0;c<CHANNELS;c++) ui.project.mute[c]&=~2; for(int i=0;i<INSERTS;i++) ui.project.insert_mute[i]&=~2; for(int l=0;l<LANES;l++) ui.project.lane_mute[l]&=~2; }
 571:             ui.context_kind=0;
 573:     } else if(kind==11) {
 574:         if(button("Rename",x+4,y+4,w-8,24,0)) { ui.rename_track=ui.rename_mixer=ui.rename_channel=-1; snprintf(ui.rename_text,sizeof ui.rename_text,"%s",ui.project.automations[target].name); ui.rename_select_all=1; open_popup(2); ui.rename_automation=target; ui.context_kind=0; }
 575:         if(button("Color",x+4,y+29,w-8,24,0)) { ui.context_kind=12; ui.context_opened=1; }
 576:         if(button("Delete",x+4,y+54,w-8,24,0)) { automation_delete(&ui.project,target); ui.automation_selected=-1; ui.automation_node=-1; ui.picker_drag=-1; ui.arrangement.gesture=IDLE; ui.arrangement.source_pattern=-1; memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected); ui.context_kind=0; }
 577:     } else if(kind==6) {
 578:         for(int i=0;i<3;i++) if(button(ui.rack_filters[i],x+4,y+4+i*25,w-8,24,ui.rack_filter==i)) { ui.rack_filter=i; ui.rack_scroll=0; ui.context_kind=0; }
 579:     } else if(kind==5 || kind==8 || kind==12) {
 580:         uint32_t *color=kind==12?&ui.project.automations[target].color:kind==8?&ui.project.channel_colors[target]:&ui.project.pattern_colors[target];
 581:         int choice=color_picker(x+8,y+10,w-16,*color);
 582:         if(choice>=0) { *color=pattern_palette[choice]; ui.context_kind=0; }
 583:     } else if(kind==7) {
 584:         if(button("Rename",x+4,y+4,w-8,24,0)) { ui.rename_track=ui.rename_mixer=-1; ui.rename_channel=target; snprintf(ui.rename_text,sizeof ui.rename_text,"%s",ui.project.channel_names[target]); ui.rename_select_all=1; open_popup(2); ui.context_kind=0; }
 585:         if(button("Color",x+4,y+29,w-8,24,0)) { ui.context_kind=8; ui.context_opened=1; }
 586:         if(button("Delete",x+4,y+54,w-8,24,0)) { ui.picker_drag=-1; delete_channel(target); ui.context_kind=0; }
 587:         if(button(ui.project.instrument[target]==INSTRUMENT_FM?"Replace with sample...":"Relink sample...",x+4,y+79,w-8,24,0)) { ui.relink_channel=target; ui.context_kind=0; project_file_action(7); }
 588:         if(button("Replace instrument...",x+4,y+104,w-8,24,0)) { open_context(18,target,ui.context_position); return; }
 589:         if(hover(x+4,y+54,w-8,24)) snprintf(ui.status,sizeof ui.status,"Delete this Audio channel and all its Playlist clips; the source file stays on disk");
 590:     } else if(kind==4) {
 591:         if(button("Rename",x+4,y+4,w-8,24,0)) { ui.pattern=target; begin_rename(); ui.context_kind=0; }
 592:         if(button("Color",x+4,y+29,w-8,24,0)) { ui.context_kind=5; ui.context_opened=1; }
 593:         if(button("Delete",x+4,y+54,w-8,24,0)) { delete_pattern(target); ui.context_kind=0; }
 594:         if(button("Replace instrument...",x+4,y+79,w-8,24,0)) { ui.instrument_replace_scroll=0; open_context(17,target,ui.context_position); return; }
 595:         if(hover(x+4,y+54,w-8,24)) snprintf(ui.status,sizeof ui.status,"Delete this pattern and all its Playlist clips; the last pattern becomes blank");
 596:     } else if(kind==1) {
 597:         if(button("Stay on top",x+4,y+4,w-8,24,ui.windows.editors[target].pinned)) { windows_pin(&ui.windows,target); ui.context_kind=0; }
 598:         if(button(ui.windows.editors[target].maximized?"Restore":"Maximize",x+4,y+29,w-8,24,0)) {
 599:             Editor *e=&ui.windows.editors[target]; if(e->maximized) { e->rect=e->restore; e->maximized=0; } else { e->restore=e->rect; e->maximized=1; } ui.context_kind=0;
 601:         if(button("Hide window",x+4,y+54,w-8,24,0)) { ui.windows.editors[target].visible=0; ui.context_kind=0; }
 602:     } else if(kind==2) {
 603:         if(button("Go to Piano Roll",x+4,y+4,w-8,24,0)) { ui.channel=ui.piano_channel=target; ui.piano_channels[ui.pattern][target]=1; windows_focus(&ui.windows,2); ui.context_kind=0; }
 604:         if(button("Mute channel",x+4,y+29,w-8,24,ui.project.mute[target]&1)) { ui.project.mute[target]^=1; ui.context_kind=0; }
 605:         if(button("Rename",x+4,y+54,w-8,24,0)) { ui.rename_track=ui.rename_mixer=-1; ui.rename_channel=target; snprintf(ui.rename_text,sizeof ui.rename_text,"%s",ui.project.channel_names[target]); ui.rename_select_all=1; open_popup(2); ui.context_kind=0; }
 606:         if(button("Delete channel",x+4,y+79,w-8,24,0)) { delete_channel(target); ui.context_kind=0; }
 607:         if(button(ui.project.instrument[target]==INSTRUMENT_FM?"Replace with sample...":"Relink sample...",x+4,y+104,w-8,24,0)) { ui.relink_channel=target; ui.context_kind=0; project_file_action(7); }
 608:         if(button("Replace instrument...",x+4,y+129,w-8,24,0)) { open_context(18,target,ui.context_position); return; }
 610:         if(button("Reset Master volume",x+4,y+4,w-8,24,0)) { ui.project.master=1; ui.context_kind=0; }
 612:         if(button("Mute insert",x+4,y+4,w-8,24,target && (ui.project.insert_mute[target-1]&1))) { if(target) ui.project.insert_mute[target-1]^=1; ui.context_kind=0; }
 613:         if(button("Solo insert",x+4,y+29,w-8,24,target && (ui.project.insert_mute[target-1]&2))) { if(target) solo_toggle(ui.project.insert_mute,ui.project.insert_count,target-1); ui.context_kind=0; }
 614:         if(button("Reset mixer channel",x+4,y+54,w-8,24,0)) { if(target) insert_reset(&ui.project,target); else ui.project.master=1; ui.context_kind=0; }
```

### draw_context

```c
 620:     int navigation=kind && kind!=5 && kind!=8 && kind!=12;
 621:     int rows[CHANNELS],total=kind==17?pattern_instruments(ui.context_target,rows):0;
 622:     if(navigation) menu_keys_begin(100+kind,ui.context_opened,0,kind==17?&ui.instrument_replace_scroll:NULL,total);
 625:     if(!ui.pattern_popup && !ui.context_kind) ui.menu_keys.id=0;
```

## ui_mixer.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### mixer_input_at

```c
   4: int mixer_input_at(Vector2 point) {
   5:     Rect r=ui.windows.editors[3].rect; if(!ui.windows.editors[3].visible) return -1;
   6:     int capacity=fmaxf(1,(r.w-MIXER_PANEL-MIXER_LEFT-8)/51-1),visible=fminf(ui.project.insert_count,capacity);
   7:     for(int col=0;col<=visible;col++) if(CheckCollisionPointRec(point,(Rectangle){r.x+MIXER_LEFT+col*51+7,r.y+r.h-46,12,12})) return col?col+ui.mixer_scroll:0;
   8:     return -1;
```

### meters_update

```c
  12:     float peaks[INSERTS+1][2]; audio_meters(peaks);
  13:     double now=GetTime(),elapsed=fmax(0,now-ui.meter_time); ui.meter_time=now;
  14:     float release=expf(-elapsed/.18),peak_release=expf(-elapsed/.45); ui.mixer_decay_active=0;
  15:     for(int id=0;id<=INSERTS;id++) for(int side=0;side<2;side++) {
  17:         if(peaks[id][side]>=ui.meter_hold[id][side] && peaks[id][side]>.00001f) { ui.meter_hold[id][side]=peaks[id][side]; ui.meter_until[id][side]=now+.5; }
  19:         ui.mixer_decay_active|=ui.meter_hold[id][side]>.001f || ui.meter_level[id][side]>.001f;
```

### spectrum_update

```c
  24:     static int initialized,last_bus=-1;
  25:     if(!initialized) { spectrum_reset(&ui.master_spectrum); spectrum_reset(&ui.eq_spectrum); initialized=1; }
  26:     int bus=ui.windows.editors[5].visible?ui.eq_bus:-1;
  28:     float pcm[8192*2]; unsigned n;
  29:     n=audio_spectrum_read(0,pcm,8192); spectrum_push(&ui.master_spectrum,pcm,n);
  30:     n=audio_spectrum_read(1,pcm,8192);
  33:     ui.spectrum_active=0;
  34:     for(int i=0;i<SPECTRUM_BINS;i++) ui.spectrum_active|=ui.master_spectrum.db[i]>-78 || (bus>=0 && ui.eq_spectrum.db[i]>-78);
```

### spectrum_draw

```c
  38:     for(int i=0;i<SPECTRUM_BINS;i++) {
  39:         float height=fmaxf(0,fminf(1,(s->db[i]+78)/78))*r.height;
  40:         Color color=colored?ColorFromHSV(260-240.f*i/(SPECTRUM_BINS-1),.75f,1):accent;
  42:         DrawRectangleRec((Rectangle){x,r.y+r.height-height,w+.1f,height},color);
```

### eq_x

```c
  46: float eq_x(float frequency,Rectangle r) { return r.x+logf(frequency/20)/logf(1000)*r.width; }
```

### eq_y

```c
  48: float eq_y(float gain,Rectangle r) { return r.y+r.height*(.5f-gain/36); }
```

### eq_editor

```c
  51:     if(ui.eq_bus<0 || ui.eq_bus>ui.project.insert_count || ui.eq_slot<0 || ui.eq_slot>=EFFECT_SLOTS || ui.project.effect_type[ui.eq_bus][ui.eq_slot]!=EFFECT_EQ) {
  52:         ui.eq_drag=-1; label("Select an Equalizer in the Mixer.",16,40,14,muted); return;
  55:     for(int i=0;i<EQ_BANDS;i++) {
  60:     label(fit_text(TextFormat("%s · Slot %d",ui.eq_bus?ui.project.insert_names[ui.eq_bus-1]:"Master",ui.eq_slot+1),width-254,12),12,30,12,muted);
  61:     if(button("Bypass",width-226,26,70,24,ui.project.effect_bypass[ui.eq_bus][ui.eq_slot])) ui.project.effect_bypass[ui.eq_bus][ui.eq_slot]^=1;
  62:     if(button("Load",width-152,26,66,24,0)) preset_action(PRESET_EQ,ui.eq_bus,ui.eq_slot,0);
  63:     if(button("Save",width-82,26,70,24,0)) preset_action(PRESET_EQ,ui.eq_bus,ui.eq_slot,1);
  64:     Rectangle graph={42,64,width-58,height-196};
  65:     DrawRectangleRec(graph,bg); spectrum_draw(&ui.eq_spectrum,graph,1);
  66:     const float frequencies[]={20,100,1000,10000,20000};
  67:     const char *names[]={"20","100","1k","10k","20k"};
  68:     for(int i=0;i<5;i++) {
  70:         DrawLineEx((Vector2){x,graph.y},(Vector2){x,graph.y+graph.height},1,ui_theme.grid_minor);
  71:         label(names[i],fminf(width-32,fmaxf(32,x-text_width(names[i],10)/2)),graph.y+graph.height+5,10,muted);
  73:     for(int db=-18;db<=18;db+=6) {
  75:         DrawLineEx((Vector2){graph.x,y},(Vector2){graph.x+graph.width,y},1,db?ui_theme.grid_minor:muted);
  76:         label(TextFormat("%+d",db),8,y-5,10,muted);
  78:     static EQSettings drawn; static float response[257]; static int cached;
  80:         for(int i=0;i<=256;i++) response[i]=equalizer_response(shown,spectrum_frequency(i/256.f));
  81:         drawn=shown; cached=1;
  83:     Vector2 last={0};
  84:     for(int x=0;x<=256;x++) {
  86:         Vector2 next={graph.x+graph.width*x/256,eq_y(fmaxf(-18,fminf(18,db)),graph)};
  88:             smooth_line(last,next,4,BLACK);
  89:             smooth_line(last,next,1.8f,ink);
  93:     for(int i=0;i<EQ_BANDS;i++) {
  94:         EQBand b=shown.bands[i]; Vector2 p={eq_x(b.frequency,graph),eq_y(b.shape==EQ_LOW_CUT || b.shape==EQ_HIGH_CUT?0:b.gain,graph)};
  95:         int over=hover(p.x-10,p.y-10,20,20);
  96:         circle(p.x,p.y,ui.eq_band==i?8:6,b.shape==EQ_OFF?muted:accent);
  97:         label(TextFormat("%d",i+1),p.x-text_width(TextFormat("%d",i+1),10)/2,p.y-5,10,ui_theme.selected_text);
  99:             snprintf(ui.status,sizeof ui.status,"EQ band %d: %.0f Hz, %+.1f dB | drag to tune; wheel changes Q; right-click to reset",i+1,b.frequency,b.gain);
 100:             if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.eq_band=i; ui.eq_drag=i; }
 102:             if(wheel) settings->bands[i].q=fmaxf(.2f,fminf(10,settings->bands[i].q+wheel*.1f));
 103:             if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
 104:                 ui.eq_band=i; open_context(16,i,(Vector2){ui.mouse.x+ui.windows.editors[5].rect.x,ui.mouse.y+ui.windows.editors[5].rect.y}); ui.input_enabled=0;
 108:     if(ui.eq_drag>=0) {
 109:         if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
 110:             EQBand *b=&settings->bands[ui.eq_drag];
 111:             b->frequency=spectrum_frequency(fmaxf(0,fminf(1,(ui.mouse.x-graph.x)/graph.width)));
 112:             if(b->shape!=EQ_LOW_CUT && b->shape!=EQ_HIGH_CUT) b->gain=fmaxf(-18,fminf(18,(.5f-(ui.mouse.y-graph.y)/graph.height)*36));
 113:         } else ui.eq_drag=-1;
 115:     label("Spectrum: -78 to 0 dBFS",graph.x,graph.y+graph.height+22,10,muted);
 116:     float y=height-55;
 117:     for(int i=0;i<EQ_BANDS;i++) if(button(TextFormat("%d",i+1),12+i*28,y-34,24,22,ui.eq_band==i)) ui.eq_band=i;
 119:     if(button(equalizer_shape_name(b->shape),216,y-34,152,22,0)) open_context(14,ui.eq_band,(Vector2){ui.mouse.x+ui.windows.editors[5].rect.x,ui.mouse.y+ui.windows.editors[5].rect.y});
 120:     if(hover(216,y-34,152,22)) snprintf(ui.status,sizeof ui.status,"Band shape: Bell, shelves, cuts or Off");
 121:     knob_style(92,y+4,&b->frequency,20,20000,equalizer_default().bands[ui.eq_band].frequency,"EQ frequency (Hz)",KNOB_LOGARITHMIC);
 123:     if(cut) label("12 dB/oct",174,y,11,muted);
 124:     else knob_style(200,y+4,&b->gain,-18,18,0,"EQ gain (dB)",KNOB_CENTER);
 125:     knob(310,y+4,&b->q,.2f,10,equalizer_default().bands[ui.eq_band].q,"EQ Q (bandwidth)");
 126:     label(TextFormat("%.0f Hz",b->frequency),66,y+24,11,muted);
 127:     label(cut?"Slope":TextFormat("%+.1f dB",b->gain),178,y+24,11,muted);
 128:     label(TextFormat("Q %.2f",b->q),288,y+24,11,muted);
 129:     knob(width-44,y+4,&ui.project.effect_mix[ui.eq_bus][ui.eq_slot],0,1,1,"Equalizer mix");
 130:     label("Mix",width-54,y+24,11,muted);
```

### meter_height

```c
 133: float meter_height(float level) { return level>.001f?fader_position(level):0; }
```

### meter_draw

```c
 136:     int height=bottom-top,spacing=bar_width+2;
 137:     float orange=meter_height(powf(10,-12.f/20)),zero=meter_height(1);
 138:     DrawRectangle(x-1,top,spacing*2,height,bg);
 139:     for(int side=0;side<2;side++) {
 141:         if(level>0) DrawRectangle(left,bottom-level*height,bar_width,level*height,ui_theme.meter_low);
 144:         if(peak>0) DrawRectangle(left-1,fmaxf(top,bottom-peak*height),bar_width+2,1,ui.meter_hold[id][side]>1?ui_theme.meter_high:ink);
 146:     if(hover(x-3,top,spacing*2+4,height)) snprintf(ui.status,sizeof ui.status,"%s output | Peak L %.1f dBFS, R %.1f dBFS | Red above 0 dBFS; markers hold 0.5 s",mixer_name(id),gain_db(ui.meter_hold[id][0]),gain_db(ui.meter_hold[id][1]));
```

### selected_meter

```c
 150:     int top=TITLE+28,bottom=height-34;
 151:     label("dBFS",8,TITLE+6,11,muted);
 152:     meter_draw(ui.mixer_selected,32,top,bottom,7);
 153:     const int ticks[]={6,0,-6,-12,-24,-36,-48,-60};
 154:     for(unsigned i=0;i<sizeof ticks/sizeof *ticks;i++) {
 155:         float y=bottom-meter_height(powf(10,ticks[i]/20.f))*(bottom-top);
 157:         label(number,16-text_width(number,10)/2,fmaxf(top,fminf(bottom-10,y-5)),10,muted);
 158:         DrawLine(29,y,31,y,muted);
 160:     float peak=fmaxf(ui.meter_hold[ui.mixer_selected][0],ui.meter_hold[ui.mixer_selected][1]);
 161:     label(peak?TextFormat("%.1f",gain_db(peak)):"-inf",8,height-29,11,peak>1?ui_theme.meter_high:ink);
 162:     if(hover(4,TITLE,54,height-TITLE-18)) snprintf(ui.status,sizeof ui.status,"Selected %s | Peak L %.1f dBFS, R %.1f dBFS",mixer_name(ui.mixer_selected),gain_db(ui.meter_hold[ui.mixer_selected][0]),gain_db(ui.meter_hold[ui.mixer_selected][1]));
```

### mixer

```c
 167:     int capacity=fmaxf(1,(width-MIXER_PANEL-MIXER_LEFT-8)/51-1),visible=ui.project.insert_count<capacity?ui.project.insert_count:capacity;
 168:     ui.mixer_scroll=(int)fmaxf(0,fminf(fmaxf(0,ui.project.insert_count-visible),ui.mixer_scroll));
 169:     if(hover(0,TITLE,width-220,height-TITLE) && ui.mouse.y>TITLE+60) {
 170:         ui.mixer_scroll=(int)fmaxf(0,fminf(fmaxf(0,ui.project.insert_count-visible),ui.mixer_scroll-GetMouseWheelMove()));
 172:     float fader_top=TITLE+76,fader_bottom=height-74;
 173:     for(int col=0;col<=visible;col++) {
 174:         int id=col?col+ui.mixer_scroll:0,x=MIXER_LEFT+col*51;
 175:         float *v=id?&ui.project.insert_volume[id-1]:&ui.project.master;
 176:         int over=hover(x,TITLE+4,48,height-TITLE-12);
 177:         ui_surface((Rectangle){x,TITLE+4,48,height-TITLE-12},ui.mixer_selected==id?ui_theme.title_focus:cell);
 179:         if(over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.mixer_selected=id;
 180:         DrawRectangle(x+3,TITLE+7,42,17,ui.mixer_selected==id?ui_theme.title_focus:cell);
 181:         label(id?TextFormat("%d",id):"Master",x+5,TITLE+9,id?12:10,ink);
 183:             label(fit_text(ui.project.insert_names[id-1],42,10),x+3,TITLE+26,10,muted);
 184:             if(hover(x+3,TITLE+25,42,16)) {
 185:                 snprintf(ui.status,sizeof ui.status,"%s: right-click to rename",ui.project.insert_names[id-1]);
 186:                 if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
 187:                     ui.rename_channel=ui.rename_track=-1; ui.rename_mixer=id-1; ui.mixer_selected=id;
 188:                     snprintf(ui.rename_text,sizeof ui.rename_text,"%s",ui.project.insert_names[id-1]); ui.rename_select_all=1; open_popup(2); ui.input_enabled=0;
 192:         int input_over=hover(x+7,height-46,12,12) || (ui.cable_drag>0 && CheckCollisionPointRec(ui.mouse,(Rectangle){x+7,height-46,12,12}));
 193:         DrawRectangleLines(x+9,height-44,8,8,input_over?ui_theme.signal:muted);
 196:         if(over && ui.mouse.y<TITLE+24 && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { ui.mixer_selected=id; open_context(3,id,(Vector2){ui.mouse.x+ui.windows.editors[3].rect.x,ui.mouse.y+ui.windows.editors[3].rect.y}); }
 197:         if(over && ui.mouse.y<TITLE+24) snprintf(ui.status,sizeof ui.status,"%s: right-click for mixer channel actions",mixer_name(id));
 198:         mute_light(x+11,TITLE+52,id?ui.project.insert_mute:&ui.project.master_mute,id?ui.project.insert_count:0,id?id-1:0,mixer_name(id));
 199:         if(id) knob_style(x+35,TITLE+52,&ui.project.insert_pan[id-1],-1,1,0,"Insert pan",KNOB_PAN);
 200:         meter_draw(id,x+8,fader_top,fader_bottom,3);
 201:         DrawRectangle(x+33,fader_top,3,fader_bottom-fader_top,bg);
 202:         if(hover(x+23,fader_top-4,22,fader_bottom-fader_top+8) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(v,0,MIXER_GAIN_MAX,1,id?"Insert volume":"Master volume");
 203:         if(hover(x+23,fader_top-4,22,fader_bottom-fader_top+8) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
 204:             capture_control(v,0,MIXER_GAIN_MAX,1); ui.control_range=fader_bottom-fader_top;
 206:             ui.control_bottom=ui.mouse.y+ui.windows.editors[3].rect.y+fader_position(*v)*ui.control_range;
 208:         if(hover(x+23,fader_top-4,22,fader_bottom-fader_top+8) || ui.control_drag==v) SetMouseCursor(MOUSE_CURSOR_RESIZE_NS);
 209:         if(hover(x+23,fader_top-4,22,fader_bottom-fader_top+8) || ui.control_drag==v) snprintf(ui.status,sizeof ui.status,"%s fader: %.1f dB | Mark = 0 dB; right-click for value / automation",mixer_name(id),gain_db(*v));
 210:         DrawLine(x+25,fader_bottom-fader_position(1)*(fader_bottom-fader_top),x+44,fader_bottom-fader_position(1)*(fader_bottom-fader_top),muted);
 212:         ui_fader_handle((Rectangle){x+26,fy-9,14,18},ui.mixer_selected==id);
 213:         knob_style(x+33,height-59,id?&ui.project.insert_width[id-1]:&ui.project.master_width,0,2,1,"Stereo width (left wider, center unchanged, right mono)",KNOB_WIDTH);
 214:         int arm_over=hover(x+4,height-67,16,16);
 215:         circle(x+12,height-59,6,arm_over || ui.record_armed[id]?ui_theme.meter_high:ui_theme.border);
 216:         circle(x+12,height-59,5,ui.record_armed[id]?ui_theme.meter_high:cell);
 219:             if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !recording_active()) { ui.record_armed[id]^=1; ui.input_enabled=0; }
 223:             int output_over=hover(x+29,height-46,12,12);
 224:             circle(x+35,height-40,5,output_over || ui.cable_drag==id?accent:muted);
 225:             circle(x+35,height-40,2,ui.project.insert_output[id-1]==255?bg:accent);
 228:                 if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) insert_connect(&ui.project,id,255);
 229:                 if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.cable_drag=id; ui.mixer_selected=id; ui.input_enabled=0; }
 233:     if(ui.mixer_selected>0) {
 234:         int col=ui.mixer_selected-ui.mixer_scroll,destination=ui.project.insert_output[ui.mixer_selected-1];
 235:         int target=destination?destination-ui.mixer_scroll:0;
 236:         if(col>=1 && col<=visible && (ui.cable_drag==ui.mixer_selected || (destination!=255 && target>=0 && target<=visible && (!destination || target>=1)))) {
 237:             Vector2 start={MIXER_LEFT+col*51+35,height-40},end=ui.cable_drag==ui.mixer_selected?ui.mouse:(Vector2){MIXER_LEFT+target*51+13,height-40};
 238:             Vector2 points[]={start,{start.x,height-21},{end.x,height-21},end};
 239:             cable(points,Fade(ui_theme.signal,.8f));
 244:     label("Recording input",fx+8,TITLE+8,11,muted);
 245:     device_button(0,fx+8,TITLE+26,MIXER_PANEL-16);
 246:     label("Effects",fx+8,TITLE+70,12,ink);
 247:     for(int slot=0;slot<EFFECT_SLOTS;slot++) {
 248:         int x=fx+8+(slot%5)*29,y=TITLE+90+(slot/5)*24;
 249:         if(button(TextFormat("%d%s",slot+1,ui.project.effect_type[ui.mixer_selected][slot]?"·":""),x,y,27,22,ui.effect_slot==slot)) ui.effect_slot=slot;
 250:         if(hover(x,y,27,22)) snprintf(ui.status,sizeof ui.status,"Effect slot %d: %s",slot+1,ui.project.effect_type[ui.mixer_selected][slot]?effect_descriptor(ui.project.effect_type[ui.mixer_selected][slot])->name:"Empty");
 254:         if(button("+ Chorus",fx+8,TITLE+148,MIXER_PANEL-16,25,0)) {
 256:             ui.project.effect_mix[bus][slot]=.5f; ui.project.effect_bypass[bus][slot]=0;
 258:         if(button("+ Equalizer",fx+8,TITLE+177,MIXER_PANEL-16,25,0)) {
 259:             ui.project.effect_type[bus][slot]=EFFECT_EQ; effect_descriptor(EFFECT_EQ)->defaults(&ui.project.eq[bus][slot]); ui.project.effect_mix[bus][slot]=1; ui.project.effect_bypass[bus][slot]=0;
 260:             ui.eq_bus=bus; ui.eq_slot=slot; ui.eq_drag=-1; windows_focus(&ui.windows,5);
 263:         label(effect_descriptor(ui.project.effect_type[bus][slot])->name,fx+8,TITLE+148,12,ink);
 264:         if(button("x",fx+MIXER_PANEL-28,TITLE+144,20,22,0)) {
 266:             for(int a=ui.project.automation_count-1;a>=0;a--) {
 270:             ui.control_drag=NULL; ui.automation_selected=ui.automation_node=-1; ui.picker_drag=-1;
 272:             if(button("Open Equalizer",fx+8,TITLE+177,MIXER_PANEL-16,25,0)) { ui.eq_bus=bus; ui.eq_slot=slot; ui.eq_drag=-1; windows_focus(&ui.windows,5); }
 273:             if(button("Bypass",fx+8,TITLE+225,MIXER_PANEL-16,24,ui.project.effect_bypass[bus][slot])) ui.project.effect_bypass[bus][slot]^=1;
 274:             if(button("Load",fx+8,TITLE+253,70,24,0)) preset_action(PRESET_EQ,bus,slot,0);
 275:             if(button("Save",fx+82,TITLE+253,70,24,0)) preset_action(PRESET_EQ,bus,slot,1);
 278:             knob(fx+27,TITLE+183,&settings->rate,.05f,5,.513f,"Chorus rate (Hz)");
 279:             knob(fx+78,TITLE+183,&settings->depth,0,8,1.85f,"Chorus depth (ms)");
 280:             knob(fx+129,TITLE+183,&ui.project.effect_mix[bus][slot],0,1,.5f,"Chorus mix");
 281:             label("Rate",fx+15,TITLE+203,10,muted); label("Depth",fx+64,TITLE+203,10,muted); label("Mix",fx+120,TITLE+203,10,muted);
 282:             if(button("Bypass",fx+8,TITLE+225,MIXER_PANEL-16,24,ui.project.effect_bypass[bus][slot])) ui.project.effect_bypass[bus][slot]^=1;
 283:             if(button("Load",fx+8,TITLE+253,70,24,0)) preset_action(PRESET_CHORUS,bus,slot,0);
 284:             if(button("Save",fx+82,TITLE+253,70,24,0)) preset_action(PRESET_CHORUS,bus,slot,1);
 287:     float area=fx-MIXER_LEFT-8,thumb=area*visible/INSERTS,pos=MIXER_LEFT+area*ui.mixer_scroll/INSERTS;
 288:     DrawRectangle(MIXER_LEFT,height-17,area,9,bg); ui_surface((Rectangle){pos,height-17,fmaxf(12,thumb),9},ui.mixer_pan || hover(MIXER_LEFT,height-20,area,14)?accent:muted);
 289:     if(hover(MIXER_LEFT,height-20,area,14)) {
 290:         snprintf(ui.status,sizeof ui.status,"Mixer: wheel over inserts or drag this scrollbar to browse all 100 inserts");
 291:         if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.mixer_pan=1; ui.mixer_scroll=fmaxf(0,fminf(INSERTS-visible,(ui.mouse.x-MIXER_LEFT-thumb/2)/area*INSERTS)); ui.input_enabled=0; }
```

## ui_piano.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### piano_visible_rows

```c
   4: float piano_visible_rows(void) { return fmaxf(8,fminf(128,25/ui.piano_zoom)); }
```

### piano_min_top

```c
   6: int piano_min_top(void) { return (int)ceilf(piano_visible_rows())-1; }
```

### edit_selection

```c
  11:     if((id!=1 && id!=2) || !ui.windows.editors[id].visible || ui.browser_focus || captured()) { snprintf(ui.status,sizeof ui.status,"Focus the Piano Roll or Arrangement to edit its selection"); return; }
  12:     if(action==3) {
  13:         if(id==2) for(int i=0;i<NOTES;i++) ui.note_selected[ui.pattern][ui.piano_channel][i]=!!ui.project.notes[ui.pattern][ui.piano_channel][i].velocity;
  14:         else for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) ui.arrangement.selected[l][b]=!!ui.project.clips[l][b];
  15:         snprintf(ui.status,sizeof ui.status,"Selected all %s",id==2?"notes":"clips"); return;
  18:     if(action<2) {
  19:         result=id==2?clipboard_copy_notes(&ui.edit_clipboard,&ui.project,ui.pattern,ui.piano_channel,ui.note_selected[ui.pattern][ui.piano_channel]):clipboard_copy_clips(&ui.edit_clipboard,&ui.project,(const uint8_t (*)[CLIPS])ui.arrangement.selected);
  20:         if(result && action==0) {
  22:             if(id==2) { for(int i=0;i<NOTES;i++) if(ui.note_selected[ui.pattern][ui.piano_channel][i]) ui.project.notes[ui.pattern][ui.piano_channel][i].velocity=0; memset(ui.note_selected[ui.pattern][ui.piano_channel],0,NOTES); }
  23:             else { for(int l=0;l<LANES;l++) for(int b=0;b<CLIPS;b++) if(ui.arrangement.selected[l][b]) { ui.project.clips[l][b]=0; ui.project.clip_steps[l][b]=0; } memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected); }
  25:         snprintf(ui.status,sizeof ui.status,result?"%s %d %s":"Nothing selected",action==0?"Cut":"Copied",result,id==2?"notes":"clips"); return;
  28:     result=id==2?clipboard_paste_notes(&ui.edit_clipboard,&ui.project,ui.pattern,ui.piano_channel,ui.piano_start[ui.pattern],ui.note_selected[ui.pattern][ui.piano_channel]):clipboard_paste_clips(&ui.edit_clipboard,&ui.project,ui.selected_track>=0?ui.selected_track:ui.edit_clipboard.lane,ui.playlist_start,ui.arrangement.selected);
  29:     if(result>0) { if(id==2) { ui.piano_channels[ui.pattern][ui.piano_channel]=1; if(ui.arrangement.source_pattern==ui.pattern) ui.arrangement.source_steps=ui.project.pattern_steps[ui.pattern]; } snprintf(ui.status,sizeof ui.status,"Pasted %d %s at the start marker",result,id==2?"notes":"clips"); }
  30:     else snprintf(ui.status,sizeof ui.status,"%s",result==-1?"Clip sources changed; copy the selection again":result==-2?"No room for the entire selection":result==-3?"Paste position is occupied; move the start marker to empty space":result==-4?"Selection does not fit at this destination":"Copy a selection from this editor first");
```

### piano_shift

```c
  34:     if(ui.windows.focused!=2 || !ui.windows.editors[2].visible || captured() || ui.browser_focus || ui.midi_take.active) return;
  35:     uint8_t *selected=ui.note_selected[ui.pattern][ui.piano_channel]; int count=0,low=127,high=0;
  37:     for(int i=0;i<NOTES;i++) if(selected[i] && ui.project.notes[ui.pattern][ui.piano_channel][i].velocity) {
  40:         if(pitch<0 || pitch>127 || n.start+dx<0) { snprintf(ui.status,sizeof ui.status,"Move stopped at the note or timeline limit"); return; }
  41:         end=fmaxf(end,n.start+dx+(n.length?n.length:1));
  56:         ui.piano_top=fmaxf(piano_min_top(),fminf(127,ui.piano_top));
```

### piano_octave

```c
  62:     if(ui.piano_tool==PENCIL) piano_shift(0,direction*12,direction>0?"up an octave":"down an octave");
```

### piano_gesture_update

```c
  69: void piano_gesture_update(float step,float pitch,float q) {
  71:     uint8_t *selected=ui.note_selected[ui.pattern][channel];
  72:     if(ui.piano_gesture==PIANO_BOX) {
  74:         for(int i=0;i<NOTES;i++) {
  76:             selected[i]=(ui.piano_additive && ui.note_selection_before[i]) || (n.velocity && n.start<edit_steps() && n.start<=right && n.start+(n.length?n.length:1)>left && n.pitch>=low && n.pitch-1<=high);
  79:         float dx=step-ui.piano_now.x,dy=pitch-ui.piano_now.y,spacing=ui.piano_gesture==PIANO_BRUSH?fmaxf(ui.piano_note_length,q):q>0?q:1;
  80:         int count=fminf(NOTES*2,ceilf(fmaxf(fabsf(dx)/spacing,fabsf(dy))*2)); if(count<1) count=1;
  81:         for(int j=0;j<=count;j++) {
  83:             if(at<0 || piano_inactive(at) || key<0 || key>127) continue;
  84:             if(ui.piano_gesture==PIANO_ERASE) {
  85:                 /* Hit the gesture's original stack so holding the button or
  88:                 if(i>=0) { ui.project.notes[ui.pattern][channel][i].velocity=0; selected[i]=0; }
  94:                 if(n) { audio_note(channel,*n); ui.piano_channels[ui.pattern][channel]=1; }
  98:     ui.piano_now=(Vector2){step,pitch};
```

### piano_note_drag_update

```c
 101: void piano_note_drag_update(float step,float row,float q) {
 104:         uint8_t *selected=ui.note_selected[ui.pattern][channel];
 105:         float dx=snap_round(step-ui.note_grab.x,q),low=0,high=0; int dy=roundf(ui.note_grab.y-row),bottom=0,top=127,first=1;
 106:         for(int i=0;i<NOTES;i++) if(selected[i]) {
 107:             Note n=ui.note_before[i]; float end=n.start+(n.length?n.length:1);
 112:             first=0;
 114:         dx=fmaxf(-low,dx); piano_extend(high+dx); dy=fmaxf(-bottom,fminf(127-top,dy));
 115:         int old_pitch=ui.note_drag->pitch;
 117:         if(ui.note_drag->pitch!=old_pitch) audio_note(channel,*ui.note_drag);
 120:         float end=notes_resize(&ui.project,ui.pattern,channel,ui.note_before,ui.note_selected[ui.pattern][channel],delta,q>0?q:.01f);
 121:         piano_extend(end); ui.piano_note_length=ui.note_drag->length;
```

### piano_black_key

```c
 126:     int pc=pitch%12; return pc==1 || pc==3 || pc==6 || pc==8 || pc==10;
```

### piano_key_rect

```c
 130:     float center=PIANO_GRID_TOP+(ui.piano_top-pitch+.5f)*rh;
 131:     if(piano_black_key(pitch)) return (Rectangle){4,center-rh*.5f,34,rh};
 132:     float top=center-rh*(piano_black_key(pitch+1)?1:.5f);
 133:     float bottom=center+rh*(piano_black_key(pitch-1)?1:.5f);
 135:     return (Rectangle){4,top,54,fmaxf(0,bottom-top)};
```

### piano_key_at

```c
 139:     if(x<4 || x>=58 || y<PIANO_GRID_TOP || y>=PIANO_GRID_TOP+rh*piano_visible_rows()) return -1;
 141:     if(piano_black_key(pitch) && x>=38) pitch+=row-floorf(row)<.5f?1:-1;
 142:     return pitch>=0 && pitch<=127?pitch:-1;
```

### piano_key_update

```c
 146:     Rect r=ui.windows.editors[2].rect; float rh=(r.h-PIANO_GRID_PADDING)/piano_visible_rows();
 148:     if(cancel || pitch<0 || pitch>127) pitch=-1;
 150:         if(ui.piano_key>=0) audio_key(96+ui.piano_key%25,ui.piano_key_channel,ui.piano_key,0);
 152:         if(pitch>=0) audio_key(96+pitch%25,ui.piano_key_channel,pitch,1);
 154:     if(cancel) ui.piano_key_drag=0;
```

### piano

```c
 159:     int gx=60,gy=PIANO_GRID_TOP; float gridw=width-84,rh=(height-PIANO_GRID_PADDING)/piano_visible_rows();
 161:     Rect r=ui.windows.editors[2].rect; float scale=ui_scale();
 163:     for(int i=0;i<3;i++) if(tool_button(i,ui.piano_tool,8+i*24,tips[i])) ui.piano_tool=i;
 164:     label(fit_text(TextFormat("%s / %s",ui.project.channel_names[channel],ui.project.pattern_names[ui.pattern]),width-158,12),88,30,12,muted);
 165:     int grid_over=hover(gx,gy,gridw,rh*piano_visible_rows());
 168:     if(playback>=0) follow_view(&ui.piano_pan[ui.pattern],extent,playback);
 170:     ui.piano_range[ui.pattern]=fmaxf(ui.piano_range[ui.pattern],fmaxf(view+extent*2,end+extent));
 172:     if(hover(4,gy,gx-6,rh*piano_visible_rows())) {
 174:         if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
 175:             ui.piano_key_drag=1; ui.piano_key_channel=channel; piano_key_update(ui.mouse.x,ui.mouse.y,0); ui.input_enabled=0;
 178:     DrawRectangleRec((Rectangle){4,gy,gx-6,rh*piano_visible_rows()},ui_theme.piano_white);
 179:     BeginScissorMode((r.x+4)*scale,(r.y+gy)*scale,54*scale,(height-PIANO_GRID_PADDING)*scale);
 180:     int hovered_key=ui.input_enabled?piano_key_at(ui.mouse.x,ui.mouse.y,rh):-1;
 182:     for(int black=0;black<2;black++) for(int row=-1;row<=(int)ceilf(piano_visible_rows());row++) {
 184:         if(pitch<0 || pitch>127 || piano_black_key(pitch)!=black) continue;
 187:         key.y=fmaxf(gy,key.y); key.height=fmaxf(0,bottom-key.y);
 188:         if(key.height<=0) continue;
 190:         Color base=black?ui_theme.piano_black:pitch%12==0?ui_theme.piano_c:ui_theme.piano_white;
 192:         if(!active && hovered_key==pitch) DrawRectangleRec(key,Fade(accent,.22f));
 193:         if(!black) DrawLineEx((Vector2){key.x,key.y+key.height},(Vector2){key.x+key.width,key.y+key.height},1,Fade(ui_theme.piano_black,.15f));
 194:         if(pitch%12==0) {
 195:             const char *name=TextFormat("C%d",pitch/12-1);
 196:             label(name,gx-5-text_width(name,10),fmaxf(gy,fminf(gy+rh*piano_visible_rows()-10,gy+(row+.5f)*rh-5)),10,active?ui_theme.selected_text:ui_theme.piano_black);
 200:     for(int row=0;row<(int)ceilf(piano_visible_rows());row++) {
 201:         int pc=(ui.piano_top-row)%12,black=pc==1 || pc==3 || pc==6 || pc==8 || pc==10;
 205:         if(pitch>=0 && pitch<=127 && (ui.keyboard_notes[channel][pitch] || ui.playing_keys[channel][pitch] ||
 207:             DrawRectangleRec((Rectangle){gx,y,gridw,rh},Fade(accent,.15f));
 208:         DrawLineEx((Vector2){gx,y},(Vector2){gx+gridw,y},1,Fade(ui_theme.grid_minor,.45f));
 209:         if(partial>end) { float left=fmaxf(gx,gx+(end-view)*cw),right=fminf(gx+gridw,gx+(partial-view)*cw); if(right>left) DrawRectangleRec((Rectangle){left,y,right-left,rh-1},ui_theme.disabled); }
 210:         if(pc==0) DrawLineEx((Vector2){gx,y+rh},(Vector2){gx+gridw,y+rh},1,ui_theme.grid_major);
 214:     for(int i=0;i<NOTES;i++) {
 216:         if(!n->velocity || n->pitch<ui.piano_top-(int)ceilf(piano_visible_rows())+1 || n->pitch>ui.piano_top || n->start>=end || n->start+(n->length?n->length:1)<=view) continue;
 217:         float note_end=fminf(n->start+(n->length?n->length:1),end);
 221:         float inset=fminf(1,rh*.15f),note_height=rh-2*inset;
 222:         if(ui.note_selected[ui.pattern][channel][i]) DrawRectangleLinesEx((Rectangle){left,y,right-left,rh},fminf(1,rh*.25f),ink);
 223:         DrawRectangleRec((Rectangle){left+1,y+inset,fmaxf(1,right-left-2),note_height},n==ui.note_drag || ui.note_selected[ui.pattern][channel][i]?ui_theme.note_drag:ui_theme.note);
 224:         DrawRectangleLinesEx((Rectangle){left+1,y+inset,fmaxf(1,right-left-2),note_height},fminf(1,rh*.25f),Fade(ui_theme.signal,.45f));
 225:         int edge=grid_over && !shortcut_down() && hover(x+w-7,y,7,rh);
 226:         if(x+w>=gx && x+w<=gx+gridw) DrawRectangleRec((Rectangle){x+w-5,y+inset,3,note_height},edge || (n==ui.note_drag && !ui.moving_note)?ink:Fade(bg,.35f));
 227:         if(edge || (n==ui.note_drag && !ui.moving_note)) SetMouseCursor(MOUSE_CURSOR_RESIZE_EW);
 228:         if(grid_over && hover(x,y,w,rh)) hit=n;
 234:         else if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
 235:             uint8_t *selected=ui.note_selected[ui.pattern][channel];
 236:             if(hit && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.piano_note_length=hit->length?hit->length:2;
 237:             Vector2 point={view+(ui.mouse.x-gx)/cw,ui.piano_top-(ui.mouse.y-gy)/rh};
 238:             int shift=IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT),group=0;
 239:             for(int i=0;i<NOTES;i++) group+=selected[i]!=0;
 241:             if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
 243:                 ui.piano_gesture=PIANO_ERASE;
 245:             else if(shortcut_down()) {
 246:                 if(!shift) memset(selected,0,NOTES);
 247:                 ui.piano_gesture=PIANO_BOX;
 248:             } else if(shift && hit) selected[hit-ui.project.notes[ui.pattern][channel]]^=1;
 250:                 if(!shift) memset(selected,0,NOTES);
 251:                 ui.piano_gesture=PIANO_BOX;
 252:             } else if(ui.piano_tool==BRUSH && !(hit && selected[hit-ui.project.notes[ui.pattern][channel]] && group>1)) ui.piano_gesture=PIANO_BRUSH;
 254:                 ui.moving_note=0;
 256:                     float edge=gx+(fminf(hit->start+(hit->length?hit->length:1),end)-view)*cw;
 257:                     ui.note_drag=hit; ui.moving_note=ui.mouse.x<edge-7;
 258:                     if(!selected[hit-ui.project.notes[ui.pattern][channel]]) { memset(selected,0,NOTES); selected[hit-ui.project.notes[ui.pattern][channel]]=1; }
 260:                     ui.note_grab=(Vector2){point.x,(ui.mouse.y-gy)/rh};
 262:                     memset(selected,0,NOTES);
 264:                     ui.note_drag=note_add(&ui.project,ui.pattern,channel,start,ceilf(point.y),ui.piano_note_length);
 265:                     if(ui.note_drag) {
 266:                         selected[ui.note_drag-ui.project.notes[ui.pattern][channel]]=1; ui.moving_note=1;
 268:                         ui.note_grab=(Vector2){point.x,(ui.mouse.y-gy)/rh};
 271:                 if(!ui.note_drag) snprintf(ui.status,sizeof ui.status,"No room here: remove a note (limit %d).",NOTES);
 272:                 else { audio_note(channel,*ui.note_drag); ui.piano_channels[ui.pattern][channel]=1; }
 274:             if(ui.piano_gesture==PIANO_BRUSH) ui.piano_from.x=start;
 275:             if(ui.piano_gesture) piano_gesture_update(point.x,point.y,q);
 276:             ui.input_enabled=0;
 279:     if(ui.piano_gesture==PIANO_BOX) {
 282:         DrawRectangleRec(box,Fade(ui_theme.signal,.15f)); DrawRectangleLinesEx(box,1,ui_theme.signal);
 286:     int vy=height-84; label("Velocity",6,vy+4,10,muted);
 287:     BeginScissorMode((r.x+gx)*scale,(r.y+vy)*scale,gridw*scale,62*scale);
 288:     DrawRectangle(gx,vy,gridw,62,ui_theme.piano_row[0]);
 289:     for(int line=1;line<5;line++) DrawLine(gx,vy+line*62/5,gx+gridw,vy+line*62/5,Fade(ui_theme.grid_minor,.6f));
 290:     timeline_grid(view,extent,gx,vy,gridw,62);
 291:     if(partial>end) { float left=fmaxf(gx,gx+(end-view)*cw),right=fminf(gx+gridw,gx+(partial-view)*cw); if(right>left) DrawRectangle(left,vy,right-left,62,ui_theme.disabled); }
 292:     for(int i=0;i<NOTES;i++) {
 295:         if(x<gx-10 || x>gx+gridw) continue;
 296:         float top=vy+62-n.velocity*.48f;
 297:         Color color=ui.note_selected[ui.pattern][channel][i]?ui_theme.note_drag:ui_theme.note;
 298:         DrawRectangleRec((Rectangle){x,top,2,n.velocity*.48f},color);
 299:         circle(x+1,top,2.5f,color); circle(x+1,top,1.25f,ui_theme.piano_row[0]);
 301:     if(hover(gx,vy,gridw,62)) {
 303:         if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
 304:             ui.velocity_drag=IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)?1:0;
 306:             ui.velocity_from=ui.velocity_now=(Vector2){view+(ui.mouse.x-gx)/cw,fmaxf(1,fminf(127,(vy+62-ui.mouse.y)/.48f))};
 308:             notes_velocity(ui.project.notes[ui.pattern][channel],ui.velocity_drag?ui.velocity_before:NULL,
 309:                 ui.velocity_from.x,ui.velocity_from.x,ui.velocity_from.y,ui.velocity_from.y,3/cw);
 310:             ui.input_enabled=0;
 313:     if(ui.velocity_drag==1 && ui.velocity_pattern==ui.pattern && ui.velocity_channel==channel)
 314:         DrawLineEx((Vector2){gx+(ui.velocity_from.x-view)*cw,vy+62-ui.velocity_from.y*.48f},
 315:             (Vector2){gx+(ui.velocity_now.x-view)*cw,vy+62-ui.velocity_now.y*.48f},1,accent);
 317:     float rail_top=gy-14,varea=rh*piano_visible_rows()+14,vthumb=varea*piano_visible_rows()/128,vypos=rail_top+(127-ui.piano_top)/128.f*varea;
 318:     DrawRectangleRec((Rectangle){width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,varea},bg);
 319:     ui_surface((Rectangle){width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,vypos,EDITOR_SCROLLBAR,vthumb},ui.piano_vdrag || hover(width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,varea)?accent:muted);
 320:     if(hover(width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,varea)) {
 322:         if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
 323:             if(ui.mouse.y<vypos || ui.mouse.y>vypos+vthumb) ui.piano_top=127-(int)fmaxf(0,fminf(127-piano_min_top(),(ui.mouse.y-rail_top-vthumb/2)/varea*128));
 324:             ui.piano_vscroll_y=ui.mouse.y+r.y; ui.piano_vscroll_start=127-ui.piano_top; ui.piano_vdrag=1; ui.input_enabled=0;
 327:     float rail_width=width-4-EDITOR_CORNER-gx,thumb=timeline_thumb(rail_width,extent,ui.piano_range[ui.pattern]),travel=rail_width-thumb,maximum=ui.piano_range[ui.pattern]-extent,x=gx+(maximum>0?fmaxf(0,ui.piano_pan[ui.pattern])/maximum*travel:0);
 328:     DrawRectangleRec((Rectangle){gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR},bg); ui_surface((Rectangle){x,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,thumb,EDITOR_SCROLLBAR},ui.piano_scroll_drag || hover(gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR)?accent:muted);
 329:     if(hover(gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR)) {
 331:         if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
 332:             if(ui.mouse.x<x || ui.mouse.x>x+thumb) ui.piano_pan[ui.pattern]=fmaxf(0,fminf(maximum,(ui.mouse.x-gx-thumb/2)/fmaxf(1,travel)*maximum));
 333:             ui.piano_scroll_x=ui.mouse.x+r.x; ui.piano_scroll_start=ui.piano_pan[ui.pattern]; ui.piano_scroll_range=maximum/fmaxf(1,travel); ui.piano_scroll_drag=1; ui.input_enabled=0;
 336:     row_zoom_button(2,width,gy-14-EDITOR_CORNER);
 337:     timeline_ruler(2,view,extent,gx,gy-14,gridw,q);
 338:     if(playback>=0) {
 340:         if(x>=gx && x<=gx+gridw) DrawLineEx((Vector2){x,gy},(Vector2){x,gy+rh*piano_visible_rows()},1.5f,ui_theme.signal);
```

## ui_playlist.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### track_height

```c
   4: float track_height(int lane) { return fmaxf(28,fminf(640,(ui.track_heights[lane]>0?ui.track_heights[lane]:52)*ui.track_zoom)); }
```

### track_position

```c
   7:     float y=0; int l=fmaxf(0,fminf(LANES,floorf(lane)));
   8:     for(int i=0;i<l;i++) y+=track_height(i);
   9:     return y+(l<LANES?(lane-l)*track_height(l):0);
```

### track_at

```c
  13:     for(int l=0;l<LANES;l++) { float h=track_height(l); if(position<h) return l+position/h; position-=h; }
```

### previews_update

```c
  18:     float pixels=(ui.windows.editors[1].rect.w-236)*ui.arrangement.zoom/(BARS*STEPS)*scale;
  19:     for(int pat=0;pat<ui.project.pattern_count;pat++) {
  22:         ui.previews[pat].valid=0;
  23:         if(!isfinite(size) || size>2048) continue; /* Keep vector detail at extreme zoom. */
  24:         int w=fmaxf(1,ceilf(size)),h=fmaxf(1,ceilf(32*scale));
  39:             float note_height=fmaxf(2,fminf(5,32.f/fmaxf(1,high-low+1)));
  40:             for(int c=0;c<ui.project.channel_count;c++) for(int n=0;n<NOTES;n++) {
  43:                 float fraction=high>low?(high-note.pitch)/(float)(high-low):.5f;
  44:                 float y=fraction*(32-note_height)*h/32;
  45:                 float length=fmaxf(scale,(note.length?note.length:1)*w/ui.project.pattern_steps[pat]-2*scale);
  46:                 DrawRectangleRec((Rectangle){x,y,length,note_height*h/32},content);
  50:         ui.previews[pat].valid=1;
```

### clip_preview

```c
  60:             Texture2D image=ui.previews[pat].image.texture;
  61:             DrawTexturePro(image,(Rectangle){(left-x)*image.width/full,0,(right-left)*image.width/full,-image.height},
  62:                            (Rectangle){left,y+16,right-left,height-20},(Vector2){0},0,WHITE);
  66:         float area=fmaxf(1,height-20),note_height=fmaxf(2,fminf(5,32.f/fmaxf(1,high-low+1)))*area/32;
  67:         for(int c=0;c<ui.project.channel_count;c++) for(int n=0;n<NOTES;n++) {
  69:             float fraction=high>low?(high-note.pitch)/(float)(high-low):.5f;
  70:             float yy=y+16+fraction*(area-note_height);
  71:             float nx=x+note.start*pixels+1,nw=fmaxf(1,fminf(note.length?note.length:1,steps-note.start)*pixels-2);
  72:             if(nx<=right && nx+nw>=left) DrawRectangleRec((Rectangle){fmaxf(left,nx),yy,fmaxf(1,fminf(right,nx+nw)-fmaxf(left,nx)),note_height},content);
```

### picker_notes

```c
  78:     Color content=clip_foreground(pattern_color(p)); int low=127,high=0,count=0; float end=0;
  79:     for(int c=0;c<ui.project.channel_count;c++) for(int n=0;n<NOTES;n++) {
  82:         end=fmaxf(end,fminf(ui.project.pattern_steps[p],note.start+(note.length?note.length:1)));
  85:     float span=fmaxf(STEPS,ceilf(end/STEPS)*STEPS),note_height=fmaxf(2,fminf(5,26.f/(high-low+1)));
  86:     for(int c=0;c<ui.project.channel_count;c++) for(int n=0;n<NOTES;n++) {
  88:         float px=x+4+104*note.start/span;
  89:         float length=fmaxf(1,104*fminf(note.length?note.length:1,span-note.start)/span-1);
  90:         float fraction=high>low?(high-note.pitch)/(float)(high-low):.5f;
  91:         float py=y+20+fraction*(26-note_height);
```

### automation_curve

```c
 107:     float top=y+19,range=fmaxf(1,height-25);
 109:         float first=origin+automation->points[0].step*pixels,last=origin+automation->points[automation->count-1].step*pixels;
 110:         float first_y=top+(1-automation->points[0].value)*range,last_y=top+(1-automation->points[automation->count-1].value)*range;
 111:         if(first>left) smooth_line((Vector2){left,first_y},(Vector2){fminf(right,first),first_y},1.5f,content);
 112:         if(last<right) smooth_line((Vector2){fmaxf(left,last),last_y},(Vector2){right,last_y},1.5f,content);
 114:     for(int i=1;i<automation->count;i++) {
 115:         AutomationPoint p=automation->points[i-1],q=automation->points[i];
 119:             if(x>=left && x<=right) smooth_line((Vector2){x,top+(1-p.value)*range},(Vector2){x,top+(1-q.value)*range},1.5f,content);
 124:         smooth_line((Vector2){l,top+(1-lv)*range},(Vector2){r,top+(1-rv)*range},1.5f,content);
 126:     if(nodes) for(int i=0;i<automation->count;i++) {
 127:         float x=origin+automation->points[i].step*pixels,yy=top+(1-automation->points[i].value)*range;
 128:         if(x>=left && x<=right+2) { x=fmaxf(left+4,fminf(right-4,x)); circle(x,yy,4,content); circle(x,yy,2,source_color(AUTOMATION_SOURCE+a)); }
```

### draw_picker_drag

```c
 132: void draw_picker_drag(void) {
 133:     if(ui.picker_drag<0 || (fabsf(ui.mouse.x-ui.picker_origin.x)<3 && fabsf(ui.mouse.y-ui.picker_origin.y)<3)) return;
 134:     int source=ui.picker_drag,audio=AUDIO_SOURCE(source);
 137:     ui_surface((Rectangle){x,y,112,50},color);
 138:     if(audio) audio_waveform(source-PATTERNS,x+4,y+20,x+4,x+108,104/fmaxf(.001f,steps),28);
 139:     else if(source>=AUTOMATION_SOURCE) automation_curve(source-AUTOMATION_SOURCE,x+4,y,104/steps,50,x+4,x+108,0);
 141:     label(fit_text(source_name(source),100,12),x+4,y+4,12,clip_foreground(color));
 142:     DrawRectangleLinesEx((Rectangle){x,y,112,50},2,WHITE);
```

### draw_browser_drag

```c
 145: void draw_browser_drag(void) {
 146:     if(!ui.sample_drag[0] || !ui.sample_moved) return;
 148:     ui_surface((Rectangle){x,y,ui.sample_drag_width,21},accent);
 149:     label(fit_text(GetFileName(ui.sample_drag),fmaxf(0,ui.sample_drag_width-ui.sample_text_offset-2),13),x+ui.sample_text_offset,y+4,13,ui_theme.selected_text);
```

### tool_button

```c
 153:     int over=hover(x,28,20,20);
 157:     ui_surface((Rectangle){x,28,20,20},surface);
 158:     icon(tool==PENCIL?ICON_PENCIL:tool==BRUSH?ICON_BRUSH:tool==CUT?ICON_CUT:tool==STRETCH?ICON_STRETCH:ICON_SELECT,x+10,38,20,c);
 160:     return over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
```

### row_zoom_update

```c
 164:     float zoom=ui.row_zoom_start*expf((ui.row_zoom_y-y)/160);
 165:     if(ui.row_zoom_drag==1) { ui.track_zoom=fmaxf(.4f,fminf(4,zoom)); ui.track_scroll=track_position(ui.row_zoom_anchor); }
 166:     else if(ui.row_zoom_drag==2) { ui.piano_zoom=fmaxf(25.f/128,fminf(25.f/8,zoom)); ui.piano_top=fmaxf(piano_min_top(),ui.piano_top); ui.piano_scroll_remainder=0; }
```

### row_zoom_button

```c
 170:     float x=width-4-EDITOR_CORNER;
 171:     int over=hover(x,top,EDITOR_CORNER,EDITOR_CORNER),active=over || ui.row_zoom_drag==id;
 172:     Color surface=ui.row_zoom_drag==id?(over?ui_theme.active_hover:accent):over?ui_theme.hover:cell;
 174:     if(over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
 175:         ui.row_zoom_drag=id; ui.row_zoom_y=ui.mouse.y+ui.windows.editors[id].rect.y;
 176:         ui.row_zoom_start=id==1?ui.track_zoom:ui.piano_zoom; ui.row_zoom_anchor=track_at(ui.track_scroll); ui.input_enabled=0;
 178:     icon(ICON_ROW_ZOOM,x+EDITOR_CORNER/2,top+EDITOR_CORNER/2,16,active?theme_foreground(surface):muted);
 179:     if(active) { SetMouseCursor(MOUSE_CURSOR_RESIZE_NS); snprintf(ui.status,sizeof ui.status,"Vertical zoom: drag up for taller %s, down to show more",id==1?"tracks":"notes"); }
```

### ruler_press

```c
 182: void ruler_press(int id,int button,float at,double time,Vector2 position) {
 183:     float *range=id==1?ui.song_loop:ui.pattern_loop[ui.pattern];
 184:     int twice=ui.ruler_last_id==id && (id==1 || ui.ruler_last_pattern==ui.pattern) && ui.ruler_last_button==button &&
 185:         time-ui.ruler_last_click<.3 && fabsf(position.x-ui.ruler_last_position.x)<6 && fabsf(position.y-ui.ruler_last_position.y)<6;
 187:     ui.stop_armed=0;
 188:     if(ui.song!=(id==1)) { ui.song=id==1; ui.reset=1; }
 189:     if(twice) { range[0]=range[1]=0; ui.marker_drag=-1; ui.ruler_last_id=-1; return; }
 190:     ui.marker_drag=id; ui.ruler_loop_drag=button==MOUSE_BUTTON_RIGHT;
 191:     if(ui.ruler_loop_drag) {
 192:         ui.ruler_anchor=range[1]>range[0]?(at<(range[0]+range[1])/2?range[1]:range[0]):at;
 193:         range[0]=fminf(ui.ruler_anchor,at); range[1]=fmaxf(ui.ruler_anchor,at);
 194:         if(range[1]>range[0]) { if(id==1) ui.playlist_start=range[0]/STEPS; else ui.piano_start[ui.pattern]=range[0]; }
 195:     } else if(id==1) ui.playlist_start=at/STEPS; else ui.piano_start[ui.pattern]=at;
 196:     if(!ui.playing) ui.reset=1;
```

### timeline_ruler

```c
 200:     float *range=id==1?ui.song_loop:ui.pattern_loop[ui.pattern],point=id==1?ui.playlist_start*STEPS:ui.piano_start[ui.pattern];
 202:     DrawRectangle(gx,y,width,14,ui_theme.browser);
 203:     if(range[1]>range[0]) {
 204:         float left=fmaxf(gx,gx+(range[0]-start)*pixels),right=fminf(gx+width,gx+(range[1]-start)*pixels);
 205:         if(right>left) DrawRectangleRec((Rectangle){left,y,right-left,14},Fade(ui_theme.loop,.6f));
 208:     float label_width=text_width(TextFormat("%.0f",fmaxf(0,start+span)/STEPS+1),11)+8;
 209:     while(spacing*pixels<label_width && spacing<1e30f) spacing*=2;
 210:     for(int i=0;i<width/(spacing*pixels)+2;i++) {
 212:         if(step>=0 && x>=gx && x<gx+width) {
 213:             const char *number=TextFormat("%.0f",step/STEPS+1);
 214:             int major=fmodf(step,STEPS*4)==0,size=major?11:10;
 215:             if(x+3+text_width(number,size)<=gx+width) label(number,x+3,y+1,size,major?ink:muted);
 218:     if(hover(gx,y,width,14)) {
 220:         int right=IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
 221:         if(right || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
 222:             float at=fmaxf(0,snap_round(start+(ui.mouse.x-gx)/pixels,q));
 224:             ui.input_enabled=0;
 228:     if(x>=gx && x<=gx+width) DrawTriangle((Vector2){x-5,y+1},(Vector2){x,y+12},(Vector2){x+5,y+1},ui.marker_drag==id?ink:ui_theme.signal);
```

### timeline_grid

```c
 234:     /* Keep phrase shading anchored to bar 1, including while panning between bars. */
 235:     float band=STEPS*4;
 236:     if(grid.band_alpha>0) for(int i=0;i<span/band+2;i++) {
 238:         if(step<0 || fmodf(step/band,2)==0) continue;
 243:     for(int i=0;i<span/grid.lines+2;i++) {
 245:         int bar=fmodf(step,STEPS)==0,beat=fmodf(step,4)==0;
 246:         if(step>=0 && x>=gx && x<=gx+width)
 247:             DrawLineEx((Vector2){x,y},(Vector2){x,y+height},1,bar?ui_theme.grid_major:Fade(ui_theme.grid_minor,beat?1:.65f));
```

### follow_view

```c
 252:     if(!ui.follow_playhead || !ui.playing || ui.navigation_active || captured() || ui.windows.grab>=0) return;
 253:     *start=position-span*.5;
```

### track_activity_update

```c
 257:     audio_pattern_activity(ui.pattern,ui.playing_notes); memset(ui.playing_keys,0,sizeof ui.playing_keys);
 258:     for(int c=0;c<ui.project.channel_count;c++) for(int n=0;n<NOTES;n++) if(ui.playing_notes[c][n] && ui.project.notes[ui.pattern][c][n].velocity) ui.playing_keys[c][ui.project.notes[ui.pattern][c][n].pitch]=1;
 259:     uint8_t channel_triggered[CHANNELS]; audio_channel_activity(ui.channel_active_ui,channel_triggered);
 260:     ui.channel_decay_active=0;
 261:     for(int c=0;c<CHANNELS;c++) {
 262:         ui.channel_flash[c]=channel_triggered[c]?1:ui.channel_flash[c]*expf(-GetFrameTime()*20);
 263:         ui.channel_decay_active|=ui.channel_active_ui[c] || ui.channel_flash[c]>.005f;
 265:     uint8_t triggered[LANES]; audio_track_activity(ui.track_active_ui,triggered);
 266:     float decay=expf(-GetFrameTime()*20);
 267:     for(int l=0;l<LANES;l++) {
 268:         if(!ui.playing || !ui.song) { ui.track_flash[l]=0; ui.track_active_ui[l]=0; }
 269:         else ui.track_flash[l]=triggered[l]?1:ui.track_flash[l]*decay;
```

### clip_edge

```c
 274:     if(clip<0) return 0;
 276:     return fminf(left,right)<7?(left<right?-1:1):0;
```

### picker_double_click

```c
 280:     static double last_click=-1;
 281:     static int last_source=-1;
 283:     static Vector2 position;
 285:     int twice=epoch==ui.sample_epoch && source==last_source && now-last_click<.35 &&
 286:         fabsf(ui.mouse.x-position.x)<5 && fabsf(ui.mouse.y-position.y)<5;
 287:     last_click=twice?-1:now; last_source=source; position=ui.mouse; epoch=ui.sample_epoch;
 289:         if(source<PATTERNS) { ui.rack_filter=0; ui.rack_scroll=0; windows_focus(&ui.windows,0); }
 290:         else if(AUDIO_SOURCE(source)) { ui.channel=ui.instrument_channel=source-PATTERNS; windows_focus(&ui.windows,4); }
 292:         ui.picker_drag=-1; ui.browser_focus=0; ui.input_enabled=0;
```

### playlist

```c
 298:     static double last_click=-1;
 299:     static int last_lane=-1,last_clip=-1;
 300:     static Vector2 last_position;
 301:     Rect rect=ui.windows.editors[1].rect;
 302:     int gx=212,gy=PLAYLIST_GRID_TOP; float gridw=width-gx-24,track_area=height-gy-PLAYLIST_BOTTOM,total_height=track_position(LANES);
 303:     ui.track_scroll=fmaxf(0,fminf(total_height-track_area,ui.track_scroll));
 306:     ui.arrangement.range=fmaxf(ui.arrangement.range,fmaxf(ui.arrangement.view_start+span*2,song_steps(&ui.project)/STEPS+span));
 309:     int divider=-1;
 310:     if(hover(120,gy,gx-120,track_area)) {
 311:         float pos=ui.track_scroll+ui.mouse.y-gy; int lane=fminf(LANES-1,fmaxf(0,track_at(pos)));
 312:         if(pos-track_position(lane)<3 && lane>0) divider=lane-1;
 313:         else if(track_position(lane+1)-pos<3) divider=lane;
 314:         if(divider>=0) {
 315:             SetMouseCursor(MOUSE_CURSOR_RESIZE_NS); snprintf(ui.status,sizeof ui.status,"Drag divider to resize Track %d",divider+1);
 316:             if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.track_resize=divider; ui.track_resize_y=ui.mouse.y+rect.y; ui.track_resize_height=track_height(divider); ui.input_enabled=0; }
 319:     if(divider<0 && hover(gx,gy,gridw,track_area)) {
 321:         if(ui.arrangement.tool!=CUT && !shortcut_down() && hit>=0 && !recording_source(ui.project.clips[lane][hit]-1) && clip_edge(lane,hit,pointer_bar,barw)) SetMouseCursor(MOUSE_CURSOR_RESIZE_EW);
 323:     if(ui.arrangement.gesture==SIZE_CLIP || ui.arrangement.gesture==STRETCH_CLIP) SetMouseCursor(MOUSE_CURSOR_RESIZE_EW);
 324:     if(ui.track_resize>=0) SetMouseCursor(MOUSE_CURSOR_RESIZE_NS);
 326:     for(int i=0;i<5;i++) if(tool_button(i,ui.arrangement.tool,8+i*24,tips[i])) ui.arrangement.tool=i;
 330:     for(int t=0;t<3;t++) {
 331:         if(button("",4+t*38,57,36,23,ui.picker_tab==t)) { ui.picker_tab=t; ui.picker_scroll=0; ui.picker_drag=-1; if(t==0) { ui.arrangement.source_pattern=ui.pattern; ui.arrangement.source_steps=ui.project.pattern_steps[ui.pattern]; ui.arrangement.source_offset=0; } }
 332:         icon(picker_icons[t],22+t*38,68,18,ui.picker_tab==t?ui_theme.selected_text:ink);
 333:         if(hover(4+t*38,57,36,23)) snprintf(ui.status,sizeof ui.status,"%s%s",picker_names[t],"");
 335:     label("Tracks",128,63,12,muted);
 336:     int audio_ids[CHANNELS],audio_count=0;
 337:     for(int c=0;c<ui.project.channel_count;c++) if(ui.project.channel_audio[c]) audio_ids[audio_count++]=c;
 338:     int picker_count=ui.picker_tab==0?ui.project.pattern_count:ui.picker_tab==1?audio_count:ui.project.automation_count;
 339:     int picker_visible=fmaxf(1,(height-66-gy)/52);
 340:     if(hover(4,gy,112,height-66-gy)) ui.picker_scroll-=GetMouseWheelMove();
 341:     ui.picker_scroll=fmaxf(0,fminf(fmaxf(0,picker_count-picker_visible),ui.picker_scroll));
 343:         int p=ui.picker_tab==1?audio_ids[row]:row;
 344:         int y=gy+(row-ui.picker_scroll)*52; if(y+50>height-66) break;
 345:         if(ui.picker_tab==2) {
 348:                 ui.automation_selected=p; ui.arrangement.source_pattern=source; ui.arrangement.source_steps=ui.project.automations[p].steps; ui.arrangement.source_offset=0; ui.picker_drag=source;
 349:                 ui.picker_origin=(Vector2){ui.mouse.x+rect.x,ui.mouse.y+rect.y}; ui.picker_offset=(Vector2){ui.mouse.x-4,ui.mouse.y-y};
 351:             label(fit_text(source_name(source),100,12),8,y+4,12,clip_foreground(source_color(source)));
 352:             automation_curve(p,8,y,104/ui.project.automations[p].steps,50,8,112,0);
 353:             if(hover(4,y,112,50)) {
 355:                 if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) open_context(11,p,(Vector2){ui.mouse.x+rect.x,ui.mouse.y+rect.y});
 359:         if(ui.picker_tab==1) {
 361:                 ui.arrangement.source_pattern=PATTERNS+p; ui.arrangement.source_steps=ui.project.audio_seconds[p]; ui.arrangement.source_offset=0; ui.picker_drag=PATTERNS+p;
 362:                 ui.picker_origin=(Vector2){ui.mouse.x+rect.x,ui.mouse.y+rect.y};
 363:                 ui.picker_offset=(Vector2){ui.mouse.x-4,ui.mouse.y-y};
 366:             if(ui.arrangement.source_pattern==PATTERNS+p) DrawRectangleLinesEx((Rectangle){4,y,112,50},2,WHITE);
 367:             label(fit_text(ui.project.channel_names[p],100,12),8,y+4,12,clip_foreground(audio_color(p)));
 368:             audio_waveform(p,8,y+20,8,112,104/fmaxf(.001f,audio_view_steps(p)),28);
 369:             if(hover(4,y,112,50)) {
 371:                 if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) open_context(7,p,(Vector2){ui.mouse.x+rect.x,ui.mouse.y+rect.y});
 376:             if(ui.pattern!=p) ui.reset=1;
 377:             ui.pattern=p; ui.arrangement.source_pattern=p; ui.arrangement.source_steps=ui.project.pattern_steps[p]; ui.arrangement.source_offset=0; ui.picker_drag=p;
 378:             ui.picker_origin=(Vector2){ui.mouse.x+rect.x,ui.mouse.y+rect.y};
 379:             ui.picker_offset=(Vector2){ui.mouse.x-4,ui.mouse.y-y};
 382:         if(ui.pattern==p) DrawRectangleLinesEx((Rectangle){4,y,112,50},2,WHITE);
 383:         label(fit_text(ui.project.pattern_names[p],100,12),8,y+4,12,clip_foreground(pattern_color(p)));
 384:         picker_notes(p,4,y);
 385:         if(hover(4,y,112,50)) {
 387:             if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { ui.pattern=p; ui.arrangement.source_pattern=p; ui.arrangement.source_steps=ui.project.pattern_steps[p]; ui.reset=1; open_context(4,p,(Vector2){ui.mouse.x+rect.x,ui.mouse.y+rect.y}); }
 390:     if(ui.sample_drag[0] && ui.sample_moved && CheckCollisionPointRec(ui.mouse,(Rectangle){4,gy,112,height-gy-66}) && windows_hit(&ui.windows,ui.mouse.x+rect.x,ui.mouse.y+rect.y)==1) {
 391:         DrawRectangleLinesEx((Rectangle){4,gy,112,height-gy-66},2,ui_theme.signal);
 394:     if(ui.picker_tab==2 && !ui.project.automation_count) { label("No automation",8,gy+8,12,muted); label("Right-click a control",8,gy+28,11,muted); }
 395:     if(ui.picker_tab==1 && !audio_count) { label("No audio clips",8,gy+8,12,muted); label("Drop on Playlist",8,gy+28,11,muted); }
 396:     if(ui.picker_tab==0 && button("+",4,height-62,112,20,0)) {
 398:             ui.pattern=ui.project.pattern_count++; ui.arrangement.source_pattern=ui.pattern; ui.arrangement.source_steps=STEPS; memset(ui.project.notes[ui.pattern],0,sizeof ui.project.notes[ui.pattern]);
 399:             ui.project.pattern_steps[ui.pattern]=STEPS; memset(ui.piano_channels[ui.pattern],0,sizeof ui.piano_channels[ui.pattern]);
 400:             for(int n=1;n<=PATTERNS+1;n++) {
 402:                 int duplicate=0;
 403:                 for(int i=0;i<ui.pattern;i++) if(!strcmp(ui.project.pattern_names[i],ui.project.pattern_names[ui.pattern])) duplicate=1;
 407:             ui.rack_view[ui.pattern]=ui.rack_range[ui.pattern]=0; ui.reset=1;
 408:             ui.picker_scroll=fmaxf(0,ui.project.pattern_count-picker_visible);
 411:     if(ui.picker_tab==0 && hover(4,height-62,112,20) && ui.project.pattern_count<PATTERNS) snprintf(ui.status,sizeof ui.status,"Add a new blank pattern");
 412:     timeline_ruler(1,ui.arrangement.view_start*STEPS,span*STEPS,gx,gy-14,gridw,ui.arrangement.snap);
 413:     for(int l=fmaxf(0,track_at(ui.track_scroll));l<LANES;l++) {
 417:         DrawRectangleRec((Rectangle){120,y,gx-122,rowh-1},cell); label(fit_text(ui.project.track_names[l],gx-142,12),128,y+fmaxf(1,(rowh-12)/2),12,ink);
 418:         float brightness=fmaxf(ui.track_active_ui[l]?.32f:0,ui.track_flash[l]);
 419:         Rectangle activity={gx-9,y+1,8,rowh-2};
 421:         if(brightness>.005f) DrawRectangleRec(activity,Fade(WHITE,brightness));
 422:         if(ui.selected_track==l) DrawRectangleLinesEx(activity,1,accent);
 423:         mute_light(gx-18,y+rowh-10,ui.project.lane_mute,LANES,l,TextFormat("Playlist track %d",l+1));
 426:         timeline_grid(ui.arrangement.view_start*STEPS,span*STEPS,gx,y,gridw,rowh-1);
 427:         for(int b=0;b<CLIPS;b++) if(ui.project.clips[l][b]) {
 428:             int pat=ui.project.clips[l][b]-1; float x=gx+(ui.project.clip_starts[l][b]-ui.arrangement.view_start)*barw,w=playlist_clip_length(l,b)*barw/STEPS;
 429:             if(w<=0 || x>gx+gridw || x+w<gx) continue;
 431:             Rectangle bounds={left,y,fmaxf(1,right-left),rowh-1};
 432:             float top=fmaxf(gy,y),bottom=fminf(height-PLAYLIST_BOTTOM,y+rowh-1);
 434:             const char *name=source_name(pat); if(pat<PATTERNS && !strcmp(name,TextFormat("Pattern %d",pat+1))) name=TextFormat("P%d",pat+1);
 435:             if(x>=gx) label(fit_text(name,fminf(gridw,w-9),10),x+3,y+2,10,clip_foreground(source_color(pat)));
 440:             if(clip_scissor) BeginScissorMode(clip_left,clip_top,ceilf((rect.x+left+bounds.width)*scale)-clip_left,fmaxf(0,ceilf((rect.y+bottom)*scale)-clip_top));
 442:             if(pat>=AUTOMATION_SOURCE) automation_curve(pat-AUTOMATION_SOURCE,origin,y,barw/STEPS,rowh,left,right,1);
 446:             int selected=ui.arrangement.gesture==MOVE_CLIPS?ui.arrangement.moved[l][b]:ui.arrangement.selected[l][b];
 447:             DrawRectangleLinesEx(bounds,selected?2:1,selected?WHITE:clip_foreground(source_color(pat)));
 452:     for(int l=fmaxf(0,track_at(ui.track_scroll));l<LANES;l++) {
 456:         DrawRectangleRec((Rectangle){gx,(bottom-1-ui.text_origin.y)/scale,gridw,1/scale},ui_theme.grid_major);
 457:         BeginScissorMode((rect.x+120)*scale,(rect.y+gy)*scale,(gx-120)*scale,track_area*scale);
 458:         DrawRectangleRec((Rectangle){120,(bottom-1-ui.text_origin.y)/scale,gx-120,1/scale},divider==l || ui.track_resize==l?accent:ui_theme.border);
 462:     if(ui.picker_drag>=0 && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
 463:         if(CheckCollisionPointRec(ui.mouse,(Rectangle){gx,gy,gridw,track_area}) && windows_hit(&ui.windows,ui.mouse.x+rect.x,ui.mouse.y+rect.y)==1) {
 465:             arrangement_place(&ui.project,track_at(ui.track_scroll+ui.mouse.y-gy),start,ui.picker_drag,clip_source_steps(&ui.project,ui.picker_drag));
 467:         ui.picker_drag=-1;
 469:     /* Curve editing owns the body; the name strip keeps normal clip gestures. */
 470:     if(ui.automation_node>=0) {
 471:         if(ui.automation_selected<0 || ui.automation_selected>=ui.project.automation_count || ui.automation_lane<0 || ui.automation_clip<0 || ui.project.clips[ui.automation_lane][ui.automation_clip]!=AUTOMATION_SOURCE+ui.automation_selected+1) ui.automation_node=-1;
 476:             if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
 482:                 float value=fmaxf(0,fminf(1,1-(ui.mouse.y-y-19)/fmaxf(1,rowh-25)));
 484:                 if(moved>=0) {
 491:             } else ui.automation_node=-1;
 494:     if(ui.automation_node<0 && ui.arrangement.tool==PENCIL && !shortcut_down() && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT) && hover(gx,gy,gridw,track_area)) {
 497:         if(slot>=0 && ui.project.clips[lane][slot]>AUTOMATION_SOURCE && ui.mouse.y>=y+15) {
 498:             int index=ui.project.clips[lane][slot]-AUTOMATION_SOURCE-1; Automation *a=&ui.project.automations[index];
 500:             int hit=-1; float nearest=49;
 501:             for(int n=0;n<a->count;n++) {
 502:                 float dx=ui.mouse.x-origin-a->points[n].step*barw/STEPS,dy=ui.mouse.y-y-19-(1-a->points[n].value)*fmaxf(1,rowh-25),distance=dx*dx+dy*dy;
 506:             if(hit>=0 || !clip_edge(lane,slot,pointer_bar,barw)) {
 507:                 if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
 509:                     if(hit<0) {
 512:                         hit=automation_point(a,fmaxf(0,fminf(a->steps,step)),fmaxf(0,fminf(1,1-(ui.mouse.y-y-19)/fmaxf(1,rowh-25))));
 515:                     if(hit>=0) ui.automation_grab_x=ui.mouse.x-origin-a->points[hit].step*barw/STEPS;
 516:                     memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected); ui.arrangement.selected[lane][slot]=1; ui.input_enabled=0;
 518:                 if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
 519:                     if(hit>0 && hit<a->count-1) { memmove(&a->points[hit],&a->points[hit+1],(a->count-hit-1)*sizeof a->points[0]); a->count--; }
 520:                     ui.input_enabled=0;
 525:     if(divider<0 && hover(120,gy,gx+gridw-120,track_area)) {
 526:         float bx=ui.mouse.x<gx?-1:ui.arrangement.view_start+(ui.mouse.x-gx)/barw,ly=track_at(ui.track_scroll+ui.mouse.y-gy); int hit=arrangement_hit(&ui.project,(int)ly,bx);
 527:         if(bx<0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.selected_track=(int)ly;
 528:         int live=hit>=0 && recording_source(ui.project.clips[(int)ly][hit]-1);
 529:         int edge=shortcut_down() || live || ui.arrangement.tool==CUT?0:clip_edge((int)ly,hit,bx,barw);
 530:         if(edge) { SetMouseCursor(MOUSE_CURSOR_RESIZE_EW); snprintf(ui.status,sizeof ui.status,"%.100s · %.2f s | Drag edge to resize",source_name(ui.project.clips[(int)ly][hit]-1),playlist_clip_length((int)ly,hit)*15/ui.project.bpm); }
 531:         else if(ui.mouse.x<gx) snprintf(ui.status,sizeof ui.status,"Track %d: drag to select tracks; right-click to rename",(int)ly+1);
 532:         else if(hit>=0) {
 533:             int source=ui.project.clips[(int)ly][hit]-1;
 534:             snprintf(ui.status,sizeof ui.status,"%.100s · %.2f s | %s",source_name(source),playlist_clip_length((int)ly,hit)*15/ui.project.bpm,source>=AUTOMATION_SOURCE?"Drag title to move; body to edit":AUDIO_SOURCE(source)?"Double-click: Sampler; drag: move; right-click: erase":"Double-click: Channel Rack; drag: move; right-click: erase");
 539:         if(ui.mouse.x<gx && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
 540:             ui.rename_channel=ui.rename_mixer=-1; ui.rename_track=(int)ly; snprintf(ui.rename_text,sizeof ui.rename_text,"%s",ui.project.track_names[ui.rename_track]); ui.rename_select_all=1; open_popup(2); ui.input_enabled=0; return;
 542:         if(!live && ui.mouse.x>=gx && hit>=0 && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) &&
 543:            ui.mouse.y<gy+track_position((int)ly)-ui.track_scroll+14) {
 544:             int source=ui.project.clips[(int)ly][hit]-1;
 545:             if(source<PATTERNS) { ui.pattern=source; open_context(4,source,(Vector2){ui.mouse.x+rect.x,ui.mouse.y+rect.y}); }
 546:             else if(AUDIO_SOURCE(source)) open_context(7,source-PATTERNS,(Vector2){ui.mouse.x+rect.x,ui.mouse.y+rect.y});
 547:             else open_context(11,source-AUTOMATION_SOURCE,(Vector2){ui.mouse.x+rect.x,ui.mouse.y+rect.y});
 548:             ui.arrangement.gesture=IDLE; last_click=-1; return;
 550:         if(shortcut_down() && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
 551:             arrangement_select_press(&ui.arrangement,bx,ly,IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
 552:             last_click=-1; ui.input_enabled=0;
 553:         } else if(!live && !recording_source(ui.arrangement.source_pattern) && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))) {
 554:             int plain_left=ui.arrangement.tool!=CUT && ui.arrangement.tool!=STRETCH && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT) && hit>=0 && (!edge || ui.project.clips[(int)ly][hit]>PATTERNS);
 556:             if(plain_left && last_lane==(int)ly && last_clip==hit && now-last_click<.35 && fabsf(ui.mouse.x-last_position.x)<5 && fabsf(ui.mouse.y-last_position.y)<5) {
 557:                 int source=ui.project.clips[(int)ly][hit]-1;
 558:                 if(source<PATTERNS) { ui.pattern=source; ui.reset=1; }
 559:                 else if(AUDIO_SOURCE(source)) { ui.channel=ui.instrument_channel=source-PATTERNS; ui.rack_filter=1; ui.rack_scroll=0; }
 561:                 ui.arrangement.gesture=IDLE; windows_focus(&ui.windows,source>=AUTOMATION_SOURCE?1:AUDIO_SOURCE(source)?4:0); last_click=-1;
 562:                 ui.input_enabled=0; return;
 564:             last_click=plain_left?now:-1; last_lane=(int)ly; last_clip=hit; last_position=ui.mouse;
 565:             arrangement_press(&ui.arrangement,&ui.project,bx,ly,IsMouseButtonPressed(MOUSE_BUTTON_RIGHT),edge,ui.pattern,IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
 566:             if(ui.arrangement.source_pattern>=0 && ui.arrangement.source_pattern<PATTERNS && ui.arrangement.source_pattern!=ui.pattern) { ui.pattern=ui.arrangement.source_pattern; ui.reset=1; }
 567:             ui.input_enabled=0;
 571:     if(ui.arrangement.tool==CUT && !shortcut_down() && !IsKeyDown(KEY_LEFT_SHIFT) && !IsKeyDown(KEY_RIGHT_SHIFT) && hover(gx,gy,gridw,track_area)) {
 573:         if(hit>=0 && !recording_source(ui.project.clips[lane][hit]-1)) {
 574:             float cut=snap_floor(pointer_bar*STEPS+.00001f,ui.arrangement.snap);
 576:             if(cut>start+.0001f && cut<end-.0001f) {
 579:                 DrawRectangleRec((Rectangle){x,y,1/scale,track_height(lane)-1},ui_theme.signal);
 583:     if(ui.picker_drag>=0 && CheckCollisionPointRec(ui.mouse,(Rectangle){gx,gy,gridw,track_area}) && windows_hit(&ui.windows,ui.mouse.x+rect.x,ui.mouse.y+rect.y)==1) {
 586:         Rectangle ghost={gx+(start-ui.arrangement.view_start)*barw,gy+track_position(lane)-ui.track_scroll,fmaxf(4,clip_source_steps(&ui.project,ui.picker_drag)*barw/STEPS),track_height(lane)-1};
 587:         DrawRectangleRec(ghost,Fade(ui_theme.signal,.2f)); DrawRectangleLinesEx(ghost,1,ui_theme.signal);
 588:         snprintf(ui.status,sizeof ui.status,"Release to place %s",source_name(ui.picker_drag));
 590:     if(ui.sample_drag[0] && ui.sample_moved && ui.mouse.x>=gx && ui.mouse.x<gx+gridw && ui.mouse.y>=gy && ui.mouse.y<height-PLAYLIST_BOTTOM && windows_hit(&ui.windows,ui.mouse.x+rect.x,ui.mouse.y+rect.y)==1) {
 592:         float length=ui.audition.frames/(float)RATE*ui.project.bpm/15;
 593:         int lane=fminf(LANES-1,fmaxf(0,track_at(ui.track_scroll+ui.mouse.y-gy)));
 594:         Rectangle ghost={gx+(start-ui.arrangement.view_start)*barw,gy+track_position(lane)-ui.track_scroll,fmaxf(4,length*barw/STEPS),track_height(lane)-1};
 596:         if(hit>=0 && AUDIO_SOURCE(ui.project.clips[lane][hit]-1)) {
 598:             ghost.width=fmaxf(4,playlist_clip_length(lane,hit)*barw/STEPS);
 599:             snprintf(ui.status,sizeof ui.status,"Release to replace %s's sample",source_name(ui.project.clips[lane][hit]-1));
 600:             DrawRectangleLinesEx(ghost,1,ui_theme.signal);
 602:             DrawRectangleRec(ghost,Fade(ui_theme.signal,.2f)); DrawRectangleLinesEx(ghost,1,ui_theme.signal);
 605:     if(ui.arrangement.gesture==BOX_SELECT) {
 608:         DrawRectangleRec(box,Fade(ui_theme.signal,.15f)); DrawRectangleLinesEx(box,1,ui_theme.signal);
 611:     if(ui.playing && ui.song) { float x=gx+(ui.visual_step/STEPS-ui.arrangement.view_start)*barw; if(x>=gx && x<=gx+gridw) DrawLineEx((Vector2){x,gy},(Vector2){x,height-PLAYLIST_BOTTOM},1.5f,ui_theme.signal); }
 612:     float rail_top=gy-14,rail_height=track_area+14;
 613:     float vthumb=timeline_thumb(rail_height,track_area,total_height),travel_y=rail_height-vthumb,maximum_y=fmaxf(0,total_height-track_area),vy=rail_top+(maximum_y>0?ui.track_scroll/maximum_y*travel_y:0);
 614:     DrawRectangleRec((Rectangle){width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,rail_height},bg); ui_surface((Rectangle){width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,vy,EDITOR_SCROLLBAR,fmaxf(12,vthumb)},ui.playlist_vpan || hover(width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,rail_height)?accent:muted);
 615:     if(hover(width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,rail_height)) snprintf(ui.status,sizeof ui.status,"Drag to scroll through the 100 Playlist tracks");
 616:     if(hover(width-4-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_top,EDITOR_SCROLLBAR,rail_height) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
 617:         if(ui.mouse.y<vy || ui.mouse.y>vy+vthumb) ui.track_scroll=fmaxf(0,fminf(maximum_y,(ui.mouse.y-rail_top-vthumb/2)/fmaxf(1,travel_y)*maximum_y));
 618:         ui.playlist_vpan=1; ui.playlist_vpan_y=ui.mouse.y+rect.y; ui.playlist_vpan_start=ui.track_scroll;
 620:     float rail_width=width-4-EDITOR_CORNER-gx,thumbw=timeline_thumb(rail_width,span,ui.arrangement.range),travel=rail_width-thumbw,maximum=ui.arrangement.range-span,thumbx=gx+(maximum>0?fmaxf(0,ui.arrangement.view_start)/maximum*travel:0);
 621:     DrawRectangleRec((Rectangle){gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR},bg); ui_surface((Rectangle){thumbx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,thumbw,EDITOR_SCROLLBAR},ui.playlist_pan || hover(gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR)?accent:muted);
 622:     if(hover(gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR)) snprintf(ui.status,sizeof ui.status,"Drag to scroll along the Playlist timeline");
 623:     if(hover(gx,gy-14-(EDITOR_CORNER+EDITOR_SCROLLBAR)/2,rail_width,EDITOR_SCROLLBAR) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
 624:         if(ui.mouse.x<thumbx || ui.mouse.x>thumbx+thumbw) ui.arrangement.view_start=fmaxf(0,fminf(maximum,(ui.mouse.x-gx-thumbw/2)/fmaxf(1,travel)*maximum));
 625:         ui.playlist_pan=1; ui.playlist_pan_x=ui.mouse.x+rect.x; ui.playlist_pan_start=ui.arrangement.view_start; ui.playlist_pan_range=maximum/fmaxf(1,travel);
 627:     row_zoom_button(1,width,gy-14-EDITOR_CORNER);
```

## ui_rack.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### rack_channels

```c
   5:     int count=0;
   6:     for(int c=0;c<ui.project.channel_count;c++) if(!ui.rack_filter || ui.project.channel_audio[c]==(ui.rack_filter==1)) { if(rows) rows[count]=c; count++; }
```

### rack_channel_at

```c
  10: int rack_channel_at(int row) { int rows[CHANNELS],count=rack_channels(rows); return row>=0 && row<count?rows[row]:-1; }
```

### channel_route

```c
  13:     if(drag_button(TextFormat("%d",ui.project.route[c]),x,y,24,22,ui.route_drag==c)) {
  14:         ui.route_drag=c; ui.route_start=ui.project.route[c]; ui.route_y=ui.mouse.y+ui.windows.editors[ui.knob_context].rect.y;
  15:         ui.channel=c; ui.mixer_selected=ui.project.route[c]; ui.input_enabled=0;
  17:     if(hover(x,y,24,22)) snprintf(ui.status,sizeof ui.status,"%s mixer destination: drag up/down; 0 = Master",ui.project.channel_names[c]);
```

### channel_controls

```c
  21:     int x=width-276;
  22:     mute_light(x,36,ui.project.mute,ui.project.channel_count,c,ui.project.channel_names[c]);
  23:     knob_style(x+28,36,&ui.project.pan[c],-1,1,0,"Channel pan",KNOB_PAN);
  24:     knob_style(x+58,36,&ui.project.volume[c],0,VOLUME_KNOB_MAX,1,"Channel volume",KNOB_VOLUME);
  25:     knob_style(x+94,36,&ui.project.channel_pitch[c],-1,1,0,"Playback pitch",KNOB_CENTER);
  26:     if(hover(x+83,25,22,22) || ui.control_drag==&ui.project.channel_pitch[c]) snprintf(ui.status,sizeof ui.status,"Playback pitch: %+.2f semitones | range ±%.0f semitones",ui.project.channel_pitch[c]*ui.project.pitch_range[c],ui.project.pitch_range[c]);
  28:     if(drag_button(TextFormat("%.0f",*range),x+150,25,30,22,ui.control_drag==range)) { capture_control(range,1,48,0); ui.control_integer=1; }
  29:     if(hover(x+150,25,30,22)) {
  30:         *range=fmaxf(1,fminf(48,*range+roundf(GetMouseWheelMove())));
  31:         if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { control_menu(range,1,48,2,"Pitch range (semitones)"); }
  32:         snprintf(ui.status,sizeof ui.status,"Pitch range: %.0f semitones | drag up/down, 1–48 whole semitones; right-click to enter",*range);
  34:     channel_route(c,x+205,25);
  35:     label("PAN",x+18,53,10,muted); label("VOL",x+48,53,10,muted);
  36:     label("PITCH",x+80,53,10,muted); label("RANGE",x+150,53,10,muted); label("ROUTE",x+196,53,10,muted);
```

### pattern_playback_position

```c
  40:     if(!ui.playing) return -1;
  42:     double latest=-1,position=-1; int solo=solo_any(ui.project.lane_mute,LANES);
  43:     for(int l=0;l<LANES;l++) if(!(ui.project.lane_mute[l]&1) && (!solo || (ui.project.lane_mute[l]&2)))
  44:         for(int b=0;b<CLIPS;b++) if(ui.project.clips[l][b]==ui.pattern+1) {
  46:             if(start>latest && local>=0 && local<clip_length(&ui.project,l,b) && local+ui.project.clip_offsets[l][b]<ui.project.pattern_steps[ui.pattern]) { latest=start; position=local+ui.project.clip_offsets[l][b]; }
```

### rack_melodic

```c
  60:     if(ui.piano_channels[ui.pattern][c]) return 1;
  61:     for(int i=0;i<NOTES;i++) { Note n=ui.project.notes[ui.pattern][c][i]; if(n.velocity && (n.pitch!=60 || n.length>0)) return 1; }
  62:     return 0;
```

### paint_rack

```c
  66:     int visible=fmaxf(1,(ui.windows.editors[0].rect.h-RACK_TOP-48)/28);
  67:     if(x<200 || x>=width-26 || y<RACK_TOP || y>=RACK_TOP+visible*28) { ui.rack_paint_channel=-1; return; }
  68:     int c=rack_channel_at(ui.rack_scroll+(y-RACK_TOP)/28); float step=floorf(ui.rack_view[ui.pattern]+(x-200)/RACK_STEP_WIDTH);
  69:     if(c<0 || c>=ui.project.channel_count || rack_melodic(c)) { ui.rack_paint_channel=-1; return; }
  71:     for(int i=0;i<=fabsf(step-from) && i<NOTES;i++) {
  73:         Note *n=note_at(&ui.project,ui.pattern,c,start,60);
  74:         if(ui.rack_paint==MOUSE_BUTTON_RIGHT) { if(n) n->velocity=0; }
  75:         else if(!n) { pattern_extend(start+1); note_add(&ui.project,ui.pattern,c,start,60,0); }
```

### rack

```c
  81:     float stepw=RACK_STEP_WIDTH,gridw=width-226,view=ui.rack_view[ui.pattern],span=gridw/stepw;
  82:     ui.rack_range[ui.pattern]=fmaxf(ui.rack_range[ui.pattern],fmaxf(view+span*2,ui.project.pattern_steps[ui.pattern]+span));
  83:     timeline_ruler(0,view,span,200,TITLE,gridw,1);
  84:     int visible=fmaxf(1,(height-RACK_TOP-48)/28),rows[CHANNELS],count=rack_channels(rows);
  85:     if(hover(0,RACK_TOP,width,height-RACK_TOP-18)) ui.rack_scroll-=GetMouseWheelMove();
  86:     ui.rack_scroll=fmaxf(0,fminf(fmaxf(0,count-visible),ui.rack_scroll));
  88:         int c=rows[index],row=RACK_TOP+(index-ui.rack_scroll)*28;
  89:         mute_light(12,row+11,ui.project.mute,ui.project.channel_count,c,ui.project.channel_names[c]);
  90:         knob_style(36,row+11,&ui.project.pan[c],-1,1,0,"Channel pan",KNOB_PAN); knob_style(62,row+11,&ui.project.volume[c],0,VOLUME_KNOB_MAX,1,"Channel volume",KNOB_VOLUME);
  91:         channel_route(c,78,row);
  92:         if(button_color(fit_text(channel_caption(c),68,13),106,row,78,22,0,ui.project.channel_audio[c]?audio_color(c):cell)) {
  93:             ui.channel=ui.instrument_channel=c; windows_focus(&ui.windows,4);
  95:         if(hover(106,row,78,22) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { ui.channel=c; open_context(2,c,(Vector2){ui.mouse.x+ui.windows.editors[0].rect.x,ui.mouse.y+ui.windows.editors[0].rect.y}); }
  96:         if(hover(106,row,78,22)) snprintf(ui.status,sizeof ui.status,"%s: open instrument; right-click for Piano Roll, rename, mute or delete; Keys: Z/Q white, S/2 black",ui.project.channel_names[c]);
  97:         if(ui.sample_drag[0] && ui.sample_moved && ui.mouse.y>=row && ui.mouse.y<row+28 && ui.mouse.x>=0 && ui.mouse.x<width && windows_hit(&ui.windows,ui.mouse.x+ui.windows.editors[0].rect.x,ui.mouse.y+ui.windows.editors[0].rect.y)==0) {
  98:             DrawRectangleLinesEx((Rectangle){106,row,78,22},1,ui_theme.signal);
 101:         float brightness=fmaxf(ui.channel_active_ui[c]?.32f:0,ui.channel_flash[c]);
 102:         DrawRectangle(187,row,3,22,Fade(WHITE,.08f+brightness*.92f));
 105:             DrawRectangle(200,row+2,gridw,20,bg);
 106:             float active=fmaxf(0,fminf(gridw,(edit_steps()-view)*stepw));
 107:             if(active<gridw) DrawRectangle(200+active,row+2,gridw-active,20,ui_theme.disabled);
 108:             for(int i=0;i<NOTES;i++) {
 109:                 Note n=ui.project.notes[ui.pattern][c][i]; float start=200+(n.start-view)*stepw,end=start+fmaxf(2,(n.length?n.length:1)*stepw-2);
 110:                 if(n.velocity && end>=200 && start<200+gridw) DrawRectangle(fmaxf(200,start),row+3+fmaxf(0,fminf(16,(72-(int)n.pitch)*.65f)),fminf(200+gridw,end)-fmaxf(200,start),2,ui.playing_notes[c][i]?ink:accent);
 112:             if(hover(200,row+2,gridw,20)) snprintf(ui.status,sizeof ui.status,"%s notes: click to open Piano Roll; right-click for channel options",ui.project.channel_names[c]);
 113:             if(hover(200,row+2,gridw,20)) {
 115:                 if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { ui.channel=ui.piano_channel=c; ui.piano_channels[ui.pattern][c]=1; ui.browser_focus=0; windows_focus(&ui.windows,2); ui.input_enabled=0; }
 117:             if(hover(200,row+2,gridw,20) && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) { ui.channel=c; open_context(2,c,(Vector2){ui.mouse.x+ui.windows.editors[0].rect.x,ui.mouse.y+ui.windows.editors[0].rect.y}); }
 120:         for(int i=0;i<span+2;i++) {
 121:             float step=floorf(view)+i,x=200+(step-view)*stepw,left=fmaxf(200,x),right=fminf(200+gridw,x+stepw-3);
 123:             Note *n=note_at(&ui.project,ui.pattern,c,step,60); int available=step<edit_steps();
 124:             ui_surface((Rectangle){left+1,row+2,fmaxf(1,right-left-2),18},!available?ui_theme.disabled:n?ui_theme.step_on[(int)floorf(step/4)%2]:fmodf(floorf(step/4),2)?ui_theme.step_alt:cell);
 125:             if(n && ui.playing_notes[c][n-ui.project.notes[ui.pattern][c]]) DrawRectangleLinesEx((Rectangle){left+1,row+2,fmaxf(1,right-left-2),18},1,clip_foreground(ui_theme.step_on[(int)floorf(step/4)%2]));
 126:             if(fmodf(step,STEPS)==0 && x>=200) DrawLine(x,row,x,row+22,muted);
 127:             if(hover(left,row+2,right-left,18)) {
 128:                 snprintf(ui.status,sizeof ui.status,"%s step %.0f: left paints%s, right erases",ui.project.channel_names[c],step+1,available?"":" and extends the pattern");
 129:                 if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
 130:                     ui.rack_paint=IsMouseButtonPressed(MOUSE_BUTTON_LEFT)?MOUSE_BUTTON_LEFT:MOUSE_BUTTON_RIGHT;
 131:                     ui.rack_paint_channel=-1; paint_rack(width,ui.mouse.x,ui.mouse.y); ui.input_enabled=0;
 137:     if(position>=0) {
 138:         float x=200+(position-view)*stepw;
 139:         if(x>=200 && x<=200+gridw)
 140:             DrawLineEx((Vector2){x,RACK_TOP-4},(Vector2){x,RACK_TOP+fminf(visible,count-ui.rack_scroll)*28-6},1.5f,ink);
 142:     int add_y=RACK_TOP+(int)fminf(visible,count-ui.rack_scroll)*28;
 143:     if(add_y+22<=height-18) {
 144:         int dropping=ui.sample_drag[0] && ui.sample_moved && ui.mouse.y>=add_y && ui.mouse.y<height-18 && ui.mouse.x>=0 && ui.mouse.x<width && windows_hit(&ui.windows,ui.mouse.x+ui.windows.editors[0].rect.x,ui.mouse.y+ui.windows.editors[0].rect.y)==0;
 145:         if(button("+",106,add_y,78,22,0)) open_popup(6);
 147:             ui_surface((Rectangle){106,add_y,78,22},Fade(ui_theme.signal,.18f));
 148:             DrawRectangleLinesEx((Rectangle){106,add_y,78,22},1,ui_theme.signal);
 150:         } else if(hover(106,add_y,78,22)) snprintf(ui.status,sizeof ui.status,"Add instrument: choose a plugin, or drop a sample here");
 152:     float area=visible*28,maximum=fmaxf(0,count-visible),thumb=fminf(area,fmaxf(24,area*visible/fmaxf(1,count))),travel=area-thumb;
 153:     float y=RACK_TOP+(maximum?ui.rack_scroll/maximum*travel:0);
 154:     if(maximum>0) {
 155:         DrawRectangle(width-12,RACK_TOP,8,area,bg); ui_surface((Rectangle){width-12,y,8,thumb},ui.rack_vdrag || hover(width-14,RACK_TOP,14,area)?accent:muted);
 156:         if(hover(width-14,RACK_TOP,14,area)) {
 158:             if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
 159:                 if(ui.mouse.y<y || ui.mouse.y>y+thumb) ui.rack_scroll=fmaxf(0,fminf(maximum,(ui.mouse.y-RACK_TOP-thumb/2)/fmaxf(1,travel)*maximum));
 160:                 ui.rack_vy=ui.mouse.y+ui.windows.editors[0].rect.y; ui.rack_vstart=ui.rack_scroll; ui.rack_vscale=maximum/fmaxf(1,travel); ui.rack_vdrag=1; ui.input_enabled=0;
 165:     float hx=200+(hmaximum>0?ui.rack_view[ui.pattern]/hmaximum*htravel:0);
 166:     DrawRectangle(200,height-16,gridw,8,bg); ui_surface((Rectangle){hx,height-16,hthumb,8},ui.rack_hdrag || hover(200,height-19,gridw,14)?accent:muted);
 167:     if(hover(200,height-19,gridw,14)) {
 169:         if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
 170:             if(ui.mouse.x<hx || ui.mouse.x>hx+hthumb) ui.rack_view[ui.pattern]=fmaxf(0,fminf(hmaximum,(ui.mouse.x-200-hthumb/2)/fmaxf(1,htravel)*hmaximum));
 171:             ui.rack_hx=ui.mouse.x+ui.windows.editors[0].rect.x; ui.rack_hstart=ui.rack_view[ui.pattern]; ui.rack_hscale=hmaximum/fmaxf(1,htravel); ui.rack_hdrag=1; ui.input_enabled=0;
```

### pattern_rgb

```c
 176: Color pattern_rgb(uint32_t rgb) {
```

### pattern_pitch_range

```c
 183:     *low=127; *high=0;
 184:     for(int c=0;c<ui.project.channel_count;c++) for(int n=0;n<NOTES;n++) {
```

## ui_shell.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### shortcut_down

```c
   4: int shortcut_down(void) {
   6:     return IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
   8:     return IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
```

### command_down

```c
  12: int command_down(void) {
  13:     return shortcut_down() || IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)
  14:         || IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER);
```

### delete_pressed

```c
  19:     return IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE);
  21:     return IsKeyPressed(KEY_DELETE);
```

### mouse_callback

```c
  43:     if(button>=0 && button<8 && action==GLFW_PRESS) ui.pending_clicks[button]++;
```

### mouse_pressed

```c
  47: int mouse_pressed(int button) { return button>=0 && button<8 && ui.frame_clicks[button]; }
```

### captured

```c
  59: int captured(void) { return ui.fm_graph_drag>=0 || ui.row_zoom_drag>=0 || ui.eq_drag>=0 || ui.automation_node>=0 || ui.picker_drag>=0 || ui.navigation_drag>=0 || ui.track_resize>=0 || ui.rack_hdrag || ui.rack_vdrag || ui.piano_key_drag || ui.control_drag || ui.route_drag>=0 || ui.note_drag || ui.velocity_drag>=0 || ui.arrangement.gesture || ui.playlist_pan || ui.browser_resize || ui.rack_paint>=0 || ui.cable_drag>0 || ui.playlist_vpan || ui.mixer_pan || ui.piano_scroll_drag || ui.piano_vdrag || ui.piano_gesture || ui.marker_drag>=0; }
```

### ui_scale

```c
  61: float ui_scale(void) { return fmaxf(1,fminf(GetScreenWidth()/1200.f,GetScreenHeight()/675.f)); }
```

### playback_start

```c
  66:     return ui.playing && !playback_loop()[1] && start>=end?0:start;
```

### transport_toggle

```c
  69: void transport_toggle(void) { ui.stop_armed=0; recording_finish(); int active=audio_stop(); ui.playing=ui.playing || active?0:1; ui.reset=1; }
```

### transport_stop

```c
  73:     recording_finish(); audio_stop(); ui.playing=0; ui.reset=1;
  74:     if(rewind) { ui.playlist_start=0; if(!ui.song) ui.piano_start[ui.pattern]=0; }
  75:     ui.stop_armed=1;
```

### draw_editor

```c
  81:     if(id==4) { e->minh=ui.project.instrument[ui.instrument_channel]==INSTRUMENT_FM?420:300; e->rect.h=fmaxf(e->minh,e->rect.h); }
  82:     Rect r=e->rect; int top=EDITORS-1;
  83:     while(top>0 && !ui.windows.editors[ui.windows.order[top]].visible) top--;
  86:     ui.text_origin=(Vector2){r.x*scale,r.y*scale};
  87:     BeginMode2D((Camera2D){.offset=ui.text_origin,.zoom=scale});
  88:     ui_surface((Rectangle){0,0,r.w,r.h},id==0?ui_theme.rack:id==3?ui_theme.mixer:panel);
  89:     ui_surface((Rectangle){0,0,r.w,TITLE},focused?ui_theme.title_focus:ui_theme.title);
  91:     label(TextFormat("%s%s",id==4 && ui.project.instrument[ui.instrument_channel]==INSTRUMENT_FM?"FM Synth":titles[id],e->pinned?" (on top)":""),8,3,13,ink);
  92:     int rack_title_knob=id==0 && ((ui.mouse.x>=r.x+r.w-72-SWING_RADIUS-1 && ui.mouse.x<r.x+r.w-72+SWING_RADIUS+1) || (ui.mouse.x>=r.x+124 && ui.mouse.x<r.x+180));
  93:     if(ui.input_enabled && !rack_title_knob && windows_hit(&ui.windows,ui.mouse.x,ui.mouse.y)==id && ui.mouse.y<r.y+TITLE && IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) open_context(1,id,ui.mouse);
  95:     int hover_slot=-1;
  96:     if(chrome && ui.mouse.x>=r.x+r.w-54) {
  97:         int slot=fminf(2,(ui.mouse.x-r.x-r.w+54)/18); hover_slot=slot;
  98:         DrawRectangle(r.w-54+slot*18,1,17,TITLE-2,ui_theme.hover);
  99:         snprintf(ui.status,sizeof ui.status,"%s window",slot==0?"Minimize":slot==1?e->maximized?"Restore":"Maximize":"Hide");
 102:     DrawLineEx((Vector2){r.w-49,9},(Vector2){r.w-41,9},2,hover_slot==0?hover_ink:ink);
 103:     int mx=r.w-27; Color maximize_ink=hover_slot==1?hover_ink:ink;
 104:     if(e->maximized) { DrawRectangleLines(mx-2,4,7,7,maximize_ink); DrawRectangle(mx-4,6,7,7,hover_slot==1?ui_theme.hover:cell); DrawRectangleLines(mx-4,6,7,7,maximize_ink); }
 105:     else DrawRectangleLines(mx-4,5,8,8,maximize_ink);
 106:     symbol("x",r.w-9,9,hover_slot==2?hover_ink:ink);
 107:     ui.input_enabled=ui.input_enabled && ui.windows.owner==id && ui.windows.grab<0;
 108:     Vector2 global=ui.mouse; ui.mouse.x-=r.x; ui.mouse.y-=r.y;
 109:     if(id==0) {
 110:         button("",124,2,56,TITLE-4,0);
 111:         label(ui.rack_filters[ui.rack_filter],124+(56-text_width(ui.rack_filters[ui.rack_filter],11))/2,3,11,ink);
 112:         if(hover(124,2,56,TITLE-4)) snprintf(ui.status,sizeof ui.status,"Channel filter: All, Audio (Playlist drops), Unsorted (Rack samples)");
 113:         knob_style(r.w-72,TITLE/2,&ui.project.swing,0,1,0,"Swing",KNOB_SWING);
 114:         if(hover(r.w-72-SWING_RADIUS-1,TITLE/2-SWING_RADIUS-1,2*SWING_RADIUS+2,2*SWING_RADIUS+2) || ui.control_drag==&ui.project.swing) snprintf(ui.status,sizeof ui.status,"Swing: %.0f%% | delays alternate steps; drag or wheel; right-click to enter",ui.project.swing*100);
 116:     ui.input_enabled=ui.input_enabled && ui.mouse.y>=TITLE && !(ui.mouse.x>r.w-12 && ui.mouse.y>r.h-12);
 117:     if(ui.midi_take.active && id<=2) ui.input_enabled=0;
 118:     if(id==0) rack(r.w,r.h); else if(id==1) playlist(r.w,r.h,scale); else if(id==2) piano(r.w,r.h); else if(id==3) mixer(r.w,r.h); else if(id==5) eq_editor(r.w,r.h); else if(ui.project.instrument[ui.instrument_channel]==INSTRUMENT_FM) fm_editor(r.w,r.h); else sampler(r.w,r.h);
 120:     DrawLine(r.w-9,r.h-2,r.w-2,r.h-9,muted);
 121:     EndMode2D(); EndScissorMode(); ui.text_origin=(Vector2){0};
```

### navigate_editors

```c
 125:     ui.navigation_active=0;
 126:     int blocked=ui.pattern_popup || ui.context_kind || ui.sample_drag[0] || !IsWindowFocused();
 127:     if(ui.navigation_drag>=0 && (blocked || !IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))) ui.navigation_drag=-1;
 128:     int id=ui.navigation_drag>=0?ui.navigation_drag:windows_hit(&ui.windows,ui.mouse.x,ui.mouse.y);
 129:     if(blocked || (id!=1 && id!=2) || (ui.navigation_drag<0 && captured()) || ui.windows.grab>=0) return;
 131:     float gx=id==1?212:60,gy=id==1?PLAYLIST_GRID_TOP:PIANO_GRID_TOP,gridw=r.w-gx-24;
 132:     float area=id==1?r.h-gy-PLAYLIST_BOTTOM:r.h-PIANO_GRID_PADDING;
 133:     Vector2 local={ui.mouse.x-r.x,ui.mouse.y-r.y};
 134:     if(ui.navigation_drag<0 && (local.x<(id==1?120:4) || local.x>=r.w-24 || local.y<gy || local.y>=gy+area)) return;
 135:     int middle=ui.navigation_drag>=0;
 136:     if(!middle && IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
 137:         windows_focus(&ui.windows,id); ui.navigation_drag=id; ui.navigation_last=ui.mouse; middle=1;
 139:     NavigationMotion motion={0};
 143:     } else motion=navigation_motion(ui.navigation_input,IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT),shortcut_down(),scale);
 144:     if(motion.x || motion.y || motion.zoom || middle) { ui.browser_focus=0; ui.navigation_active=1; }
 145:     if(id==1) {
 147:         ui.arrangement.view_start=fmaxf(0,ui.arrangement.view_start+motion.x*span/gridw);
 148:         if(motion.zoom) arrangement_zoom(&ui.arrangement,motion.zoom,fmaxf(0,fminf(1,(local.x-gx)/gridw)));
 149:         ui.track_scroll=fmaxf(0,fminf(fmaxf(0,track_position(LANES)-area),ui.track_scroll+motion.y));
 151:         ui.piano_pan[ui.pattern]=fmaxf(0,ui.piano_pan[ui.pattern]+motion.x*ui.piano_span[ui.pattern]/gridw);
 152:         if(motion.zoom && local.x>=gx) timeline_zoom(&ui.piano_span[ui.pattern],&ui.piano_pan[ui.pattern],motion.zoom,fmaxf(0,fminf(1,(local.x-gx)/gridw)),STEPS);
 155:         ui.piano_top=fmaxf(piano_min_top(),fminf(127,ui.piano_top-rows)); ui.piano_scroll_remainder-=rows;
 156:         if(ui.piano_top==piano_min_top() || ui.piano_top==127) ui.piano_scroll_remainder=0;
```

## ui_state.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
   6:     .typing_keys=1,
   7:     .output_volume=1,
   8:     .input_enabled=1,
   9:     .picker_drag=-1,
  10:     .automation_selected=-1,
  11:     .automation_node=-1,
  12:     .automation_lane=-1,
  13:     .automation_clip=-1,
  14:     .rename_automation=-1,
  15:     .arrangement={.source_steps=STEPS,.source_pattern=-1,.zoom=1},
  16:     .track_zoom=1,
  17:     .row_zoom_drag=-1,
  18:     .selected_track=-1,
  19:     .track_resize=-1,
  20:     .marker_drag=-1,
  21:     .ruler_last_id=-1,
  22:     .ruler_last_click=-1,
  24:     .route_drag=-1,
  25:     .mixer_selected=1,
  26:     .cable_drag=-1,
  27:     .browser_progress=-1,
  28:     .browser_focus=1,
  29:     .browser_width=174,
  30:     .last_sidebar=174,
  31:     .piano_top=72,
  32:     .piano_zoom=1,
  33:     .piano_key=-1,
  34:     .piano_note_length=2,
  35:     .piano_span={16,16,16,16,16,16,16,16},
  36:     .velocity_drag=-1,
  37:     .rack_paint=-1,
  38:     .rack_paint_channel=-1,
  39:     .navigation_drag=-1,
  40:     .eq_drag=-1,
  41:     .fm_graph_drag=-1,
  44:     .relink_channel=-1,
  45:     .file_last_row=-1,
  46:     .file_last_click=-1,
  47:     .knob_context=-1,
  48:     .rename_track=-1,
  49:     .rename_mixer=-1,
  50:     .rename_channel=-1,
```

## ui_waveform.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### source_rgb

```c
   4: Color source_rgb(uint32_t rgb) {
   5:     Color color={rgb>>16,(rgb>>8)&255,rgb&255,255};
   6:     Vector3 hsv=ColorToHSV(color);
   8:     return ColorFromHSV(hsv.x,fminf(hsv.y,.25f),hsv.z);
```

### audio_color

```c
  17:     float t=fminf(1,log1pf(ui.samples[c].frames/(float)RATE)/log1pf(30));
  19:     Color color=ColorFromHSV(320-235*t,.25f,.98f);
```

### sampler_view

```c
  31:         view->offset=source.data?(view->crop.data-source.data)/sample_channels(source):0;
  32:         view->settings=settings; view->generation=ui.sample_generation[c]; view->epoch=ui.sample_epoch; view->valid=1;
  35:         view->gain=(settings.flags&SAMPLE_NORMALIZE) && maximum>0?1/maximum:1;
```

### sampler_view_frames

```c
  47:     return frames?fmax(1,llround(frames*(double)ui.project.sampler[c].time)):0;
```

### sampler_view_envelope

```c
  52:     double ratio=frames?count/(double)frames:0;
```

### audio_view_steps

```c
  62:     return seconds*audio_source_bpm(&ui.project,c)/15/channel_speed(&ui.project,c);
```

### playlist_clip_length

```c
  66:     int c=ui.project.clips[lane][clip]-PATTERNS-1;
  67:     if(c>=0 && c<CHANNELS && !ui.project.clip_steps[lane][clip] && sampler_live_preview(c)) {
  68:         double remaining=fmax(0,sampler_view_frames(c)/(double)RATE-ui.project.clip_offsets[lane][clip])*audio_source_bpm(&ui.project,c)/15;
```

### processed_waveform

```c
  83:         ui.audio_wave_states[c].revision++; ui.audio_wave_states[c].valid=1;
```

### wave_source

```c
  90:         view->start+fmax(0,x-view->origin)/view->pixels_per_step)*view->frames_per_step;
```

### wave_quad

```c
  95:     rlTexCoord2f(.5f,v); rlVertex2f(x,top); rlTexCoord2f(.5f,vv); rlVertex2f(x,bottom);
  96:     rlTexCoord2f(.5f,vv); rlVertex2f(xx,next_bottom); rlTexCoord2f(.5f,v); rlVertex2f(xx,next_top);
```

### draw_waveform

```c
 100:     if(!view.sample.frames || view.width<=0 || right<=left || area.height<=0) return;
 101:     float step=1/ui_scale(),feather=.5f/ui.font_scale,mid=area.y+area.height*.5f,amplitude=fmaxf(0,area.height*.5f-2/ui.font_scale);
 104:     rlSetTexture(ui.cable_texture.id); rlBegin(RL_QUADS); rlColor4ub(color.r,color.g,color.b,color.a);
 105:     for(int i=0;i<=columns;i++) {
 111:         float top=mid-fmaxf(0,peak.high)*amplitude,bottom=mid-fminf(0,peak.low)*amplitude;
 114:             wave_quad(previous_x,x,previous_top-feather,top-feather,previous_top+feather,top+feather,0,.25f);
 115:             wave_quad(previous_x,x,previous_top+feather,top+feather,previous_bottom-feather,bottom-feather,.5f,.5f);
 116:             wave_quad(previous_x,x,previous_bottom-feather,bottom-feather,previous_bottom+feather,bottom+feather,.75f,1);
 120:     rlEnd(); rlSetTexture(0);
```

### audio_waveform_at

```c
 124:     Sample sample=ui.samples[c]; Waveform *wave=&ui.audio_waves[c]; int recording=0,preview=sampler_live_preview(c);
 126:     if(ui.recording_ui.active) for(int i=0;i<ui.recording_ui.count;i++) if(ui.recording_ui.takes[i].channel==c) {
 127:         sample=ui.recording_ui.takes[i].sample; wave=&ui.recording_ui.takes[i].wave; recording=1; preview=0; break;
 129:     if(recording) lane=-1;
 132:     AudioTimeline map; double start=0,offset=0;
 133:     float full=sample.frames/(float)RATE*audio_source_bpm(&ui.project,c)/15/channel_speed(&ui.project,c)*pixels_per_step;
 134:     if(lane>=0) {
 136:         full=audio_timeline_duration(&map,start,fmax(0,sample.frames-offset)/RATE*audio_source_bpm(&ui.project,c)/15)*pixels_per_step;
 138:     WaveDisplay view={.sample=sample,.wave=wave,.channel=c,.preview=preview,.timeline=lane>=0?&map:NULL,
 139:         .start=start,.offset=offset,.frames_per_step=RATE*15/audio_source_bpm(&ui.project,c),.origin=x,.width=full,.pixels_per_step=pixels_per_step};
 140:     draw_waveform(view,(Rectangle){x,y+14,full,height-14},left,fminf(right,x+full),clip_foreground(audio_color(c)));
```

### audio_waveform

```c
 144:     audio_waveform_at(c,x,y-14,left,right,pixels_per_step,height+14,-1,-1);
```

## ui_widgets.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### label_moving

```c
  19:     DrawTextEx(font,text,(Vector2){x,y},font.baseSize/ui.font_scale,0,color);
```

### backspace

```c
  28:     do { n--; } while(n && ((unsigned char)text[n]&0xc0)==0x80);
  29:     text[n]=0;
```

### circles_init

```c
  37:     Image image=GenImageColor(64,64,WHITE); Color *pixels=image.data;
  38:     for(int y=0;y<64;y++) for(int x=0;x<64;x++) {
  39:         float dx=x+.5f-32,dy=y+.5f-32;
  40:         pixels[y*64+x].a=255*fmaxf(0,fminf(1,(32-sqrtf(dx*dx+dy*dy))/3));
```

### circle

```c
  47:     DrawTexturePro(ui.circle_texture,(Rectangle){0,0,64,64},(Rectangle){x-radius,y-radius,radius*2,radius*2},(Vector2){0},0,color);
```

### icons_init

```c
  52:     ui.icon_large=fmaxf(1,roundf(28*scale)); ui.icon_small=fmaxf(1,roundf(16*scale));
  53:     ui.icon_medium=fmaxf(1,roundf(20*scale));
  54:     int high=ui.icon_large*4;
  55:     RenderTexture2D target=LoadRenderTexture(ICON_COUNT*high,high);
  56:     BeginTextureMode(target); ClearBackground((Color){255,255,255,0});
  57:     for(int id=0;id<ICON_COUNT;id++) {
  58:         BeginMode2D((Camera2D){.offset={id*high,0},.zoom=high/28.f});
  59:         if(id==ICON_RACK) for(int row=0;row<3;row++) {
  60:             DrawRectangle(5,7+row*6,4,3,WHITE);
  61:             for(int step=0;step<3;step++) DrawRectangle(12+step*4,7+row*6,3,3,WHITE);
  63:         if(id==ICON_PLAYLIST) for(int row=0;row<3;row++) {
  64:             DrawLineEx((Vector2){5,8+row*6},(Vector2){23,8+row*6},1,Fade(WHITE,.45f));
  65:             DrawRectangle(5+(row%2)*7,6+row*6,10,4,WHITE);
  68:             DrawRectangleLinesEx((Rectangle){4,6,20,16},1.5f,WHITE);
  69:             for(int key=1;key<4;key++) DrawLineEx((Vector2){4+key*5,6},(Vector2){4+key*5,22},1,WHITE);
  70:             for(int key=0;key<2;key++) DrawRectangle(7+key*5,6,4,9,WHITE);
  71:             DrawRectangle(21,6,3,9,WHITE); /* Keep the partial F-sharp distinct from the border at small sizes. */
  73:         if(id==ICON_MIXER) for(int strip=0;strip<3;strip++) {
  74:             int x=7+strip*7,y=9+strip*4;
  75:             DrawLineEx((Vector2){x,5},(Vector2){x,23},1.5f,WHITE);
  76:             DrawRectangle(x-3,y-2,6,4,WHITE);
  79:             DrawLineEx((Vector2){8,20},(Vector2){21,7},3.5f,WHITE);
  80:             DrawTriangle((Vector2){5,23},(Vector2){10,21},(Vector2){7,18},WHITE);
  83:             DrawLineEx((Vector2){14,14},(Vector2){21,6},3,WHITE);
  84:             DrawCircle(11,17,4,WHITE);
  85:             DrawTriangle((Vector2){5,23},(Vector2){13,21},(Vector2){8,15},WHITE);
  87:         if(id==ICON_SELECT) for(int i=0;i<3;i++) {
  88:             int p=6+i*6;
  89:             DrawLineEx((Vector2){p,6},(Vector2){p+3,6},1.5f,WHITE);
  90:             DrawLineEx((Vector2){p,22},(Vector2){p+3,22},1.5f,WHITE);
  91:             DrawLineEx((Vector2){6,p},(Vector2){6,p+3},1.5f,WHITE);
  92:             DrawLineEx((Vector2){22,p},(Vector2){22,p+3},1.5f,WHITE);
  95:             DrawCircleLines(8,19,4,WHITE); DrawCircleLines(20,19,4,WHITE);
  96:             DrawLineEx((Vector2){10,16},(Vector2){22,5},2,WHITE);
  97:             DrawLineEx((Vector2){18,16},(Vector2){6,5},2,WHITE);
  98:             DrawCircle(14,12,1.5f,WHITE);
 101:             DrawLineEx((Vector2){5,14},(Vector2){23,14},2,WHITE);
 102:             DrawLineEx((Vector2){5,14},(Vector2){10,9},2,WHITE);
 103:             DrawLineEx((Vector2){5,14},(Vector2){10,19},2,WHITE);
 104:             DrawLineEx((Vector2){23,14},(Vector2){18,9},2,WHITE);
 105:             DrawLineEx((Vector2){23,14},(Vector2){18,19},2,WHITE);
 108:             DrawLineEx((Vector2){14,5},(Vector2){14,23},1.75f,WHITE);
 109:             DrawLineEx((Vector2){9,10},(Vector2){14,5},1.75f,WHITE);
 110:             DrawLineEx((Vector2){19,10},(Vector2){14,5},1.75f,WHITE);
 111:             DrawLineEx((Vector2){9,18},(Vector2){14,23},1.75f,WHITE);
 112:             DrawLineEx((Vector2){19,18},(Vector2){14,23},1.75f,WHITE);
 115:             DrawLineEx((Vector2){7,22},(Vector2){11,6},1.8f,WHITE);
 116:             DrawLineEx((Vector2){11,6},(Vector2){16,22},1.8f,WHITE);
 117:             DrawLineEx((Vector2){7,22},(Vector2){16,22},1.8f,WHITE);
 118:             DrawLineEx((Vector2){12,19},(Vector2){21,8},1.8f,WHITE); DrawCircle(21,8,2,WHITE);
 121:             DrawLineEx((Vector2){7,5},(Vector2){7,23},1.5f,WHITE);
 122:             DrawLineEx((Vector2){12,14},(Vector2){23,14},1.8f,WHITE);
 123:             DrawLineEx((Vector2){18,9},(Vector2){23,14},1.8f,WHITE);
 124:             DrawLineEx((Vector2){18,19},(Vector2){23,14},1.8f,WHITE);
 127:             DrawRectangleLinesEx((Rectangle){3,7,22,14},1.5f,WHITE);
 128:             for(int row=0;row<2;row++) for(int key=0;key<5;key++) DrawRectangle(6+key*3.5f,10+row*4,2,2,WHITE);
 129:             DrawRectangleRec((Rectangle){9,18,10,1.5f},WHITE);
 132:             Vector2 previous={4,14};
 133:             for(int step=1;step<=48;step++) {
 134:                 float phase=step/48.f;
 135:                 Vector2 next={4+20*phase,14-7*sinf(phase*2*PI)};
 136:                 DrawLineEx(previous,next,2,WHITE); previous=next;
 139:         if(id==ICON_AUTOMATION) { DrawLineEx((Vector2){5,21},(Vector2){14,7},2,WHITE); DrawLineEx((Vector2){14,7},(Vector2){23,17},2,WHITE); DrawCircle(5,21,3,WHITE); DrawCircle(14,7,3,WHITE); DrawCircle(23,17,3,WHITE); }
 140:         if(id==ICON_PLAY) DrawTriangle((Vector2){9,6},(Vector2){9,22},(Vector2){22,14},WHITE);
 141:         if(id==ICON_BACK) DrawTriangle((Vector2){19,6},(Vector2){6,14},(Vector2){19,22},WHITE);
 142:         if(id==ICON_STOP) DrawRectangle(7,7,14,14,WHITE);
 143:         if(id==ICON_PAUSE) { DrawRectangle(7,6,5,16,WHITE); DrawRectangle(16,6,5,16,WHITE); }
 145:             DrawLineEx((Vector2){7,7},(Vector2){21,21},2,WHITE);
 146:             DrawLineEx((Vector2){7,21},(Vector2){21,7},2,WHITE);
 148:         EndMode2D();
 155:     for(int variant=0;variant<3;variant++) {
 156:         int size=variant==2?ui.icon_small:variant==1?ui.icon_medium:ui.icon_large,row=variant==2?ui.icon_large+ui.icon_medium:variant==1?ui.icon_large:0;
 158:         for(int id=0;id<ICON_COUNT;id++) for(int y=0;y<size;y++) for(int x=0;x<size;x++) {
 159:             float left=x*ratio,top=y*ratio,right=(x+1)*ratio,bottom=(y+1)*ratio,alpha=0;
 161:                 float weight=(fminf(right,sx+1)-fmaxf(left,sx))*(fminf(bottom,sy+1)-fmaxf(top,sy));
 164:             output[(row+y)*image.width+id*ui.icon_large+x]=(Color){255,255,255,roundf(alpha/(ratio*ratio))};
```

### icon

```c
 172:     int variant=size<=16?2:size<=20?1:0,pixels=variant==2?ui.icon_small:variant==1?ui.icon_medium:ui.icon_large;
 173:     float left=raster_position(x-pixels/ui.font_scale/2,ui.text_origin.x);
 174:     float top=raster_position(y-pixels/ui.font_scale/2,ui.text_origin.y);
 175:     DrawTexturePro(ui.icons,(Rectangle){id*ui.icon_large,variant==2?ui.icon_large+ui.icon_medium:variant==1?ui.icon_large:0,pixels,pixels},(Rectangle){left,top,pixels/ui.font_scale,pixels/ui.font_scale},(Vector2){0},0,color);
```

### arcs_init

```c
 180:     Image image=GenImageColor(1536,288,BLANK);
 182:     for(int bank=0;bank<3;bank++) for(int level=0;level<=128;level++) for(int y=0;y<32;y++) for(int x=0;x<32;x++) {
 183:         float dx=x+.5f-16,dy=y+.5f-16,distance=sqrtf(dx*dx+dy*dy);
 184:         float origin=bank==0?2.4f:bank==1?PI/2:-PI/2;
 185:         float angle=atan2f(dy,dx)-origin; if(angle<0) angle+=2*PI;
 186:         float end=level/128.f*(bank==0?4.6f:2*PI),start=bank==1?PI:0;
 187:         float alpha=fmaxf(0,fminf(1,fminf(16-distance,distance-(16-48.f/KNOB_RADIUS))/1.4f));
 188:         if(bank!=2 || level!=128) alpha*=fmaxf(0,fminf(1,fminf(angle-fminf(start,end),fmaxf(start,end)-angle)*distance));
 189:         pixels[(level/16*32+y)*1536+bank*512+level%16*32+x]=(Color){255,255,255,255*alpha};
```

### cables_init

```c
 196:     Image image=GenImageColor(1,16,WHITE); Color *pixels=image.data;
 197:     for(int y=0;y<16;y++) pixels[y].a=255*fminf(1,(8-fabsf(y+.5f-8))/4);
```

### cable

```c
 202: void cable(Vector2 points[4],Color color) {
 203:     Vector2 previous[2]={{0}},normal={2,0};
 204:     rlSetTexture(ui.cable_texture.id); rlBegin(RL_QUADS); rlColor4ub(color.r,color.g,color.b,color.a);
 205:     for(int i=0;i<=64;i++) {
 206:         float t=i/64.f,u=1-t;
 207:         Vector2 p=GetSplinePointBezierCubic(points[0],points[1],points[2],points[3],t);
 208:         float dx=3*u*u*(points[1].x-points[0].x)+6*u*t*(points[2].x-points[1].x)+3*t*t*(points[3].x-points[2].x);
 209:         float dy=3*u*u*(points[1].y-points[0].y)+6*u*t*(points[2].y-points[1].y)+3*t*t*(points[3].y-points[2].y);
 210:         float length=hypotf(dx,dy); if(length>.001f) normal=(Vector2){-dy/length*2,dx/length*2};
 211:         Vector2 edge[2]={{p.x+normal.x,p.y+normal.y},{p.x-normal.x,p.y-normal.y}};
 213:             rlTexCoord2f(.5f,0); rlVertex2f(previous[0].x,previous[0].y);
 214:             rlTexCoord2f(.5f,0); rlVertex2f(edge[0].x,edge[0].y);
 215:             rlTexCoord2f(.5f,1); rlVertex2f(edge[1].x,edge[1].y);
 216:             rlTexCoord2f(.5f,1); rlVertex2f(previous[1].x,previous[1].y);
 218:         previous[0]=edge[0]; previous[1]=edge[1];
 220:     rlEnd(); rlSetTexture(0);
```

### smooth_line

```c
 223: void smooth_line(Vector2 start,Vector2 end,float width,Color color) {
 225:     if(length<.001f) return;
 226:     float radius=width*2/3;
 227:     Vector2 normal={-dy/length*radius,dx/length*radius};
 228:     rlSetTexture(ui.cable_texture.id); rlBegin(RL_QUADS); rlColor4ub(color.r,color.g,color.b,color.a);
 229:     rlTexCoord2f(.5f,0); rlVertex2f(start.x+normal.x,start.y+normal.y);
 230:     rlTexCoord2f(.5f,0); rlVertex2f(end.x+normal.x,end.y+normal.y);
 231:     rlTexCoord2f(.5f,1); rlVertex2f(end.x-normal.x,end.y-normal.y);
 232:     rlTexCoord2f(.5f,1); rlVertex2f(start.x-normal.x,start.y-normal.y);
 233:     rlEnd(); rlSetTexture(0);
```

### hover

```c
 236: int hover(float x,float y,float w,float h) { return ui.input_enabled && CheckCollisionPointRec(ui.mouse,(Rectangle){x,y,w,h}); }
```

### menu_key

```c
 238: int menu_key(int key) { return IsKeyPressed(key) || IsKeyPressedRepeat(key); }
```

### menu_keys_begin

```c
 242:         ui.menu_keys.id=id; ui.menu_keys.selected=default_item; ui.menu_keys.count=0; ui.menu_keys.pointer=ui.mouse;
 246:     if(moved || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
 247:         for(int i=0;i<ui.menu_keys.count;i++) if(CheckCollisionPointRec(ui.mouse,ui.menu_keys.bounds[i])) {
 251:     ui.menu_keys.active=1; ui.menu_keys.items=0; ui.menu_keys.enter=0;
 254:     if(!opened && ui.menu_keys.count>0) {
 256:             if(scroll && next && ui.menu_keys.selected==ui.menu_keys.count-1 && *scroll+ui.menu_keys.count<total) ++*scroll;
 257:             else if(scroll && previous && ui.menu_keys.selected==0 && *scroll>0) --*scroll;
 259:                 if(scroll && next && ui.menu_keys.selected==ui.menu_keys.count-1) *scroll=0;
 260:                 if(scroll && previous && ui.menu_keys.selected==0) *scroll=fmaxf(0,total-ui.menu_keys.count);
 261:                 ui.menu_keys.selected=(ui.menu_keys.selected+(next?1:ui.menu_keys.count-1))%ui.menu_keys.count;
 264:         ui.menu_keys.enter=IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER);
```

### menu_keys_end

```c
 269:     ui.menu_keys.count=ui.menu_keys.items<128?ui.menu_keys.items:128; ui.menu_keys.active=0; ui.menu_keys.enter=0;
 270:     if(ui.menu_keys.count>0 && ui.menu_keys.selected>=ui.menu_keys.count) ui.menu_keys.selected=ui.menu_keys.count-1;
```

### symbol

```c
 280:     else return 0;
 281:     icon(id,x,y,16,c);
 282:     return 1;
```

### button_color

```c
 286:     int over=hover(x,y,w,h);
 287:     int menu_item=ui.menu_keys.active && ui.input_enabled && strcmp(text,"x")?ui.menu_keys.items++:-1;
 288:     if(menu_item>=0 && menu_item<128) ui.menu_keys.bounds[menu_item]=(Rectangle){x,y,w,h};
 289:     int focused=menu_item>=0 && menu_item==ui.menu_keys.selected;
 291:     if(activate) ui.menu_keys.enter=0;
 293:     if((menu_item>=0?focused:over) && *text) {
 319:     int highlighted=menu_item>=0?focused:over && !ui.menu_keys.active;
 320:     Color surface=menu_item>=0?(focused?ui_theme.hover:base):
 323:     Color foreground=(menu_item>=0?focused:active || highlighted)?theme_foreground(surface):ink;
 324:     if(w>52 || !symbol(text,x+w/2,y+h/2,foreground)) {
 325:         int available=w-(w<=72?8:16);
 328:         label(caption,x+((w<=72 || !strcmp(text,"+"))?(w-text_width(caption,size))/2:8),y+(h-size)/2,size,foreground);
 330:     if(menu_item>=0 && over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) ui.menu_keys.selected=menu_item;
 331:     return activate || (over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
```

### drag_button

```c
 340:     if(hover(x,y,w,h) || active || clicked) SetMouseCursor(MOUSE_CURSOR_RESIZE_NS);
```

### color_picker

```c
 344: int color_picker(int x,int y,int width,uint32_t selected) {
 345:     int chosen=-1,spacing=width/COLOR_HUES;
 346:     int hover_index=-1;
 347:     for(int i=0;i<COLOR_COUNT;i++)
 348:         if(hover(x+(i%COLOR_HUES)*spacing,y+(i/COLOR_HUES)*28,spacing-4,24)) hover_index=i;
 349:     for(int i=0;i<COLOR_COUNT;i++) {
 351:         Rectangle swatch={x+(i%COLOR_HUES)*spacing,y+(i/COLOR_HUES)*28,spacing-4,24};
 352:         int over=hover(swatch.x,swatch.y,swatch.width,swatch.height);
 354:         if(hover_index>=0?i==hover_index:selected==rgb) DrawRectangleLinesEx((Rectangle){swatch.x+2,swatch.y+2,swatch.width-4,swatch.height-4},2,clip_foreground(color));
 355:         if(over) { SetMouseCursor(MOUSE_CURSOR_POINTING_HAND); snprintf(ui.status,sizeof ui.status,"Choose color #%06X",rgb); if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) chosen=i; }
```

### picker_button

```c
 361:     int over=hover(4,y,112,50);
 363:     ui_surface((Rectangle){4,y,112,50},color);
 364:     if(over || selected) DrawRectangleLinesEx((Rectangle){4,y,112,50},2,WHITE);
 365:     return over && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
```

### automation_display_update

```c
 369:     memset(ui.automation_visible,0,sizeof ui.automation_visible);
 372:     double held_end[AUTOMATIONS]; for(int a=0;a<ui.project.automation_count;a++) held_end[a]=-INFINITY;
 373:     for(int pass=0;pass<2;pass++) for(int l=0;l<LANES;l++) if(!(ui.project.lane_mute[l]&1) && (!solo || (ui.project.lane_mute[l]&2))) for(int b=0;b<CLIPS;b++) {
 374:         int a=ui.project.clips[l][b]-AUTOMATION_SOURCE-1; if(a<0 || a>=ui.project.automation_count) continue;
 377:         if(pass==0?(ui.visual_step<end || end<held_end[a]):(ui.visual_step<start || ui.visual_step>=end)) continue;
 379:         float normalized=automation_value(&ui.project.automations[a],(pass==0?end:ui.visual_step)-start+ui.project.clip_offsets[l][b]);
 380:         for(int i=0;i<ui.project.automation_count;i++) {
 382:             if(t.parameter==target.parameter && t.owner==target.owner && t.slot==target.slot) { ui.automation_visible[i]=1; ui.automation_shown[i]=normalized; if(pass==0) held_end[i]=end; }
```

### displayed_value

```c
 391:     for(int a=0;a<ui.project.automation_count;a++) {
```

### mute_light

```c
 400: void mute_light(float x,float y,uint8_t *states,int count,int selected,const char *name) {
 401:     uint8_t state=states[selected]; if(displayed_value(&states[selected],!!(state&1))>=.5f) state|=1; else state&=~1; int solo=count?solo_any(states,count):0;
 402:     int over=hover(x-8,y-8,16,16);
 404:     int enabled=!(state&1) && (!solo || (state&2));
 405:     Color color=enabled?(state&2?ui_theme.meter_mid:ui_theme.signal):cell;
 406:     if(enabled && !(state&2) && !over)
 407:         color=ColorAlphaBlend(cell,Fade(color,.48f),WHITE);
 408:     circle(x,y,6,over?ink:state&2?ui_theme.meter_mid:ui_theme.border); circle(x,y,5,color);
 410:         snprintf(ui.status,sizeof ui.status,"%s: %s | Left-click: mute/unmute; right-click: %s",name,state&1?"Muted":state&2?"Solo":solo?"Silenced by solo":"Enabled",states==ui.project.lane_mute?"add/remove solo":"automation / solo menu");
 411:         if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { states[selected]^=1; ui.input_enabled=0; }
 412:         if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
 415:                 Vector2 position=ui.mouse; if(ui.knob_context>=0) { position.x+=ui.windows.editors[ui.knob_context].rect.x; position.y+=ui.windows.editors[ui.knob_context].rect.y; }
 416:                 open_context(10,0,position); return;
 420:                 for(int i=0;i<INSERTS;i++) ui.project.insert_mute[i]&=~2;
 421:                 for(int c=0;c<CHANNELS;c++) ui.project.mute[c]&=~2;
 422:                 for(int l=0;l<LANES;l++) ui.project.lane_mute[l]&=~2;
 424:             ui.input_enabled=0;
```

### editor_button

```c
 431:     int front=0;
 432:     for(int i=EDITORS-1;i>=0;i--) {
 436:     int clicked=button("",x,8,22,22,0);
 437:     if(clicked) { if(front) ui.windows.editors[id].visible=0; else windows_focus(&ui.windows,id); }
 438:     icon(id,x+11,19,20,ink);
 439:     if(hover(x,8,22,22)) snprintf(ui.status,sizeof ui.status,"%s: %s",names[id],front?"hide this window":"show / bring this window to the front");
```

### open_context

```c
 443: void open_context(int kind,int target,Vector2 position) {
 444:     ui.context_kind=kind; ui.context_target=target; ui.context_position=position; ui.context_opened=1; ui.input_enabled=0;
```

### preset_directory

```c
 449:     if(!slash) return 0;
 451:     if(snprintf(root,sizeof root,"%.*s/presets",(int)(slash-ui.browser.config),ui.browser.config)>=(int)sizeof root || MakeDirectory(root)) return 0;
 453:     if(snprintf(out,capacity,"%s/%s",root,device)>=(int)capacity || MakeDirectory(out)) return 0;
 454:     if(kind==PRESET_FM) for(int i=0;i<FM_FACTORY_COUNT;i++) {
 460:     return 1;
```

### capture_control

```c
 468:     ui.control_raw=*value; ui.control_drag=value;
 470:     ui.control_fader=fader; ui.control_reverse=0; ui.input_enabled=0;
```

### knob_style

```c
 475:     int over=hover(x-size-1,y-size-1,size*2+2,size*2+2);
 478:         high=3; initial=(int)initial&3; shown=(int)roundf(shown)&3;
 479:         /* Old DX files may store aliases 4..31. Canonicalize on interaction,
 481:         if(over && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || GetMouseWheelMove()))
 482:             *value=(int)roundf(*value)&3;
 484:     if(over || ui.control_drag==value) {
 485:         if(style==KNOB_VOLUME) snprintf(ui.status,sizeof ui.status,"%s: %.4g (%.2f dB) | Dot marks 0 dB; drag or wheel; right-click for value / automation",name,shown,gain_db(shown));
 486:         else snprintf(ui.status,sizeof ui.status,"%s: %.4g | Drag up/down or wheel; right-click for value / automation",name,shown);
 489:         if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) control_menu(value,low,high,initial,name);
 490:         if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) { capture_control(value,low,high,0); ui.control_low=low; ui.control_high=high; ui.control_reverse=style==KNOB_WIDTH; }
 496:             float next=style==KNOB_LOGARITHMIC?*value*powf(high/low,wheel/20):*value+wheel*(integer?1:(high-low)/20)*(style==KNOB_WIDTH?-1:1);
 502:     if(style==KNOB_CENTER) fraction=shown<initial?.5f*(shown-low)/(initial-low):.5f+.5f*(shown-initial)/(high-initial);
 503:     if(style==KNOB_WIDTH) fraction=1-fraction;
 504:     if(style==KNOB_VOLUME) fraction=shown<=1?.75f*(shown):.75f+.25f*(shown-1)/(high-1);
 506:     float origin=centered?PI/2:style==KNOB_SWING?2.4f:style==KNOB_VOLUME?PI/2:-PI/2;
 507:     float angle=origin+fraction*(style==KNOB_SWING?4.6f:2*PI);
 508:     Color color=style==KNOB_SWING || style==KNOB_CENTER?ui_theme.swing:style==KNOB_PAN?(fraction<.5f?ui_theme.pan_left:ui_theme.pan_right):style==KNOB_WIDTH?(fraction<.5f?ui_theme.stereo:ui_theme.mono):ui_theme.knob;
 509:     if((over || ui.control_drag==value) && (style==KNOB_NORMAL || style==KNOB_VOLUME || style==KNOB_LOGARITHMIC || style==KNOB_FIXED_COARSE)) color=accent;
 511:         DrawTexturePro(ui.knob_arcs,(Rectangle){0,256,32,32},(Rectangle){x-radius,y-radius,radius*2,radius*2},(Vector2){0},0,ui_theme.knob_track);
 512:         circle(x,y,radius*.68f,cell);
 513:     } else { circle(x,y,radius,ui_theme.knob_track); circle(x,y,radius*.8f,cell); }
 517:         float opacity=.35f;
 518:         Rectangle ring={x,y,radius*2,radius*2}; Vector2 center={radius,radius};
 519:         DrawTexturePro(ui.knob_arcs,(Rectangle){512,0,32,32},ring,center,0,Fade(left,opacity));
 520:         DrawTexturePro(ui.knob_arcs,(Rectangle){512,256,32,32},ring,center,0,Fade(right,opacity));
 522:     int level=fmaxf(0,fminf(128,roundf(fraction*128)));
 523:     float rotation=style==KNOB_SWING || centered?0:(origin+PI/2)*RAD2DEG;
 524:     DrawTexturePro(ui.knob_arcs,(Rectangle){(style==KNOB_SWING?0:centered?512:1024)+level%16*32,level/16*32,32,32},(Rectangle){x,y,radius*2,radius*2},(Vector2){radius,radius},rotation,color);
 526:         float unity=origin+.75f*2*PI;
 527:         circle(x+cosf(unity)*size*1.22f,y+sinf(unity)*size*1.22f,1.2f,muted);
 529:     Vector2 pointer={x+cosf(angle)*radius*.85f,y+sinf(angle)*radius*.85f};
 531:     circle(pointer.x,pointer.y,2.3f*pointer_scale,(Color){35,24,49,255});
 532:     circle(pointer.x,pointer.y,1.7f*pointer_scale,WHITE);
```

## waveform.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
   5: #define BLOCK 256
```

### scan

```c
   8:     WavePeak peak={0};
   9:     for(unsigned i=start;i<end;i++) for(unsigned side=0;side<sample_channels(s);side++) {
```

### waveform_build

```c
  17:     free(wave->tree); *wave=(Waveform){0};
  18:     if(!sample.frames) return 1;
  19:     unsigned blocks=(sample.frames-1)/BLOCK+1,leaves=1;
  20:     while(leaves<blocks) leaves*=2;
  21:     WavePeak *tree=calloc((size_t)leaves*2,sizeof *tree); if(!tree) return 0;
  22:     for(unsigned i=0;i<blocks;i++) {
  23:         unsigned end=(i+1)*BLOCK; if(end>sample.frames) end=sample.frames;
  26:     for(unsigned i=leaves-1;i;i--) tree[i]=merge(tree[i*2],tree[i*2+1]);
  27:     *wave=(Waveform){tree,leaves}; return 1;
```

### waveform_range

```c
  31:     if(start>=end) return (WavePeak){0};
  34:     unsigned first=(start+BLOCK-1)/BLOCK,last=end/BLOCK;
  37:     for(unsigned a=first+wave->leaves,b=last+wave->leaves;a<b;a/=2,b/=2) {
  38:         if(a&1) peak=merge(peak,wave->tree[a++]);
  39:         if(b&1) peak=merge(peak,wave->tree[--b]);
```

### envelope_bin

```c
  47:     unsigned start=base+(uint64_t)bin*size,end=fmin((uint64_t)start+size,(uint64_t)base+frames);
  48:     if(wave->tree && size>=BLOCK && size/BLOCK<=wave->leaves && base%size==0 &&
```

### envelope_level

```c
  56:     double at=fmax(0,fmin((frames-1)/size,position/size-.5));
  57:     unsigned first=floor(at),last=fmin(first+1,(frames-1)/size);
```

### waveform_envelope_region

```c
  61:     if(start>=sample.frames || !frames || !isfinite(position) || !isfinite(width)) return (WavePeak){0};
  64:     double level=log2(fmax(1,fmin(width,frames)));
  65:     unsigned size=(unsigned)exp2(floor(level));
  66:     return blend(envelope_level(wave,sample,start,frames,position,size),envelope_level(wave,sample,start,frames,position,size*2),level-floor(level));
```

### waveform_envelope

```c
  69:     return waveform_envelope_region(wave,sample,0,sample.frames,position,width);
```

### waveform_append

```c
  72:     unsigned blocks=sample.frames?(sample.frames-1)/BLOCK+1:0;
  75:         unsigned end=(i+1)*BLOCK; if(end>sample.frames) end=sample.frames;
  77:         for(at/=2;at;at/=2) wave->tree[at]=merge(wave->tree[at*2],wave->tree[at*2+1]);
  79:     return 1;
```

## windows.c

### file declarations

```c
   1: // SPDX-License-Identifier: GPL-3.0-only
```

### windows_init

```c
   6:     float left=392,available=width-left-8,mixer_width=fminf(720,available);
   8:         {{left+(available-486)/2,156,486,168},{0},438,152,1,0,0},
   9:         {{180,42,width-184,height-66},{0},440,240,1,0,0},
  10:         {{320,128,760,430},{0},440,392,0,0,0},
  11:         {{left+(available-mixer_width)/2,height-324,mixer_width,300},{0},460,300,0,0,0},
  12:         {{(width-560)/2,(height-340)/2,560,340},{0},520,300,0,0,0},
  13:         {{(width-620)/2,(height-380)/2,620,380},{0},480,330,0,0,0}},
  14:         .order={1,2,3,4,5,0},.owner=-1,.grab=-1,.title_id=-1};
  15:     w->editors[1].restore=w->editors[1].rect;
  16:     w->editors[1].maximized=1;
```

### windows_hit

```c
  19:     for(int i=EDITORS-1;i>=0;i--) { int id=w->order[i]; if(w->editors[id].visible && inside(w->editors[id].rect,x,y)) return id; }
  20:     return -1;
```

### windows_focus

```c
  23:     int i=0; while(i<EDITORS && w->order[i]!=id) i++;
  24:     for(;i<EDITORS-1;i++) w->order[i]=w->order[i+1];
  25:     w->order[EDITORS-1]=id; w->editors[id].visible=1; w->focused=id;
  26:     for(int pass=0;pass<EDITORS;pass++) for(int j=0;j<EDITORS-1;j++)
  27:         if(w->editors[w->order[j]].pinned && !w->editors[w->order[j+1]].pinned) {
  28:             int swap=w->order[j]; w->order[j]=w->order[j+1]; w->order[j+1]=swap;
```

### windows_pin

```c
  31: void windows_pin(Windows *w,int id) { w->editors[id].pinned^=1; windows_focus(w,id); }
```

### windows_update

```c
  37:     for(int i=0;i<EDITORS;i++) { Editor *e=&w->editors[i]; e->rect=e->maximized?d:constrain(e->rect,d,e->minw,e->minh); }
  38:     if(!down) w->grab=-1;
  39:     if(w->grab>=0) {
  45:     int id=inside(d,x,y)?windows_hit(w,x,y):-1;
  46:     if(pressed && id>=0) {
  49:             int control=x>=r.x+r.w-54;
  50:             int maximize=(x>=r.x+r.w-36 && x<r.x+r.w-18) ||
  51:                 (!control && time>0 && w->title_id==id && time-w->title_time<.3 && fabsf(x-w->title_x)<4 && fabsf(y-w->title_y)<4);
  52:             int rack_control=id==0 && x>=r.x+r.w-79 && x<r.x+r.w-65;
  53:             if(rack_control) { w->grab=-1; w->title_id=-1; }
  55:                 if(e->maximized) { e->rect=e->restore; e->maximized=0; }
  56:                 else { e->restore=r; e->rect=d; e->maximized=1; }
  57:                 w->grab=-1; w->title_id=-1;
  58:             } else if(control) { e->visible=0; w->title_id=-1; }
  61:                 if(!e->maximized) { w->grab=id; w->resize=0; w->dx=x-r.x; w->dy=y-r.y; }
  63:         } else if(!e->maximized && x>r.x+r.w-12 && y>r.y+r.h-12) {
  64:             w->grab=id; w->resize=1; w->dx=r.x+r.w-x; w->dy=r.y+r.h-y;
  66:         if(y<r.y+TITLE && !(id==0 && x>=r.x+r.w-79 && x<r.x+r.w-65)) id=-1;
  68:     w->owner=w->grab>=0?-1:id;
```
