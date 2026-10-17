// SPDX-License-Identifier: GPL-3.0-only
#include "preset.h"
#include "engine.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(void) {
    char root[1024]="/tmp/libreloop-presets-XXXXXX",canonical[PATH_MAX],path[1024],audio[1024],moved[1024]; CHECK(mkdtemp(root));
    CHECK(realpath(root,canonical)); snprintf(root,sizeof root,"%s",canonical);
    snprintf(path,sizeof path,"%s/patch.llpreset",root); snprintf(audio,sizeof audio,"%s/sample.wav",root);
    FILE *f=fopen(audio,"wb"); CHECK(f); fclose(f);
    DevicePreset preset={.kind=PRESET_SAMPLER,.sampler={.time=1,.length=1,.pitch=3,.flags=SAMPLE_REVERSE}};
    snprintf(preset.sample_path,sizeof preset.sample_path,"%s",audio);
    CHECK(preset_save(path,&preset)); DevicePreset loaded={0}; CHECK(preset_load(path,&loaded));
    CHECK(loaded.kind==PRESET_SAMPLER && loaded.sampler.pitch==3 && !strcmp(loaded.sample_path,audio));
    snprintf(moved,sizeof moved,"%s-moved",root); CHECK(!rename(root,moved)); snprintf(path,sizeof path,"%s/patch.llpreset",moved);
    CHECK(preset_load(path,&loaded)); snprintf(audio,sizeof audio,"%s/sample.wav",moved); CHECK(!strcmp(loaded.sample_path,audio));
    preset=(DevicePreset){.kind=PRESET_FM,.fm=fm_default()}; preset.fm.ratio=3; preset.fm.depth=4;
    CHECK(preset_save(path,&preset) && preset_load(path,&loaded)); CHECK(loaded.kind==PRESET_FM && loaded.fm.ratio==3 && loaded.fm.depth==4);
    preset=(DevicePreset){.kind=PRESET_CHORUS,.chorus={1.2f,5},.mix=.3f};
    CHECK(preset_save(path,&preset) && preset_load(path,&loaded)); CHECK(loaded.kind==PRESET_CHORUS && loaded.mix==.3f && loaded.chorus.depth==5);
    preset.chorus.depth=NAN; CHECK(!preset_save(path,&preset)); CHECK(preset_load(path,&loaded) && loaded.chorus.depth==5);
    f=fopen(path,"w"); CHECK(f); fputs("LIBRELOOP_PRESET 2 2\n3 4 .02 .5 .4 .3\n",f); fclose(f);
    CHECK(preset_load(path,&loaded) && loaded.fm.mod_sustain==1 && loaded.fm.vibrato==0 && loaded.fm.tremolo==0);
    DevicePreset before=loaded; f=fopen(path,"w"); CHECK(f); fputs("LIBRELOOP_PRESET 1 2\n2 nan .1 .2 .3 .4\n",f); fclose(f);
    CHECK(!preset_load(path,&loaded) && !memcmp(&before,&loaded,sizeof loaded));
    f=fopen(path,"w"); CHECK(f); fputs("LIBRELOOP_PRESET 99 3\n1 3 .5\n",f); fclose(f); CHECK(!preset_load(path,&loaded));
    CHECK(!unlink(path) && !unlink(audio) && !rmdir(moved));
    puts("All device presets, relative sample references, atomic saves and malformed input checks passed."); return 0;
}
