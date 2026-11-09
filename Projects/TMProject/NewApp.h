#pragma once

class EventTranslator;
class RenderDevice;
class CSoundManager;
class TMVideoWnd;
class DS_SOUND_MANAGER;
class JBlur;
class TimerManager;
class ObjectManager;
class CPSock;

struct TMConfig
{
    int version = 10000;   // [VERSION]  (read, unused by the client)
    int res = 2;           // [RES]      1..11 index into the 748 resolution table
    int animation = 1;     // [ANIMATION]
    int sound = 1;         // [SOUND]
    int music = 0;         // [MUSIC]
    int server = -1;       // [SERVER]   (read, unused by the client)
    int bright = 50;       // [BRIGHT]   0..100
    int cursor = 0;        // [CURSOR]
    int demo = 1;          // [DEMO]
    int window = -1;       // [WINDOW]   -1 = keep command line default
    int classic = 0;       // [CLASSIC]  (the 748 client forces UI2 regardless)
    int cameraRotate = 0;  // [CAMERAROTATE]
    int dxt = 0;           // [DXT]
    int keyType = 0;       // [KEYTYPE]
    int cameraView = 1;    // [CAMERAVIEW]
};

class NewApp
{
public:
    NewApp();
    virtual ~NewApp();

    virtual HRESULT Initialize(HINSTANCE hInstance, int nFull);
    HRESULT InitDevice();
    void InitServerName();
    void InitMusicList();
    virtual HRESULT Finalize();
    virtual DWORD Run();
    virtual void OnCreate(HWND hWnd, DWORD wParam, int lParam);
    virtual HRESULT RenderScene();
    void PrintMousePosition();
    virtual void SetObjectManager(ObjectManager* pManager);
    virtual EventTranslator* GetEventTranslator();

    HWND GetSafeHwnd();
    void SwitchWebBrowserState(int nEmptyCargo);
    void SwitchWebBoard();
    DWORD GetHttpRequest(char* httpname, char* Request, int MaxBuffer);
    void MixHelp();
    int BASE_Initialize_NewServerList();
    void InitServerNameMR();
    HRESULT MsgProc(HWND hWnd, DWORD uMsg, DWORD wParam, int lParam);
    bool CheckResolution(DWORD x, DWORD y, DWORD bpp);

    char base_chinaTid(char* TID, char* Id);

public:
    HWND m_hWnd;
    int m_bActive;
    unsigned int m_dwWindowStyle;
    unsigned int m_dwScreenWidth;
    unsigned int m_dwScreenHeight;
    unsigned int m_dwColorBit;
    int m_bwFullScreen;
    char m_strWindowTitle[64];
    char m_szServerIP[128];
    char m_szLoginServerIP[128];
    RenderDevice* m_pRenderDevice;
    CSoundManager* m_pSoundManager;
    TMVideoWnd* m_pAviPlayer;
    DS_SOUND_MANAGER* m_pBGMManager;
    EventTranslator* m_pEventTranslator;
    TimerManager* m_pTimerManager;
    ObjectManager* m_pObjectManager;
    CPSock* m_pSocketManager;
    int m_bTex;
    int m_nSound;
    int m_nMusic;
    int m_nCameraView;
    int m_Winstate;
    bool m_binactive;
    HBITMAP m_hBMBtnBG;
    bool china_bWrite;
    int china_Playtime;
    JBlur* m_pBlur;
};

void InitServerName2();
void CreateConsole();