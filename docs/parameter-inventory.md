# Parameters and project schema

Verbatim documentary excerpts of frozen source; IDs, ranges and counts are compatibility commitments. Stored inactive parameters remain part of the file format. Values in factory patches may differ from descriptor initial values; preserve both.

## parameter.h

```c
1: // SPDX-License-Identifier: GPL-3.0-only
2: #ifndef PARAMETER_H
3: #define PARAMETER_H
4: /* Saved IDs, independent of UI addresses. The device range is reserved for future built-in processors. */
5: enum { PARAM_CHANNEL_VOLUME=1, PARAM_CHANNEL_PAN, PARAM_CHANNEL_PITCH, PARAM_INSERT_VOLUME, PARAM_INSERT_PAN, PARAM_INSERT_WIDTH, PARAM_MASTER_VOLUME, PARAM_MASTER_WIDTH, PARAM_MASTER_PITCH, PARAM_CHANNEL_MUTE, PARAM_INSERT_MUTE, PARAM_MASTER_MUTE, PARAM_SWING, PARAM_PITCH_RANGE, PARAM_PLUGIN=1024, PARAM_CHORUS_RATE=1025, PARAM_CHORUS_DEPTH, PARAM_EFFECT_MIX, PARAM_FM_RATIO=1101, PARAM_FM_DEPTH, PARAM_FM_ATTACK, PARAM_FM_DECAY, PARAM_FM_SUSTAIN, PARAM_FM_RELEASE, PARAM_FM_MOD_DECAY, PARAM_FM_MOD_SUSTAIN, PARAM_FM_VELOCITY, PARAM_FM_LFO_RATE, PARAM_FM_VIBRATO, PARAM_FM_TREMOLO, PARAM_FM_CARRIER_RATIO, PARAM_FM_CARRIER_DETUNE, PARAM_FM_BODY_DETUNE, PARAM_FM_MOD_ATTACK, PARAM_FM_MOD_RELEASE, PARAM_FM_ATTACK_RATIO, PARAM_FM_ATTACK_DETUNE, PARAM_FM_ATTACK_DEPTH, PARAM_FM_ATTACK_ATTACK, PARAM_FM_ATTACK_DECAY, PARAM_FM_ATTACK_SUSTAIN, PARAM_FM_ATTACK_RELEASE, PARAM_FM_ROUTING, PARAM_FM_LFO_SHAPE, PARAM_FM_LFO_FADE, PARAM_FM_BODY_PITCH, PARAM_FM_ENGINE, PARAM_FM_ANALOG_WAVE, PARAM_FM_ANALOG_DETUNE, PARAM_FM_ANALOG_MIX, PARAM_FM_ANALOG_SUB, PARAM_FM_ANALOG_NOISE, PARAM_FM_ANALOG_PULSE, PARAM_FM_ANALOG_PWM, PARAM_FM_FILTER_CUTOFF, PARAM_FM_FILTER_RESONANCE, PARAM_FM_FILTER_ENV, PARAM_FM_ANALOG_CHORUS, PARAM_FM_LAST=PARAM_FM_ANALOG_CHORUS, PARAM_EQ_FIRST=1201, PARAM_EQ_LAST=1221, PARAM_DX7_FIRST=2001, PARAM_DX7_LAST=2151 };
6: typedef struct { unsigned parameter,owner,slot; } ParameterTarget;
7: /* Common control metadata; independent of project layout and UI addresses. */
8: enum { PARAMETER_CONTINUOUS, PARAMETER_INTEGER, PARAMETER_TOGGLE, PARAMETER_LOGARITHMIC };
9: typedef struct {
10:     unsigned id;
11:     const char *name;
12:     float low,high,initial;
13:     int kind;
14: } ParameterDescriptor;
15: const ParameterDescriptor *parameter_descriptor(unsigned id);
16: #endif
```

## parameter.c

