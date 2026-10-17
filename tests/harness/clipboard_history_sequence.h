// SPDX-License-Identifier: GPL-3.0-only
static FILE *trace;
static int stage;
static void snapshot(const char *name) {
    char file[64];snprintf(file,sizeof file,"project-%02d.llp",stage++);
    if(!project_save(file,&ui.project))exit(2);
    fprintf(trace,"%s %d %d %d %d %d %d %.9g %.9g %d %s\n",name,ui.edit_history.count,ui.edit_history.cursor,ui.edit_clipboard.kind,ui.edit_clipboard.count,ui.pattern,ui.piano_channel,ui.piano_start[0],ui.arrangement.source_steps,ui.piano_top,ui.status);
    fwrite(ui.note_selected,sizeof ui.note_selected,1,selections);fwrite(ui.arrangement.selected,sizeof ui.arrangement.selected,1,selections);
}
static void sequence(void) {
    history_checkpoint();snapshot("initial");edit_selection(3);snapshot("select-all");
    edit_selection(1);snapshot("copy");ui.piano_start[0]=8;edit_selection(2);snapshot("paste");history_checkpoint();
    undo_redo(-1);snapshot("undo-paste");undo_redo(1);snapshot("redo-paste");
    edit_selection(3);edit_selection(0);snapshot("cut");history_checkpoint();undo_redo(-1);snapshot("undo-cut");
    edit_selection(3);piano_shift(1,1,"up/right");snapshot("shift");history_checkpoint();piano_octave(1);snapshot("octave");history_checkpoint();
    undo_redo(-1);snapshot("undo-octave");undo_redo(-1);snapshot("undo-shift");
    edit_selection(3);ui.browser_focus=1;edit_selection(0);snapshot("browser-block");ui.browser_focus=0;
    test_capture=1;undo_redo(-1);snapshot("capture-block");test_capture=0;
    ui.midi_take.active=1;edit_selection(0);snapshot("take-block");ui.midi_take.active=0;
    windows_focus(&ui.windows,1);memset(ui.arrangement.selected,0,sizeof ui.arrangement.selected);ui.arrangement.selected[0][0]=1;
    edit_selection(1);snapshot("copy-clip");ui.selected_track=3;ui.playlist_start=4;edit_selection(2);snapshot("paste-clip");history_checkpoint();
    undo_redo(-1);snapshot("undo-clip");undo_redo(1);snapshot("redo-clip");
    ui.project.channel_count++;edit_selection(2);snapshot("changed-source-paste");
}
