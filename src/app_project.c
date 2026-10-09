// SPDX-License-Identifier: GPL-3.0-only
#include "ui_internal.h"





int channel_missing(int c) {
    return c>=0 && c<ui.project.channel_count && !ui.originals[c].frames && ui.project.paths[c][0] &&
        strcmp(ui.project.paths[c],SAMPLE_EMPTY) && !recording_source(PATTERNS+c);
}

const char *channel_caption(int c) {
    return channel_missing(c)?TextFormat("%s [missing]",ui.project.channel_names[c]):ui.project.channel_names[c];
}

void sample_duration(Project *p,int c,Sample processed,Sample original) {
    if(original.frames || !p->paths[c][0] || !strcmp(p->paths[c],SAMPLE_EMPTY)) p->audio_seconds[c]=processed.frames/(float)RATE;
}

Sample sample_copy(Sample source) {
    Sample copy={0}; sample_clone(source,&copy); return copy;
}

void *sampler_worker(void *unused) {
    (void)unused;
    ui.sampler_job.ok=sample_process(ui.sampler_job.input,ui.sampler_job.settings,&ui.sampler_job.result);
    atomic_store(&ui.sampler_job.done,1); glfwPostEmptyEvent(); return NULL;
}

void sampler_update(void) {
    if(ui.sampler_job.busy && atomic_load(&ui.sampler_job.done)) {
        pthread_join(ui.sampler_job.thread,NULL); int c=ui.sampler_job.channel;
        if(ui.sampler_job.epoch==ui.sample_epoch && c<ui.project.channel_count && ui.sampler_job.generation==ui.sample_generation[c] && sampler_processing_equal(ui.sampler_job.settings,ui.project.sampler[c])) {
            if(ui.sampler_job.ok) {
                Sample old=ui.samples[c]; audio_sample(c,ui.sampler_job.result); ui.samples[c]=ui.sampler_job.result; ui.sampler_job.result=(Sample){0}; sample_free(old);
            } else ui.project.sampler[c]=ui.sampler_applied[c];
            sample_duration(&ui.project,c,ui.samples[c],ui.originals[c]);
            ui.sampler_applied[c]=ui.project.sampler[c]; ui.sampler_generation[c]=ui.sample_generation[c];
        }
        sample_free(ui.sampler_job.input); sample_free(ui.sampler_job.result); ui.sampler_job.busy=0;
    }
}

int sampler_flush(void) {
    if(ui.sampler_job.busy) {
        pthread_join(ui.sampler_job.thread,NULL); sample_free(ui.sampler_job.input); sample_free(ui.sampler_job.result); ui.sampler_job.busy=0;
    }
    for(int c=0;c<ui.project.channel_count;c++) if(ui.sampler_generation[c]!=ui.sample_generation[c] || !sampler_processing_equal(ui.sampler_applied[c],ui.project.sampler[c])) {
        Sample next; if(!sample_process(ui.originals[c],ui.project.sampler[c],&next)) return 0;
        Sample old=ui.samples[c]; audio_sample(c,next); ui.samples[c]=next; sample_free(old);
        sample_duration(&ui.project,c,ui.samples[c],ui.originals[c]);
        ui.sampler_applied[c]=ui.project.sampler[c]; ui.sampler_generation[c]=ui.sample_generation[c];
    }
    return 1;
}

void sampler_queue(void) {
    if(ui.recording_ui.active) return;
    sampler_update();
    if(ui.sampler_job.busy || ui.control_drag || ui.arrangement.gesture==STRETCH_CLIP || ui.pattern_popup==5) return;
    for(int c=0;c<ui.project.channel_count;c++) if(ui.sampler_generation[c]!=ui.sample_generation[c] || !sampler_processing_equal(ui.sampler_applied[c],ui.project.sampler[c])) {
        Sample copy=sample_copy(ui.originals[c]);
        if(ui.originals[c].frames && !copy.data) return;
        ui.sampler_job.input=copy; ui.sampler_job.result=(Sample){0}; ui.sampler_job.channel=c;
        ui.sampler_job.settings=ui.project.sampler[c]; ui.sampler_job.epoch=ui.sample_epoch; ui.sampler_job.generation=ui.sample_generation[c]; atomic_store(&ui.sampler_job.done,0);
        if(pthread_create(&ui.sampler_job.thread,NULL,sampler_worker,NULL)) { sample_free(copy); return; }
        ui.sampler_job.busy=1; return;
    }
}

