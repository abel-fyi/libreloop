// SPDX-License-Identifier: GPL-3.0-only
#include "windows_utf.h"
#include "files.h"
#include "file_io.h"
#include "browser_files.h"
#include "atomic_file.h"
#include "recording_writer.h"
#include "project_check.h"
#include <stdatomic.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"Failed line %d: %s (Win32 %lu)\n",__LINE__,#x,GetLastError()); return 1; } } while(0)
static Project original,loaded;
typedef struct { unsigned read; atomic_uint available; } Source;
static unsigned read_audio(void *context,int take,float *out,unsigned count) {
    Source *s=context;(void)take;
    unsigned n=atomic_load(&s->available)-s->read;if(n>count)n=count;
    for(unsigned i=0;i<n;i++){out[i*2]=.25f;out[i*2+1]=-.5f;}
    s->read+=n;return n;
}
int main(void) {
    wchar_t temporary[MAX_PATH];char root[LL_PATH_CAPACITY],path[LL_PATH_CAPACITY],canonical[LL_PATH_CAPACITY];
    CHECK(GetTempPathW(MAX_PATH,temporary));CHECK(windows_utf8(temporary,root,sizeof root));
    size_t n=strlen(root);
    CHECK(snprintf(root+n,sizeof root-n,"libreloop-\xc3\xa9-\xe9\x9f\xb3-XXXXXX")<(int)(sizeof root-n));
    CHECK(platform_asset_directory(root));
    CHECK(platform_join_path(root,"song with spaces.llp",path,sizeof path));
    project_demo(&original);CHECK(project_save(path,&original));CHECK(project_load(path,&loaded));
    CHECK(project_equal(&original,&loaded));
    CHECK(platform_canonical_path(path,canonical,sizeof canonical));CHECK(platform_path_exists(canonical));
    AtomicFile output;CHECK(atomic_file_open(&output,path));
    CHECK(fwrite("discard",1,7,output.file)==7);atomic_file_abort(&output);
    CHECK(project_load(path,&loaded) && project_equal(&original,&loaded));
    CHECK(atomic_file_open(&output,path));CHECK(fwrite("replacement",1,11,output.file)==11);
    CHECK(atomic_file_commit(&output));
    FILE *f=platform_file_open(path,"rb");char text[12]={0};
    CHECK(f && fread(text,1,11,f)==11 && !strcmp(text,"replacement"));CHECK(!fclose(f));
    RecordingWriter *writer=recording_writer_open(root);CHECK(writer);
    Source source={0};atomic_init(&source.available,8192);
    CHECK(recording_writer_start(writer,read_audio,&source,0));
    for(int i=0;i<400 && recording_writer_frames(writer)<8192;i++)Sleep(5);
    CHECK(recording_writer_frames(writer)==8192 && !recording_writer_failed(writer));
    Sample first,second;CHECK(sample_map_recording(recording_writer_path(writer),8192,&first));
    atomic_store(&source.available,16384);CHECK(recording_writer_finish(writer));
    CHECK(sample_map_wav(recording_writer_path(writer),&second) && second.frames==16384);
    CHECK(first.data[8191*2+1]==-.5f && second.data[16383*2]==.25f);
    sample_free(first);sample_free(second);recording_writer_free(writer,1);
    BrowserFileList list=platform_browser_files(root);CHECK(list.count==1);platform_browser_files_free(list);
    CHECK(!platform_file_remove(path));platform_remove_directory(root);CHECK(!platform_path_exists(root));
    puts("Windows Unicode paths, project roundtrip, atomic replacement and mapped recording passed");return 0;
}
