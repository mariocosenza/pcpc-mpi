#include <windows.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")

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

BOOL ValidateMatrixFileSize(FILE *f, long long expected_size_bytes) {
    if (fseeko(f, 0, SEEK_END) != 0) return FALSE;
    long long file_size = (long long)ftello(f);
    if (file_size < 0) return FALSE;
    if (fseeko(f, 0, SEEK_SET) != 0) return FALSE;
    return file_size == expected_size_bytes;
}

// FIX: parsing corretto per il formato matrix_MxN_seed..._pattern....bin
BOOL InferMatrixDimensionsFromPath(const char *path, long long *rows, long long *cols) {
    const char *base = strrchr(path, '\\');
    if (!base) base = strrchr(path, '/');
    base = base ? base + 1 : path;

    long long parsed_rows = 0, parsed_cols = 0;

    // Formato generatore: matrix_840x840_seed1234_pattern0.bin
    if (sscanf(base, "matrix_%llux%llu_seed", &parsed_rows, &parsed_cols) == 2
        && parsed_rows > 0 && parsed_cols > 0) {
        *rows = parsed_rows;
        *cols = parsed_cols;
        return TRUE;
    }
    // Formato generico fallback: matrix_840x840...
    if (sscanf(base, "matrix_%llux%llu", &parsed_rows, &parsed_cols) == 2
        && parsed_rows > 0 && parsed_cols > 0) {
        *rows = parsed_rows;
        *cols = parsed_cols;
        return TRUE;
    }

    return FALSE;
}

// Costruisce la BITMAPINFO_256 con colori corretti (ordine BGR per RGBQUAD)
void BuildBitmapInfo(BITMAPINFO_256 *bmi, int slot) {
    // slot 0 = Gen0 (giallo), slot 1 = Finale (magenta), slot 2 = fade
    ZeroMemory(bmi, sizeof(BITMAPINFO_256));
    bmi->bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi->bmiHeader.biWidth       = (LONG)g_N;
    bmi->bmiHeader.biHeight      = -(LONG)g_M; // top-down
    bmi->bmiHeader.biPlanes      = 1;
    bmi->bmiHeader.biBitCount    = 8;
    bmi->bmiHeader.biCompression = BI_RGB;

    // RGBQUAD ordine: Blue, Green, Red, Reserved
    // indice 0 = cella morta (sfondo scuro)
    bmi->bmiColors[0].rgbBlue  = 15;
    bmi->bmiColors[0].rgbGreen = 15;
    bmi->bmiColors[0].rgbRed   = 15;

    // indice 1 = cella viva
    if (slot == 0) {
        // Gen0: giallo (R=255, G=200, B=0)
        bmi->bmiColors[1].rgbBlue  = 0;
        bmi->bmiColors[1].rgbGreen = 200;
        bmi->bmiColors[1].rgbRed   = 255;
    } else if (slot == 1) {
        // Finale: magenta (R=200, G=30, B=110)
        bmi->bmiColors[1].rgbBlue  = 110;
        bmi->bmiColors[1].rgbGreen = 30;
        bmi->bmiColors[1].rgbRed   = 200;
    }
    // slot 2 (fade) viene gestito separatamente
}