int project_dirty(void) { return project_document_dirty(&ui.document,&ui.project); }

int import_sample(const char *path,int c) {
    recording_finish();
    char full[PATH_MAX]; Sample s;
    if(!realpath(path,full) || strlen(full)>=sizeof ui.project.paths[c] || !sample_load(full,&s)) { snprintf(ui.status,sizeof ui.status,"Cannot load sample: unreadable, unsupported, too large or insufficient memory."); return 0; }
    Sample cooked;
    if(!sample_process(s,ui.project.sampler[c],&cooked)) { sample_free(s); snprintf(ui.status,sizeof ui.status,"Sample processing failed; channel unchanged"); return 0; }
    if(ui.project.instrument[c]==INSTRUMENT_FM) {
        for(int a=ui.project.automation_count-1;a>=0;a--) if(ui.project.automations[a].target.owner==(unsigned)c && ((ui.project.automations[a].target.parameter>=PARAM_FM_RATIO && ui.project.automations[a].target.parameter<=PARAM_FM_LAST) || (ui.project.automations[a].target.parameter>=PARAM_DX7_FIRST && ui.project.automations[a].target.parameter<=PARAM_DX7_LAST))) automation_delete(&ui.project,a);
        ui.project.instrument[c]=INSTRUMENT_SAMPLER;
    }
    Sample old=ui.samples[c],raw=ui.originals[c]; audio_sample(c,cooked); ui.samples[c]=cooked; ui.originals[c]=s; sample_free(old); sample_free(raw);
    ui.sample_generation[c]++; ui.sampler_generation[c]=ui.sample_generation[c]; ui.sampler_applied[c]=ui.project.sampler[c];
    sample_duration(&ui.project,c,ui.samples[c],ui.originals[c]);
    snprintf(ui.project.paths[c],sizeof ui.project.paths[c],"%s",full);
    memset(ui.project.channel_names[c],0,PATTERN_NAME); snprintf(ui.project.channel_names[c],PATTERN_NAME,"%s",GetFileNameWithoutExt(full));
    snprintf(ui.status,sizeof ui.status,"Loaded %.100s into %s",GetFileName(path),ui.project.channel_names[c]); return 1;
}

void audition_entry(int entry) {
    browser_select(&ui.browser,entry);
    if(entry<0 || entry>=ui.browser.items) return;
    const char *path=ui.browser.nodes[entry].path;
    if(!IsFileExtension(path,".wav;.flac;.mp3") || DirectoryExists(path)) return;
    Sample next;
    if(!sample_load(path,&next)) { snprintf(ui.status,sizeof ui.status,"Cannot preview %.120s",GetFileName(path)); return; }
    audio_preview(next); sample_free(ui.audition); ui.audition=next;
    snprintf(ui.audition_path,sizeof ui.audition_path,"%s",path);
    waveform_build(&ui.audition_wave,next);
    snprintf(ui.status,sizeof ui.status,"Preview: %.140s",GetFileName(path));
}

void rack_reveal_last(void) {
    Editor *e=&ui.windows.editors[0];
    int count=rack_channels(NULL);
    e->rect.h=fminf(GetScreenHeight()/ui_scale()-66,RACK_TOP+48+count*28);
    ui.rack_scroll=fmaxf(0,count-(int)((e->rect.h-RACK_TOP-48)/28));
}

void add_instrument(int type) {
    if(!instrument_descriptor(type)) return;
    int c=ui.project.channel_count;
    if(c>=CHANNELS) { snprintf(ui.status,sizeof ui.status,"Channel Rack limit: %d channels.",CHANNELS); return; }
    ui.project.instrument[c]=type; ui.project.fm[c]=fm_default();
    ui.project.channel_pitch[c]=0; ui.project.pitch_range[c]=2; ui.project.channel_audio[c]=0; ui.project.audio_seconds[c]=0; ui.rack_filter=2; ui.project.channel_count++; instrument_descriptor(INSTRUMENT_SAMPLER)->defaults(&ui.project.sampler[c]); ui.project.volume[c]=1; ui.project.pan[c]=0; ui.project.mute[c]=0; ui.project.route[c]=0;
    snprintf(ui.project.paths[c],sizeof ui.project.paths[c],"%s",SAMPLE_EMPTY);
    memset(ui.project.channel_names[c],0,PATTERN_NAME); snprintf(ui.project.channel_names[c],PATTERN_NAME,"%s",instrument_descriptor(type)->name);
    for(int pat=0;pat<PATTERNS;pat++) {
        memset(ui.project.notes[pat][c],0,sizeof ui.project.notes[pat][c]); memset(ui.note_selected[pat][c],0,NOTES); ui.piano_channels[pat][c]=0;
    }
    Sample old=ui.samples[c],raw=ui.originals[c]; ui.originals[c]=ui.samples[c]=(Sample){0}; audio_channels(&ui.project,ui.samples); sample_free(old); sample_free(raw);
    ui.sample_generation[c]++; ui.sampler_generation[c]=ui.sample_generation[c]; ui.sampler_applied[c]=ui.project.sampler[c];
    ui.channel=ui.instrument_channel=c; ui.browser_focus=0; rack_reveal_last(); windows_focus(&ui.windows,4);
}