```c
4: static const ParameterDescriptor descriptors[]={
5:     {PARAM_EQ_FIRST+0,"EQ 1 frequency",20,20000,60,PARAMETER_LOGARITHMIC},
6:     {PARAM_EQ_FIRST+1,"EQ 1 gain",-18,18,0,PARAMETER_CONTINUOUS},
7:     {PARAM_EQ_FIRST+2,"EQ 1 Q",0.2,10,0.707,PARAMETER_CONTINUOUS},
8:     {PARAM_EQ_FIRST+3,"EQ 2 frequency",20,20000,150,PARAMETER_LOGARITHMIC},
9:     {PARAM_EQ_FIRST+4,"EQ 2 gain",-18,18,0,PARAMETER_CONTINUOUS},
10:     {PARAM_EQ_FIRST+5,"EQ 2 Q",0.2,10,1,PARAMETER_CONTINUOUS},
11:     {PARAM_EQ_FIRST+6,"EQ 3 frequency",20,20000,400,PARAMETER_LOGARITHMIC},
12:     {PARAM_EQ_FIRST+7,"EQ 3 gain",-18,18,0,PARAMETER_CONTINUOUS},
13:     {PARAM_EQ_FIRST+8,"EQ 3 Q",0.2,10,1,PARAMETER_CONTINUOUS},
14:     {PARAM_EQ_FIRST+9,"EQ 4 frequency",20,20000,1000,PARAMETER_LOGARITHMIC},
15:     {PARAM_EQ_FIRST+10,"EQ 4 gain",-18,18,0,PARAMETER_CONTINUOUS},
16:     {PARAM_EQ_FIRST+11,"EQ 4 Q",0.2,10,1,PARAMETER_CONTINUOUS},
17:     {PARAM_EQ_FIRST+12,"EQ 5 frequency",20,20000,2500,PARAMETER_LOGARITHMIC},
18:     {PARAM_EQ_FIRST+13,"EQ 5 gain",-18,18,0,PARAMETER_CONTINUOUS},
19:     {PARAM_EQ_FIRST+14,"EQ 5 Q",0.2,10,1,PARAMETER_CONTINUOUS},
20:     {PARAM_EQ_FIRST+15,"EQ 6 frequency",20,20000,6000,PARAMETER_LOGARITHMIC},
21:     {PARAM_EQ_FIRST+16,"EQ 6 gain",-18,18,0,PARAMETER_CONTINUOUS},
22:     {PARAM_EQ_FIRST+17,"EQ 6 Q",0.2,10,1,PARAMETER_CONTINUOUS},
23:     {PARAM_EQ_FIRST+18,"EQ 7 frequency",20,20000,12000,PARAMETER_LOGARITHMIC},
24:     {PARAM_EQ_FIRST+19,"EQ 7 gain",-18,18,0,PARAMETER_CONTINUOUS},
25:     {PARAM_EQ_FIRST+20,"EQ 7 Q",0.2,10,0.707,PARAMETER_CONTINUOUS},
26:     {PARAM_FM_RATIO,"Body harmonic ratio",1,8,1,PARAMETER_INTEGER},
27:     {PARAM_FM_DEPTH,"Body FM amount",0,8,2.6f,PARAMETER_CONTINUOUS},
28:     {PARAM_FM_ATTACK,"Sound attack",.001f,2,.0015f,PARAMETER_CONTINUOUS},
29:     {PARAM_FM_DECAY,"Sound decay",.01f,3,2.5f,PARAMETER_CONTINUOUS},
30:     {PARAM_FM_SUSTAIN,"Sound sustain",0,1,.08f,PARAMETER_CONTINUOUS},
31:     {PARAM_FM_RELEASE,"Sound release",.01f,3,.45f,PARAMETER_CONTINUOUS},
32:     {PARAM_FM_MOD_DECAY,"Body decay",.01f,5,1.2f,PARAMETER_CONTINUOUS},
33:     {PARAM_FM_MOD_SUSTAIN,"Body sustain",0,1,.12f,PARAMETER_CONTINUOUS},
34:     {PARAM_FM_VELOCITY,"Velocity brightness",0,1,.8f,PARAMETER_CONTINUOUS},
35:     {PARAM_FM_LFO_RATE,"Motion rate",.1f,12,4.8f,PARAMETER_CONTINUOUS},
36:     {PARAM_FM_VIBRATO,"FM vibrato",0,100,0,PARAMETER_CONTINUOUS},
37:     {PARAM_FM_TREMOLO,"FM tremolo",0,1,0,PARAMETER_CONTINUOUS},
38:     {PARAM_FM_CARRIER_RATIO,"Carrier pitch ratio",.125f,32,1,PARAMETER_LOGARITHMIC},
39:     {PARAM_FM_CARRIER_DETUNE,"Carrier fine tuning (cents)",-100,100,0,PARAMETER_CONTINUOUS},
40:     {PARAM_FM_BODY_DETUNE,"Body fine tuning (cents)",-100,100,0,PARAMETER_CONTINUOUS},
41:     {PARAM_FM_MOD_ATTACK,"Body attack",0,2,.001f,PARAMETER_CONTINUOUS},
42:     {PARAM_FM_MOD_RELEASE,"Body release",0,5,.4f,PARAMETER_CONTINUOUS},
43:     {PARAM_FM_ATTACK_RATIO,"Attack pitch ratio",.125f,32,14,PARAMETER_LOGARITHMIC},
44:     {PARAM_FM_ATTACK_DETUNE,"Attack fine tuning (cents)",-100,100,0,PARAMETER_CONTINUOUS},
45:     {PARAM_FM_ATTACK_DEPTH,"Attack FM amount",0,12,4.5f,PARAMETER_CONTINUOUS},
46:     {PARAM_FM_ATTACK_ATTACK,"Strike attack",.001f,2,.001f,PARAMETER_LOGARITHMIC},
47:     {PARAM_FM_ATTACK_DECAY,"Strike decay",.005f,5,.09f,PARAMETER_LOGARITHMIC},
48:     {PARAM_FM_ATTACK_SUSTAIN,"Strike sustain",0,1,0,PARAMETER_CONTINUOUS},
49:     {PARAM_FM_ATTACK_RELEASE,"Strike release",.01f,5,.07f,PARAMETER_LOGARITHMIC},
50:     {PARAM_FM_ROUTING,"FM routing",0,1,0,PARAMETER_INTEGER},
51:     {PARAM_FM_LFO_SHAPE,"LFO shape",0,3,0,PARAMETER_INTEGER},
52:     {PARAM_FM_LFO_FADE,"LFO fade in",0,5,.3f,PARAMETER_CONTINUOUS},
53:     {PARAM_FM_BODY_PITCH,"Body pitch (semitones)",-48,48,0,PARAMETER_CONTINUOUS},
54:     {PARAM_FM_ENGINE,"Synth engine",0,2,1,PARAMETER_INTEGER},
55:     {PARAM_FM_ANALOG_WAVE,"Oscillator waveform",0,3,0,PARAMETER_INTEGER},
56:     {PARAM_FM_ANALOG_DETUNE,"Oscillator detune",0,50,8,PARAMETER_CONTINUOUS},
57:     {PARAM_FM_ANALOG_MIX,"Second oscillator",0,1,.65f,PARAMETER_CONTINUOUS},
58:     {PARAM_FM_ANALOG_SUB,"Sub oscillator",0,1,.2f,PARAMETER_CONTINUOUS},
59:     {PARAM_FM_ANALOG_NOISE,"Noise",0,1,0,PARAMETER_CONTINUOUS},
60:     {PARAM_FM_ANALOG_PULSE,"Pulse width",.05f,.95f,.5f,PARAMETER_CONTINUOUS},
61:     {PARAM_FM_ANALOG_PWM,"Pulse modulation",0,.45f,.15f,PARAMETER_CONTINUOUS},
62:     {PARAM_FM_FILTER_CUTOFF,"Filter cutoff",20,20000,5000,PARAMETER_LOGARITHMIC},
63:     {PARAM_FM_FILTER_RESONANCE,"Filter resonance",0,.95f,.2f,PARAMETER_CONTINUOUS},
64:     {PARAM_FM_FILTER_ENV,"Filter envelope octaves",-6,6,1,PARAMETER_CONTINUOUS},
65:     {PARAM_FM_ANALOG_CHORUS,"Synth chorus",0,1,.35f,PARAMETER_CONTINUOUS},
66:     {PARAM_CHORUS_RATE,"Chorus rate",.05f,5,.513f,PARAMETER_CONTINUOUS},
67:     {PARAM_CHORUS_DEPTH,"Chorus depth",0,8,1.85f,PARAMETER_CONTINUOUS},
68:     {PARAM_EFFECT_MIX,"Effect mix",0,1,.5f,PARAMETER_CONTINUOUS},
69:     {PARAM_CHANNEL_VOLUME,"Volume",0,VOLUME_KNOB_MAX,1,PARAMETER_CONTINUOUS},
70:     {PARAM_CHANNEL_PAN,"Pan",-1,1,0,PARAMETER_CONTINUOUS},
71:     {PARAM_CHANNEL_PITCH,"Pitch",-1,1,0,PARAMETER_CONTINUOUS},
72:     {PARAM_INSERT_VOLUME,"Volume",0,MIXER_GAIN_MAX,1,PARAMETER_CONTINUOUS},
73:     {PARAM_INSERT_PAN,"Pan",-1,1,0,PARAMETER_CONTINUOUS},
74:     {PARAM_INSERT_WIDTH,"Stereo width",0,2,1,PARAMETER_CONTINUOUS},
75:     {PARAM_MASTER_VOLUME,"Master volume",0,MIXER_GAIN_MAX,1,PARAMETER_CONTINUOUS},
76:     {PARAM_MASTER_WIDTH,"Master stereo width",0,2,1,PARAMETER_CONTINUOUS},
77:     {PARAM_MASTER_PITCH,"Master pitch",-12,12,0,PARAMETER_CONTINUOUS},
78:     {PARAM_CHANNEL_MUTE,"Mute",0,1,0,PARAMETER_TOGGLE},
79:     {PARAM_INSERT_MUTE,"Mute",0,1,0,PARAMETER_TOGGLE},
80:     {PARAM_MASTER_MUTE,"Master mute",0,1,0,PARAMETER_TOGGLE},
81:     {PARAM_SWING,"Swing",0,1,0,PARAMETER_CONTINUOUS},
82:     {PARAM_PITCH_RANGE,"Pitch range",1,48,2,PARAMETER_INTEGER},
83: };
84: const ParameterDescriptor *parameter_descriptor(unsigned id) {
85:     if(id>=PARAM_DX7_FIRST && id<=PARAM_DX7_LAST) return dx7_parameter_descriptor(id-PARAM_DX7_FIRST);
86:     for(size_t i=0;i<sizeof descriptors/sizeof *descriptors;i++) if(descriptors[i].id==id) return &descriptors[i];
87:     return NULL;
88: }
89: static const float *parameter_pointer(const Project *p,ParameterTarget t,float *lo,float *hi) {
90:     const ParameterDescriptor *info=parameter_descriptor(t.parameter);
91:     *lo=info?info->low:0; *hi=info?info->high:1;
92:     if(t.parameter>=PARAM_EQ_FIRST && t.parameter<=PARAM_EQ_LAST) {
93:         if(t.owner>(unsigned)p->insert_count || t.slot>=EFFECT_SLOTS || p->effect_type[t.owner][t.slot]!=EFFECT_EQ) return NULL;
94:         return device_parameter(effect_descriptor(EFFECT_EQ),&p->eq[t.owner][t.slot],t.parameter);
```

