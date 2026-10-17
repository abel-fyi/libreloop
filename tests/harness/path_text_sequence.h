// SPDX-License-Identifier: GPL-3.0-only
static int path_text_sequence(void){
    FILE *out=fopen("paths.txt","w");if(!out)return 1;
    const char *names[]={"tone.WAV","tone.wav","/tmp/tone.FlAc","C:\\samples\\tone.mp3",".wav","/tmp/.wav","dir.wav/plain","dir.wav/plain.wav","dir\\FILE.WAV","a.",".hidden","a/.hidden","plain","multi.part.wav","../tone.wav","x.c","x.123456789012345","x.123456789012345more","x.LONGEXTENSION123456789","/a.b/c","a..wav","a/.wav","a/..wav","dir/"};
    const char *filters[]={".wav",".WAV",".wav;.flac;.mp3","",";",";.wav;","..wav",".hidden",".123456789012345",".123456789012345more"," .wav",".wav; .mp3",NULL};
    for(unsigned n=0;n<sizeof names/sizeof *names;n++){
        fprintf(out,"%u %s",n,fixture_stem(names[n]));
        for(unsigned f=0;f<sizeof filters/sizeof *filters;f++)fprintf(out," %d",fixture_extension(names[n],filters[f]));fputc('\n',out);
    }
    fprintf(out,"nullstem '%s'\n",fixture_stem(NULL));
    for(int count=126;count<=129;count++){
        char filters_long[600]={0};for(int i=0;i<count;i++)strcat(filters_long,".x;");strcat(filters_long,".wav");
        fprintf(out,"split %d %d\n",count,fixture_extension("x.wav",filters_long));
    }
    FILE *binary=fopen("format.bin","wb");if(!binary)return 1;
    const int lengths[]={0,1020,1021,1022,1023,1024,1040,1300};
    for(unsigned i=0;i<sizeof lengths/sizeof *lengths;i++){
        char text[1400];for(int j=0;j<lengths[i];j++)text[j]='A'+j%26;text[lengths[i]]=0;
        const char *p=fixture_format("%s",text);fprintf(out,"format %d %zu\n",lengths[i],strlen(p));fwrite(p,1024,1,binary);
    }
    const char *saved[4];for(int i=0;i<4;i++)saved[i]=fixture_format("slot %d",i);
    const char *wrapped=fixture_format("wrapped");fprintf(out,"wrap %d",wrapped==saved[0]);
    for(int i=0;i<4;i++)fprintf(out," '%s'",saved[i]);fputc('\n',out);
    return fclose(binary) || fclose(out);
}
