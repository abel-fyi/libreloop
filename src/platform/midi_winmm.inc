// SPDX-License-Identifier: GPL-3.0-only
/* WinMM source candidate. Only this OS message thread produces MIDI events;
   the UI consumes the unchanged2048-entry SPSC ring. Native Windows unverified. */
#include <mmsystem.h>
#include <dbt.h>
static HANDLE win_thread,win_ready;
static DWORD win_thread_id;
static atomic_int win_stopping,win_opened,win_requested;
static HMIDIIN win_device; /* Message-thread owned. */
static LRESULT CALLBACK win_midi_window(HWND window,UINT message,WPARAM wp,LPARAM lp) {
    (void)wp;
    if(!atomic_load(&win_stopping)) {
        if(message==MM_MIM_DATA || message==MM_MIM_MOREDATA) {
            DWORD packed=(DWORD)lp;
            emit(NULL,(MidiEvent){(unsigned char)packed,(unsigned char)(packed>>8),
                (unsigned char)(packed>>16),midi_input_time()});return 0;
        }
        if(message==MM_MIM_ERROR || message==MM_MIM_LONGERROR) {
            atomic_store(&failed,1);notify_change();return 0;
        }
        if(message==MM_MIM_CLOSE) {
            atomic_store(&failed,2);notify_change();return 0;
        }
        if(message==WM_DEVICECHANGE)notify_change();
    }
    return DefWindowProcW(window,message,wp,lp);
}
static DWORD WINAPI win_receive(LPVOID unused) {
    (void)unused;MSG message;PeekMessageW(&message,NULL,0,0,PM_NOREMOVE);
    HINSTANCE instance=GetModuleHandleW(NULL);
    WNDCLASSW cls={0};cls.lpfnWndProc=win_midi_window;cls.hInstance=instance;cls.lpszClassName=L"LibreLoopMidiInput";
    if(!RegisterClassW(&cls) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) {
        SetEvent(win_ready);return 1;
    }
    HWND window=CreateWindowExW(WS_EX_TOOLWINDOW,cls.lpszClassName,L"",WS_POPUP,
        0,0,0,0,NULL,NULL,instance,NULL);
    if(!window){SetEvent(win_ready);return 1;}
    DEV_BROADCAST_DEVICEINTERFACE_W filter={0};filter.dbcc_size=sizeof filter;filter.dbcc_devicetype=DBT_DEVTYP_DEVICEINTERFACE;
    HDEVNOTIFY notification=RegisterDeviceNotificationW(window,&filter,
        DEVICE_NOTIFY_WINDOW_HANDLE|DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);
    int requested=atomic_load(&win_requested),ok=requested<0;
    if(requested>=0 && midiInOpen(&win_device,(UINT)requested,(DWORD_PTR)window,0,
        CALLBACK_WINDOW|MIDI_IO_STATUS)==MMSYSERR_NOERROR) {
        ok=midiInStart(win_device)==MMSYSERR_NOERROR;
    }
    atomic_store(&win_opened,ok);SetEvent(win_ready);
    if(ok)while(!atomic_load(&win_stopping)) {
        int result=(int)GetMessageW(&message,NULL,0,0);
        if(result<=0){if(result<0){atomic_store(&failed,2);notify_change();}break;}
        TranslateMessage(&message);DispatchMessageW(&message);
    }
    atomic_store(&win_stopping,1);
    if(win_device){midiInStop(win_device);midiInReset(win_device);midiInClose(win_device);win_device=NULL;}
    if(notification)UnregisterDeviceNotification(notification);DestroyWindow(window);return 0;
}
int midi_input_devices(MidiDevice *devices,int capacity) {
    int count=0;UINT total=midiInGetNumDevs();
    for(UINT i=0;i<total && count<capacity;i++) {
        MIDIINCAPSW caps;if(midiInGetDevCapsW(i,&caps,sizeof caps)!=MMSYSERR_NOERROR)continue;
        snprintf(devices[count].id,sizeof devices[count].id,"winmm:%u",i);
        if(!WideCharToMultiByte(CP_UTF8,0,caps.szPname,-1,devices[count].name,
            sizeof devices[count].name,NULL,NULL))snprintf(devices[count].name,sizeof devices[count].name,"MIDI input %u",i+1);
        count++;
    }
    return count;
}
void midi_input_close(void) {
    if(win_thread) {
        atomic_store(&win_stopping,1);PostThreadMessageW(win_thread_id,WM_QUIT,0,0);
        WaitForSingleObject(win_thread,INFINITE);CloseHandle(win_thread);win_thread=NULL;
    }
    if(win_ready){CloseHandle(win_ready);win_ready=NULL;}
    win_thread_id=0;midi_input_flush();
}
int midi_input_open(const char *id,char error[256]) {
    midi_input_close();unsigned index=0;char tail;
    if(*id && (sscanf(id,"winmm:%u%c",&index,&tail)!=1 || index>=midiInGetNumDevs())) {
        snprintf(error,256,"WinMM MIDI input unavailable");return 0;
    }
    atomic_store(&win_requested,*id?(int)index:-1);atomic_store(&win_stopping,0);atomic_store(&win_opened,0);
    win_ready=CreateEventW(NULL,TRUE,FALSE,NULL);
    if(win_ready)win_thread=CreateThread(NULL,0,win_receive,NULL,0,&win_thread_id);
    if(!win_thread){midi_input_close();snprintf(error,256,"Cannot start WinMM input thread");return 0;}
    WaitForSingleObject(win_ready,INFINITE);
    if(!atomic_load(&win_opened)){midi_input_close();snprintf(error,256,"Cannot connect to WinMM MIDI input");return 0;}
    return 1;
}