void replace_instrument(int c,int type) {
    if(ui.recording_ui.active) { snprintf(ui.status,sizeof ui.status,"Finish recording before replacing an instrument."); return; }
    if(!sampler_flush()) { snprintf(ui.status,sizeof ui.status,"Could not prepare the current sample; instrument unchanged."); return; }
    if(!channel_replace_instrument(&ui.project,c,type)) {
        snprintf(ui.status,sizeof ui.status,"Audio clip channels use a Sampler. Replace an instrument in a pattern to use FM Synth."); return;
    }
    ui.automation_selected=ui.automation_node=-1; ui.control_drag=NULL; ui.menu_value=NULL;
    if(!strcmp(ui.project.channel_names[c],"Sampler") || !strcmp(ui.project.channel_names[c],"FM Synth"))
        snprintf(ui.project.channel_names[c],PATTERN_NAME,"%s",instrument_descriptor(type)->name);
    ui.channel=ui.instrument_channel=c; ui.browser_focus=0; windows_focus(&ui.windows,4);
    snprintf(ui.status,sizeof ui.status,"%s now uses %s; notes and routing kept across all patterns.",ui.project.channel_names[c],instrument_descriptor(type)->name);
}

int pattern_instruments(int pat,int rows[CHANNELS]) {
    int count=0;
    if(pat<0 || pat>=ui.project.pattern_count) return 0;
    for(int c=0;c<ui.project.channel_count;c++) for(int n=0;n<NOTES;n++) if(ui.project.notes[pat][c][n].velocity) { rows[count++]=c; break; }
    if(!count && ui.channel>=0 && ui.channel<ui.project.channel_count) rows[count++]=ui.channel;
    return count;
}

