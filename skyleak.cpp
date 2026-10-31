#include <windows.h>
#include <math.h>
#include <random>

#pragma comment(lib, "winmm.lib")

enum
{
    SAMPLE_RATE_A = 24000,
    SAMPLE_RATE_B = 72000
};

#define Seconds 30

#define BUFFER_SIZE_A (SAMPLE_RATE_A * Seconds)
#define BUFFER_SIZE_B (SAMPLE_RATE_B * Seconds)

#define MAX_ICONS 35
#define NOTSRCINVERT 0x999999

struct IconParticle
{
    HICON hIcon;
    float x, y;
    float vx, vy;
};

typedef union
{
    COLORREF rgb;

    struct
    {
        BYTE r;
        BYTE g;
        BYTE b;

        BYTE reserved;
    };
} RGBQUAD_t;

typedef RGBQUAD_t* PRGBQUAD_t;

typedef struct
{
    FLOAT h;
    FLOAT s;
    FLOAT l;
} HSL;

static float HueToRGB(float p, float q, float t)
{
    if (t < 0.0f) t += 1.0f;
    if (t > 1.0f) t -= 1.0f;
    if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f / 2.0f) return q;
    if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
    return p;
}

RGBQUAD_t HSLtoRGB(HSL hsl)
{
    RGBQUAD_t rgb;
    rgb.reserved = 0;

    if (hsl.s == 0.0f)
    {
        BYTE val = (BYTE)(hsl.l * 255.0f);
        rgb.r = val;
        rgb.g = val;
        rgb.b = val;
    }

    else
    {
        float q =
            (hsl.l < 0.5f) ? (hsl.l * (1.0f + hsl.s)) :
            (hsl.l + hsl.s - hsl.l * hsl.s);

        float p = 2.0f * hsl.l - q;

        rgb.r = (BYTE)(HueToRGB(p, q, hsl.h + 1.0f / 3.0f) * 255.0f);
        rgb.g = (BYTE)(HueToRGB(p, q, hsl.h) * 255.0f);
        rgb.b = (BYTE)(HueToRGB(p, q, hsl.h - 1.0f / 3.0f) * 255.0f);
    }

    return rgb;
}

HSL RGBtoHSL(RGBQUAD_t rgb)
{
    float r = rgb.r / 255.0f;
    float g = rgb.g / 255.0f;
    float b = rgb.b / 255.0f;
    float max = fmaxf(r, fmaxf(g, b));
    float min = fminf(r, fminf(g, b));
    float h, s, l = (max + min) / 2.0f;

    if (max == min)
    {
        h = s = 0;
    }

    else
    {
        float d = max - min;
        s = l > 0.5f ? d / (2 - max - min) : d / (max + min);
        if (max == r) h = (g - b) / d + (g < b ? 6 : 0);
        else if (max == g) h = (b - r) / d + 2;
        else h = (r - g) / d + 4;
        h /= 6;
    }

    return { h, s, l };
}


VOID PressKey(WORD key, INT time)
{
    INPUT pInputs = { 0 };

    pInputs.type = INPUT_KEYBOARD;
    pInputs.ki.wVk = key;

    SendInput(1, (LPINPUT)&pInputs, sizeof(INPUT));

    Sleep(time);

    pInputs.ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(1, (LPINPUT)&pInputs, sizeof(INPUT));
}

BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
{
    const WCHAR* labelText = L"skyleak" + rand() % 128;

    if (GetWindowLongW(hwnd, GWL_STYLE) & WS_VISIBLE)
    {
        SendMessageW(hwnd, WM_SETTEXT, 0, (LPARAM)labelText);
    }

    return true;
}

int w = GetSystemMetrics(0);
int h = GetSystemMetrics(1);

