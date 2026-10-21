// SPDX-License-Identifier: GPL-3.0-only
static void snapshot(FILE *f,const char *name){fprintf(f,"%s %d %d %d %d %d %d %d %d %d %d %.9g %.17g %d %d %d %d %d %d %d %s\n",name,ui.project.channel_count,ui.project.pattern_count,ui.recording_ui.active,ui.recording_ui.count,ui.midi_take.active,ui.midi_take.pattern,ui.pattern,ui.channel,ui.windows.focused,ui.browser_focus,ui.playlist_start,ui.midi_take.time,ui.playing,ui.song,ui.reset,checkpoint_calls,record_starts,record_ends,record_modes,ui.status);}
static int run_recording(int number){mode=number;project_new(&ui.project);ui.project.channel_count=2;ui.project.pattern_count=2;ui.project.insert_count=2;ui.playlist_start=.5f;ui.output_volume=1;ui.browser_focus=1;ui.arrangement.source_pattern=0;ui.arrangement.source_steps=8;windows_init(&ui.windows,1200,675);
    ui.record_mask=mode==0?0:mode==1 || mode==2?2:mode==3?4:mode==4?1:mode==14?7:1;ui.midi_connected=mode!=1;
    if(mode>=5)ui.record_armed[0]=1;if(mode==5)ui.project.channel_count=CHANNELS;if(mode==14)ui.record_armed[1]=1;
    if(mode==2){ui.midi_keys[0]=(MidiHeld){1,1,0,64,0,90};}
    if(mode==15){ui.playing=ui.song=1;visual_frame=RATE*2;}
    FILE *out=fopen("state.txt","wb");if(!out)return 1;recording_start();snapshot(out,"start");
    if(!project_save("started.llp",&ui.project))return 2;
    if(ui.midi_take.active){fixture_time=10.5;midi_take_note(&ui.midi_take,&ui.project,0,67,100,fixture_time);midi_take_control(&ui.midi_take,&ui.project,(ParameterTarget){PARAM_CHANNEL_VOLUME,0,0},.7f,fixture_time);fixture_time=11;}
    recording_finish();snapshot(out,"finish");fprintf(out,"calls %d %d %d %d\n",publications,updates,panic_calls,popup);fclose(out);
    FILE *samples=fopen("samples.f32","wb");if(!samples)return 1;for(int c=0;c<CHANNELS;c++)if(ui.samples[c].frames)fwrite(ui.samples[c].data,8,ui.samples[c].frames,samples);fclose(samples);
    int ok=project_save("finished.llp",&ui.project);for(int c=0;c<CHANNELS;c++){sample_free(ui.samples[c]);sample_free(ui.originals[c]);}return ok?0:2;}