void drop_sample(const char *path) {
    int target=windows_hit(&ui.windows,ui.mouse.x,ui.mouse.y);
    if(target==1) {
        Rect r=ui.windows.editors[1].rect;
        float gx=r.x+212,gy=r.y+PLAYLIST_GRID_TOP,gridw=r.w-236;
        int list=ui.mouse.x>=r.x+4 && ui.mouse.x<r.x+116 && ui.mouse.y>=gy && ui.mouse.y<r.y+r.h-66;
        if(!list && (ui.mouse.x<gx || ui.mouse.x>=gx+gridw || ui.mouse.y<gy || ui.mouse.y>=r.y+r.h-PLAYLIST_BOTTOM)) { snprintf(ui.status,sizeof ui.status,"Drop audio onto the Playlist grid or Audio list."); return; }
        if(!list) {
            float pixels=gridw/BARS*ui.arrangement.zoom;
            float at=ui.arrangement.view_start+(ui.mouse.x-gx)/pixels;
            int lane=track_at(ui.track_scroll+ui.mouse.y-gy),hit=arrangement_hit(&ui.project,lane,at);
            if(hit>=0 && AUDIO_SOURCE(ui.project.clips[lane][hit]-1)) {
                int c=ui.project.clips[lane][hit]-PATTERNS-1;
                if(import_sample(path,c)) {
                    ui.channel=ui.instrument_channel=c; ui.browser_focus=0;
                    ui.arrangement.source_pattern=PATTERNS+c; ui.arrangement.source_steps=ui.project.audio_seconds[c]; ui.arrangement.source_offset=0;
                    memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected); ui.arrangement.selected[lane][hit]=1;
                    audio_channels(&ui.project,ui.samples); windows_focus(&ui.windows,1);
                    snprintf(ui.status,sizeof ui.status,"Replaced sample with %.100s",ui.project.channel_names[c]);
                }
                return;
            }
        }
        int c=ui.project.channel_count;
        if(c>=CHANNELS) { snprintf(ui.status,sizeof ui.status,"Channel Rack limit: %d channels.",CHANNELS); return; }
        float pixels=gridw/BARS*ui.arrangement.zoom,q=grid_interval(pixels/STEPS);
        float start=snap_floor((ui.arrangement.view_start+(ui.mouse.x-gx)/pixels)*STEPS,q)/STEPS;
        int lane=track_at(ui.track_scroll+ui.mouse.y-gy);
        int slot=-1;
        if(!list) {
            /* Check placement before decoding or creating a channel. */
            Project next=ui.project; next.channel_count++; next.channel_audio[c]=1; next.audio_seconds[c]=.001f;
            slot=arrangement_place(&next,lane,start,PATTERNS+c,.001f*ui.project.bpm/15);
            if(slot<0) { snprintf(ui.status,sizeof ui.status,"No space here: choose an empty part of the track."); return; }
        }
        if(!import_sample(path,c)) return;
        if(!list) {
            float length=ui.samples[c].frames/(float)RATE*ui.project.bpm/15;
            slot=arrangement_place(&ui.project,lane,start,PATTERNS+c,length);
            if(slot<0) { /* Discard the import without adding a channel. */
                Sample empty={0}; audio_sample(c,empty); sample_free(ui.samples[c]); sample_free(ui.originals[c]); ui.samples[c]=ui.originals[c]=empty;
                ui.project.paths[c][0]=0; snprintf(ui.status,sizeof ui.status,"Audio overlaps another clip: choose an empty part of the track."); return;
            }
        }
        uint32_t colors[CHANNELS]; int count=0;
        for(int i=0;i<ui.project.channel_count;i++) if(ui.project.channel_audio[i]) colors[count++]=ui.project.channel_colors[i];
        ui.project.channel_colors[c]=next_source_color(colors,count);
        ui.project.channel_audio[c]=1; ui.project.channel_count++;
        ui.channel=ui.instrument_channel=c; ui.rack_filter=1; ui.rack_scroll=0;
        ui.arrangement.source_pattern=PATTERNS+c; ui.arrangement.source_steps=ui.project.audio_seconds[c]; ui.arrangement.source_offset=0;
        if(!list) { memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected); ui.arrangement.selected[lane][slot]=1; }
        ui.picker_tab=1; ui.picker_scroll=count;
        ui.browser_focus=0; audio_channels(&ui.project,ui.samples); windows_focus(&ui.windows,1);
        snprintf(ui.status,sizeof ui.status,list?"Imported %.100s into the Audio list; drag it to the Playlist when ready.":"Placed %.100s; available in the Audio list and Rack Audio group.",ui.project.channel_names[c]); return;
    }
    if(target==4 && ui.mouse.y>=ui.windows.editors[4].rect.y+TITLE) {
        if(import_sample(path,ui.instrument_channel)) {
            ui.channel=ui.instrument_channel; ui.browser_focus=0; audio_channels(&ui.project,ui.samples); windows_focus(&ui.windows,4);
        }
        return;
    }
    Rect r=ui.windows.editors[0].rect;
    if(windows_hit(&ui.windows,ui.mouse.x,ui.mouse.y)!=0 || ui.mouse.y<r.y+TITLE) {
        snprintf(ui.status,sizeof ui.status,"Drop onto the Playlist grid, a sampler, a Rack row, or empty Rack space."); return;
    }
    int c=rack_channel_at(ui.rack_scroll+(ui.mouse.y-r.y-RACK_TOP)/28);
    int visible=fmaxf(1,(r.h-RACK_TOP-48)/28);
    int replace=ui.mouse.y>=r.y+RACK_TOP && ui.mouse.y<r.y+RACK_TOP+visible*28 && c>=0 && c<ui.project.channel_count;
    if(!replace) c=ui.project.channel_count;
    if(c>=CHANNELS) { snprintf(ui.status,sizeof ui.status,"Channel Rack limit: %d channels.",CHANNELS); return; }
    if(import_sample(path,c)) {
        if(!replace) {
            ui.project.channel_audio[c]=0; ui.project.channel_count++; ui.rack_filter=2;
            rack_reveal_last();
        }
        ui.channel=c; ui.browser_focus=0; windows_focus(&ui.windows,0); audio_channels(&ui.project,ui.samples);
    }
}