## dx7.c

```c
23: static int min_int(int a,int b) { return a<b?a:b; }
24: static int max_int(int a,int b) { return a>b?a:b; }
25: static int parameter_max(int i) {
26:     if(i>=DX7_NATIVE_PARAMETERS) return 100;
27:     static const int operator_max[21]={99,99,99,99,99,99,99,99,99,99,99,3,3,7,3,7,99,1,31,99,14};
28:     static const int global_max[19]={99,99,99,99,99,99,99,99,31,7,1,99,99,99,99,1,5,7,48};
29:     return i<126?operator_max[i%21]:global_max[i-126];
30: }
31: static void prepare(void) {
32:     double dphase=2*PI/1024; int32_t c=floor(cos(dphase)*(1<<30)+.5),s=floor(sin(dphase)*(1<<30)+.5),u=1<<30,v=0;
33:     for(int i=0;i<512;i++) {
34:         sine_table[2*i+1]=(v+32)>>6; sine_table[2*(i+512)+1]=-((v+32)>>6);
35:         int32_t t=((int64_t)u*s+(int64_t)v*c+(1<<29))>>30;
36:         u=((int64_t)u*c-(int64_t)v*s+(1<<29))>>30; v=t;
37:     }
38:     for(int i=0;i<1023;i++) sine_table[2*i]=sine_table[2*i+3]-sine_table[2*i+1];
39:     sine_table[2046]=-sine_table[2047];
40:     double y=1<<30,inc=exp2(1./1024);
41:     for(int i=0;i<1024;i++) { exponent_table[2*i+1]=floor(y+.5); y*=inc; }
42:     for(int i=0;i<1023;i++) exponent_table[2*i]=exponent_table[2*i+3]-exponent_table[2*i+1];
43:     exponent_table[2046]=(int64_t)(1ULL<<31)-exponent_table[2047];
44:     y=(1ULL<<44)/(double)RATE;
45:     for(int i=0;i<=1024;i++) { frequency_table[i]=floor(y+.5); y*=inc; }
46:     static const char *op_names[]={"Rate 1","Rate 2","Rate 3","Release rate","Level 1","Level 2","Sustain level","Release level","Breakpoint","Left depth","Right depth","Left curve","Right curve","Rate scaling","Amp sensitivity","Velocity","Output","Frequency mode","Coarse","Fine","Detune"};
47:     static const char *global_names[]={"Pitch rate 1","Pitch rate 2","Pitch rate 3","Pitch release rate","Pitch level 1","Pitch level 2","Pitch sustain","Pitch release","Algorithm","Feedback","Oscillator sync","LFO speed","LFO delay","Pitch modulation","Amplitude modulation","LFO sync","LFO waveform","Pitch sensitivity","Transpose"};
48:     for(int i=0;i<DX7_PARAMETERS;i++) {
49:         if(i>=DX7_NATIVE_PARAMETERS) {
50:             snprintf(parameter_names[i],sizeof parameter_names[i],"Operator %d key tracking",6-(i-DX7_NATIVE_PARAMETERS));
51:             descriptors[i]=(ParameterDescriptor){PARAM_DX7_FIRST+i,parameter_names[i],0,100,100,PARAMETER_CONTINUOUS};
52:             continue;
53:         }
54:         if(i<126) snprintf(parameter_names[i],sizeof parameter_names[i],"Operator %d %s",6-i/21,op_names[i%21]);
55:         else snprintf(parameter_names[i],sizeof parameter_names[i],"DX7 %s",global_names[i-126]);
56:         descriptors[i]=(ParameterDescriptor){PARAM_DX7_FIRST+i,parameter_names[i],0,parameter_max(i),factory[0][i],PARAMETER_INTEGER};
57:     }
58: }
59: void dx7_init(void) { pthread_once(&initialized,prepare); }
```

