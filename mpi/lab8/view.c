#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define ID_EDIT_M       101
#define ID_EDIT_N       102
#define ID_BTN_UPDATE   103
#define ID_BTN_LOAD_0   104
#define ID_BTN_LOAD_F   105
#define ID_SLIDER_WIPE  106
#define ID_BTN_PLAY_W   107 
#define ID_BTN_PLAY_F   108 
#define ID_TIMER_ANIM   1

// Variabili Globali
uint8_t *g_buf_0 = NULL, *g_buf_f = NULL;
long long g_M = 840, g_N = 840;
int g_wipe_percent = 50; 
float g_pop_0 = 0.0f, g_pop_f = 0.0f;
long long g_diff_count = 0;

// Gestione Animazione
int g_anim_mode = 0; // 0: Manuale/Wipe, 1: Auto-Wipe, 2: Auto-Fade
float g_wipe_float = 50.0f; 
float g_anim_speed = 0.6f;   
int g_anim_direction = 1;
float g_pulse = 0.0f;

HWND hEditM, hEditN, hBtnUpdate, hBtnLoad0, hBtnLoadF, hSliderWipe, hBtnPlayW, hBtnPlayF;
HWND hLblM, hLblN;
HFONT hFont, hFontBold, hFontSmall;
HBRUSH hBgBrush, hEditBgBrush; 

typedef struct { BITMAPINFOHEADER bmiHeader; RGBQUAD bmiColors[256]; } BITMAPINFO_256;

// --- UTILITY ---
void SetControlFont(HWND hCtrl, HFONT font) { SendMessage(hCtrl, WM_SETFONT, (WPARAM)font, TRUE); }

void AnalyzeData() {
    if (!g_buf_0 || !g_buf_f) return;
    long long c0 = 0, cf = 0, diff = 0, total = g_M * g_N;
    for (long long i = 0; i < total; i++) {
        if (g_buf_0[i] > 0) c0++;
        if (g_buf_f[i] > 0) cf++;
        if (g_buf_0[i] != g_buf_f[i]) diff++;
    }
    g_pop_0 = (float)c0 / total * 100.0f;
    g_pop_f = (float)cf / total * 100.0f;
    g_diff_count = diff;
}

BOOL ChooseBinFile(HWND hwnd, char* out_path) {
    OPENFILENAME ofn = {0};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = out_path;
    out_path[0] = '\0';
    ofn.nMaxFile = 260;
    ofn.lpstrFilter = "Binary Files\0*.bin\0All Files\0*.*\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    return GetOpenFileName(&ofn);
}