void delete_channel(int c) {
    if(ui.edit_clipboard.kind==COPY_CLIPS) ui.edit_clipboard.kind=COPY_EMPTY;
    recording_finish();
    ui.automation_selected=ui.automation_node=-1; ui.picker_drag=-1;
    Sample old=ui.samples[c],raw=ui.originals[c];
    if(!channel_delete(&ui.project,c)) return;
    ui.sample_epoch++;
    for(int i=c;i<ui.project.channel_count;i++) {
        ui.samples[i]=ui.samples[i+1]; ui.originals[i]=ui.originals[i+1]; ui.sampler_applied[i]=ui.sampler_applied[i+1];
        ui.sample_generation[i]=ui.sample_generation[i+1]; ui.sampler_generation[i]=ui.sampler_generation[i+1];
    }
    ui.originals[ui.project.channel_count]=(Sample){0};
    ui.samples[ui.project.channel_count]=(Sample){0};
    for(int pat=0;pat<PATTERNS;pat++) {
        memmove(ui.note_selected[pat][c],ui.note_selected[pat][c+1],(ui.project.channel_count-c)*NOTES); memset(ui.note_selected[pat][ui.project.channel_count],0,NOTES);
        memmove(&ui.piano_channels[pat][c],&ui.piano_channels[pat][c+1],ui.project.channel_count-c);
        ui.piano_channels[pat][ui.project.channel_count]=0;
    }
    ui.channel=fmaxf(0,fminf(ui.channel-(ui.channel>c),ui.project.channel_count-1));
    ui.piano_channel=fmaxf(0,fminf(ui.piano_channel-(ui.piano_channel>c),ui.project.channel_count-1));
    ui.instrument_channel=fmaxf(0,fminf(ui.instrument_channel-(ui.instrument_channel>c),ui.project.channel_count-1));
    ui.arrangement.gesture=IDLE; ui.arrangement.source_pattern=-1; memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected);
    audio_channels(&ui.project,ui.samples); sample_free(old); sample_free(raw); ui.reset=1;
}

void install_project(Project next,Sample fresh[CHANNELS],Sample processed[CHANNELS],const char *filename) {
    recording_finish();midi_panic();ui.midi_learning=0;
    ui.sample_epoch++; audio_stop(); ui.fm_graph_drag=-1;
    sample_free(ui.audition); free(ui.audition_wave.tree); ui.audition_wave=(Waveform){0}; ui.audition=(Sample){0}; ui.audition_path[0]=0; ui.browser_progress=-1;
    memset(ui.keyboard_notes,0,sizeof ui.keyboard_notes); ui.piano_note_length=2; if(ui.edit_clipboard.kind==COPY_CLIPS) ui.edit_clipboard.kind=COPY_EMPTY;
    ui.picker_tab=ui.picker_scroll=0; ui.picker_drag=-1; ui.automation_selected=ui.automation_node=ui.automation_lane=ui.automation_clip=-1; ui.sample_drag[0]=0;
    ui.rack_hdrag=ui.rack_vdrag=ui.playlist_pan=ui.playlist_vpan=ui.piano_scroll_drag=ui.piano_vdrag=ui.mixer_pan=0;
    ui.piano_scroll_remainder=0; ui.track_zoom=ui.piano_zoom=1; ui.row_zoom_drag=-1; memset(ui.track_heights,0,sizeof ui.track_heights);
    ui.navigation_active=0; ui.navigation_drag=-1; ui.track_resize=-1; ui.selected_track=-1; ui.ruler_loop_drag=0; ui.stop_armed=0; ui.ruler_last_id=-1;
    ui.visual_step=0;
    ui.playing=0; ui.pattern=(int)fminf(ui.pattern,next.pattern_count-1); audio_update(&next,0,ui.song,ui.pattern,1,ui.output_volume,0,0,0);
    for(int c=0;c<CHANNELS;c++) sample_duration(&next,c,processed[c],fresh[c]);
    audio_channels(&next,processed);
    for(int c=0;c<CHANNELS;c++) {
        Sample old=ui.samples[c],raw=ui.originals[c]; ui.samples[c]=processed[c]; ui.originals[c]=fresh[c]; sample_free(old); sample_free(raw);
        ui.sample_generation[c]++; ui.sampler_generation[c]=ui.sample_generation[c]; ui.sampler_applied[c]=next.sampler[c];
    }
    ui.project=next; project_document_saved(&ui.document,&ui.project,filename);
    ui.project.insert_count=INSERTS; ui.rack_scroll=0; ui.rack_filter=0; ui.channel=fmaxf(0,fminf(ui.channel,ui.project.channel_count-1));
    ui.piano_channel=ui.instrument_channel=ui.channel;
    for(int i=0;i<PATTERNS;i++) { ui.piano_span[i]=fmaxf(STEPS,next.pattern_steps[i]); ui.piano_pan[i]=0; ui.piano_range[i]=0; }
    memset(ui.note_selected,0,sizeof ui.note_selected); ui.piano_gesture=PIANO_IDLE; ui.playlist_start=0; memset(ui.piano_start,0,sizeof ui.piano_start); memset(ui.song_loop,0,sizeof ui.song_loop); memset(ui.pattern_loop,0,sizeof ui.pattern_loop); ui.marker_drag=-1;
    memset(ui.piano_channels,0,sizeof ui.piano_channels); ui.mixer_selected=ui.project.route[ui.channel]; ui.mixer_scroll=0;
    ui.piano_key_drag=0; ui.piano_key=-1;
    ui.note_drag=NULL; ui.velocity_drag=-1; ui.control_drag=NULL; ui.route_drag=-1; ui.rack_paint=-1; ui.cable_drag=-1;
    memset(&ui.arrangement,0,sizeof ui.arrangement); ui.arrangement.source_pattern=-1; ui.arrangement.source_steps=STEPS; ui.arrangement.zoom=1; ui.playlist_pan=0; ui.playlist_vpan=0; ui.track_scroll=0; memset(ui.rack_view,0,sizeof ui.rack_view); memset(ui.rack_range,0,sizeof ui.rack_range);
    project_document_saved(&ui.document,&ui.project,NULL);
    history_clear(&ui.edit_history); history_checkpoint();
    snprintf(ui.status,sizeof ui.status,"Loaded %.150s",filename); ui.reset=1;
}