namespace GDIPayloads
{
    DWORD WINAPI Hatch(LPVOID lpParam)
    {
        while (1)
        {
            HDC hdc = GetDC(0);
            HDC dcCopy = CreateCompatibleDC(hdc);

            HBRUSH brush = CreateHatchBrush(rand() % 4, rand() % 255);

            while (true)
            {
                SelectObject(hdc, brush);
                SetBkColor(hdc, RGB(rand() % 25, rand() % 25, rand() % 25));
                PatBlt(hdc, 0, 0, w, h, PATINVERT);
                Sleep(1);
            }

            DeleteObject(brush);
            DeleteDC(hdc);
            return 0;
        }
    }

    DWORD WINAPI Masher1(LPVOID lpParam)
    {
        while (true)
        {
            HDC hdc = GetDC(0);

            BitBlt
            (
                hdc,
                rand() % 5, rand() % 5, rand() % w,
                rand() % h,
                hdc,
                rand() % 5, rand() % 5,
                SRCCOPY
            );

            Sleep(1);
            ReleaseDC(0, hdc);
        }
    }

    DWORD WINAPI GDIShader1(LPVOID lpParam)
    {

        while (true)
        {
            HDC hdc = GetDC(NULL);
            HDC dcCopy = CreateCompatibleDC(hdc);

            BITMAPINFO bmpi = { 0 };
            bmpi.bmiHeader.biSize = sizeof(bmpi);
            bmpi.bmiHeader.biWidth = w;
            bmpi.bmiHeader.biHeight = -h;
            bmpi.bmiHeader.biPlanes = 1;
            bmpi.bmiHeader.biBitCount = 32;

            PRGBQUAD_t rgbquad;
            HBITMAP bmp = CreateDIBSection(hdc, &bmpi, DIB_RGB_COLORS, (void**)&rgbquad, NULL, 0);
            SelectObject(dcCopy, bmp);

            StretchBlt(dcCopy, 0, 0, w, h, hdc, 0, 0, w, h, SRCCOPY);

            for (int i = 0; i < w * h; i++)
            {
                HSL hsl = RGBtoHSL(rgbquad[i]);
                hsl.h = fmodf(hsl.h + 0.05f, 1.0f);
                rgbquad[i] = HSLtoRGB(hsl);
            }

            StretchBlt(hdc, 0, 0, w, h, dcCopy, 0, 0, w, h, SRCCOPY);
            DeleteObject(bmp);
            DeleteDC(dcCopy);
            ReleaseDC(NULL, hdc);

            Sleep(10);
        }
    }

    DWORD WINAPI GDIShader2(LPVOID lpParam)
    {
        HDC hdc = GetDC(0);
        HDC dcCopy = CreateCompatibleDC(hdc);

        BITMAPINFO bmpi = { 0 };
        HBITMAP bmp;

        bmpi.bmiHeader.biSize = sizeof(bmpi);
        bmpi.bmiHeader.biWidth = w;
        bmpi.bmiHeader.biHeight = h;
        bmpi.bmiHeader.biPlanes = 1;
        bmpi.bmiHeader.biBitCount = 32;
        bmpi.bmiHeader.biCompression = BI_RGB;


        PRGBQUAD_t rgbquad = NULL;
        HSL hslcolor;

        bmp = CreateDIBSection(hdc, &bmpi, DIB_RGB_COLORS, (void**)&rgbquad, NULL, 0);
        SelectObject(dcCopy, bmp);

        INT i = 0;

        while (1)
        {
            hdc = GetDC(NULL);
            StretchBlt(dcCopy, 0, 0, w, h, hdc, 0, 0, w, h, SRCCOPY);

            RGBQUAD_t rgbquadCopy;

            for (int x = 0; x < w; x++)
            {
                for (int y = 0; y < h; y++)
                {
                    int index = y * w + x;

                    int fx = (int)((i ^ 4) + (i * 4) * cbrt(x * y ^ y));

                    rgbquadCopy = rgbquad[index];

                    hslcolor = RGBtoHSL(rgbquadCopy);
                    hslcolor.h = fmod(fx / 700.f + y / h * .2f, 1.f);

                    rgbquad[index] = HSLtoRGB(hslcolor);
                }
            }

            i++;

            StretchBlt(hdc, 0, 0, w, h, dcCopy, 0, 0, w, h, SRCCOPY);
            ReleaseDC(NULL, hdc);
            DeleteDC(hdc);
        }

        return 0;
    }

