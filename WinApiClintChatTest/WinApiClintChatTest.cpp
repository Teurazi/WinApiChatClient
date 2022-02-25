#include "framework.h"
#include "WinApiClintChatTest.h"
#include <WinSock2.h>
#include <process.h>
#include <iostream>

#pragma comment(lib,"ws2_32.lib")   //ws2_32.lib 의경로 명시

#define MAX_LOADSTRING 100

//WinApi 컨트롤, 에디트 등의 아이디와 핸들선언
#define ID_EDIT_Write 1001  //채팅을 입력하는 edit
#define ID_EDIT_Read 1002   //서버로부터 입력받은 문자를 입력하는 edit
#define ID_EDIT_IP 1003     //서버연결을위해 ip를 입력하는 edit
#define ID_EDIT_Name 1004   //채팅에서 보여질 자신의 이름을 입력하는 edit

#define ID_BUTTON_IP 2001   //ID_EDIT_IP 에서 입력된 ip를 서버에 연결하는데 이용할 button
#define ID_BUTTON_Enter 2002    //ID_EDIT_Write 에서 입력된 문자열을 서버에 전달하는데 이용할 button
#define ID_BUTTON_Change 2003   //ID_EDIT_Name 에서 입력된 문자열을 이름을 변경하고 서버에 알리는데 이용할 button

HWND hEdit_write;   //ID_EDIT_Write 의 핸들
HWND hEdit_read;    //ID_EDIT_Read 의 핸들
HWND hEdit_ip;      //ID_EDIT_IP 의 핸들
HWND hEdit_name;    //ID_EDIT_Name 의 핸들

//인터넷 통신을위한 변수들
#define PORT	3500    //포트번호
#define BUF_SIZE 1024   //버퍼사이즈
#define NAME_SIZE 128   //이름사이즈

SOCKADDR_IN addr;   //서버 주소를 저장할 구조체
SOCKET sock;    //서버와 통신할 소켓 
HANDLE  recvThread;	// 송신스레드
char name[NAME_SIZE] = "[DEFAULT]";     //이름문자열

unsigned WINAPI RecvMsg(void* arg);//쓰레드 수신함수