void new_project(int demo) {
    Project next; Sample fresh[CHANNELS]={0},processed[CHANNELS]={0};
    if(demo) { project_demo(&next); samples_default(fresh); }
    else project_new(&next);
    for(int c=0;c<CHANNELS;c++) {
        if((demo && c<4 && !fresh[c].data) || !sample_process(fresh[c],next.sampler[c],&processed[c])) {
            for(int i=0;i<CHANNELS;i++) { sample_free(fresh[i]); sample_free(processed[i]); }
            snprintf(ui.status,sizeof ui.status,"Project unchanged: could not prepare samples."); return;
        }
    }
    ui.pattern=ui.channel=0; ui.song=demo; install_project(next,fresh,processed,demo?"demo" PROJECT_FILE_SUFFIX:"project" PROJECT_FILE_SUFFIX); ui.document.saved_on_disk=0;
    snprintf(ui.status,sizeof ui.status,demo?"Demo loaded: press Play to hear the eight-bar groove.":"New project: load a sample to get started.");
}

int load_project(const char *path) {
    Project next; Sample fresh[CHANNELS]={0},processed[CHANNELS]={0}; int missing=0;
    if(strlen(path)>=sizeof ui.document.path || !project_load_assets(path,&next,fresh,processed,sample_load,&missing)) {
        snprintf(ui.status,sizeof ui.status,"Project unchanged: invalid project, sample path too long or processing failed: %.100s",path); return 0;
    }
    char filename[PATH_MAX]; snprintf(filename,sizeof filename,"%s",path);
    install_project(next,fresh,processed,filename);
    if(missing) snprintf(ui.status,sizeof ui.status,"Loaded with %d missing samples. Right-click a channel and choose Relink sample, or drop its file onto the channel.",missing);
    return 1;
}

void replace_project(int action) {
    recording_finish();
    if(project_document_request(&ui.document,&ui.project,action)) { open_popup(12); return; }
    if(action==4) load_project(ui.document.replacement_path); else if(action==3) project_file_action(4); else if(action==REPLACE_QUIT) ui.document.quit=1; else new_project(action==2);
}

void request_load_project(const char *path) {
    if(strlen(path)>=sizeof ui.document.replacement_path) { snprintf(ui.status,sizeof ui.status,"Project path is too long."); return; }
    snprintf(ui.document.replacement_path,sizeof ui.document.replacement_path,"%s",path); replace_project(4);
}

void replacement_saved(void) {
    project_document_saved(&ui.document,&ui.project,NULL);
    int action=ui.document.pending_action; ui.document.pending_action=0;
    if(action==4) load_project(ui.document.replacement_path); else if(action==3) project_file_action(4); else if(action==REPLACE_QUIT) ui.document.quit=1; else if(action) new_project(action==2);
}

