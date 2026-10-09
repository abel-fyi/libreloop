// SPDX-License-Identifier: GPL-3.0-only
#include "project_assets.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
static Project project,loaded;
int main(void) {
    char root[]="/tmp/libreloop-assets-XXXXXX",directory[PATH_MAX],moved[PATH_MAX],path[PATH_MAX],audio[PATH_MAX];
    CHECK(mkdtemp(root)); snprintf(directory,sizeof directory,"%s/original",root); CHECK(!mkdir(directory,0700));
    snprintf(moved,sizeof moved,"%s/moved",root); snprintf(path,sizeof path,"%s/session.llp",directory);
    project_new(&project); project.channel_audio[0]=1; snprintf(project.paths[0],sizeof project.paths[0],"/old/machine/take.wav");
    float pcm[]={.25f,-.5f,1,-1}; Sample originals[CHANNELS]={{.data=pcm,.frames=2,.channels=2}};
    CHECK(project_save_assets(path,&project,originals,1)); CHECK(project_load(path,&loaded));
    CHECK(loaded.paths[0][0]!='/' && strstr(loaded.paths[0],".samples-") && project.paths[0][0]=='/' && strstr(project.paths[0],".samples-"));
    CHECK(project_sample_path(path,loaded.paths[0],audio,sizeof audio)); Sample sample;
    CHECK(sample_map_wav(audio,&sample) && sample.frames==2 && !memcmp(sample.data,pcm,sizeof pcm)); sample_free(sample);
    CHECK(!rename(directory,moved)); snprintf(path,sizeof path,"%s/session.llp",moved);
    CHECK(project_sample_path(path,loaded.paths[0],audio,sizeof audio) && sample_map_wav(audio,&sample)); sample_free(sample);
    /* Saving references outside the project also rebases them relative to its directory. */
    snprintf(project.paths[0],sizeof project.paths[0],"%s",audio);
    CHECK(project_save_assets(path,&project,originals,0)); CHECK(project_load(path,&loaded)); CHECK(loaded.paths[0][0]!='/');
    Sample fresh[CHANNELS],processed[CHANNELS]; int missing=-1;
    CHECK(project_load_assets(path,&loaded,fresh,processed,sample_map_wav,&missing) && !missing);
    CHECK(fresh[0].storage && processed[0].storage==fresh[0].storage && !memcmp(processed[0].data,pcm,sizeof pcm));
    for(int c=0;c<CHANNELS;c++) { sample_free(fresh[c]); sample_free(processed[c]); }
    CHECK(!unlink(audio));
    /* Missing references survive ordinary saves; collect fails without replacing the project. */
    originals[0]=(Sample){0}; CHECK(project_save_assets(path,&project,originals,0));
    CHECK(project_load_assets(path,&loaded,fresh,processed,sample_map_wav,&missing) && missing==1 && !fresh[0].frames && loaded.paths[0][0]);
    for(int c=0;c<CHANNELS;c++) { sample_free(fresh[c]); sample_free(processed[c]); }
    CHECK(!project_save_assets(path,&project,originals,1)); CHECK(project_load(path,&loaded) && loaded.paths[0][0]);
    char *slash=strrchr(audio,'/'); CHECK(slash); *slash=0; CHECK(!rmdir(audio));
    unlink(path); CHECK(!rmdir(moved) && !rmdir(root));
    puts("Collected original stereo audio and relative references survive moving projects and preserve missing references."); return 0;
}