// 전역 변수:
HINSTANCE hInst;                                // 현재 인스턴스입니다.
WCHAR szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트입니다.
WCHAR szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름입니다.

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    //인터넷 통신을 위한 초기작업
    WSADATA WSAData;

    if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);   //소켓 할당
    if (sock == INVALID_SOCKET) {
        return 1;
    }


    // 전역 문자열을 초기화합니다.
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_WINAPICLINTCHATTEST, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 애플리케이션 초기화를 수행합니다:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WINAPICLINTCHATTEST));

    MSG msg;

    // 기본 메시지 루프입니다:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINAPICLINTCHATTEST));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_WINAPICLINTCHATTEST);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 인스턴스 핸들을 전역 변수에 저장합니다.

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE: 
        //컨트롤생성
        hEdit_write = CreateWindow(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER |
            ES_AUTOVSCROLL, 10, 360, 500, 25, hWnd, (HMENU)ID_EDIT_Write, hInst, NULL);
        hEdit_read = CreateWindow(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER |
            ES_AUTOVSCROLL | ES_READONLY | ES_MULTILINE, 10, 50, 600, 300, hWnd, (HMENU)ID_EDIT_Read, hInst, NULL);
        hEdit_ip = CreateWindow(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER |
            ES_AUTOVSCROLL, 10, 10, 200, 25, hWnd, (HMENU)ID_EDIT_IP, hInst, NULL);
        hEdit_name = CreateWindow(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER |
            ES_AUTOVSCROLL, 300, 10, 200, 25, hWnd, (HMENU)ID_EDIT_Name, hInst, NULL);

        CreateWindow(L"button", L"접속하기", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            210, 10, 80, 25, hWnd, (HMENU)ID_BUTTON_IP, hInst, NULL);
        CreateWindow(L"button", L"보내기", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            520, 360, 80, 25, hWnd, (HMENU)ID_BUTTON_Enter, hInst, NULL);
        CreateWindow(L"button", L"닉네임변경", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            500, 10, 80, 25, hWnd, (HMENU)ID_BUTTON_Change, hInst, NULL);
        break;
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
            case ID_BUTTON_IP: {
                char inputIP[128];
                wchar_t w_input[128];
                GetWindowText(hEdit_ip, w_input, 256);

                //wchar_t를 char형으로 바꾸는 코드
                int nLen = WideCharToMultiByte(CP_ACP, 0, w_input, -1, NULL, 0, NULL, NULL);
                WideCharToMultiByte(CP_ACP, 0, w_input, -1, inputIP, nLen, NULL, NULL);

                addr.sin_family = AF_INET;
                addr.sin_port = htons(PORT);
                addr.sin_addr.S_un.S_addr = inet_addr(inputIP); //아이피입력

                if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
                    SetWindowText(hEdit_read, L"연결실패\n"); // hEditOUTPUT의 Text 내보내기
                    TerminateThread(recvThread,0);  //연결실패시에도 스레드가 살아있을경우 강제 종료하기
                    closesocket(sock);  //소켓닫기
                    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);   //소켓 재할당
                }
                else {
                    SetWindowText(hEdit_read, L"연결성공\r\n"); // hEditOUTPUT의 Text 내보내기
                    recvThread = (HANDLE)_beginthreadex(NULL, 0, RecvMsg, (void*)&sock, 0, NULL);//메시지 수신용 쓰레드가 실행된다.
                }
            }
            break;
            case ID_BUTTON_Enter:
            {
                char nameMsg[NAME_SIZE + BUF_SIZE];
                char msg[BUF_SIZE];
                TCHAR w_input[256]; //hEdit_write edit 의 문자를 받을 아스키코드를 이용하는 문자열
                GetWindowText(hEdit_write, w_input, 256);

                //wchar_t를 char형으로 바꾸는 코드
                int nLen = WideCharToMultiByte(CP_ACP, 0, w_input, -1, NULL, 0, NULL, NULL);
                WideCharToMultiByte(CP_ACP, 0, w_input, -1, msg, nLen, NULL, NULL);

                sprintf(nameMsg, "%s %s", name, msg);//nameMsg에 name + msg 메시지를 출력한다
                send(sock, nameMsg, strlen(nameMsg), 0);//nameMsg를 서버에게 전송한다.
                SetWindowText(hEdit_write, L""); // hEditOUTPUT의 text를 비워주기
            }
            break;
            case ID_BUTTON_Change:
            {
                wchar_t w_input[256]; //hEdit_name edit 의 문장을 받을 아스키코드를 이용하는 문자열
                char tempName[NAME_SIZE];   //기존의 이름을 저장할 문자열
                char tempChar[NAME_SIZE + BUF_SIZE];    //서버로 문장을 보낼 문자열 및 새로운 이름을 저장할 문자열
                strcpy(tempName, name); //기존의 이름 복사
                GetWindowText(hEdit_name, w_input, 256);  //hEdit_name edit 의 문장받기

                //wchar_t를 char형으로 바꾸는 코드
                int nLen = WideCharToMultiByte(CP_ACP, 0, w_input, -1, NULL, 0, NULL, NULL);
                WideCharToMultiByte(CP_ACP, 0, w_input, -1, tempChar, nLen, NULL, NULL);

                sprintf(name, "[%s]", tempChar);    //새로받은 이름을 복사
                sprintf(tempChar, "%s 님이 %s 으로 이름을 변경했습니다.", tempName, name);//nameMsg에 메시지를 출력한다.
                send(sock, tempChar, strlen(tempChar), 0);//nameMsg를 서버에게 전송한다.
            }
            break;
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_DESTROY:
        closesocket(sock);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

unsigned WINAPI RecvMsg(void* arg) {
    SOCKET sock = *((SOCKET*)arg);          //서버용 소켓을 전달한다.
    char nameMsg[NAME_SIZE + BUF_SIZE];
    TCHAR *tchar;
    int strLen;
    int len;

    while (1) {
        strLen = recv(sock, nameMsg, NAME_SIZE + BUF_SIZE - 1, 0);//서버로부터 메시지를 수신한다.
        if (strLen == -1) {
            break;
        }
        
        //윈도우에서는 \r\n을 이용해야 줄바꿈이 된다
        nameMsg[strLen] = '\r';
        nameMsg[strLen+1] = '\n';
        nameMsg[strLen+2] = '\0';//문자열의 끝을 알리기 위해 설정
        
        int len = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, nameMsg, -1, NULL, NULL);
        tchar = new TCHAR[len]; 
        MultiByteToWideChar(CP_ACP, 0, nameMsg, -1, tchar, len);

        len = GetWindowTextLength(hEdit_read); // hEdit_read의 Text 길이를 가져온다
        // hEdit_read의 선택영역의 처음과 끝을 len으로 지정
        // 그 결과 캐럿은 len의 위치로 이동하게 됨
        SendMessage(hEdit_read, EM_SETSEL, len, len);
        // 선택 영역을 nameMsg 문자열로 대체한다.
        SendMessage(hEdit_read, EM_REPLACESEL,  TRUE, (LPARAM)tchar);
    }
    return 0;
}