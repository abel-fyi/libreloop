// SPDX-License-Identifier: GPL-3.0-only
static void popup_value_sequence(FILE *out) {
    const char *values[]={"", " ", "1", "-0", "1,5", "1.5", " 2\t", "2x", "1,2,3",
        "nan", "inf", "-inf", "1e999", "1e-999", "0x1p2", "+3", "3.0000001",
        "-0.25", "4", "4.000001", "\xc2\xa0" "2", "1\n", "2e0", ".5",
        "0000000000000000000000000000000000000000000000000000000000000002x"};
    const float ranges[][2]={{0,4},{-4,4},{1,1},{-100,100}};
    for(int r=0;r<4;r++)for(int integer=0;integer<2;integer++)for(int i=0;i<25;i++) {
        float value=-123.5f;int accepted=popup_number(values[i],ranges[r][0],ranges[r][1],integer,&value);
        unsigned bits;memcpy(&bits,&value,sizeof bits);
        fprintf(out,"number %d %d %d %d %08x\n",r,integer,i,accepted,bits);
    }
    const char *names[]={"", " ", "   ", " one ", "two", "\t", "\tone\t", "a\n",
        "\xc2\xa0", " \xc3\xa9 ", " \xf0\x9f\x8e\xb5 ", "a  b", "a/b", " a\t ",
        "a\r", "a.b", "Long names keep their original trailing-space and truncation behavior"};
    for(int i=0;i<17;i++) {
        char text[128];snprintf(text,sizeof text,"%s",names[i]);
        const char *name=popup_name(text);
        fprintf(out,"name %d %d ",i,name!=NULL);
        for(size_t n=0;n<strlen(text);n++)fprintf(out,"%02x",(unsigned char)text[n]);
        fputc('|',out);
        if(name)for(size_t n=0;n<strlen(name);n++)fprintf(out,"%02x",(unsigned char)name[n]);
        fputc('\n',out);
    }
}
