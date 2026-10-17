// SPDX-License-Identifier: GPL-3.0-only
static int frame_wait_sequence(FILE *out){
    const double seconds[]={-1,-.001,-0.0,0,.0000001,.000000999,.000001,.000001001,.001,.016,.016666666666666666,.1,.999999999,1,1.000000001,10.25};
    for(unsigned i=0;i<sizeof seconds/sizeof *seconds;i++){
        sleep_calls=0;sleep_value=0;fixture_wait(seconds[i]);
        fprintf(out,"%u %.17g %d %lu\n",i,seconds[i],sleep_calls,sleep_value);
    }
    return 0;
}
