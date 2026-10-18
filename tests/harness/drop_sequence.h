// SPDX-License-Identifier: GPL-3.0-only
static int load_calls,finish_calls,sample_commands,channel_commands;
static FILE *trace,*selection;
static int sample_load_fixture(const char *path,Sample *sample){load_calls++;if(strstr(path,"bad"))return 0;*sample=(Sample){.data=malloc(48000*2*sizeof(float)),.frames=48000,.channels=2};if(!sample->data)return 0;for(unsigned i=0;i<48000;i++){sample->data[i*2]=(int)(i%17)*.001f;sample->data[i*2+1]=-(int)(i%23)*.001f;}return 1;}
static void seed(int mode){
    project_new(&ui.project);ui.project.insert_count=INSERTS; /* Frozen reference project geometry. */ui.project.channel_count=2;ui.project.pattern_count=2;
    windows_init(&ui.windows,1200,675);for(int i=0;i<EDITORS;i++)ui.windows.editors[i].visible=0;
    ui.arrangement.zoom=1;ui.arrangement.source_pattern=-1;ui.track_zoom=1;ui.browser_focus=1;ui.rack_paint=-1;
    ui.project.notes[0][0][0]=(Note){60,90,0,2};
    windows_focus(&ui.windows,mode>=6?mode<9?0:4:1);
    int id=ui.windows.focused;Rect r=ui.windows.editors[id].rect;
    ui.mouse=(POINT){r.x+300,r.y+150};
    if(mode==1)ui.mouse.x=r.x+8;
    if(mode==2)ui.mouse.y=r.y+TITLE-1;
    if(mode==3){ui.project.channel_audio[0]=1;ui.project.audio_seconds[0]=2;ui.project.clips[0][0]=PATTERNS+1;ui.project.clip_starts[0][0]=0;ui.project.clip_steps[0][0]=0;ui.mouse=(POINT){r.x+214,r.y+100};}
    if(mode==4){ui.project.clips[0][0]=1;ui.project.clip_starts[0][0]=0;ui.project.clip_steps[0][0]=16;ui.mouse=(POINT){r.x+214,r.y+100};}
    if(mode==5)ui.project.channel_count=CHANNELS;
    if(mode==6)ui.mouse=(POINT){r.x+210,r.y+40};
    if(mode==7)ui.mouse=(POINT){r.x+210,r.y+r.h-10};
    if(mode==8)ui.mouse=(POINT){r.x+210,r.y+TITLE-1};
    if(mode==9){ui.project.instrument[0]=INSTRUMENT_FM;automation_create(&ui.project,(ParameterTarget){PARAM_FM_DEPTH,0,0},"FM",16);automation_create(&ui.project,(ParameterTarget){PARAM_CHANNEL_VOLUME,0,0},"Volume",16);}
    if(mode==10)ui.mouse=(POINT){r.x+50,r.y+5};
#ifdef DROP_SETUP_EXTRA
    DROP_SETUP_EXTRA(mode);
#endif
}
static int run(int mode){seed(mode);drop_sample(mode==11?"bad.wav":"tone.wav");
    trace=fopen("state.txt","w");selection=fopen("selection.bin","wb");if(!trace || !selection)return 1;
    fprintf(trace,"%d %d %d %d %d %d %d %d %d %d %.9g %.9g %d %d %s\n",load_calls,finish_calls,sample_commands,channel_commands,ui.channel,ui.instrument_channel,ui.rack_filter,ui.rack_scroll,(ui.arrangement.source_pattern>=PATTERNS?ui.arrangement.source_pattern-(PATTERNS-LEGACY_PATTERNS):ui.arrangement.source_pattern),ui.picker_tab,ui.arrangement.source_steps,ui.arrangement.source_offset,ui.windows.focused,ui.browser_focus,ui.status);
    for(int c=0;c<CHANNELS;c++)fprintf(trace,"%u %u %u %u %.9g\n",ui.sample_generation[c],ui.sampler_generation[c],ui.originals[c].frames,ui.samples[c].frames,ui.project.audio_seconds[c]);
    fwrite(ui.arrangement.selected,sizeof ui.arrangement.selected,1,selection);fclose(trace);fclose(selection);
    FILE *pcm=fopen("samples.f32","wb");if(!pcm)return 1;for(int c=0;c<CHANNELS;c++)if(ui.samples[c].frames)fwrite(ui.samples[c].data,8,ui.samples[c].frames,pcm);fclose(pcm);
    int ok=project_save("expected.llp",&ui.project);for(int c=0;c<CHANNELS;c++){sample_free(ui.samples[c]);sample_free(ui.originals[c]);}return ok?0:1;
}