    DWORD WINAPI Texts(LPVOID lpParam)
    {

        while (true)
        {
            HDC hdc = GetDC(NULL);

            SetBkMode(hdc, 0);

            int tw = rand() % 50;
            int th = rand() % 100;

            SetTextColor(hdc, RGB(rand() % 255, rand() % 255, rand() % 255));

            LPCSTR strings[10] =
            {
                "Aether", "skyleak.exe",
                "Danoni631", "??????????????????????????????????????????????",
                "NicolasBickHoff11", "you are nothing on this virtual trash",
                "you can discover the truth, but you can't stop him", "github.com/Danoni631",
                "You are from void", "l0st"
            };

            HFONT Font =
                CreateFontA
                (
                    th, tw, rand() % 20, rand() % 20,
                    FW_REGULAR, 0, 0, 0, ANSI_CHARSET, 0, 0, 0, 0, "Impact"
                );

            SelectObject(hdc, Font);
            TextOutA(hdc, rand() % w, rand() % h, strings[rand() % 10], strlen(strings[rand() % 10]));

            DeleteObject(Font);
            ReleaseDC(0, hdc);
            Sleep(10);
        }
    }

    DWORD WINAPI Masher2(LPVOID lpParam)
    {
        while (true)
        {
            HDC hdc = GetDC(0);

            BitBlt
            (
                hdc,
                rand() % 30, rand() % 30, rand() % w,
                rand() % h,
                hdc,
                rand() % 30, rand() % 30,
                SRCERASE
            );

            Sleep(1);
            ReleaseDC(0, hdc);
        }
    }

    DWORD WINAPI Blit(LPVOID lpParam)
    {
        DWORD randomrops[2] =
        {
            SRCAND, SRCPAINT
        };

        while (true)
        {
            HDC hdc = GetDC(0);

            for (int i = 0;; i++)
            {
                int rx = rand() % w;
                int ry = rand() % h;
                BitBlt(hdc, rx, 10, 100, h, hdc, rx, 0, randomrops[rand() % 2]);
                BitBlt(hdc, rx, -10, -100, h, hdc, rx, 0, randomrops[rand() % 2]);
                BitBlt(hdc, 10, ry, w, 96, hdc, 0, ry, randomrops[rand() % 2]);
                BitBlt(hdc, -10, ry, w, -96, hdc, 0, ry, randomrops[rand() % 2]);
                Sleep(1);
            }
        }
    }

    DWORD WINAPI GenericPat(LPVOID lpParam)
    {
        while (true)
        {
            HDC hdc = GetDC(0);
            HDC dcCopy = CreateCompatibleDC(hdc);
            HBITMAP bm = CreateCompatibleBitmap(hdc, w, h);
            SelectObject(dcCopy, bm);
            HBRUSH brush = CreateSolidBrush(RGB(rand() % 255, rand() % 255, rand() % 255));
            SelectObject(hdc, brush);
            PatBlt(hdc, 0, 0, w, h, PATINVERT);

            DeleteObject(brush);
            DeleteObject(dcCopy);
            DeleteObject(bm);
            ReleaseDC(0, hdc);
            Sleep(125);
        }
    }