## project_io.c

```c
74: typedef struct { const char *name; FieldType type; size_t offset,count,inner,outer_stride,inner_stride,width; } Field;
75: #define FLAT(m,t,k) {#m,k,offsetof(Project,m),sizeof(((Project *)0)->m)/sizeof(t),1,sizeof(t),0,sizeof(t)}
76: #define SCALAR(m,t,k) {#m,k,offsetof(Project,m),1,1,sizeof(t),0,sizeof(t)}
77: #define TEXT(m,n,w) {#m,STRING,offsetof(Project,m),n,1,w,0,w}
78: #define MEMBER(m,t,f,k,n) {#m "." #f,k,offsetof(Project,m)+offsetof(t,f),n,1,sizeof(t),0,sizeof(((t *)0)->f)}
79: #define SUB(m,t,s,u,f,k,n) {#m "." #s "." #f,k,offsetof(Project,m)+offsetof(t,s)+offsetof(u,f),n,1,sizeof(t),0,sizeof(((u *)0)->f)}
80: /* Stable file schema; offsets/strides map logical values without saving padding. */
81: static const Field fields[]={
82:     SCALAR(bpm,float,F32),
83:     SCALAR(master,float,F32),
84:     SCALAR(master_pitch,float,F32),
85:     SCALAR(master_width,float,F32),
86:     SCALAR(swing,float,F32),
87:     SCALAR(pattern_count,int,I32),
88:     SCALAR(channel_count,int,I32),
89:     SCALAR(insert_count,int,I32),
90:     SCALAR(automation_count,int,I32),
91:     SCALAR(midi_binding_count,int,I32),
92:     SCALAR(master_mute,uint8_t,U8),
93:     FLAT(volume,float,F32),
94:     FLAT(pan,float,F32),
95:     FLAT(channel_pitch,float,F32),
96:     FLAT(pitch_range,float,F32),
97:     FLAT(clip_steps,float,F32),
98:     FLAT(clip_starts,float,F32),
99:     FLAT(clip_offsets,float,F32),
100:     FLAT(audio_seconds,float,F32),
101:     FLAT(pattern_steps,float,F32),
102:     FLAT(insert_volume,float,F32),
103:     FLAT(insert_pan,float,F32),
104:     FLAT(insert_width,float,F32),
105:     FLAT(effect_mix,float,F32),
106:     FLAT(mute,uint8_t,U8),
107:     FLAT(clips,uint8_t,U8),
108:     FLAT(channel_audio,uint8_t,U8),
109:     FLAT(route,uint8_t,U8),
110:     FLAT(insert_mute,uint8_t,U8),
111:     FLAT(insert_output,uint8_t,U8),
112:     FLAT(effect_type,uint8_t,U8),
113:     FLAT(effect_bypass,uint8_t,U8),
114:     FLAT(lane_mute,uint8_t,U8),
115:     FLAT(instrument,uint8_t,U8),
116:     FLAT(pattern_colors,uint32_t,U32),
117:     FLAT(channel_colors,uint32_t,U32),
118:     TEXT(paths,CHANNELS,1024),
119:     TEXT(channel_names,CHANNELS,PATTERN_NAME),
120:     TEXT(pattern_names,PATTERNS,PATTERN_NAME),
121:     TEXT(audio_io,(INSERTS+1)*2,128),
122:     TEXT(track_names,LANES,PATTERN_NAME),
123:     TEXT(insert_names,INSERTS,PATTERN_NAME),
124:     MEMBER(notes,Note,pitch,U8,PATTERNS*CHANNELS*NOTES),
125:     MEMBER(notes,Note,velocity,U8,PATTERNS*CHANNELS*NOTES),
126:     MEMBER(notes,Note,start,F32,PATTERNS*CHANNELS*NOTES),
127:     MEMBER(notes,Note,length,F32,PATTERNS*CHANNELS*NOTES),
128:     MEMBER(sampler,Sampler,pitch,F32,CHANNELS),
129:     MEMBER(sampler,Sampler,time,F32,CHANNELS),
130:     MEMBER(sampler,Sampler,start,F32,CHANNELS),
131:     MEMBER(sampler,Sampler,length,F32,CHANNELS),
132:     MEMBER(sampler,Sampler,trim,F32,CHANNELS),
133:     MEMBER(sampler,Sampler,fit_bpm,F32,CHANNELS),
134:     MEMBER(sampler,Sampler,flags,U8,CHANNELS),
135:     MEMBER(sampler,Sampler,stretch,U8,CHANNELS),
136:     MEMBER(fm,FMSettings,ratio,F32,CHANNELS),
137:     MEMBER(fm,FMSettings,depth,F32,CHANNELS),
138:     MEMBER(fm,FMSettings,attack,F32,CHANNELS),
139:     MEMBER(fm,FMSettings,decay,F32,CHANNELS),
140:     MEMBER(fm,FMSettings,sustain,F32,CHANNELS),
141:     MEMBER(fm,FMSettings,release,F32,CHANNELS),
142:     MEMBER(fm,FMSettings,mod_decay,F32,CHANNELS),
143:     MEMBER(fm,FMSettings,mod_sustain,F32,CHANNELS),
144:     MEMBER(fm,FMSettings,velocity,F32,CHANNELS),
145:     MEMBER(fm,FMSettings,lfo_rate,F32,CHANNELS),
146:     MEMBER(fm,FMSettings,vibrato,F32,CHANNELS),
147:     MEMBER(fm,FMSettings,tremolo,F32,CHANNELS),
148:     MEMBER(fm,FMSettings,carrier_ratio,F32,CHANNELS),
149:     MEMBER(fm,FMSettings,carrier_detune,F32,CHANNELS),
150:     MEMBER(fm,FMSettings,body_detune,F32,CHANNELS),
151:     MEMBER(fm,FMSettings,mod_attack,F32,CHANNELS),
152:     MEMBER(fm,FMSettings,mod_release,F32,CHANNELS),
153:     MEMBER(fm,FMSettings,attack_ratio,F32,CHANNELS),
154:     MEMBER(fm,FMSettings,attack_detune,F32,CHANNELS),
155:     MEMBER(fm,FMSettings,attack_depth,F32,CHANNELS),
156:     MEMBER(fm,FMSettings,attack_attack,F32,CHANNELS),
157:     MEMBER(fm,FMSettings,attack_decay,F32,CHANNELS),
158:     MEMBER(fm,FMSettings,attack_sustain,F32,CHANNELS),
159:     MEMBER(fm,FMSettings,attack_release,F32,CHANNELS),
160:     MEMBER(fm,FMSettings,routing,F32,CHANNELS),
161:     MEMBER(fm,FMSettings,lfo_shape,F32,CHANNELS),
162:     MEMBER(fm,FMSettings,lfo_fade,F32,CHANNELS),
163:     MEMBER(fm,FMSettings,body_pitch,F32,CHANNELS),
164:     MEMBER(fm,FMSettings,engine,F32,CHANNELS),
165:     SUB(fm,FMSettings,analog,AnalogSettings,wave,F32,CHANNELS),
166:     SUB(fm,FMSettings,analog,AnalogSettings,detune,F32,CHANNELS),
167:     SUB(fm,FMSettings,analog,AnalogSettings,mix,F32,CHANNELS),
168:     SUB(fm,FMSettings,analog,AnalogSettings,sub,F32,CHANNELS),
169:     SUB(fm,FMSettings,analog,AnalogSettings,noise,F32,CHANNELS),
170:     SUB(fm,FMSettings,analog,AnalogSettings,pulse,F32,CHANNELS),
171:     SUB(fm,FMSettings,analog,AnalogSettings,pwm,F32,CHANNELS),
172:     SUB(fm,FMSettings,analog,AnalogSettings,cutoff,F32,CHANNELS),
173:     SUB(fm,FMSettings,analog,AnalogSettings,resonance,F32,CHANNELS),
174:     SUB(fm,FMSettings,analog,AnalogSettings,filter_env,F32,CHANNELS),
175:     SUB(fm,FMSettings,analog,AnalogSettings,chorus,F32,CHANNELS),
176:     {"fm.dx7.value",F32,offsetof(Project,fm)+offsetof(FMSettings,dx7)+offsetof(DX7Settings,value),CHANNELS*DX7_PARAMETERS,DX7_PARAMETERS,sizeof(FMSettings),sizeof(float),sizeof(float)},
177:     MEMBER(chorus,ChorusSettings,rate,F32,(INSERTS+1)*EFFECT_SLOTS),
178:     MEMBER(chorus,ChorusSettings,depth,F32,(INSERTS+1)*EFFECT_SLOTS),
179:     {"eq.bands.frequency",F32,offsetof(Project,eq)+offsetof(EQSettings,bands)+offsetof(EQBand,frequency),(INSERTS+1)*EFFECT_SLOTS*EQ_BANDS,EQ_BANDS,sizeof(EQSettings),sizeof(EQBand),sizeof(((EQBand *)0)->frequency)},
180:     {"eq.bands.gain",F32,offsetof(Project,eq)+offsetof(EQSettings,bands)+offsetof(EQBand,gain),(INSERTS+1)*EFFECT_SLOTS*EQ_BANDS,EQ_BANDS,sizeof(EQSettings),sizeof(EQBand),sizeof(((EQBand *)0)->gain)},
181:     {"eq.bands.q",F32,offsetof(Project,eq)+offsetof(EQSettings,bands)+offsetof(EQBand,q),(INSERTS+1)*EFFECT_SLOTS*EQ_BANDS,EQ_BANDS,sizeof(EQSettings),sizeof(EQBand),sizeof(((EQBand *)0)->q)},
182:     {"eq.bands.shape",U32,offsetof(Project,eq)+offsetof(EQSettings,bands)+offsetof(EQBand,shape),(INSERTS+1)*EFFECT_SLOTS*EQ_BANDS,EQ_BANDS,sizeof(EQSettings),sizeof(EQBand),sizeof(((EQBand *)0)->shape)},
183:     SUB(automations,Automation,target,ParameterTarget,parameter,U32,AUTOMATIONS),
184:     SUB(automations,Automation,target,ParameterTarget,owner,U32,AUTOMATIONS),
185:     SUB(automations,Automation,target,ParameterTarget,slot,U32,AUTOMATIONS),
186:     MEMBER(automations,Automation,steps,F32,AUTOMATIONS),
187:     MEMBER(automations,Automation,count,I32,AUTOMATIONS),
188:     MEMBER(automations,Automation,color,U32,AUTOMATIONS),
189:     {"automations.name",STRING,offsetof(Project,automations)+offsetof(Automation,name),AUTOMATIONS,1,sizeof(Automation),0,48},
190:     {"automations.points.step",F32,offsetof(Project,automations)+offsetof(Automation,points)+offsetof(AutomationPoint,step),AUTOMATIONS*AUTOMATION_POINTS,AUTOMATION_POINTS,sizeof(Automation),sizeof(AutomationPoint),sizeof(float)},
191:     {"automations.points.value",F32,offsetof(Project,automations)+offsetof(Automation,points)+offsetof(AutomationPoint,value),AUTOMATIONS*AUTOMATION_POINTS,AUTOMATION_POINTS,sizeof(Automation),sizeof(AutomationPoint),sizeof(float)},
192:     MEMBER(midi_bindings,MidiBinding,channel,U32,MIDI_BINDINGS),
193:     MEMBER(midi_bindings,MidiBinding,controller,U32,MIDI_BINDINGS),
194:     SUB(midi_bindings,MidiBinding,target,ParameterTarget,parameter,U32,MIDI_BINDINGS),
195:     SUB(midi_bindings,MidiBinding,target,ParameterTarget,owner,U32,MIDI_BINDINGS),
196:     SUB(midi_bindings,MidiBinding,target,ParameterTarget,slot,U32,MIDI_BINDINGS),
197: };
198: #undef FLAT
199: #undef SCALAR
200: #undef TEXT
201: #undef MEMBER
202: #undef SUB
203: #define FIELD_COUNT (sizeof fields/sizeof *fields)
```

