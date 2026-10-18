// SPDX-License-Identifier: GPL-3.0-only
#include "ui_internal.h"

/* Keep large empty project/view storage in zero-fill rather than embedding a
   mostly-zero UiState image in the executable. Called once before startup. */
UiState ui;
void ui_state_init(void){
    ui.song=1;
    ui.record_mask=RECORD_NOTES;
    ui.record_count_steps=16;
    ui.typing_keys=1;
    ui.output_volume=1;
    ui.input_enabled=1;
    ui.picker_drag=-1;
    ui.automation_selected=-1;
    ui.automation_node=-1;
    ui.automation_lane=-1;
    ui.automation_clip=-1;
    ui.rename_automation=-1;
    ui.arrangement=(Arrangement){.source_steps=STEPS,.source_pattern=-1,.zoom=1};
    ui.track_zoom=1;
    ui.row_zoom_drag=-1;
    ui.selected_track=-1;
    ui.track_resize=-1;
    ui.marker_drag=-1;
    ui.ruler_last_id=-1;
    ui.ruler_last_click=-1;
    ui.rack_filters[0]="All";ui.rack_filters[1]="Audio";ui.rack_filters[2]="Unsorted";
    ui.route_drag=-1;
    ui.mixer_selected=1;
    ui.cable_drag=-1;
    ui.browser_progress=-1;
    ui.browser_focus=1;
    ui.browser_width=174;
    ui.last_sidebar=174;
    ui.piano_top=72;
    ui.piano_zoom=1;
    ui.piano_key=-1;
    ui.piano_note_length=2;
    for(int p=0;p<PATTERNS;p++)ui.piano_span[p]=STEPS;
    ui.velocity_drag=-1;
    ui.rack_paint=-1;
    ui.rack_paint_channel=-1;
    ui.navigation_drag=-1;
    ui.eq_drag=-1;
    ui.fm_graph_drag=-1;
    ui.fm_graph_tab=-1;
    snprintf(ui.status,sizeof ui.status,"Use the toolbar to open editors; drag their title bars to arrange them");
    snprintf(ui.document.path,sizeof ui.document.path,"project" PROJECT_FILE_SUFFIX);
    ui.relink_channel=-1;
    ui.file_last_row=-1;
    ui.file_last_click=-1;
    ui.knob_context=-1;
    ui.rename_track=-1;
    ui.rename_mixer=-1;
    ui.rename_channel=-1;
    static const unsigned ids[3][4]={
        {PARAM_FM_ATTACK,PARAM_FM_DECAY,PARAM_FM_SUSTAIN,PARAM_FM_RELEASE},
        {PARAM_FM_MOD_ATTACK,PARAM_FM_MOD_DECAY,PARAM_FM_MOD_SUSTAIN,PARAM_FM_MOD_RELEASE},
        {PARAM_FM_ATTACK_ATTACK,PARAM_FM_ATTACK_DECAY,PARAM_FM_ATTACK_SUSTAIN,PARAM_FM_ATTACK_RELEASE}
    };
    memcpy(ui.fm_envelope_ids,ids,sizeof ids);
}