    DWORD WINAPI Rotate(LPVOID lpParam)
    {
        while (1)
        {
            HDC hdc = GetDC(0);
            HDC dcCopy = CreateCompatibleDC(hdc);
            
            HBITMAP bm = CreateCompatibleBitmap(hdc, w, h);
            SelectObject(dcCopy, bm);

            RECT rect;
            GetWindowRect(GetDesktopWindow(), &rect);

            POINT pt[3];
            pt[0].x = rect.left + rand() % 20;  pt[0].y = rect.top + rand() % 20;
            pt[1].x = rect.right - rand() % 20; pt[1].y = rect.top + rand() % 20;
            pt[2].x = rect.left + rand() % 20;  pt[2].y = rect.bottom - rand() % 20;

            PlgBlt(dcCopy, pt, hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, 0, 0, 0);
            BitBlt(hdc, rand() % 20, rand() % 20, w, h, dcCopy, rand() % 20, rand() % 20, SRCCOPY);

            DeleteObject(bm);
            DeleteDC(dcCopy);
            ReleaseDC(0, hdc);
            Sleep(10);
        }
    }

    DWORD WINAPI Icons(LPVOID lpParam)
    {
        HICON icons[] =
        {
            LoadIcon(NULL, IDI_APPLICATION),
            LoadIcon(NULL, IDI_ASTERISK),
            LoadIcon(NULL, IDI_EXCLAMATION),
            LoadIcon(NULL, IDI_ERROR),
            LoadIcon(NULL, IDI_WARNING)
        };
        int iconCount = sizeof(icons) / sizeof(icons[0]);
        
        IconParticle particles[MAX_ICONS];

        for (int i = 0; i < MAX_ICONS; i++)
        {
            particles[i].hIcon = icons[rand() % iconCount];
            particles[i].x = (float)(rand() % (w - 64));
            particles[i].y = (float)(rand() % (h - 64));

            particles[i].vx = ((float)(rand() % 10) - 5.0f) * 1.2f;

            if (particles[i].vx == 0) particles[i].vx = 2.0f;

            particles[i].vy = ((float)(rand() % 10) - 5.0f) * 1.2f;

            if (particles[i].vy == 0) particles[i].vy = 2.0f;
        }

        const int iconSize = 32;

        while (true)
        {
            HDC hdc = GetDC(NULL);
        
            if (!hdc)
            {
                Sleep(10);
                continue;
            }

            for (int i = 0; i < MAX_ICONS; i++)
            {
                particles[i].x += particles[i].vx;
                particles[i].y += particles[i].vy;

                if (particles[i].x <= 0)
                {
                    particles[i].x = 0;
                    particles[i].vx = -particles[i].vx;
                }

                else if (particles[i].x >= w - iconSize)
                {
                    particles[i].x = (float)(w - iconSize);
                    particles[i].vx = -particles[i].vx;
                }

                else if (particles[i].y <= 0)
                {
                    particles[i].y = 0;
                    particles[i].vy = -particles[i].vy;
                }
                
                else if (particles[i].y >= h - iconSize)
                {
                    particles[i].y = (float)(h - iconSize);
                    particles[i].vy = -particles[i].vy;
                }

                DrawIcon(hdc, (int)particles[i].x, (int)particles[i].y, particles[i].hIcon);
            }

            ReleaseDC(NULL, hdc);
            Sleep(15);
        }

        return 0;
    }

    DWORD WINAPI GDIShader3(LPVOID lpParam)
    {
        HDC hdc = GetDC(NULL);
        HDC dcCopy = CreateCompatibleDC(hdc);

        BITMAPINFO bmi = { 0 };
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = w;
        bmi.bmiHeader.biHeight = -h;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;

        PRGBQUAD_t prgbDst = nullptr;
        HBITMAP hbmDst = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&prgbDst, NULL, 0);
        SelectObject(dcCopy, hbmDst);