## sampler.h

```c
1: // SPDX-License-Identifier: GPL-3.0-only
2: #ifndef SAMPLER_H
3: #define SAMPLER_H
4: #include "sample.h"
5: enum { SAMPLE_NORMALIZE=1, SAMPLE_REVERSE=2, SAMPLE_POLARITY=4 };
6: typedef struct { float pitch,time,start,length,trim; uint8_t flags,stretch; float fit_bpm; /* reference BPM; zero disables tempo fitting */ } Sampler;
7: /* Per-note DSP state; no project, mixer, UI or file access. Reset specifies
8:    the source position and note speed, clearing tempo/grain history. */
9: typedef struct {
10:     double position,speed,tempo_rate,grains[2];
11:     unsigned grain_phase;
12:     int grain_ready;
13: } SamplerVoice;
14: void sampler_voice_reset(SamplerVoice *voice,double position,double speed);
15: /* Realtime: writes stereo PCM and advances source phase; never allocates.
16:    pitch is the total playback ratio, rate the tempo ratio. */
17: void sampler_voice_sample(SamplerVoice *voice,Sample sample,double pitch,double rate,int stretch,float stereo[2]);
18: int sampler_valid(Sampler settings);
19: int sampler_equal(Sampler a,Sampler b);
20: int sampler_processing_equal(Sampler a,Sampler b);
21: /* Non-owning source view: do not release it with sample_free. */
22: Sample sample_trim(Sample source,float threshold);
23: /* Offline/worker processing: may allocate. Release result with sample_free. */
24: int sample_process(Sample source,Sampler settings,Sample *result);
25: #endif
```