void history_stamps(uint64_t stamps[CHANNELS]) {
    for(int c=0;c<CHANNELS;c++) stamps[c]=((uint64_t)ui.sample_epoch<<32)|ui.sample_generation[c];
}

int history_checkpoint(void) {
    uint64_t stamps[CHANNELS]; history_stamps(stamps);
    if(!history_capture(&ui.edit_history,&ui.project,ui.originals,stamps)) {
        snprintf(ui.status,sizeof ui.status,"Could not retain undo history: insufficient memory."); return 0;
    }
    return 1;
}

void undo_redo(int direction) {
    if(recording_active() || captured()) { snprintf(ui.status,sizeof ui.status,"Finish the current edit or recording before undo/redo."); return; }
    if(!history_checkpoint()) return;
    uint8_t keep[CHANNELS]={0}; uint64_t current_stamps[CHANNELS]; history_stamps(current_stamps);
    Project next; Sample retained[CHANNELS],fresh[CHANNELS]={0},processed[CHANNELS]={0};
    if(!history_peek(&ui.edit_history,direction,&next,retained)) { snprintf(ui.status,sizeof ui.status,direction<0?"Nothing to undo.":"Nothing to redo."); return; }
    for(int c=0;c<CHANNELS;c++) {
        if(history_source_matches(&ui.edit_history,direction,c,ui.originals[c],current_stamps[c]) && sampler_processing_equal(next.sampler[c],ui.sampler_applied[c])) {
            keep[c]=1; fresh[c]=ui.originals[c]; processed[c]=ui.samples[c]; sample_duration(&next,c,ui.samples[c],ui.originals[c]); continue;
        }
        fresh[c]=sample_copy(retained[c]);
        if((retained[c].frames && !fresh[c].data) || !sample_process(fresh[c],next.sampler[c],&processed[c])) goto failed;
        sample_duration(&next,c,processed[c],fresh[c]);
    }
    if(ui.sampler_job.busy) { pthread_join(ui.sampler_job.thread,NULL); sample_free(ui.sampler_job.input); sample_free(ui.sampler_job.result); ui.sampler_job.busy=0; }
    audio_channels(&next,processed);
    for(int c=0;c<CHANNELS;c++) {
        if(keep[c]) continue;
        sample_free(ui.samples[c]); sample_free(ui.originals[c]); ui.samples[c]=processed[c]; ui.originals[c]=fresh[c];
        ui.sample_generation[c]++; ui.sampler_generation[c]=ui.sample_generation[c]; ui.sampler_applied[c]=next.sampler[c];
    }
    ui.project=next; history_step(&ui.edit_history,direction);
    uint64_t stamps[CHANNELS]; history_stamps(stamps); history_rebind(&ui.edit_history,ui.originals,stamps);
    ui.pattern=fmaxf(0,fminf(ui.pattern,ui.project.pattern_count-1)); ui.channel=fmaxf(0,fminf(ui.channel,ui.project.channel_count-1));
    ui.piano_channel=ui.instrument_channel=ui.channel; ui.automation_selected=ui.automation_node=-1; ui.picker_drag=-1;
    ui.note_drag=NULL; ui.control_drag=NULL; ui.menu_value=NULL; ui.context_kind=0;
    memset(ui.note_selected,0,sizeof ui.note_selected); memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected);
    ui.arrangement.source_pattern=-1; ui.arrangement.source_steps=STEPS; ui.arrangement.source_offset=0;
    ui.rack_scroll=0; ui.picker_scroll=0; ui.piano_gesture=PIANO_IDLE;
    snprintf(ui.status,sizeof ui.status,direction<0?"Undone.":"Redone."); return;
failed:
    for(int c=0;c<CHANNELS;c++) if(!keep[c]) { sample_free(fresh[c]); sample_free(processed[c]); }
    snprintf(ui.status,sizeof ui.status,"Undo/redo unchanged: could not prepare audio.");
}