        PRGBQUAD_t prgbSrc =
            (PRGBQUAD_t)VirtualAlloc(NULL, w * h * sizeof(RGBQUAD_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

        int t = 0;

        while (1)
        {
            BitBlt(dcCopy, 0, 0, w, h, hdc, 0, 0, SRCCOPY);
            memcpy(prgbSrc, prgbDst, w * h * sizeof(RGBQUAD_t));

            for (INT y = 0; y < h; y++)
            {
                FLOAT ha = y / 60.f;

                for (INT x = 0; x < w; x++)
                {
                    FLOAT a = cos(ha + t) * 60.f;
                    FLOAT b = tan(2.f);

                    UINT u = x + (INT)a;
                    UINT v = y + (INT)b;

                    u %= w;
                    v %= h;

                    RGBQUAD_t rgbDst = prgbSrc[v * w + u];

                    rgbDst.rgb =
                    (((rgbDst.b | rgbDst.g) << 21) | ((rgbDst.g | rgbDst.r) >> 23) | ((rgbDst.r | rgbDst.b) >> 19));
                    prgbDst[y * w + x] = rgbDst;
                }
            }

            BitBlt(hdc, 0, 0, w, h, dcCopy, 0, 0, SRCPAINT);
            Sleep(1);
        }

        DeleteObject(hbmDst);
        DeleteDC(dcCopy);
        ReleaseDC(0, hdc);
        return 0;
    }

    DWORD WINAPI BitBlt222(LPVOID lpParam)
    {
        while (true)
        {
            HDC hdc = GetDC(0);

            BitBlt
            (
                hdc,
                rand() % 222, rand() % 222, w,
                h,
                hdc,
                rand() % 222, rand() % 222,
                SRCINVERT
            );

            Sleep(1);
            ReleaseDC(0, hdc);
        }
    }

    DWORD WINAPI EdgeDetection(LPVOID lpParam)
    {
        while (true)
        {
            HDC hdc = GetDC(0);

            BitBlt
            (
                hdc,
                2, 2, w,
                h,
                hdc,
                0, 0,
                NOTSRCINVERT
            );

            Sleep(1);
            ReleaseDC(0, hdc);
        }
    }
}

namespace Bytebeats
{
    DWORD WINAPI Sound1(LPVOID lpvd)
    {
        HWAVEOUT hWaveOut;
        WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, SAMPLE_RATE_A, SAMPLE_RATE_A, 1, 8, 0 };

        if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
        {
            return 1;
        }

        std::vector<unsigned char> buffer(BUFFER_SIZE_A);

        for (int t = 0; t < BUFFER_SIZE_A; ++t)
        {
            buffer[t] =
            (unsigned char)
            (((t * t / (1 + (t >> 9 & t >> 8))) & 128) + ((t * (t >> 13 | t >> 8)) | (t >> 16) ^ t) - 64);
        }

        WAVEHDR header = { (LPSTR)buffer.data(), (DWORD)buffer.size(), 0, 0, 0, 0, 0, 0 };

        waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
        waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));

        while (!(header.dwFlags & WHDR_DONE))
        {
            Sleep(100);
        }

        waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
        waveOutClose(hWaveOut);
        return 0;
    }

    DWORD WINAPI Sound2(LPVOID lpvd)
    {
        HWAVEOUT hWaveOut;
        WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, SAMPLE_RATE_A, SAMPLE_RATE_A, 1, 8, 0 };

        if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
        {
            return 1;
        }

        std::vector<unsigned char> buffer(BUFFER_SIZE_A);

        for (int t = 0; t < BUFFER_SIZE_A; ++t)
        {
            buffer[t] =
            (unsigned char)
            ((t >> 6 ^ t & t >> 9 ^ t >> 12 | (t << (t >> 6) % 4 ^ -t & -t >> 13) % 128 ^ -t >> 1) +
            (t * (t ^ t + (t >> 15 | 1) ^ (t - 1280 ^ t) >> 10)));
        }

        WAVEHDR header = { (LPSTR)buffer.data(), (DWORD)buffer.size(), 0, 0, 0, 0, 0, 0 };

        waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
        waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));

        while (!(header.dwFlags & WHDR_DONE))
        {
            Sleep(100);
        }

        waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
        waveOutClose(hWaveOut);

        return 0;
    }

    DWORD WINAPI Sound3(LPVOID lpvd)
    {
        HWAVEOUT hWaveOut;
        WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, SAMPLE_RATE_A, SAMPLE_RATE_A, 1, 8, 0 };

        if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
        {
            return 1;
        }

        std::vector<unsigned char> buffer(BUFFER_SIZE_A);

        for (int t = 0; t < BUFFER_SIZE_A; ++t)
        {
            buffer[t] =
                (unsigned char)
                (((t / 2 * (4 | 7 & t >> 13) >> (~t >> 11 & 1) & 127) +
                (t * 2 * (t >> 11 & t >> 13) * (~t >> 9 & 3) & 128)) +
                ((((((t * t)) + 2) / ((t >> 10) + 2))) / 40.75) +
                ((((t * (t >> 5 & t >> 6)) >> (7 & t >> 13)) * 100) - t));
        }

        WAVEHDR header = { (LPSTR)buffer.data(), (DWORD)buffer.size(), 0, 0, 0, 0, 0, 0 };

        waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
        waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));

        while (!(header.dwFlags & WHDR_DONE))
        {
            Sleep(100);
        }

        waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
        waveOutClose(hWaveOut);

        return 0;
    }

    DWORD WINAPI Sound4(LPVOID lpvd)
    {
        HWAVEOUT hWaveOut;
        WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, SAMPLE_RATE_A, SAMPLE_RATE_A, 1, 8, 0 };

        if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
        {
            return 1;
        }

        std::vector<unsigned char> buffer(BUFFER_SIZE_A);

        for (int t = 0; t < BUFFER_SIZE_A; ++t)
        {
            buffer[t] =
            (unsigned char)
            (((t * ((3 + (1 ^ t >> 10 & 5)) * (5 + (3 & t >> 14)))) >> (t >> 8 & 3)) +
            ((t * (t >> 13 | t >> 8) | t >> 16 ^ t) + 64));
        }

        WAVEHDR header = { (LPSTR)buffer.data(), (DWORD)buffer.size(), 0, 0, 0, 0, 0, 0 };

        waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
        waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));

        while (!(header.dwFlags & WHDR_DONE))
        {
            Sleep(100);
        }

        waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
        waveOutClose(hWaveOut);

        return 0;
    }

    DWORD WINAPI Sound5(LPVOID lpvd)
    {
        HWAVEOUT hWaveOut;
        WAVEFORMATEX wfx = { WAVE_FORMAT_PCM, 1, SAMPLE_RATE_B, SAMPLE_RATE_B, 1, 8, 0 };

        if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
        {
            return 1;
        }

        std::vector<unsigned char> buffer(BUFFER_SIZE_B);

        for (int t = 0; t < BUFFER_SIZE_B; ++t)
        {
            buffer[t] =
            (unsigned char)
            (((((2 * t & (t & 255 ? -t : t) >> 8 & (t & 256 ? -t : t) >> 6)) * 10) + (t >> 1)) & 128);
        }

        WAVEHDR header = { (LPSTR)buffer.data(), (DWORD)buffer.size(), 0, 0, 0, 0, 0, 0 };

        waveOutPrepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
        waveOutWrite(hWaveOut, &header, sizeof(WAVEHDR));

        while (!(header.dwFlags & WHDR_DONE))
        {
            Sleep(100);
        }

        waveOutUnprepareHeader(hWaveOut, &header, sizeof(WAVEHDR));
        waveOutClose(hWaveOut);

        return 0;
    }
}

namespace System
{
    DWORD WINAPI MsgBox(LPVOID lpParam)
    {
        for (;;)
        {
            MessageBoxA
            (
                0, "Go to shit MazeLarp. You are a idiot.\n\nMazeLarp sucks",
                "skyleak - Error",
                MB_OK | MB_ICONERROR
            );
        }
    }