## fm_synth.h

```c
1: // SPDX-License-Identifier: GPL-3.0-only
2: #ifndef FM_SYNTH_H
3: #define FM_SYNTH_H
4: #include "parameter.h"
5: #include "dx7.h"
6: #include "analog.h"
7: /* Carrier plus independently tuned Body and Attack modulators. Legacy fields
8:    keep their saved IDs; new fields are appended and default to neutral on load. */
9: typedef struct {
10:     float ratio,depth,attack,decay,sustain,release,mod_decay,mod_sustain,velocity,lfo_rate,vibrato,tremolo;
11:     float carrier_ratio,carrier_detune,body_detune,mod_attack,mod_release;
12:     float attack_ratio,attack_detune,attack_depth,attack_attack,attack_decay,attack_sustain,attack_release;
13:     float routing,lfo_shape,lfo_fade,body_pitch,engine;
14:     AnalogSettings analog;
15:     DX7Settings dx7;
16: } FMSettings;
17: typedef struct {
18:     double carrier,modulator,frequency,current_frequency,lfo_phase,attack_modulator;
19:     float envelope,ratio,depth,release_step,mod_envelope,velocity,lfo_rate,vibrato,tremolo;
20:     float carrier_ratio,attack_ratio,attack_depth,carrier_detune,body_detune,attack_detune;
21:     float body_pitch,attack_envelope,body_release_step,attack_release_step,lfo_age;
22:     int stage,body_stage,attack_stage,engine;
23:     union { DX7Voice dx7; AnalogVoice analog; };
24: } FMVoice;
25: enum { FM_FACTORY_COUNT=9 };
26: FMSettings fm_default(void);
27: FMSettings fm_epiano(void);
28: FMSettings fm_factory(int preset);
29: const char *fm_factory_name(int preset);
30: FMSettings fm_legacy(void);
31: int fm_valid(FMSettings settings);
32: const float *fm_parameter_pointer(const FMSettings *settings,unsigned id);
33: float fm_lfo_value(float phase,int shape);
34: void fm_note_on(FMVoice *voice,double frequency,FMSettings settings);
35: void fm_note_on_velocity(FMVoice *voice,double frequency,FMSettings settings,float velocity);
36: void fm_note_off(FMVoice *voice,FMSettings settings);
37: int fm_active(const FMVoice *voice);
38: /* Fixed 48 kHz, allocation-free. pitch is channel/master tuning. */
39: void fm_sample_stereo(FMVoice *voice,const FMSettings *settings,double pitch,float stereo[2]);
40: float fm_sample(FMVoice *voice,FMSettings settings,double pitch);
41: #endif
```