// --- WNDPROC ---
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            int dark = 1; DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark));
            hFont      = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                    DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            hFontBold  = CreateFont(16, 0, 0, 0, FW_BOLD,   FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                    DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            hFontSmall = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                    DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            hBgBrush     = CreateSolidBrush(RGB(20, 20, 22));
            hEditBgBrush = CreateSolidBrush(RGB(40, 40, 45));

            hLblM    = CreateWindow("STATIC", "Righe (M):",    WS_CHILD|WS_VISIBLE, 30, 25, 70, 20, hwnd, NULL, NULL, NULL);
            hEditM   = CreateWindow("EDIT",   "840",           WS_CHILD|WS_VISIBLE|ES_NUMBER|ES_CENTER, 100, 22, 70, 24, hwnd, (HMENU)ID_EDIT_M, NULL, NULL);
            hLblN    = CreateWindow("STATIC", "Colonne (N):",  WS_CHILD|WS_VISIBLE, 190, 25, 85, 20, hwnd, NULL, NULL, NULL);
            hEditN   = CreateWindow("EDIT",   "840",           WS_CHILD|WS_VISIBLE|ES_NUMBER|ES_CENTER, 280, 22, 70, 24, hwnd, (HMENU)ID_EDIT_N, NULL, NULL);
            hBtnUpdate = CreateWindow("BUTTON", "APPLICA",     WS_CHILD|WS_VISIBLE|BS_OWNERDRAW, 380, 20, 100, 28, hwnd, (HMENU)ID_BTN_UPDATE, NULL, NULL);

            hBtnLoad0  = CreateWindow("BUTTON", "GEN 0",       WS_CHILD|WS_VISIBLE|BS_OWNERDRAW, 30,  65, 140, 35, hwnd, (HMENU)ID_BTN_LOAD_0, NULL, NULL);
            hBtnLoadF  = CreateWindow("BUTTON", "FINALE",      WS_CHILD|WS_VISIBLE|BS_OWNERDRAW, 180, 65, 140, 35, hwnd, (HMENU)ID_BTN_LOAD_F, NULL, NULL);
            hBtnPlayW  = CreateWindow("BUTTON", "ANIM. WIPE",  WS_CHILD|WS_VISIBLE|BS_OWNERDRAW, 330, 65, 150, 35, hwnd, (HMENU)ID_BTN_PLAY_W, NULL, NULL);
            hBtnPlayF  = CreateWindow("BUTTON", "ANIM. FADE",  WS_CHILD|WS_VISIBLE|BS_OWNERDRAW, 490, 65, 150, 35, hwnd, (HMENU)ID_BTN_PLAY_F, NULL, NULL);

            hSliderWipe = CreateWindowEx(0, TRACKBAR_CLASS, "Wipe", WS_CHILD|WS_VISIBLE|TBS_NOTICKS,
                                         20, 140, 800, 30, hwnd, (HMENU)ID_SLIDER_WIPE, NULL, NULL);
            SendMessage(hSliderWipe, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
            SendMessage(hSliderWipe, TBM_SETPOS,   TRUE, 50);

            SetControlFont(hLblM,  hFont); SetControlFont(hEditM, hFont);
            SetControlFont(hLblN,  hFont); SetControlFont(hEditN, hFont);
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
        case WM_CTLCOLORSTATIC: SetTextColor((HDC)wParam, RGB(220,220,220)); SetBkMode((HDC)wParam, TRANSPARENT); return (INT_PTR)hBgBrush;
        case WM_CTLCOLOREDIT:   SetTextColor((HDC)wParam, RGB(255,255,255)); SetBkColor((HDC)wParam, RGB(40,40,45)); return (INT_PTR)hEditBgBrush;

        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT p = (LPDRAWITEMSTRUCT)lParam;
            COLORREF col = RGB(60, 60, 65);
            if      (p->CtlID == ID_BTN_LOAD_0)  col = RGB(0,   160, 210);
            else if (p->CtlID == ID_BTN_LOAD_F)  col = RGB(200,  30, 110);
            else if (p->CtlID == ID_BTN_PLAY_W)  col = (g_anim_mode == 1) ? RGB(220, 50, 50) : RGB(50, 150, 80);
            else if (p->CtlID == ID_BTN_PLAY_F)  col = (g_anim_mode == 2) ? RGB(220, 50, 50) : RGB(100, 50, 200);
            else if (p->CtlID == ID_BTN_UPDATE)  col = RGB(60, 60, 70);

            HBRUSH br = CreateSolidBrush(col);
            FillRect(p->hDC, &p->rcItem, br);
            DeleteObject(br);

            char t[64]; GetWindowText(p->hwndItem, t, 64);
            SetTextColor(p->hDC, RGB(255,255,255));
            SetBkMode(p->hDC, TRANSPARENT);
            SelectObject(p->hDC, hFontBold);
            DrawText(p->hDC, t, -1, &p->rcItem, DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            return TRUE;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == ID_BTN_UPDATE) {
                char m[16], n[16];
                GetWindowText(hEditM, m, 16);
                GetWindowText(hEditN, n, 16);
                g_M = atoll(m);
                g_N = atoll(n);
                InvalidateRect(hwnd, NULL, FALSE);

            } else if (id == ID_BTN_LOAD_0 || id == ID_BTN_LOAD_F) {
                char path[260];
                if (ChooseBinFile(hwnd, path)) {
                    // Inferisci dimensioni dal nome file
                    long long inferred_m = g_M, inferred_n = g_N;
                    if (InferMatrixDimensionsFromPath(path, &inferred_m, &inferred_n)) {
                        g_M = inferred_m;
                        g_N = inferred_n;
                        char m_text[32], n_text[32];
                        sprintf(m_text, "%lld", g_M);
                        sprintf(n_text, "%lld", g_N);
                        SetWindowText(hEditM, m_text);
                        SetWindowText(hEditN, n_text);
                    }

                    FILE *f = fopen(path, "rb");
                    if (f) {
                        long long expected_size = g_M * g_N;
                        if (!ValidateMatrixFileSize(f, expected_size)) {
                            char errmsg[256];
                            sprintf(errmsg,
                                "Dimensioni non corrispondenti.\n"
                                "File: %lld bytes\n"
                                "Atteso: %lld x %lld = %lld bytes\n\n"
                                "Verifica M e N negli edit box.",
                                (fseeko(f,0,SEEK_END), (long long)ftello(f)),
                                g_M, g_N, expected_size);
                            MessageBox(hwnd, errmsg, "GoLVizMaterial", MB_ICONERROR | MB_OK);
                            fclose(f);
                            return 0;
                        }

                        uint8_t **b = (id == ID_BTN_LOAD_0) ? &g_buf_0 : &g_buf_f;
                        if (*b) free(*b);
                        *b = (uint8_t*)malloc(g_M * g_N);
                        if (!*b) { MessageBox(hwnd, "Errore allocazione memoria.", "GoLVizMaterial", MB_ICONERROR|MB_OK); fclose(f); return 0; }
                        fread(*b, 1, g_M * g_N, f);
                        fclose(f);

                        AnalyzeData();
                        // FIX: forza ridisegno completo dopo caricamento
                        InvalidateRect(hwnd, NULL, TRUE);
                        UpdateWindow(hwnd);
                    }
                }

            } else if (id == ID_BTN_PLAY_W || id == ID_BTN_PLAY_F) {
                int target = (id == ID_BTN_PLAY_W) ? 1 : 2;
                if (g_anim_mode == target) {
                    KillTimer(hwnd, ID_TIMER_ANIM);
                    g_anim_mode = 0;
                } else {
                    g_anim_mode = target;
                    SetTimer(hwnd, ID_TIMER_ANIM, 16, NULL);
                }
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
        }

        case WM_HSCROLL:
            if (g_anim_mode != 1) {
                g_wipe_percent = (int)SendMessage(hSliderWipe, TBM_GETPOS, 0, 0);
                g_wipe_float   = (float)g_wipe_percent;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT r;
            GetClientRect(hwnd, &r);

            // Double buffering
            HDC mdc       = CreateCompatibleDC(hdc);
            HBITMAP mbm   = CreateCompatibleBitmap(hdc, r.right, r.bottom);
            HBITMAP oldBm = (HBITMAP)SelectObject(mdc, mbm);
            FillRect(mdc, &r, hBgBrush);

            // Statistiche
            char stats[256];
            if (g_buf_0 && g_buf_f) {
                sprintf(stats, "Gen 0: %.2f%%  |  Finale: %.2f%%  |  Variazioni: %lld",
                        g_pop_0, g_pop_f, g_diff_count);
            } else if (g_buf_0) {
                sprintf(stats, "Gen 0: %.2f%%  |  (carica FINALE per confronto)", g_pop_0);
            } else if (g_buf_f) {
                sprintf(stats, "(carica GEN 0 per confronto)  |  Finale: %.2f%%", g_pop_f);
            } else {
                sprintf(stats, "Carica un file con i pulsanti GEN 0 o FINALE");
            }
            SetTextColor(mdc, RGB(0, 210, 255));
            SelectObject(mdc, hFontBold);
            SetBkMode(mdc, TRANSPARENT);
            RECT stR = {0, 110, r.right, 135};
            DrawText(mdc, stats, -1, &stR, DT_CENTER|DT_VCENTER|DT_SINGLELINE);

            // Area di visualizzazione
            int dsy = 180;
            int dh  = r.bottom - dsy;

            if (dh > 0 && (g_buf_0 || g_buf_f)) {
                SetStretchBltMode(mdc, COLORONCOLOR);

                int sx = (g_wipe_percent * r.right) / 100;

                if (g_anim_mode == 2) {
                    // --- FADE: sfumatura cromatica dell'unica buffer disponibile ---
                    uint8_t *b_raw = g_buf_f ? g_buf_f : g_buf_0;
                    if (b_raw) {
                        int slot = g_buf_f ? 1 : 0;
                        
                        // Allineamento a 32-bit (multipli di 4 byte) per riga richiesto da StretchDIBits
                        int row_pitch = (g_N + 3) & ~3; 
                        uint8_t* b_padded = (uint8_t*)malloc(row_pitch * g_M);
                        if (b_padded) {
                            for (int row = 0; row < g_M; row++) {
                                memcpy(b_padded + row * row_pitch, b_raw + row * g_N, g_N);
                            }
                        }

                        BITMAPINFO_256 bmi;
                        BuildBitmapInfo(&bmi, slot);

                        // Modula il colore vivo in base al pulse
                        int pct = g_wipe_percent; // 0..100
                        // sfuma fra giallo e magenta
                        BYTE rr = (BYTE)(255 - pct * 55 / 100);
                        BYTE gg = (BYTE)(200 - pct * 170 / 100);
                        BYTE bb = (BYTE)(pct * 110 / 100);
                        bmi.bmiColors[1].rgbRed   = rr;
                        bmi.bmiColors[1].rgbGreen = gg;
                        bmi.bmiColors[1].rgbBlue  = bb;

                        if (b_padded) {
                            StretchDIBits(mdc,
                                0, dsy, r.right, dh,
                                0, 0, (int)g_N, (int)g_M,
                                b_padded, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, SRCCOPY);
                            free(b_padded);
                        }
                    }

                } else {
                    // --- WIPE: divisione verticale Gen0 | Finale ---
                    for (int s = 0; s < 2; s++) {
                        uint8_t *b_raw = (s == 0) ? g_buf_0 : g_buf_f;
                        if (!b_raw) continue;

                        int row_pitch = (g_N + 3) & ~3;
                        uint8_t* b_padded = (uint8_t*)malloc(row_pitch * g_M);
                        if (b_padded) {
                            for (int row = 0; row < g_M; row++) {
                                memcpy(b_padded + row * row_pitch, b_raw + row * g_N, g_N);
                            }
                        }

                        BITMAPINFO_256 bmi;
                        BuildBitmapInfo(&bmi, s); // s==0 -> giallo, s==1 -> magenta

                        // Clip region: sinistra per Gen0, destra per Finale
                        HRGN hr = (s == 0)
                            ? CreateRectRgn(0,  dsy, sx,      r.bottom)
                            : CreateRectRgn(sx, dsy, r.right, r.bottom);

                        SelectClipRgn(mdc, hr);
                        if (b_padded) {
                            StretchDIBits(mdc,
                                0, dsy, r.right, dh,
                                0, 0, (int)g_N, (int)g_M,
                                b_padded, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, SRCCOPY);
                            free(b_padded);
                        }
                        SelectClipRgn(mdc, NULL);
                        DeleteObject(hr);
                    }

                    // Linea di divisione wipe
                    HPEN hP = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
                    HPEN oldP = (HPEN)SelectObject(mdc, hP);
                    MoveToEx(mdc, sx, dsy, NULL);
                    LineTo(mdc, sx, r.bottom);
                    SelectObject(mdc, oldP);
                    DeleteObject(hP);
                }

                // Griglia Hi-Fi (solo per matrici piccole)
                if (g_M <= 200 && g_N <= 200) {
                    HPEN hG    = CreatePen(PS_SOLID, 1, RGB(50, 50, 55));
                    HPEN oldPG = (HPEN)SelectObject(mdc, hG);
                    for (int i = 0; i <= (int)g_N; i++) {
                        int x = (int)(i * r.right / g_N);
                        MoveToEx(mdc, x, dsy, NULL);
                        LineTo(mdc, x, r.bottom);
                    }
                    for (int i = 0; i <= (int)g_M; i++) {
                        int y = dsy + (int)(i * dh / g_M);
                        MoveToEx(mdc, 0, y, NULL);
                        LineTo(mdc, r.right, y);
                    }
                    SelectObject(mdc, oldPG);
                    DeleteObject(hG);
                }

                // Label laterali Gen0 / Finale in modalità wipe
                if (g_anim_mode != 2 && g_buf_0 && g_buf_f) {
                    SetBkMode(mdc, TRANSPARENT);
                    SelectObject(mdc, hFontSmall);

                    if (sx > 60) {
                        SetTextColor(mdc, RGB(255, 200, 0));
                        RECT lr = {5, dsy + 5, sx - 5, dsy + 25};
                        DrawText(mdc, "GEN 0", -1, &lr, DT_LEFT|DT_TOP|DT_SINGLELINE);
                    }
                    if (r.right - sx > 60) {
                        SetTextColor(mdc, RGB(200, 30, 110));
                        RECT lr = {sx + 5, dsy + 5, r.right - 5, dsy + 25};
                        DrawText(mdc, "FINALE", -1, &lr, DT_LEFT|DT_TOP|DT_SINGLELINE);
                    }
                }
            }

            BitBlt(hdc, 0, 0, r.right, r.bottom, mdc, 0, 0, SRCCOPY);
            SelectObject(mdc, oldBm);
            DeleteObject(mbm);
            DeleteDC(mdc);
            EndPaint(hwnd, &ps);
            break;
        }

        case WM_SIZE: {
            int w = LOWORD(lParam);
            if (hSliderWipe) MoveWindow(hSliderWipe, 20, 140, w - 40, 30, TRUE);
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        }

        case WM_DESTROY:
            KillTimer(hwnd, ID_TIMER_ANIM);
            if (g_buf_0) free(g_buf_0);
            if (g_buf_f) free(g_buf_f);
            DeleteObject(hFont);
            DeleteObject(hFontBold);
            DeleteObject(hFontSmall);
            DeleteObject(hBgBrush);
            DeleteObject(hEditBgBrush);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE h, HINSTANCE p, LPSTR l, int s) {
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_BAR_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSEX wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = h;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = "GoLVizMaterial";
    RegisterClassEx(&wc);

    CreateWindowEx(0, "GoLVizMaterial",
        "HPC Lab Analizer - Material Wipe & Fade",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
        CW_USEDEFAULT, CW_USEDEFAULT, 950, 850,
        NULL, NULL, h, NULL);

    MSG m;
    while (GetMessage(&m, NULL, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessage(&m);
    }
    return 0;
}