    DWORD WINAPI MoveCursor(LPVOID lpParam)
    {
        POINT pt;

        while (true)
        {
            if (GetCursorPos(&pt))
            {
                int dx = rand() % 11 - 5; // random between -5 and 5
                int dy = rand() % 11 - 5; // random between -5 and 5
                SetCursorPos(pt.x + dx, pt.y + dy);
            }

            Sleep(1);
        }

        return 0;
    }

    DWORD WINAPI KeyboardCAPS(LPVOID lpvd)
    {
        while (true)
        {
            PressKey(VK_CAPITAL, 1);
            PressKey(VK_SCROLL, 1);
        }

        return 0;
    }

    DWORD WINAPI MessedLabels(LPVOID lpvd)
    {
        while (true)
        {
            HWND hwnd = FindWindowW(NULL, L"skyleak" + rand() % 128);
            EnumChildWindows(hwnd, EnumChildProc, NULL);
        }
    }

}

void InitPayloads()
{
    Sleep(5000);
    CreateThread(0, 0, System::MsgBox, 0, 0, 0);
    CreateThread(0, 0, System::KeyboardCAPS, 0, 0, 0);
    CreateThread(0, 0, System::MoveCursor, 0, 0, 0);
    CreateThread(0, 0, System::MessedLabels, 0, 0, 0);
    Sleep(5000);
    CreateThread(0, 0, GDIPayloads::Masher1, 0, 0, 0);
    HANDLE GDIThread1B = CreateThread(0, 0, GDIPayloads::GDIShader1, 0, 0, 0);
    CreateThread(0, 0, Bytebeats::Sound1, 0, 0, 0);
    Sleep(30000);
    TerminateThread(GDIThread1B, NULL);
    HANDLE GDIThread2 = CreateThread(0, 0, GDIPayloads::GDIShader2, 0, 0, 0);
    CreateThread(0, 0, Bytebeats::Sound2, 0, 0, 0);
    CreateThread(0, 0, GDIPayloads::Texts, 0, 0, 0);
    Sleep(30000);
    TerminateThread(GDIThread2, NULL);
    CreateThread(0, 0, GDIPayloads::Blit, 0, 0, 0);
    CreateThread(0, 0, GDIPayloads::Icons, 0, 0, 0);
    CreateThread(0, 0, GDIPayloads::GenericPat, 0, 0, 0);
    CreateThread(0, 0, Bytebeats::Sound3, 0, 0, 0);
    Sleep(30000);
    CreateThread(0, 0, GDIPayloads::Masher2, 0, 0, 0);
    CreateThread(0, 0, GDIPayloads::Rotate, 0, 0, 0);
    CreateThread(0, 0, GDIPayloads::GDIShader3, 0, 0, 0);
    CreateThread(0, 0, Bytebeats::Sound4, 0, 0, 0);
    Sleep(30000);
    CreateThread(0, 0, GDIPayloads::Hatch, 0, 0, 0);
    CreateThread(0, 0, GDIPayloads::BitBlt222, 0, 0, 0);
    CreateThread(0, 0, GDIPayloads::EdgeDetection, 0, 0, 0);
    CreateThread(0, 0, Bytebeats::Sound5, 0, 0, 0);
    Sleep(30000);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    if
        (
            MessageBoxW
            (
                NULL,
                L"You know perfectly what gonna happen.\n\nDo you want to run?",
                L"'Galaxy Gran Prime fucking smells'",
                MB_ICONWARNING | MB_YESNO
            ) != IDYES
            ) return 1;

    if
        (
            MessageBoxW
            (
                NULL,
                L"THIS IS THE LAST WARNING!!!\n\nthis shit gonna destroy your PC.\n\nARE YOU SURE???!!!",
                L"skyleak - Final warning",
                MB_ICONWARNING | MB_YESNO
            ) != IDYES
            ) return 1;

    MessageBoxA
    (
        0, "Creators\n\nGDI: DaniloSenseiPlus\nMBR: NicolasBickHoff11",
        "skyleak - Credits",
        MB_OK | MB_ICONASTERISK
    );

    InitPayloads();
}