int preset_commit(const char *chosen,int save) {
    DevicePreset preset={.kind=ui.preset_kind}; int owner=ui.preset_owner,slot=ui.preset_slot;
    if(ui.preset_kind==PRESET_CHORUS || ui.preset_kind==PRESET_EQ) {
        if(owner<0 || owner>ui.project.insert_count || slot<0 || slot>=EFFECT_SLOTS || ui.project.effect_type[owner][slot]!=(ui.preset_kind==PRESET_EQ?EFFECT_EQ:EFFECT_CHORUS)) return 0;
        preset.eq=ui.project.eq[owner][slot]; preset.chorus=ui.project.chorus[owner][slot]; preset.mix=ui.project.effect_mix[owner][slot];
    } else {
        if(owner<0 || owner>=ui.project.channel_count || (ui.preset_kind==PRESET_FM)!=(ui.project.instrument[owner]==INSTRUMENT_FM)) return 0;
        preset.fm=ui.project.fm[owner]; preset.sampler=ui.project.sampler[owner];
        snprintf(preset.sample_path,sizeof preset.sample_path,"%s",ui.project.paths[owner]);
    }
    if(save) {
        int ok=preset_save(chosen,&preset); snprintf(ui.status,sizeof ui.status,ok?"Preset saved: %.180s":"Cannot save preset: %.180s",chosen); return ok;
    }
    if(!preset_load(chosen,&preset) || preset.kind!=ui.preset_kind) { snprintf(ui.status,sizeof ui.status,"Invalid preset or wrong device type"); return 0; }
    if(ui.preset_kind==PRESET_FM) ui.project.fm[owner]=preset.fm;
    else if(ui.preset_kind==PRESET_EQ) { ui.project.eq[owner][slot]=preset.eq; ui.project.effect_mix[owner][slot]=preset.mix; ui.project.effect_bypass[owner][slot]=0; }
    else if(ui.preset_kind==PRESET_CHORUS) { ui.project.chorus[owner][slot]=preset.chorus; ui.project.effect_mix[owner][slot]=preset.mix; ui.project.effect_bypass[owner][slot]=0; }
    else {
        Sample raw={0},processed={0}; int ok;
        if(!sampler_flush()) return 0;
        if(!strcmp(preset.sample_path,SAMPLE_EMPTY)) ok=1;
        else if(preset.sample_path[0]) ok=sample_load(preset.sample_path,&raw);
        else ok=sample_clone(ui.originals[owner],&raw);
        if(!ok || !sample_process(raw,preset.sampler,&processed)) {
            sample_free(raw); sample_free(processed); snprintf(ui.status,sizeof ui.status,"Preset sample missing or processing failed; channel unchanged"); return 0;
        }
        Sample old=ui.samples[owner],source=ui.originals[owner];
        audio_sample(owner,processed); ui.samples[owner]=processed; ui.originals[owner]=raw;
        sample_free(old); sample_free(source); ui.project.sampler[owner]=preset.sampler;
        if(preset.sample_path[0]) snprintf(ui.project.paths[owner],sizeof ui.project.paths[owner],"%s",preset.sample_path);
        ui.sample_generation[owner]++; ui.sampler_generation[owner]=ui.sample_generation[owner]; ui.sampler_applied[owner]=preset.sampler;
        sample_duration(&ui.project,owner,processed,raw);
    }
    snprintf(ui.status,sizeof ui.status,"Loaded preset: %.150s",GetFileNameWithoutExt(chosen)); return 1;
}

int project_file_commit(const char *chosen) {
    if(ui.file_action==8 || ui.file_action==9) return preset_commit(chosen,ui.file_action==8);
    int ok;
    if(ui.file_action==4) return load_project(chosen);
    if(ui.file_action==7) {
        if(ui.relink_channel<0 || ui.relink_channel>=ui.project.channel_count) return 0;
        char name[PATTERN_NAME]; memcpy(name,ui.project.channel_names[ui.relink_channel],sizeof name);
        int loaded=import_sample(chosen,ui.relink_channel);
        if(loaded) memcpy(ui.project.channel_names[ui.relink_channel],name,sizeof name);
        return loaded;
    }
    if(ui.file_action==5) {
        ok=sampler_flush() && export_wav(chosen,&ui.project,ui.samples);
        if(ok) snprintf(ui.document.export_path,sizeof ui.document.export_path,"%s",chosen);
        snprintf(ui.status,sizeof ui.status,ok?"Exported %.220s":"Export failed: %.220s",chosen); return ok;
    }
    ok=sampler_flush() && project_save_assets(chosen,&ui.project,ui.originals,ui.file_action==6);
    if(ok) project_document_saved(&ui.document,&ui.project,chosen);
    snprintf(ui.status,sizeof ui.status,ok?"Saved %.220s":"Save failed: %.220s",chosen); return ok;
}