// --- WNDPROC ---
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            int dark = 1; DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark)); 
            hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            hFontBold = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            hFontSmall = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            hBgBrush = CreateSolidBrush(RGB(20, 20, 22)); hEditBgBrush = CreateSolidBrush(RGB(40, 40, 45));

            hLblM = CreateWindow("STATIC", "Righe (M):", WS_CHILD | WS_VISIBLE, 30, 25, 70, 20, hwnd, NULL, NULL, NULL);
            hEditM = CreateWindow("EDIT", "840", WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_CENTER, 100, 22, 70, 24, hwnd, (HMENU)ID_EDIT_M, NULL, NULL);
            hLblN = CreateWindow("STATIC", "Colonne (N):", WS_CHILD | WS_VISIBLE, 190, 25, 85, 20, hwnd, NULL, NULL, NULL);
            hEditN = CreateWindow("EDIT", "840", WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_CENTER, 280, 22, 70, 24, hwnd, (HMENU)ID_EDIT_N, NULL, NULL);
            hBtnUpdate = CreateWindow("BUTTON", "APPLICA", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 380, 20, 100, 28, hwnd, (HMENU)ID_BTN_UPDATE, NULL, NULL);
            
            hBtnLoad0 = CreateWindow("BUTTON", "GEN 0", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 30, 65, 140, 35, hwnd, (HMENU)ID_BTN_LOAD_0, NULL, NULL);
            hBtnLoadF = CreateWindow("BUTTON", "FINALE", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 180, 65, 140, 35, hwnd, (HMENU)ID_BTN_LOAD_F, NULL, NULL);
            
            hBtnPlayW = CreateWindow("BUTTON", "ANIM. WIPE", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 330, 65, 150, 35, hwnd, (HMENU)ID_BTN_PLAY_W, NULL, NULL);
            hBtnPlayF = CreateWindow("BUTTON", "ANIM. FADE", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW, 490, 65, 150, 35, hwnd, (HMENU)ID_BTN_PLAY_F, NULL, NULL);

            hSliderWipe = CreateWindowEx(0, TRACKBAR_CLASS, "Wipe", WS_CHILD | WS_VISIBLE | TBS_NOTICKS, 20, 140, 800, 30, hwnd, (HMENU)ID_SLIDER_WIPE, NULL, NULL);
            SendMessage(hSliderWipe, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
            SendMessage(hSliderWipe, TBM_SETPOS, TRUE, 50);

            SetControlFont(hLblM, hFont); SetControlFont(hEditM, hFont);
            SetControlFont(hLblN, hFont); SetControlFont(hEditN, hFont);
            return 0;
        }

        case WM_TIMER: {
            if (wParam == ID_TIMER_ANIM) {
                if (g_anim_mode == 1) { // Auto Wipe
                    g_wipe_float += (g_anim_speed * g_anim_direction);
                    if (g_wipe_float >= 100.0f || g_wipe_float <= 0.0f) g_anim_direction *= -1;
                    g_wipe_percent = (int)g_wipe_float;
                    SendMessage(hSliderWipe, TBM_SETPOS, TRUE, g_wipe_percent);
                } else if (g_anim_mode == 2) { // Auto Fade
                    g_pulse += 0.04f;
                    g_wipe_percent = (int)(50 + sin(g_pulse) * 50);
                }
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }

        case WM_ERASEBKGND: return 1;
        case WM_CTLCOLORSTATIC: SetTextColor((HDC)wParam, RGB(220, 220, 220)); SetBkMode((HDC)wParam, TRANSPARENT); return (INT_PTR)hBgBrush;
        case WM_CTLCOLOREDIT: SetTextColor((HDC)wParam, RGB(255, 255, 255)); SetBkColor((HDC)wParam, RGB(40, 40, 45)); return (INT_PTR)hEditBgBrush;

        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT p = (LPDRAWITEMSTRUCT)lParam;
            COLORREF col = RGB(60, 60, 65);
            if (p->CtlID == ID_BTN_LOAD_0) col = RGB(0, 160, 210);
            else if (p->CtlID == ID_BTN_LOAD_F) col = RGB(200, 30, 110);
            else if (p->CtlID == ID_BTN_PLAY_W) col = (g_anim_mode == 1) ? RGB(220, 50, 50) : RGB(50, 150, 80);
            else if (p->CtlID == ID_BTN_PLAY_F) col = (g_anim_mode == 2) ? RGB(220, 50, 50) : RGB(100, 50, 200);

            HBRUSH br = CreateSolidBrush(col); FillRect(p->hDC, &p->rcItem, br); DeleteObject(br);
            char t[64]; GetWindowText(p->hwndItem, t, 64);
            SetTextColor(p->hDC, RGB(255, 255, 255)); SetBkMode(p->hDC, TRANSPARENT);
            SelectObject(p->hDC, hFontBold); DrawText(p->hDC, t, -1, &p->rcItem, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            return TRUE;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == ID_BTN_UPDATE) {
                char m[16], n[16]; GetWindowText(hEditM, m, 16); GetWindowText(hEditN, n, 16);
                g_M = atoll(m); g_N = atoll(n); InvalidateRect(hwnd, NULL, FALSE);
            } else if (id == ID_BTN_LOAD_0 || id == ID_BTN_LOAD_F) {
                char path[260]; if (ChooseBinFile(hwnd, path)) {
                    FILE *f = fopen(path, "rb"); if(f) {
                        uint8_t **b = (id == ID_BTN_LOAD_0) ? &g_buf_0 : &g_buf_f;
                        if(*b) free(*b); *b = malloc(g_M * g_N);
                        fread(*b, 1, g_M * g_N, f); fclose(f); AnalyzeData();
                        InvalidateRect(hwnd, NULL, FALSE);
                    }
                }
            } else if (id == ID_BTN_PLAY_W || id == ID_BTN_PLAY_F) {
                int target = (id == ID_BTN_PLAY_W) ? 1 : 2;
                if (g_anim_mode == target) { KillTimer(hwnd, ID_TIMER_ANIM); g_anim_mode = 0; }
                else { g_anim_mode = target; SetTimer(hwnd, ID_TIMER_ANIM, 16, NULL); }
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
        }

        case WM_HSCROLL: 
            if (g_anim_mode != 1) { // L'utente comanda solo se non c'è auto-wipe
                g_wipe_percent = SendMessage(hSliderWipe, TBM_GETPOS, 0, 0); 
                g_wipe_float = (float)g_wipe_percent;
                InvalidateRect(hwnd, NULL, FALSE); 
            }
            break;

        case WM_PAINT: {
            PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps);
            RECT r; GetClientRect(hwnd, &r);
            HDC mdc = CreateCompatibleDC(hdc); HBITMAP mbm = CreateCompatibleBitmap(hdc, r.right, r.bottom);
            SelectObject(mdc, mbm); FillRect(mdc, &r, hBgBrush);

            char stats[256];
            sprintf(stats, "Gen 0: %.2f%%  |  Finale: %.2f%%  |  Variazioni: %lld", g_pop_0, g_pop_f, g_diff_count);
            SetTextColor(mdc, RGB(0, 210, 255)); SelectObject(mdc, hFontBold); SetBkMode(mdc, TRANSPARENT);
            RECT stR = {0, 110, r.right, 135}; DrawText(mdc, stats, -1, &stR, DT_CENTER|DT_VCENTER|DT_SINGLELINE);

            int dsy = 180, dh = r.bottom - dsy, sx = (g_wipe_percent * r.right) / 100;
            if (g_buf_0 || g_buf_f) {
                SetStretchBltMode(mdc, COLORONCOLOR);
                if (g_anim_mode == 2) { // LOGICA FADE: Sfumatura cromatica su tutta la griglia
                    uint8_t* b = (g_wipe_percent < 50) ? (g_buf_0?g_buf_0:g_buf_f) : (g_buf_f?g_buf_f:g_buf_0);
                    BITMAPINFO_256 bmi = {{sizeof(BITMAPINFOHEADER), (LONG)g_N, -(LONG)g_M, 1, 8, BI_RGB, 0,0,0,0,0}};
                    bmi.bmiColors[0] = (RGBQUAD){12,12,14,0};
                    int rf = (g_wipe_percent*200)/100, gf = 200-(g_wipe_percent*180)/100, bf = 255;
                    bmi.bmiColors[1] = (RGBQUAD){(BYTE)rf, (BYTE)gf, (BYTE)bf, 0};
                    StretchDIBits(mdc, 0, dsy, r.right, dh, 0, 0, (int)g_N, (int)g_M, b, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, SRCCOPY);
                } else { // LOGICA WIPE (Manuale o Animata): Divisione fisica verticale
                    for(int s=0; s<2; s++) {
                        uint8_t* b = (s==0)?g_buf_0:g_buf_f; if(!b) continue;
                        BITMAPINFO_256 bmi = {{sizeof(BITMAPINFOHEADER), (LONG)g_N, -(LONG)g_M, 1, 8, BI_RGB, 0,0,0,0,0}};
                        bmi.bmiColors[0] = (RGBQUAD){15,15,15,0};
                        bmi.bmiColors[1] = (s==0)?(RGBQUAD){255,200,0,0}:(RGBQUAD){200,30,110,0};
                        HRGN hr = (s==0)?CreateRectRgn(0, dsy, sx, r.bottom) : CreateRectRgn(sx, dsy, r.right, r.bottom);
                        SelectClipRgn(mdc, hr);
                        StretchDIBits(mdc, 0, dsy, r.right, dh, 0, 0, (int)g_N, (int)g_M, b, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, SRCCOPY);
                        SelectClipRgn(mdc, NULL); DeleteObject(hr);
                    }
                    HPEN hP = CreatePen(PS_SOLID, 2, RGB(255, 255, 255)); SelectObject(mdc, hP);
                    MoveToEx(mdc, sx, dsy, NULL); LineTo(mdc, sx, r.bottom); DeleteObject(hP);
                }
                if (g_M <= 200 && g_N <= 200) { // Griglia Hi-Fi
                    HPEN hG = CreatePen(PS_SOLID, 1, RGB(40, 40, 45)); SelectObject(mdc, hG);
                    for(int i=0; i<=g_N; i++) { int x=(i*r.right)/g_N; MoveToEx(mdc, x, dsy, NULL); LineTo(mdc, x, r.bottom); }
                    for(int i=0; i<=g_M; i++) { int y=dsy+(i*dh)/g_M; MoveToEx(mdc, 0, y, NULL); LineTo(mdc, r.right, y); }
                    DeleteObject(hG);
                }
            }
            BitBlt(hdc, 0, 0, r.right, r.bottom, mdc, 0, 0, SRCCOPY);
            DeleteObject(mbm); DeleteDC(mdc); EndPaint(hwnd, &ps);
            break;
        }
        case WM_SIZE: {
            int w = LOWORD(lParam);
            if(hSliderWipe) MoveWindow(hSliderWipe, 20, 140, w - 40, 30, TRUE);
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }
        case WM_DESTROY: PostQuitMessage(0); break;
        default: return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE h, HINSTANCE p, LPSTR l, int s) {
    WNDCLASS wc = {0}; wc.lpfnWndProc = WndProc; wc.hInstance = h; wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.lpszClassName = "GoLVizMaterial";
    RegisterClass(&wc);
    CreateWindow("GoLVizMaterial", "HPC Lab Analizer - Material Wipe & Fade", WS_OVERLAPPEDWINDOW|WS_VISIBLE|WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, 950, 850, NULL, NULL, h, NULL);
    MSG m; while (GetMessage(&m, NULL, 0, 0)) { TranslateMessage(&m); DispatchMessage(&m); }
    return 0;
}