## dx7.h

```c
1: // SPDX-License-Identifier: Apache-2.0
2: #ifndef DX7_H
3: #define DX7_H
4: #include <stdint.h>
5: #include "parameter.h"
6: #define DX7_NATIVE_PARAMETERS 145
7: #define DX7_PARAMETERS (DX7_NATIVE_PARAMETERS+6)
8: #define DX7_BLOCK 64
9: typedef struct { float value[DX7_PARAMETERS]; } DX7Settings;
10: typedef struct {
11:     int rates[4],levels[4],stage,down,outlevel,rate_scale,rising,hold;
12:     int32_t level,target,increment;
13: } DX7Envelope;
14: typedef struct {
15:     DX7Envelope envelope[6],pitch;
16:     uint32_t phase[6],frequency[6],lfo_phase,lfo_step,delay_state,delay_inc[2],random;
17:     int32_t gain[6],feedback[2],base_pitch[6],output[DX7_BLOCK],bus[2][DX7_BLOCK];
18:     uint8_t patch[DX7_NATIVE_PARAMETERS];
19:     float key_tracking[6];
20:     int midi,base_midi,velocity,down,cursor,algorithm;
21:     double note_frequency,pitch_ratio;
22: } DX7Voice;
23: void dx7_init(void); /* Call outside playback; tables become immutable. */
24: DX7Settings dx7_factory(int preset); /* EP, Bass, Marimba, Tubular Bells; -1 brighter EP. */
25: const char *dx7_factory_name(int preset);
26: int dx7_valid(DX7Settings settings);
27: void dx7_legacy_tracking(DX7Settings *settings);
28: const ParameterDescriptor *dx7_parameter_descriptor(unsigned index);
29: void dx7_note_on(DX7Voice *v,double frequency,float velocity,DX7Settings settings);
30: void dx7_note_off(DX7Voice *v);
31: int dx7_active(const DX7Voice *v);
32: float dx7_sample(DX7Voice *v,const DX7Settings *settings,double pitch);
33: #endif
```

