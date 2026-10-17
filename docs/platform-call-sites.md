# Platform assumption call sites

Lexical audit companion to platform-assumptions.md; includes header requirements, paths, compile-time branches, OS and POSIX calls. Each line retains original path/line. This is a source audit, not Windows validation.

```c
app_midi.c:19: #ifndef __APPLE__
app_midi.c:24: #ifndef __APPLE__
app_midi.c:55:     midi_input_wake(glfwPostEmptyEvent);char error[256];midi_input_open("",error);midi_refresh();
app_project.c:28:     atomic_store(&ui.sampler_job.done,1); glfwPostEmptyEvent(); return NULL;
app_project.c:33:         pthread_join(ui.sampler_job.thread,NULL); int c=ui.sampler_job.channel;
app_project.c:47:         pthread_join(ui.sampler_job.thread,NULL); sample_free(ui.sampler_job.input); sample_free(ui.sampler_job.result); ui.sampler_job.busy=0;
app_project.c:67:         if(pthread_create(&ui.sampler_job.thread,NULL,sampler_worker,NULL)) { sample_free(copy); return; }
app_project.c:76:     char full[PATH_MAX]; Sample s;
app_project.c:77:     if(!realpath(path,full) || strlen(full)>=sizeof ui.project.paths[c] || !sample_load(full,&s)) { snprintf(ui.status,sizeof ui.status,"Cannot load sample: unreadable, unsupported, too large or insufficient memory."); return 0; }
app_project.c:303:     char filename[PATH_MAX]; snprintf(filename,sizeof filename,"%s",path);
app_project.c:352:     if(ui.sampler_job.busy) { pthread_join(ui.sampler_job.thread,NULL); sample_free(ui.sampler_job.input); sample_free(ui.sampler_job.result); ui.sampler_job.busy=0; }
app_recording.c:57:     char directory[PATH_MAX]; if(!realpath("recordings",directory)) { snprintf(ui.status,sizeof ui.status,"Cannot open recordings/ in the working directory."); return; }
atomic_file.c:6: #include <sys/stat.h>
atomic_file.c:12:     int fd=mkstemp(out->temporary); if(fd<0) return 0;
atomic_file.c:13:     struct stat info;
atomic_file.c:14:     if(!stat(path,&info)) fchmod(fd,info.st_mode&0777);
atomic_file.c:15:     out->file=fdopen(fd,"wb");
atomic_file.c:16:     if(!out->file) { close(fd); unlink(out->temporary); return 0; }
atomic_file.c:21:     if(out->temporary[0]) unlink(out->temporary);
atomic_file.c:27:     if(fflush(out->file) || fsync(fileno(out->file))) ok=0;
atomic_file.h:6: typedef struct { FILE *file; char temporary[PATH_MAX],destination[PATH_MAX]; } AtomicFile;
audio.c:30:     unsigned read,write;
audio.c:50: static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
audio.c:78: typedef struct { float pcm[ANALYZER_FRAMES][2]; atomic_uint read,write; } AnalyzerRing;
audio.c:84:     AnalyzerRing *ring=&analyzer[stream]; unsigned w=atomic_load_explicit(&ring->write,memory_order_relaxed);
audio.c:87:     atomic_store_explicit(&ring->write,w+1,memory_order_release);
audio.c:93:     unsigned n=atomic_load_explicit(&ring->write,memory_order_acquire)-r; if(n>capacity) n=capacity;
audio.c:100:     atomic_uint_fast64_t read,write;
audio.c:110:     uint64_t w=atomic_load_explicit(&ring->write,memory_order_relaxed);
audio.c:116:     atomic_store_explicit(&ring->write,w+1,memory_order_release); return 1;
audio.c:120:     if(r==atomic_load_explicit(&ring->write,memory_order_acquire)) return 0;
audio.c:143:     struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
audio.c:150:     if(!pthread_mutex_trylock(&mutex)) {
audio.c:152:         pthread_mutex_unlock(&mutex);
audio.c:221:     if(!pthread_mutex_trylock(&mutex)) {
audio.c:223:         pthread_mutex_unlock(&mutex);
audio.c:262:     while(pending.read!=pending.write) {
audio.c:355:     struct timespec pause={0,100000}; nanosleep(&pause,NULL);
audio.c:369:     while(pending.write-pending.read==AUDIO_COMMANDS) {
audio.c:371:         pthread_mutex_unlock(&mutex); wait_control(); pthread_mutex_lock(&mutex);
audio.c:374:     pending.commands[pending.write++%AUDIO_COMMANDS]=command;
audio.c:381:             pthread_mutex_lock(&mutex); consume_stopped(); pthread_mutex_unlock(&mutex);
audio.c:450:             snprintf(error,256,"Cannot open input %.120s. Check microphone permission and device availability.",name); goto failed;
audio.c:481:     pthread_mutex_lock(&mutex);
audio.c:485:     pthread_mutex_unlock(&mutex);
audio.c:489:     pthread_mutex_lock(&mutex);
audio.c:491:     pthread_mutex_unlock(&mutex); wait_acknowledged(serial);
audio.c:495:     pthread_mutex_lock(&mutex); pending.dirty|=project_snapshot_update(&pending.snapshot,p,effects!=NULL,recording.active?&recording.io:NULL);
audio.c:497:     pthread_mutex_unlock(&mutex); wait_acknowledged(serial);
audio.c:500:     pthread_mutex_lock(&mutex);
audio.c:502:     pthread_mutex_unlock(&mutex); wait_acknowledged(serial);
audio.c:505:     pthread_mutex_lock(&mutex); submit((AudioCommand){.kind=NOTE,.channel=c,.note=n}); pthread_mutex_unlock(&mutex);
audio.c:510:     pthread_mutex_lock(&mutex);
audio.c:512:     pthread_mutex_unlock(&mutex);
audio.c:515:     pthread_mutex_lock(&mutex); uint64_t serial=submit((AudioCommand){.kind=STOP}); pthread_mutex_unlock(&mutex);
audio.c:519:     pthread_mutex_lock(&mutex);
audio.c:525:     pthread_mutex_unlock(&mutex); return progress;
audio.c:529:     pthread_mutex_lock(&mutex);
audio.c:533:     for(unsigned i=pending.read;i!=pending.write;i++) {
audio.c:547:     pthread_mutex_unlock(&mutex); return progress;
audio.c:561:     pthread_mutex_lock(&mutex);
audio.c:564:     pthread_mutex_unlock(&mutex);
audio.c:574:     pthread_mutex_lock(&mutex);
audio.c:582:     pthread_mutex_unlock(&mutex); return frame;
audio.c:589:     pthread_mutex_lock(&mutex); consume_commands(); publish_view(); pthread_mutex_unlock(&mutex);
automation.c:80:         if((a->target.parameter<PARAM_PLUGIN && a->target.slot) || (((a->target.parameter>=PARAM_MASTER_VOLUME && a->target.parameter<=PARAM_MASTER_PITCH) || a->target.parameter==PARAM_MASTER_MUTE || a->target.parameter==PARAM_SWING) && a->target.owner) || !a->target.parameter || (!parameter_info(p,a->target,&v,&lo,&hi) && a->target.parameter<PARAM_PLUGIN) || a->target.slot>65535 || a->target.owner>65535 || a->color>0xffffff || !a->name[0] || strnlen(a->name,sizeof a->name)==sizeof a->name || strchr(a->name,'\n') || strchr(a->name,'\r') || !isfinite(a->steps) || a->steps<=0 || a->steps>1e15f || a->count<1 || a->count>AUTOMATION_POINTS) return 0;
browser.c:20:     if(b->nodes[entry].open) {
browser.c:27:         b->items-=removed; b->nodes[entry].open=0; return 1;
browser.c:35:         char *path=strdup(files.paths[i]); if(!path) { ok=0; break; }
browser.c:45:     b->items+=n; nodes[entry].open=1;
browser.c:51:     if(b->nodes[i].dir && b->nodes[i].open) { browser_toggle(b,i); return; }
browser.c:57:     if(!b->nodes[i].open) browser_toggle(b,i);
browser.c:61:     char *copy=strdup(path); if(!copy) return -1;
browser.c:68:     const char *home=getenv("HOME"),*xdg=getenv("XDG_CONFIG_HOME"); char base[PATH_MAX],dir[PATH_MAX];
browser.c:76:     if(!realpath(samples,b->samples)) snprintf(b->samples,sizeof b->samples,"%s",samples);
browser.c:81:         char line[PATH_MAX];
browser.c:84:             if(*line && strcmp(line,b->samples) && root_add(b,line)>=0) snprintf(b->folders[b->count++],PATH_MAX,"%s",line);
browser.c:90:     char full[PATH_MAX];
browser.c:91:     if(!realpath(path,full) || !DirectoryExists(full) || strchr(full,'\n')) return 0;
browser.c:93:         browser_select(b,i); if(!b->nodes[i].open) browser_toggle(b,i); return 1;
browser.c:101:     snprintf(b->folders[b->count++],PATH_MAX,"%s",full);
browser.h:7: typedef struct { char *path; int depth,dir,open; } BrowserNode;
browser.h:9:     char path[PATH_MAX],samples[PATH_MAX],folders[FOLDERS][PATH_MAX],config[PATH_MAX];
dx7.c:22: static pthread_once_t initialized=PTHREAD_ONCE_INIT;
dx7.c:59: void dx7_init(void) { pthread_once(&initialized,prepare); }
file_chooser.c:5: #include <sys/stat.h>
file_chooser.c:12: static char recent[FILE_PURPOSES][PATH_MAX],locations_path[PATH_MAX];
file_chooser.c:18:     char line[PATH_MAX+32];
file_chooser.c:22:         if(end!=line && *end=='\t' && end[1]=='/' && purpose>=0 && purpose<FILE_PURPOSES && strlen(end+1)<PATH_MAX)
file_chooser.c:23:             snprintf(recent[purpose],PATH_MAX,"%s",end+1);
file_chooser.c:28:     if(purpose<0 || purpose>=FILE_PURPOSES || !folder[0] || strlen(folder)>=PATH_MAX || !strcmp(recent[purpose],folder)) return;
file_chooser.c:29:     snprintf(recent[purpose],PATH_MAX,"%s",folder);
file_chooser.c:37:     char parent[PATH_MAX]; if(strlen(chosen)>=sizeof parent) return;
file_chooser.c:49:     return x->directory!=y->directory?y->directory-x->directory:strcasecmp(x->name,y->name);
file_chooser.c:56:         if(n==length && !strncasecmp(dot+1,at,n)) return 1;
file_chooser.c:63:     char resolved[PATH_MAX];
file_chooser.c:64:     if(!realpath(path,resolved)) { snprintf(c->error,sizeof c->error,"Cannot open folder: %s",strerror(errno)); return 0; }
file_chooser.c:65:     DIR *dir=opendir(resolved);
file_chooser.c:69:         errno=0; entry=readdir(dir);
file_chooser.c:72:         char full[PATH_MAX]; struct stat info;
file_chooser.c:73:         if(snprintf(full,sizeof full,"%s/%s",resolved,entry->d_name)>=(int)sizeof full || stat(full,&info)) continue;
file_chooser.c:74:         int directory=S_ISDIR(info.st_mode);
file_chooser.c:75:         if(!directory && (!S_ISREG(info.st_mode) || !matches(entry->d_name,c->extension))) continue;
file_chooser.c:81:         char *name=strdup(entry->d_name); if(!name) { failed=1; break; }
file_chooser.c:84:     closedir(dir);
file_chooser.c:91:     char path[PATH_MAX]; snprintf(path,sizeof path,"%s",c->directory);
file_chooser.c:97:     char path[PATH_MAX];
file_chooser.c:101:     else { snprintf(c->name,sizeof c->name,"%s",initial); if(!getcwd(path,sizeof path)) return 0; }
file_chooser.c:103:         if(!getcwd(path,sizeof path) || !file_chooser_folder(c,path)) return 0;
file_chooser.c:112:     struct stat info;
file_chooser.c:113:     if(!stat(path,&info) && S_ISDIR(info.st_mode)) return file_chooser_folder(c,path)?0:-1;
file_chooser.c:119:     char parent[PATH_MAX],resolved[PATH_MAX];
file_chooser.c:124:     if(!realpath(parent,resolved) || access(resolved,c->save?W_OK:R_OK)) { snprintf(c->error,sizeof c->error,"Folder is unavailable or access is denied."); return -1; }
file_chooser.c:125:     const char *name=strrchr(path,'/')+1; char final[PATH_MAX];
file_chooser.c:128:     if(!stat(path,&info)) {
file_chooser.c:129:         if(!S_ISREG(info.st_mode)) { snprintf(c->error,sizeof c->error,"Choose a regular file."); return -1; }
file_chooser.c:130:         if(!c->save && (!matches(path,c->extension) || access(path,R_OK))) { snprintf(c->error,sizeof c->error,"Choose a readable .%s file.",c->extension); return -1; }
file_chooser.h:8:     char directory[PATH_MAX],name[PATH_MAX],extension[16],error[160];
macos_input.m:2: #import <Cocoa/Cocoa.h>
macos_input.m:5: #include <GLFW/glfw3.h>
macos_input.m:6: #include <GLFW/glfw3native.h>
macos_input.m:12:     NSWindow *target=glfwGetCocoaWindow((GLFWwindow *)window);
main.c:14: #ifdef __APPLE__
main.c:17:         const char *home=getenv("HOME"); char directory[PATH_MAX];
main.c:20:             fprintf(stderr,"Cannot open a writable LibreLoop working directory.\n"); return 1;
main.c:27: #ifdef __APPLE__
main.c:42:     ui.app_window=glfwGetCurrentContext();
main.c:43:     ui.raylib_mouse_callback=glfwSetMouseButtonCallback(ui.app_window,mouse_callback);
main.c:44: #ifdef __APPLE__
main.c:47:     ui.resize_cursor=glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
main.c:48:     ui.track_cursor=glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
main.c:49:     ui.hand_cursor=glfwCreateStandardCursor(GLFW_HAND_CURSOR);
main.c:53:             glfwSetWindowShouldClose(ui.app_window,GLFW_FALSE);
main.c:62: #ifdef __APPLE__
main.c:74:         glfwGetFramebufferSize(ui.app_window,&framebuffer_width,&framebuffer_height);
main.c:75:         glfwGetWindowSize(ui.app_window,&window_width,&window_height);
main.c:140: #ifndef __APPLE__
main.c:410:                     if(node->open) DrawTriangle((Vector2){indent,y+8},(Vector2){indent+4,y+13},(Vector2){indent+8,y+8},color);
main.c:415:                 if(hover(8,y,sidebar-16,21)) snprintf(ui.status,sizeof ui.status,"%.80s: %s | j/k: select; h/l: fold",path,node->dir?"click to expand/collapse":IsFileExtension(path,PROJECT_FILE_SUFFIX)?"click to open project":"click to preview; drag to Rack or Playlist");
main.c:474:     if(ui.sampler_job.busy) { pthread_join(ui.sampler_job.thread,NULL); sample_free(ui.sampler_job.input); sample_free(ui.sampler_job.result); }
main.c:476:     #ifdef __APPLE__
main.c:480:     browser_close(&ui.browser); audio_close(); sample_free(ui.audition); free(ui.audition_wave.tree); if(ui.resize_cursor) glfwDestroyCursor(ui.resize_cursor); if(ui.track_cursor) glfwDestroyCursor(ui.track_cursor); if(ui.hand_cursor) glfwDestroyCursor(ui.hand_cursor); CloseWindow();
midi_input.c:15: double midi_input_time(void) {struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec/1e9;}
midi_input.c:30: #ifdef __APPLE__
midi_input.c:31: #include <CoreMIDI/CoreMIDI.h>
midi_input.c:53:     midi_input_close();if(!prepare()){snprintf(error,256,"CoreMIDI is unavailable");return 0;}
midi_input.c:61: #include <alsa/asoundlib.h>
midi_input.c:63: #include <poll.h>
midi_input.c:65: static snd_seq_t *seq;static int port=-1;static pthread_t worker;static atomic_int running;static int started;
midi_input.c:82:         poll(fds,count,50);snd_seq_event_t *e;int result;
midi_input.c:97:     if(started){atomic_store(&running,0);pthread_join(worker,NULL);}started=0;
midi_input.c:106:     atomic_store(&running,1);if(pthread_create(&worker,NULL,receive,NULL)){midi_input_close();snprintf(error,256,"Cannot start MIDI input");return 0;}started=1;return 1;
navigation.h:7: #ifdef __APPLE__
preset.c:13:     if(p->kind==PRESET_SAMPLER) return strnlen(p->sample_path,sizeof p->sample_path)<sizeof p->sample_path && !strchr(p->sample_path,'\n') && !strchr(p->sample_path,'\r');
project_assets.c:8: #include <sys/stat.h>
project_assets.c:10: static int project_directory(const char *path,char directory[PATH_MAX]) {
project_assets.c:11:     char parent[PATH_MAX]; if(strlen(path)>=sizeof parent) return 0;
project_assets.c:14:     return realpath(parent,directory)!=NULL;
project_assets.c:17:     char directory[PATH_MAX];
project_assets.c:36:     char directory[PATH_MAX],canonical[PATH_MAX];
project_assets.c:38:     return relative_path(directory,realpath(absolute,canonical)?canonical:absolute,out,capacity);
project_assets.c:63:     char directory[PATH_MAX],asset_dir[PATH_MAX]={0},created[CHANNELS][PATH_MAX]={{0}};
project_assets.c:69:         if(snprintf(asset_dir,sizeof asset_dir,"%s/%s.samples-XXXXXX",directory,name)>=(int)sizeof asset_dir || !mkdtemp(asset_dir)) ok=0;
project_assets.c:77:             char absolute[PATH_MAX];
project_assets.c:79:             else if(!realpath(project->paths[c],absolute)) { ok=0; break; }
project_assets.c:88:     if(!ok && asset_dir[0]) { for(int c=0;c<CHANNELS;c++) if(created[c][0]) unlink(created[c]); rmdir(asset_dir); }
project_assets.c:100:             char resolved[PATH_MAX]; Sample decoded={0};
project_document.h:8:     char path[PATH_MAX],export_path[PATH_MAX],replacement_path[PATH_MAX];
project_io.c:35:     for(int id=0;id<=INSERTS;id++) for(int io=0;io<2;io++) if(strnlen(p->audio_io[id][io],128)==128 || strchr(p->audio_io[id][io],'\n') || strchr(p->audio_io[id][io],'\r')) return 0;
project_io.c:37:     for(int l=0;l<LANES;l++) if(!p->track_names[l][0] || strnlen(p->track_names[l],PATTERN_NAME)==PATTERN_NAME || strchr(p->track_names[l],'\n') || strchr(p->track_names[l],'\r')) return 0;
project_io.c:38:     for(int i=0;i<INSERTS;i++) if(!p->insert_names[i][0] || strnlen(p->insert_names[i],PATTERN_NAME)==PATTERN_NAME || strchr(p->insert_names[i],'\n') || strchr(p->insert_names[i],'\r')) return 0;
recording.h:12:     char path[PATH_MAX];
recording_writer.c:14:     pthread_t thread;
recording_writer.c:18:     char path[PATH_MAX];
recording_writer.c:28:         ssize_t n=pwrite(w->fd,data+at,sizeof data-at,(off_t)at);
recording_writer.c:38:     w->fd=mkstemps(w->path,4);
recording_writer.c:41:     if(!header(w,0) || lseek(w->fd,44,SEEK_SET)!=44) { close(w->fd); unlink(w->path); free(w); return NULL; }
recording_writer.c:52:             struct timespec pause={0,5000000}; nanosleep(&pause,NULL); continue;
recording_writer.c:60:             ssize_t written=write(w->fd,bytes+at,total-at);
recording_writer.c:69:     if(ftruncate(w->fd,44+(off_t)frames*8) || !header(w,frames) || fsync(w->fd)) atomic_store(&w->failed,1);
recording_writer.c:76:     if(pthread_create(&w->thread,NULL,worker,w)) return 0;
recording_writer.c:82:     if(w->started) { atomic_store(&w->stop,1); pthread_join(w->thread,NULL); w->started=0; }
recording_writer.c:83:     if(w->fd>=0) { if(close(w->fd)) atomic_store(&w->failed,1); w->fd=-1; }
recording_writer.c:88:     recording_writer_finish(w); if(remove_file) unlink(w->path); free(w);
sample_storage.c:8: #include <sys/stat.h>
sample_storage.c:15:         munmap(sample.storage->mapping,sample.storage->bytes); free(sample.storage);
sample_storage.c:35:     int fd=open(path,O_RDONLY); if(fd<0) return 0;
sample_storage.c:36:     struct stat info;
sample_storage.c:37:     if(fstat(fd,&info) || info.st_size<(off_t)bytes) { close(fd); return 0; }
sample_storage.c:38:     void *mapping=mmap(NULL,bytes,PROT_READ,MAP_SHARED,fd,0); close(fd);
sample_storage.c:45:         munmap(mapping,bytes); return 0;
sample_storage.c:48:     if(!storage) { munmap(mapping,bytes); return 0; }
sampler.h:7: /* Per-note DSP state; no project, mixer, UI or file access. Reset specifies
text_fonts.c:11: static char font_path[PATH_MAX];
ui_internal.h:29: #include <GLFW/glfw3.h>
ui_internal.h:79:     char samples_path[PATH_MAX],font_path[PATH_MAX],logo_path[PATH_MAX];
ui_internal.h:94:     char midi_selected[128],midi_selected_name[128],midi_settings_path[PATH_MAX];
ui_internal.h:102:         pthread_t thread; atomic_int done; int busy,channel,ok;
ui_internal.h:149:     char rename_text[PATTERN_NAME],folder_text[PATH_MAX],number_text[64];
ui_internal.h:159:     char audition_path[PATH_MAX];
ui_internal.h:163:     char sample_drag[PATH_MAX];
ui_internal.h:204:     char file_directory[PATH_MAX],file_pending[PATH_MAX];
ui_menus.c:220: #ifdef __APPLE__
ui_menus.c:238: #ifdef __APPLE__
ui_menus.c:240:         help_binding(left,&ly,"Cmd + S / O","Save / open (Shift+S: As)");
ui_menus.c:243:         help_binding(left,&ly,"Ctrl + S / O","Save / open (Shift+S: As)");
ui_menus.c:253: #ifdef __APPLE__
ui_menus.c:262: #ifdef __APPLE__
ui_menus.c:280: #ifdef __APPLE__
ui_menus.c:344:     char initial[PATH_MAX],directory[PATH_MAX];
ui_menus.c:348:         snprintf(ui.status,sizeof ui.status,"Cannot open preset chooser"); return;
ui_menus.c:367:     char initial[PATH_MAX];
ui_menus.c:403:     const char *home=getenv("HOME");
ui_menus.c:458:             char path[PATH_MAX];
ui_rack.c:96:         if(hover(106,row,78,22)) snprintf(ui.status,sizeof ui.status,"%s: open instrument; right-click for Piano Roll, rename, mute or delete; Keys: Z/Q white, S/2 black",ui.project.channel_names[c]);
ui_rack.c:112:             if(hover(200,row+2,gridw,20)) snprintf(ui.status,sizeof ui.status,"%s notes: click to open Piano Roll; right-click for channel options",ui.project.channel_names[c]);
ui_shell.c:5: #ifdef __APPLE__
ui_shell.c:18: #ifdef __APPLE__
ui_shell.c:26: #ifdef __APPLE__
ui_shell.c:53:     glfwSetCursor(ui.app_window,ui.requested_cursor==MOUSE_CURSOR_RESIZE_EW?ui.resize_cursor:ui.requested_cursor==MOUSE_CURSOR_RESIZE_NS?ui.track_cursor:ui.requested_cursor==MOUSE_CURSOR_POINTING_HAND?ui.hand_cursor:NULL);
ui_state.c:42:     .status="Use the toolbar to open editors; drag their title bars to arrange them",
ui_widgets.c:33:     static char fitted[PATH_MAX]; text_fit(fitted,sizeof fitted,text,width,size); return fitted;
ui_widgets.c:298:         else if(!strcmp(text,"Open...")) description="Choose a project to open";
ui_widgets.c:450:     char root[PATH_MAX];
ui_widgets.c:455:         char factory[PATH_MAX];
```
