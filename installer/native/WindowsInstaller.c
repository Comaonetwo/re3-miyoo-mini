#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#define MSG_LINE (WM_APP+1)
#define MSG_DONE (WM_APP+2)
static HWND win,fields[3],choose[3],startButton,cancelButton,statusLabel,bar;
static wchar_t base[4096],cancelPath[4096],logPath[4096],selected[3][4096];
static wchar_t portVersion[32]=L"unknown";
static BOOL busy=FALSE,closeAfter=FALSE,reported=FALSE;
static int scale=100;
static int S(int x){return x*scale/100;}
static void enableControls(BOOL on){for(int i=0;i<3;i++)EnableWindow(choose[i],on);EnableWindow(startButton,on);EnableWindow(cancelButton,!on);}
static void cancelWork(void){HANDLE f=CreateFileW(cancelPath,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);if(f!=INVALID_HANDLE_VALUE)CloseHandle(f);SetWindowTextW(statusLabel,L"Cancelling safely. Please keep the card connected...");}
static void quoteArg(wchar_t*cmd,const wchar_t*arg){
 size_t n=wcslen(cmd);cmd[n++]=L'"';
 for(const wchar_t*p=arg;;){int sl=0;while(*p==L'\\'){sl++;p++;}if(*p==0||*p==L'"')sl*=2;while(sl--)cmd[n++]=L'\\';if(!*p)break;if(*p==L'"')cmd[n++]=L'\\';cmd[n++]=*p++;}
 cmd[n++]=L'"';cmd[n++]=L' ';cmd[n]=0;
}
static DWORD WINAPI worker(LPVOID unused){
 wchar_t python[4096],script[4096],cmd[32768]=L"";swprintf(python,4096,L"%ls\\backend\\runtime\\python.exe",base);swprintf(script,4096,L"%ls\\backend\\backend.py",base);
 const wchar_t*args[]={python,L"-u",script,L"--game",selected[0],L"--audio",selected[1],L"--card",selected[2],L"--cancel-file",cancelPath,L"--log",logPath};
 for(unsigned i=0;i<sizeof(args)/sizeof(args[0]);i++)quoteArg(cmd,args[i]);
 SECURITY_ATTRIBUTES sa={sizeof(sa),NULL,TRUE};HANDLE readPipe,writePipe;
 if(!CreatePipe(&readPipe,&writePipe,&sa,0)){PostMessageW(win,MSG_DONE,1,0);return 0;}SetHandleInformation(readPipe,HANDLE_FLAG_INHERIT,0);
 STARTUPINFOW si={0};si.cb=sizeof(si);si.dwFlags=STARTF_USESTDHANDLES;si.hStdOutput=writePipe;si.hStdError=writePipe;si.hStdInput=GetStdHandle(STD_INPUT_HANDLE);PROCESS_INFORMATION pi={0};
 BOOL ok=CreateProcessW(python,cmd,NULL,NULL,TRUE,CREATE_NO_WINDOW,NULL,base,&si,&pi);CloseHandle(writePipe);
 if(!ok){CloseHandle(readPipe);PostMessageW(win,MSG_DONE,1,0);return 0;}
 char chunk[2048],line[16384];size_t used=0;DWORD count;
 while(ReadFile(readPipe,chunk,sizeof(chunk),&count,NULL)&&count){
  for(DWORD i=0;i<count;i++){
   if(chunk[i]=='\n'){
    line[used]=0;int size=MultiByteToWideChar(CP_UTF8,0,line,-1,NULL,0);wchar_t*text=calloc(size+1,sizeof(wchar_t));if(text){MultiByteToWideChar(CP_UTF8,0,line,-1,text,size);PostMessageW(win,MSG_LINE,0,(LPARAM)text);}used=0;
   }else if(used<sizeof(line)-1)line[used++]=chunk[i];
  }
 }
 CloseHandle(readPipe);WaitForSingleObject(pi.hProcess,INFINITE);DWORD code=1;GetExitCodeProcess(pi.hProcess,&code);CloseHandle(pi.hThread);CloseHandle(pi.hProcess);PostMessageW(win,MSG_DONE,code,0);return 0;
}
static void browse(int kind){
 wchar_t path[4096]=L"";
 if(kind<2){OPENFILENAMEW of={0};of.lStructSize=sizeof(of);of.hwndOwner=win;of.lpstrFilter=L"ISO disc images\0*.iso\0";of.lpstrFile=path;of.nMaxFile=4096;of.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;if(!GetOpenFileNameW(&of))return;}
 else{BROWSEINFOW bi={0};bi.hwndOwner=win;bi.lpszTitle=L"Choose the root of the Miyoo SD card";bi.ulFlags=BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE;LPITEMIDLIST id=SHBrowseForFolderW(&bi);if(!id)return;BOOL ok=SHGetPathFromIDListW(id,path);CoTaskMemFree(id);if(!ok)return;}
 wcsncpy(selected[kind],path,4095);SetWindowTextW(fields[kind],path);
}
static HWND control(const wchar_t*cls,const wchar_t*text,DWORD style,int x,int y,int w,int h,int id){HWND c=CreateWindowExW(0,cls,text,WS_CHILD|WS_VISIBLE|style,S(x),S(y),S(w),S(h),win,(HMENU)(INT_PTR)id,GetModuleHandleW(NULL),NULL);SendMessageW(c,WM_SETFONT,(WPARAM)GetStockObject(DEFAULT_GUI_FONT),TRUE);return c;}
static LRESULT CALLBACK wndproc(HWND h,UINT msg,WPARAM wp,LPARAM lp){
 switch(msg){
 case WM_CREATE:{win=h;HDC dc=GetDC(h);scale=GetDeviceCaps(dc,LOGPIXELSX)*100/96;ReleaseDC(h,dc);
  control(L"STATIC",L"GTA III Installer",0,25,22,560,30,0);wchar_t banner[128];swprintf(banner,128,L"Miyoo Mini | OnionOS | Port %ls",portVersion);control(L"STATIC",banner,0,25,54,560,22,0);
  control(L"STATIC",L"Prepare your own classic PC game. Nothing is downloaded.",0,25,87,560,24,0);
  const wchar_t*titles[]={L"Game disc",L"Audio disc",L"Miyoo SD card (drive root)"};const wchar_t*placeholder[]={L"Drop GTAIII.iso here",L"Drop GTA3_audio.iso here",L"Choose the SD card itself"};
  for(int i=0;i<3;i++){int y=130+i*75;control(L"STATIC",titles[i],0,25,y,550,20,0);fields[i]=control(L"EDIT",placeholder[i],ES_READONLY|ES_AUTOHSCROLL|WS_BORDER,25,y+24,448,28,100+i);choose[i]=control(L"BUTTON",L"Choose...",WS_TABSTOP,484,y+23,95,29,200+i);}
  bar=control(PROGRESS_CLASSW,L"",0,25,365,554,18,0);SendMessageW(bar,PBM_SETRANGE,0,MAKELPARAM(0,100));
  statusLabel=control(L"STATIC",L"Choose both original PC discs and the Miyoo SD card.",0,25,397,554,62,0);
  control(L"STATIC",L"Existing saves and settings are kept.",0,25,477,310,23,0);cancelButton=control(L"BUTTON",L"Cancel",WS_TABSTOP,335,470,80,32,301);startButton=control(L"BUTTON",L"Prepare and install",WS_TABSTOP|BS_DEFPUSHBUTTON,425,470,154,32,300);EnableWindow(cancelButton,FALSE);DragAcceptFiles(h,TRUE);return 0;}
 case WM_COMMAND:{int id=LOWORD(wp);if(id>=200&&id<203&&!busy)browse(id-200);
  if(id==300&&!busy){if(!selected[0][0]||!selected[1][0]||!selected[2][0]){SetWindowTextW(statusLabel,L"Choose both discs and the SD card first.");return 0;}DeleteFileW(cancelPath);busy=TRUE;reported=FALSE;enableControls(FALSE);SendMessageW(bar,PBM_SETPOS,0,0);HANDLE th=CreateThread(NULL,0,worker,NULL,0,NULL);if(th)CloseHandle(th);else{busy=FALSE;enableControls(TRUE);}}
  if(id==301&&busy)cancelWork();return 0;}
 case WM_DROPFILES:{if(busy){DragFinish((HDROP)wp);return 0;}POINT pt;DragQueryPoint((HDROP)wp,&pt);UINT n=DragQueryFileW((HDROP)wp,0xFFFFFFFF,NULL,0);int slot=pt.y>=S(275)?2:pt.y>=S(200)?1:0;
  for(UINT i=0;i<n;i++){wchar_t path[4096];DragQueryFileW((HDROP)wp,i,path,4096);DWORD at=GetFileAttributesW(path);int dest=(at!=INVALID_FILE_ATTRIBUTES&&(at&FILE_ATTRIBUTE_DIRECTORY))?2:slot;if(dest<2){const wchar_t*ext=wcsrchr(path,L'.');if(!ext||_wcsicmp(ext,L".iso"))continue;}wcsncpy(selected[dest],path,4095);SetWindowTextW(fields[dest],path);if(slot==0)slot=1;}DragFinish((HDROP)wp);return 0;}
 case MSG_LINE:{wchar_t*t=(wchar_t*)lp;if(!wcsncmp(t,L"PROGRESS\t",9)){wchar_t*next=wcschr(t+9,L'\t');if(next){*next=0;SendMessageW(bar,PBM_SETPOS,_wtoi(t+9),0);SetWindowTextW(statusLabel,next+1);}}
  else if(!wcsncmp(t,L"ERROR\t",6)||!wcsncmp(t,L"CANCELLED\t",10)){SetWindowTextW(statusLabel,wcschr(t,L'\t')+1);reported=TRUE;}free(t);return 0;}
 case MSG_DONE:busy=FALSE;enableControls(TRUE);DeleteFileW(cancelPath);if(wp!=0&&!reported)SetWindowTextW(statusLabel,L"Installation stopped. See the GTA3 Installer log in your local AppData folder.");if(closeAfter)DestroyWindow(h);return 0;
 case WM_CLOSE:if(busy){if(MessageBoxW(h,L"Cancel installation and close when it has stopped? Keep the card connected.",L"Installation is running",MB_YESNO|MB_ICONQUESTION)==IDYES){closeAfter=TRUE;cancelWork();}return 0;}DestroyWindow(h);return 0;
 case WM_DESTROY:PostQuitMessage(0);return 0;
 }return DefWindowProcW(h,msg,wp,lp);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE prev,PWSTR args,int show){
 SetProcessDPIAware();CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);INITCOMMONCONTROLSEX cc={sizeof(cc),ICC_PROGRESS_CLASS};InitCommonControlsEx(&cc);
 GetModuleFileNameW(NULL,base,4096);wchar_t*slash=wcsrchr(base,L'\\');if(slash)*slash=0;
 wchar_t versionPath[4096];swprintf(versionPath,4096,L"%ls\\backend\\VERSION",base);FILE*vf=_wfopen(versionPath,L"rb");if(vf){char v[32]={0};if(fgets(v,sizeof(v),vf)){v[strcspn(v,"\r\n")]=0;MultiByteToWideChar(CP_UTF8,0,v,-1,portVersion,32);}fclose(vf);}
 wchar_t tmp[4096];GetTempPathW(4096,tmp);swprintf(cancelPath,4096,L"%lsgta3-cancel-%lu.tmp",tmp,GetCurrentProcessId());
 wchar_t appdata[4096];SHGetFolderPathW(NULL,CSIDL_LOCAL_APPDATA,NULL,0,appdata);swprintf(logPath,4096,L"%ls\\GTA3 Installer\\installation.log",appdata);
 SetEnvironmentVariableW(L"PYTHONHOME",NULL);SetEnvironmentVariableW(L"PYTHONPATH",NULL);SetEnvironmentVariableW(L"PYTHONNOUSERSITE",L"1");SetEnvironmentVariableW(L"PYTHONDONTWRITEBYTECODE",L"1");
 WNDCLASSW c={0};c.lpfnWndProc=wndproc;c.hInstance=instance;c.lpszClassName=L"GTA3MiyooInstaller";c.hCursor=LoadCursor(NULL,IDC_ARROW);c.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&c);
 HDC dc=GetDC(NULL);scale=GetDeviceCaps(dc,LOGPIXELSX)*100/96;ReleaseDC(NULL,dc);
 HWND h=CreateWindowExW(0,c.lpszClassName,L"GTA III Installer",WS_OVERLAPPEDWINDOW&~(WS_MAXIMIZEBOX|WS_THICKFRAME),CW_USEDEFAULT,CW_USEDEFAULT,S(625),S(565),NULL,NULL,instance,NULL);ShowWindow(h,show);
 MSG m;while(GetMessageW(&m,NULL,0,0)>0){if(!IsDialogMessageW(h,&m)){TranslateMessage(&m);DispatchMessageW(&m);}}CoUninitialize();return(int)m.wParam;
}
