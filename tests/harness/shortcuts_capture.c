// SPDX-License-Identifier: GPL-3.0-only
#include "shortcuts.h"
#include <stdio.h>
#include <string.h>
#include "shortcut_sequence.h"
int main(void){FILE *out=fopen("actions.txt","wb");if(!out)return 1;
    for(int mac=0;mac<2;mac++)for(int mode=0;mode<20;mode++) {
        AppInput input;input_init(&input);ShortcutContext context={.mac=mac};
        shortcut_sequence(mode,mac,&context.modal,&context.captured,&context.dragging,&context.browser_focus,&context.editor,&context.piano_tool);
        for(int i=0;i<512;i++){input.keys[i]=script_keys[i];input.previous_keys[i]=script_previous[i];input.repeat[i]=script_repeat[i];}
        input.frame_timestamp_ns=125000000+(uint64_t)mode*16666667;
        fprintf(out,"%d %d\n",mac,mode);ShortcutActions actions=shortcut_actions(&input,context);
        for(int i=0;i<actions.count;i++)fprintf(out,"%d %d\n",actions.actions[i].kind,actions.actions[i].argument);
    }
    fclose(out);return 0;
}