## equalizer.h

```c
1: // SPDX-License-Identifier: GPL-3.0-only
2: #ifndef EQUALIZER_H
3: #define EQUALIZER_H
4: #define EQ_BANDS 7
5: enum { EQ_BELL, EQ_LOW_SHELF, EQ_HIGH_SHELF, EQ_LOW_CUT, EQ_HIGH_CUT, EQ_OFF, EQ_SHAPES };
6: typedef struct { float frequency,gain,q; unsigned shape; } EQBand;
7: typedef struct { EQBand bands[EQ_BANDS]; } EQSettings;
8: typedef struct { double b[3],a[2],z[2][2]; } EQFilter;
9: typedef struct {
10:     EQFilter filters[EQ_BANDS],next[EQ_BANDS];
11:     unsigned shape[EQ_BANDS],next_shape[EQ_BANDS];
12:     float transition[EQ_BANDS];
13:     EQSettings current,target;
14:     float wet;
15:     unsigned tick;
16:     int ready;
17: } Equalizer;
18: EQSettings equalizer_default(void);
19: EQSettings equalizer_legacy(void);
20: const char *equalizer_shape_name(unsigned shape);
21: int equalizer_valid(EQSettings settings);
22: void equalizer_reset(Equalizer *eq);
23: /* Seven stereo biquads at 48 kHz; fixed storage, no callback allocation. */
24: void equalizer_process(Equalizer *eq,EQSettings settings,float wet,float stereo[2]);
25: float equalizer_response(EQSettings settings,float frequency);
26: #endif
```

## chorus.h

```c
1: // SPDX-License-Identifier: GPL-3.0-only
2: #ifndef CHORUS_H
3: #define CHORUS_H
4: #define CHORUS_DELAY_FRAMES 1024
5: /* Saved controls: rate in Hz, depth in milliseconds. */
6: typedef struct { float rate,depth; } ChorusSettings;
7: typedef struct {
8:     float delay[CHORUS_DELAY_FRAMES][2];
9:     double phase;
10:     float rate,depth,wet;
11:     float pre[2],post[2];
12:     unsigned cursor;
13:     int ready;
14: } Chorus;
15: ChorusSettings chorus_default(void);
16: int chorus_valid(ChorusSettings settings);
17: void chorus_reset(Chorus *chorus);
18: /* Fixed 48 kHz stereo processing, no allocation. Zero reported latency for the
19:    direct signal; delayed wet signal is the effect, not compensation latency. */
20: void chorus_process(Chorus *chorus,ChorusSettings settings,float wet,float stereo[2]);
21: #endif
```
