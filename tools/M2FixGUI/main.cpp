#include "settings.hpp"
#include <atomic>
#include <commctrl.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <thread>
#include <uxtheme.h>
#pragma comment(lib, "comctl32.lib")
namespace {
HWND settingsWindow = nullptr, builderWindow = nullptr;
HFONT font = nullptr, boldFont = nullptr, titleFont = nullptr;
HBRUSH bgBrush = nullptr, fieldBrush = nullptr;
COLORREF bg, fg, field;
bool dark = false;
int themeMode = 0;
std::atomic<bool> busy = false;
Ini ini;
auto definitions = schema();
std::vector<std::string> sections;
std::vector<size_t> shown;
int current = -1;
bool filling = false;
std::vector<Missing> absent;
constexpr UINT LOG = WM_APP + 1, DONE = WM_APP + 2;
enum {
    Section = 100,
    List,
    Value,
    Choice,
    Help,
    PathLabel,
    Notice,
    Status,
    Open,
    Reload,
    Defaults,
    Save,
    SaveAs,
    Builder,
    Details,
    Theme,
    BuildPPF,
    Source = 200,
    Input,
    BrowseInput,
    Operation,
    Disc,
    Output,
    BrowseOutput,
    Story,
    VR,
    Grenade,
    English,
    InstallAfter,
    InstallTarget,
    BrowseInstall,
    BuildLog,
    BuildStatus,
    Progress,
    Build
};
int scale = 100;
int px(int n) { return MulDiv(n, scale, 100); }
HWND item(HWND w, int id) { return GetDlgItem(w, id); }
void set(HWND w, int id, const std::string &s) { SetWindowTextW(item(w, id), wide(s).c_str()); }
std::wstring getW(HWND h) {
    int n = GetWindowTextLengthW(h);
    std::wstring s(n + 1, 0);
    GetWindowTextW(h, s.data(), n + 1);
    s.resize(n);
    return s;
}
std::string get(HWND w, int id) { return utf8(getW(item(w, id))); }
int choice(HWND w, int id) { return (int)SendMessageW(item(w, id), CB_GETCURSEL, 0, 0); }
bool checked(HWND w, int id) { return SendMessageW(item(w, id), BM_GETCHECK, 0, 0) == BST_CHECKED; }
HWND control(HWND parent, const wchar_t *cls, const wchar_t *text, int id, int x, int y, int width,
             int height, DWORD style = 0) {
    if (wcscmp(cls, L"STATIC") == 0)
        style |= SS_NOPREFIX;
    HWND h = CreateWindowExW(
        (wcscmp(cls, L"EDIT") == 0 || wcscmp(cls, L"LISTBOX") == 0) ? WS_EX_CLIENTEDGE : 0, cls,
        text, WS_CHILD | WS_VISIBLE | style, px(x), px(y), px(width), px(height), parent,
        (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
    SendMessageW(h, WM_SETFONT, (WPARAM)font, TRUE);
    return h;
}
void button(HWND w, const wchar_t *text, int id, int x, int y, int width) {
    control(w, L"BUTTON", text, id, x, y, width, 30, WS_TABSTOP | BS_OWNERDRAW);
}
void label(HWND w, const wchar_t *text, int x, int y, int width, int height = 22, int id = 0) {
    control(w, L"STATIC", text, id, x, y, width, height);
}
void combo(HWND w, int id, int x, int y, int width, std::initializer_list<const wchar_t *> strings,
           bool editable = false) {
    auto h = control(
        w, L"COMBOBOX", L"", id, x, y, width, 300,
        WS_TABSTOP | WS_VSCROLL |
            (editable ? CBS_DROPDOWN : CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS));
    SendMessageW(h, CB_SETITEMHEIGHT, (WPARAM)-1, px(24));
    SendMessageW(h, CB_SETITEMHEIGHT, 0, px(24));
    for (auto s : strings)
        SendMessageW(h, CB_ADDSTRING, 0, (LPARAM)s);
    SendMessageW(h, CB_SETCURSEL, 0, 0);
}
void move(HWND w, int id, int x, int y, int width, int height) {
    MoveWindow(item(w, id), px(x), px(y), px(width), px(height), TRUE);
}
void error(HWND w, const std::exception &e) {
    MessageBoxW(w, wide(e.what()).c_str(), L"M2FixGUI", MB_OK | MB_ICONERROR);
}
bool ask(HWND w, const std::wstring &text, const wchar_t *title = L"M2FixGUI") {
    return MessageBoxW(w, text.c_str(), title, MB_YESNO | MB_ICONQUESTION) == IDYES;
}
fs::path pick(HWND owner, bool folder, bool save = false, const fs::path &initial = {}) {
    IFileDialog *dialog = nullptr;
    HRESULT hr = save ? CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER,
                                         IID_PPV_ARGS(&dialog))
                      : CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
                                         IID_PPV_ARGS(&dialog));
    need(SUCCEEDED(hr), "Cannot open file dialog");
    DWORD flags = 0;
    dialog->GetOptions(&flags);
    dialog->SetOptions(flags | FOS_FORCEFILESYSTEM | (folder ? FOS_PICKFOLDERS : 0) |
                       (save ? 0 : FOS_PATHMUSTEXIST));
    if (!folder) {
        COMDLG_FILTERSPEC filter[] = {{L"M2Fix settings", L"*.ini"}, {L"All files", L"*.*"}};
        dialog->SetFileTypes(2, filter);
        if (save) {
            dialog->SetDefaultExtension(L"ini");
            dialog->SetFileName(L"MGSM2Fix.ini");
        }
    }
    if (!initial.empty()) {
        IShellItem *shell = nullptr;
        if (SUCCEEDED(
                SHCreateItemFromParsingName(initial.c_str(), nullptr, IID_PPV_ARGS(&shell)))) {
            dialog->SetFolder(shell);
            shell->Release();
        }
    }
    fs::path result;
    if (SUCCEEDED(dialog->Show(owner))) {
        IShellItem *item = nullptr;
        if (SUCCEEDED(dialog->GetResult(&item))) {
            PWSTR p = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &p))) {
                result = p;
                CoTaskMemFree(p);
            }
            item->Release();
        }
    }
    dialog->Release();
    return result;
}

// Retain native input/accessibility behavior while painting surfaces that the
// Windows common controls leave in their light palette.
LRESULT CALLBACK darkSurface(HWND h, UINT message, WPARAM a, LPARAM b, UINT_PTR kind, DWORD_PTR) {
    if (message == WM_NCDESTROY) {
        RemoveWindowSubclass(h, darkSurface, kind);
        return DefSubclassProc(h, message, a, b);
    }
    if (kind == 3 && message == WM_TIMER && dark && busy) {
        InvalidateRect(h, nullptr, FALSE);
        return 0;
    }
    if (!dark || (message != WM_PAINT && message != WM_PRINTCLIENT))
        return DefSubclassProc(h, message, a, b);
    if (kind == 1) {
        auto result = DefSubclassProc(h, message, a, b);
        HDC dc = message == WM_PRINTCLIENT ? (HDC)a : GetDC(h);
        COMBOBOXINFO info{sizeof(info)};
        if (GetComboBoxInfo(h, &info)) {
            FillRect(dc, &info.rcButton, fieldBrush);
            RECT r;
            GetClientRect(h, &r);
            auto pen = CreatePen(PS_SOLID, 1, RGB(85, 89, 99));
            auto oldPen = SelectObject(dc, pen);
            auto oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));
            Rectangle(dc, r.left, r.top, r.right, r.bottom);
            SelectObject(dc, oldBrush);
            SelectObject(dc, oldPen);
            DeleteObject(pen);
            int x = (info.rcButton.left + info.rcButton.right) / 2;
            int y = (info.rcButton.top + info.rcButton.bottom) / 2;
            pen = CreatePen(PS_SOLID, px(1), IsWindowEnabled(h) ? fg : RGB(140, 140, 140));
            oldPen = SelectObject(dc, pen);
            MoveToEx(dc, x - px(4), y - px(2), nullptr);
            LineTo(dc, x, y + px(2));
            LineTo(dc, x + px(4), y - px(2));
            SelectObject(dc, oldPen);
            DeleteObject(pen);
        }
        if (message != WM_PRINTCLIENT)
            ReleaseDC(h, dc);
        return result;
    }
    PAINTSTRUCT ps{};
    HDC dc = message == WM_PRINTCLIENT ? (HDC)a : BeginPaint(h, &ps);
    RECT r;
    GetClientRect(h, &r);
    FillRect(dc, &r, fieldBrush);
    auto oldFont = SelectObject(dc, font);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, fg);
    if (kind == 4) {
        FillRect(dc, &r, bgBrush);
        RECT box{px(1), (r.bottom - px(14)) / 2, px(15), (r.bottom + px(14)) / 2};
        FillRect(dc, &box, fieldBrush);
        auto border = CreateSolidBrush(RGB(110, 115, 125));
        FrameRect(dc, &box, border);
        DeleteObject(border);
        COLORREF ink = IsWindowEnabled(h) ? fg : RGB(140, 140, 140);
        if (SendMessageW(h, BM_GETCHECK, 0, 0) == BST_CHECKED) {
            auto pen = CreatePen(PS_SOLID, px(2), ink);
            auto previous = SelectObject(dc, pen);
            MoveToEx(dc, box.left + px(3), box.top + px(7), nullptr);
            LineTo(dc, box.left + px(6), box.top + px(10));
            LineTo(dc, box.left + px(11), box.top + px(4));
            SelectObject(dc, previous);
            DeleteObject(pen);
        }
        RECT label = r;
        label.left = px(21);
        SetTextColor(dc, ink);
        auto text = getW(h);
        DrawTextW(dc, text.c_str(), -1, &label, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        if (GetFocus() == h) {
            InflateRect(&label, -1, -1);
            DrawFocusRect(dc, &label);
        }
    } else if (kind == 2) {
        for (int i = 0; i < Header_GetItemCount(h); ++i) {
            RECT cell;
            Header_GetItemRect(h, i, &cell);
            wchar_t text[256]{};
            HDITEMW entry{};
            entry.mask = HDI_TEXT;
            entry.pszText = text;
            entry.cchTextMax = 256;
            Header_GetItem(h, i, &entry);
            auto brush = CreateSolidBrush(RGB(55, 58, 64));
            FillRect(dc, &cell, brush);
            DeleteObject(brush);
            auto pen = CreatePen(PS_SOLID, 1, RGB(85, 89, 99));
            auto prev = SelectObject(dc, pen);
            MoveToEx(dc, cell.right - 1, cell.top, nullptr);
            LineTo(dc, cell.right - 1, cell.bottom);
            SelectObject(dc, prev);
            DeleteObject(pen);
            cell.left += px(5);
            DrawTextW(dc, text, -1, &cell, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }
    } else if (kind == 3 && busy) {
        int width = std::max(1L, r.right), span = std::max(px(30), width / 5);
        int start = (int)((GetTickCount64() / 8) % (width + span)) - span;
        RECT pulse{std::max(0, start), r.top, std::min(width, start + span), r.bottom};
        auto brush = CreateSolidBrush(RGB(64, 132, 198));
        FillRect(dc, &pulse, brush);
        DeleteObject(brush);
    }
    SelectObject(dc, oldFont);
    if (message != WM_PRINTCLIENT)
        EndPaint(h, &ps);
    return 0;
}
LRESULT drawFeatureRow(NMLVCUSTOMDRAW *draw) {
    if (draw->nmcd.dwDrawStage == CDDS_PREPAINT)
        return CDRF_NOTIFYITEMDRAW;
    if (draw->nmcd.dwDrawStage != CDDS_ITEMPREPAINT)
        return CDRF_DODEFAULT;
    HWND list = draw->nmcd.hdr.hwndFrom;
    int row = (int)draw->nmcd.dwItemSpec;
    bool selected = (ListView_GetItemState(list, row, LVIS_SELECTED) & LVIS_SELECTED) != 0;
    COLORREF rowBackground = field;
    if (selected)
        rowBackground = dark ? RGB(40, 75, 110) : RGB(226, 238, 252);
    bool nonDefault = row >= 0 && row < (int)shown.size();
    if (nonDefault) {
        const auto &spec = definitions[shown[row]];
        auto value = ini.values.find(spec.key);
        nonDefault = value == ini.values.end() || value->second != spec.def;
    }
    auto brush = CreateSolidBrush(rowBackground);
    RECT r;
    ListView_GetItemRect(list, row, &r, LVIR_BOUNDS);
    FillRect(draw->nmcd.hdc, &r, brush);
    DeleteObject(brush);
    auto dc = draw->nmcd.hdc;
    SetTextColor(dc, fg);
    SetBkMode(dc, TRANSPARENT);
    for (int column = 0; column < 2; ++column) {
        wchar_t value[1024]{};
        ListView_GetItemText(list, row, column, value, 1024);
        RECT cell;
        ListView_GetSubItemRect(list, row, column, LVIR_LABEL, &cell);
        if (column == 0)
            cell.right = cell.left + ListView_GetColumnWidth(list, 0);
        cell.left += px(5);
        cell.right -= px(5);
        bool trueValue = column == 1 && _wcsicmp(value, L"true") == 0;
        bool falseValue = column == 1 && _wcsicmp(value, L"false") == 0;
        auto previousFont = SelectObject(dc, column == 1 && nonDefault ? boldFont : font);
        SetTextColor(dc, column == 1 && nonDefault
                             ? (dark ? RGB(125, 190, 255) : RGB(0, 85, 165))
                             : fg);
        DrawTextW(dc, value, -1, &cell,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
        if (trueValue || falseValue) {
            SIZE textSize{};
            GetTextExtentPoint32W(dc, value, (int)wcslen(value), &textSize);
            int left = cell.left + textSize.cx + px(8);
            int top = (cell.top + cell.bottom - px(14)) / 2;
            COLORREF markColor = trueValue ? (dark ? RGB(80, 220, 135) : RGB(0, 145, 65))
                                           : (dark ? RGB(255, 110, 110) : RGB(190, 35, 35));
            HPEN pen = CreatePen(PS_SOLID, px(2), markColor);
            auto oldPen = SelectObject(dc, pen);
            if (trueValue) {
                MoveToEx(dc, left, top + px(7), nullptr);
                LineTo(dc, left + px(4), top + px(11));
                LineTo(dc, left + px(13), top + px(2));
            } else {
                MoveToEx(dc, left + px(2), top + px(2), nullptr);
                LineTo(dc, left + px(12), top + px(12));
                MoveToEx(dc, left + px(12), top + px(2), nullptr);
                LineTo(dc, left + px(2), top + px(12));
            }
            SelectObject(dc, oldPen);
            DeleteObject(pen);
        }
        SelectObject(dc, previousFont);
    }
    if (selected && GetFocus() == list) {
        InflateRect(&r, -1, -1);
        DrawFocusRect(dc, &r);
    }
    return CDRF_SKIPDEFAULT;
}
void paintWindow(HWND w) {
    if (!w)
        return;
    BOOL use = dark;
    DwmSetWindowAttribute(w, 20, &use, sizeof(use));
    EnumChildWindows(
        w,
        [](HWND h, LPARAM) -> BOOL {
            SetWindowTheme(h, dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
            wchar_t cls[64];
            GetClassNameW(h, cls, 64);
            if (wcscmp(cls, L"Button") == 0 &&
                (GetWindowLongPtrW(h, GWL_STYLE) & BS_TYPEMASK) == BS_AUTOCHECKBOX) {
                SetWindowTheme(h, L"", L"");
                SetWindowSubclass(h, darkSurface, 4, 0);
            }
            if (wcscmp(cls, L"ComboBox") == 0)
                SetWindowSubclass(h, darkSurface, 1, 0);
            if (wcscmp(cls, WC_HEADERW) == 0)
                SetWindowSubclass(h, darkSurface, 2, 0);
            if (wcscmp(cls, PROGRESS_CLASSW) == 0) {
                SetWindowSubclass(h, darkSurface, 3, 0);
                if (dark && busy)
                    SetTimer(h, 42, 50, nullptr);
                else
                    KillTimer(h, 42);
            }
            if (wcscmp(cls, WC_LISTVIEWW) == 0) {
                ListView_SetBkColor(h, field);
                ListView_SetTextBkColor(h, field);
                ListView_SetTextColor(h, fg);
            }
            return TRUE;
        },
        0);
    SendMessageW(item(w, Theme), CB_SETCURSEL, themeMode, 0);
    RedrawWindow(w, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE);
}
void theme(bool force = false) {
    DWORD light = 1, n = 4;
    RegGetValueW(HKEY_CURRENT_USER,
                 L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                 L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &light, &n);
    bool value = themeMode == 2 || (themeMode == 0 && !light);
    if (!force && value == dark)
        return;
    dark = value;
    bg = dark ? RGB(32, 33, 36) : RGB(243, 243, 243);
    field = dark ? RGB(43, 45, 49) : RGB(255, 255, 255);
    fg = dark ? RGB(238, 238, 238) : RGB(30, 30, 30);
    if (bgBrush)
        DeleteObject(bgBrush);
    if (fieldBrush)
        DeleteObject(fieldBrush);
    bgBrush = CreateSolidBrush(bg);
    fieldBrush = CreateSolidBrush(field);
    paintWindow(settingsWindow);
    paintWindow(builderWindow);
}
void setTheme(HWND w) {
    themeMode = choice(w, Theme);
    HKEY key;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\M2FixGUI", 0, nullptr, 0, KEY_WRITE, nullptr,
                        &key, nullptr) == ERROR_SUCCESS) {
        DWORD mode = themeMode;
        RegSetValueExW(key, L"Appearance", 0, REG_DWORD, (BYTE *)&mode, 4);
        RegCloseKey(key);
    }
    theme(true);
}
void updateNotice() {
    absent = missing(ini);
    std::string message;
    if (!absent.empty()) {
        message = ini.path.empty() ? "Save this INI in the game folder to check required PPFs: "
                                   : "Missing installed PPFs: ";
        for (size_t i = 0; i < absent.size(); ++i) {
            if (i)
                message += ", ";
            message += absent[i].label;
        }
        message += ". Build and install a PPF package.";
        if (std::any_of(absent.begin(), absent.end(), [](auto &m) { return !m.buildable; }))
            message += " Unlock PPFs are separate and not included in the builder.";
    }
    set(settingsWindow, Notice, message);
    ShowWindow(item(settingsWindow, BuildPPF), message.empty() ? SW_HIDE : SW_SHOW);
    ShowWindow(item(settingsWindow, Details), message.empty() ? SW_HIDE : SW_SHOW);
}
void updatePath() {
    set(settingsWindow, PathLabel,
        ini.path.empty() ? "New configuration - Save As in your game folder" : str(ini.path));
    set(settingsWindow, Status, ini.dirty() ? "Unsaved changes" : "No unsaved changes");
    updateNotice();
}
void selectSetting() {
    int row = ListView_GetNextItem(item(settingsWindow, List), -1, LVNI_SELECTED);
    current = row >= 0 && row < (int)shown.size() ? (int)shown[row] : -1;
    filling = true;
    ShowWindow(item(settingsWindow, Value), SW_HIDE);
    ShowWindow(item(settingsWindow, Choice), SW_HIDE);
    if (current >= 0) {
        auto &def = definitions[current];
        auto it = ini.values.find(def.key);
        auto value = it == ini.values.end() ? "(not set)" : it->second;
        set(settingsWindow, Help,
            "[" + def.key.first + "] " + def.key.second + "\r\nRelease default: " + def.def +
                "\r\n\r\n" + def.help +
                (it == ini.values.end()
                     ? "\r\nNot set: leave unchanged to preserve the mod fallback."
                     : ""));
        if (def.kind == "bool" || def.kind == "choice") {
            auto h = item(settingsWindow, Choice);
            SendMessageW(h, CB_RESETCONTENT, 0, 0);
            std::vector<std::string> options;
            if (it == ini.values.end())
                options.push_back("(not set)");
            if (def.kind == "bool")
                options.insert(options.end(), {"true", "false"});
            else if (def.key.second == "BrightnessText")
                options.insert(options.end(), {"fixed", "original", "collection"});
            else
                options.insert(options.end(),
                               {"false", "true", "select1", "select2", "select3", "select4"});
            if (std::find(options.begin(), options.end(), value) == options.end())
                options.push_back(value);
            for (auto &s : options)
                SendMessageW(h, CB_ADDSTRING, 0, (LPARAM)wide(s).c_str());
            SetWindowTextW(h, wide(value).c_str());
            ShowWindow(h, SW_SHOW);
        } else {
            set(settingsWindow, Value, it == ini.values.end() ? "" : value);
            ShowWindow(item(settingsWindow, Value), SW_SHOW);
        }
    } else
        set(settingsWindow, Help, "Select a feature above to edit its value.");
    filling = false;
}
void populateList() {
    filling = true;
    current = -1;
    shown.clear();
    auto list = item(settingsWindow, List);
    ListView_DeleteAllItems(list);
    int sec = (int)SendMessageW(item(settingsWindow, Section), LB_GETCURSEL, 0, 0);
    if (sec < 0)
        sec = 0;
    for (size_t i = 0; i < definitions.size(); ++i) {
        auto &d = definitions[i];
        if (d.key.first != sections[sec])
            continue;
        shown.push_back(i);
        auto name = wide(d.key.second);
        LVITEMW v{};
        v.mask = LVIF_TEXT;
        v.iItem = (int)shown.size() - 1;
        v.pszText = name.data();
        ListView_InsertItem(list, &v);
        auto value = wide(ini.values.count(d.key) ? ini.values.at(d.key) : "(not set)");
        ListView_SetItemText(list, v.iItem, 1, value.data());
    }
    if (!shown.empty())
        ListView_SetItemState(list, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    filling = false;
    selectSetting();
    updatePath();
}
void editValue(int id, int code) {
    if (filling || current < 0)
        return;
    std::string value;
    if (id == Choice && code == CBN_SELCHANGE) {
        auto h = item(settingsWindow, Choice);
        int index = (int)SendMessageW(h, CB_GETCURSEL, 0, 0);
        int n = (int)SendMessageW(h, CB_GETLBTEXTLEN, index, 0);
        std::wstring s(n + 1, 0);
        SendMessageW(h, CB_GETLBTEXT, index, (LPARAM)s.data());
        s.resize(n);
        value = utf8(s);
    } else
        value = get(settingsWindow, id);
    auto key = definitions[current].key;
    if (value == "(not set)")
        ini.values.erase(key);
    else
        ini.values[key] = value;
    int row = ListView_GetNextItem(item(settingsWindow, List), -1, LVNI_SELECTED);
    auto s = wide(value);
    ListView_SetItemText(item(settingsWindow, List), row, 1, s.data());
    InvalidateRect(item(settingsWindow, List), nullptr, TRUE);
    set(settingsWindow, Status, ini.dirty() ? "Unsaved changes" : "No unsaved changes");
    updateNotice();
}
bool discard() { return !ini.dirty() || ask(settingsWindow, L"Discard unsaved settings changes?"); }
void saveIni(bool saveAs) {
    for (auto &d : definitions) {
        auto it = ini.values.find(d.key);
        if (it != ini.values.end() &&
            (!ini.loaded.count(d.key) || ini.loaded.at(d.key) != it->second))
            it->second = validate(d, it->second);
    }
    auto path = ini.path;
    if (path.empty() || saveAs)
        path = pick(settingsWindow, false, true, path.parent_path());
    if (path.empty())
        return;
    ini.save(path);
    populateList();
    set(settingsWindow, Status, "Saved. Restart the game to apply settings.");
}
void showDetails() {
    updateNotice();
    std::wstring text = L"Checked beside INI: " +
                        (ini.path.empty() ? std::wstring(L"save the INI in the game folder first")
                                          : ini.path.parent_path().wstring()) +
                        L"\r\n\r\nBefore building, select Install into MGS1 after building to "
                        L"install the generated PPFs. A ZIP or disc image alone is not an "
                        L"installed PPF. Missing/empty-file checks do "
                        L"not validate compatibility.\r\n";
    for (auto &g : absent) {
        text +=
            L"\r\n" + wide(g.label) + (g.buildable ? L":\r\n" : L" (not included in builder):\r\n");
        for (auto &n : g.paths)
            text += wide(n) + L"\r\n";
    }
    auto w =
        CreateWindowExW(WS_EX_CONTROLPARENT, L"M2FixDetails", L"M2FixGUI - Required patch files",
                        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, px(850),
                        px(550), settingsWindow, nullptr, GetModuleHandleW(nullptr), nullptr);
    auto h = control(w, L"EDIT", text.c_str(), Help, 12, 12, 800, 480,
                     ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL);
    SendMessageW(h, EM_SETLIMITTEXT, 200000, 0);
    paintWindow(w);
}
void settingsLayout(HWND w) {
    RECT r;
    GetClientRect(w, &r);
    int width = MulDiv(r.right, 100, scale), height = MulDiv(r.bottom, 100, scale);
    move(w, PathLabel, 20, 65, width - 40, 36);
    move(w, Notice, 20, 143, width - 280, 58);
    move(w, BuildPPF, width - 240, 144, 220, 28);
    move(w, Details, width - 240, 176, 220, 28);
    move(w, Section, 20, 216, 190, height - 454);
    move(w, List, 225, 216, width - 245, height - 454);
    int columns = px(width - 249), first = columns * 2 / 3;
    ListView_SetColumnWidth(item(w, List), 0, first);
    ListView_SetColumnWidth(item(w, List), 1, columns - first);
    move(w, Value, 315, height - 226, width - 335, 27);
    move(w, Choice, 315, height - 226, width - 335, 300);
    move(w, Help, 225, height - 190, width - 245, 128);
    move(w, Status, 20, height - 38, width - 320, 24);
    move(w, Save, width - 286, height - 42, 130, 30);
    move(w, SaveAs, width - 145, height - 42, 125, 30);
}
void createSettings(HWND w) {
    settingsWindow = w;
    auto heading = control(w, L"STATIC", L"M2Fix Settings", 0, 20, 18, 310, 34);
    SendMessageW(heading, WM_SETFONT, (WPARAM)titleFont, TRUE);
    button(w, L"Open Patch & Disc Builder", Builder, 480, 20, 230);
    label(w, L"Appearance", 735, 25, 90);
    combo(w, Theme, 820, 20, 120, {L"System", L"Light", L"Dark"});
    label(w, L"", 20, 65, 900, 36, PathLabel);
    button(w, L"Open INI...", Open, 20, 105, 115);
    button(w, L"Reload", Reload, 145, 105, 90);
    button(w, L"Reset to release defaults", Defaults, 245, 105, 210);
    label(w, L"", 20, 143, 650, 65, Notice);
    button(w, L"Build and install PPFs", BuildPPF, 720, 144, 220);
    button(w, L"Missing files / help", Details, 720, 176, 220);
    control(w, L"LISTBOX", L"", Section, 20, 216, 190, 300, LBS_NOTIFY | WS_VSCROLL | WS_TABSTOP);
    for (auto &d : definitions)
        if (std::find(sections.begin(), sections.end(), d.key.first) == sections.end())
            sections.push_back(d.key.first);
    for (auto &s : sections)
        SendMessageW(item(w, Section), LB_ADDSTRING, 0, (LPARAM)wide(s).c_str());
    SendMessageW(item(w, Section), LB_SETCURSEL, 0, 0);
    auto list = control(w, WC_LISTVIEWW, L"", List, 225, 216, 715, 300,
                        LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_TABSTOP);
    ListView_SetExtendedListViewStyle(list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    LVCOLUMNW col{};
    col.mask = LVCF_TEXT | LVCF_WIDTH;
    col.cx = px(430);
    col.pszText = (LPWSTR)L"Feature";
    ListView_InsertColumn(list, 0, &col);
    col.cx = px(220);
    col.pszText = (LPWSTR)L"Value";
    ListView_InsertColumn(list, 1, &col);
    control(w, L"EDIT", L"", Value, 315, 540, 625, 27, WS_TABSTOP | ES_AUTOHSCROLL);
    combo(w, Choice, 315, 540, 625, {}, true);
    control(w, L"EDIT", L"", Help, 225, 580, 715, 130, ES_MULTILINE | ES_READONLY | WS_VSCROLL);
    label(w, L"", 20, 738, 620, 24, Status);
    button(w, L"Save settings", Save, 654, 734, 130);
    button(w, L"Save As...", SaveAs, 795, 734, 125);
    wchar_t exe[32768];
    GetModuleFileNameW(nullptr, exe, 32768);
    fs::path adjacent = fs::path(exe).parent_path() / L"MGSM2Fix.ini", game = findGame();
    if (fs::exists(adjacent))
        ini.load(adjacent);
    else if (!game.empty() && fs::exists(game / L"MGSM2Fix.ini"))
        ini.load(game / L"MGSM2Fix.ini");
    else
        ini.load();
    populateList();
    settingsLayout(w);
    SetTimer(w, 1, 10000, nullptr);
    theme(true);
}
void builderRefresh() {
    bool patches = choice(builderWindow, Operation) == 0;
    EnableWindow(item(builderWindow, Disc), !patches);
    EnableWindow(item(builderWindow, English), !patches);
    EnableWindow(item(builderWindow, InstallAfter), patches);
    bool install = patches && checked(builderWindow, InstallAfter);
    EnableWindow(item(builderWindow, InstallTarget), install);
    EnableWindow(item(builderWindow, BrowseInstall), install);
    std::string status;
    if (!patches)
        status = "Disc-image builds create BIN/CUE files; installation applies to PPF builds.";
    else if (install)
        status = "PPFs will be installed after a successful build. Close MGS1 first.";
    else
        status = "Choose a package output folder; installation is optional.";
    set(builderWindow, BuildStatus, status);
}
void builderLayout(HWND w) {
    RECT r;
    GetClientRect(w, &r);
    int width = MulDiv(r.right, 100, scale), height = MulDiv(r.bottom, 100, scale);
    move(w, Input, 150, 128, width - 280, 27);
    move(w, BrowseInput, width - 115, 127, 95, 30);
    move(w, Output, 150, 258, width - 280, 27);
    move(w, BrowseOutput, width - 115, 257, 95, 30);
    move(w, InstallTarget, 150, 407, width - 280, 27);
    move(w, BrowseInstall, width - 115, 406, 95, 30);
    move(w, BuildStatus, 20, 445, width - 40, 36);
    move(w, Progress, 20, 485, width - 40, 18);
    move(w, BuildLog, 20, 518, width - 40, height - 582);
    move(w, Build, width - 180, height - 50, 160, 32);
}
void createBuilder(HWND w) {
    builderWindow = w;
    label(w, L"Patch & Disc Builder", 20, 20, 480, 36);
    SendMessageW(GetWindow(w, GW_CHILD), WM_SETFONT, (WPARAM)titleFont, TRUE);
    label(w, L"Appearance", 570, 25, 90);
    combo(w, Theme, 660, 20, 120, {L"System", L"Light", L"Dark"});
    label(w,
          L"Choose source files, patch features, and an output folder. Originals "
          L"stay untouched.",
          20, 65, 760, 38);
    label(w, L"Game source", 20, 100, 125);
    combo(w, Source, 150, 95, 390, {L"Steam installation", L"Redump images"});
    label(w, L"Source folder", 20, 133, 125);
    control(w, L"EDIT", findGame().c_str(), Input, 150, 128, 515, 27, ES_AUTOHSCROLL | WS_TABSTOP);
    button(w, L"Browse...", BrowseInput, 685, 127, 95);
    label(w, L"Build", 20, 180, 125);
    combo(w, Operation, 150, 175, 515,
          {L"M2Fix patch package (PPF)", L"Disc images (BIN/CUE)"});
    label(w, L"Discs", 20, 220, 125);
    combo(w, Disc, 150, 215, 515, {L"All three discs", L"Disc 1", L"Disc 2", L"VR disc"});
    label(w, L"Save to", 20, 263, 125);
    control(w, L"EDIT", L"", Output, 150, 258, 515, 27, ES_AUTOHSCROLL | WS_TABSTOP);
    button(w, L"Browse...", BrowseOutput, 685, 257, 95);
    for (auto [id, name, x, y] : std::vector<std::tuple<int, const wchar_t *, int, int>>{
             {Story, L"Story discs: English text", 20, 309},
             {VR, L"VR disc: English text", 410, 309},
             {Grenade, L"VR grenade display correction", 20, 344},
             {English, L"Start disc images in English", 410, 344}}) {
        auto h = control(w, L"BUTTON", name, id, x, y, 370, 26, BS_AUTOCHECKBOX | WS_TABSTOP);
        SendMessageW(h, BM_SETCHECK, BST_CHECKED, 0);
    }
    control(w, L"BUTTON", L"Install into MGS1 after building", InstallAfter, 20, 376, 370, 26,
            BS_AUTOCHECKBOX | WS_TABSTOP);
    label(w, L"Install to", 20, 412, 125);
    control(w, L"EDIT", findGame().c_str(), InstallTarget, 150, 407, 515, 27,
            ES_AUTOHSCROLL | WS_TABSTOP);
    button(w, L"Browse...", BrowseInstall, 685, 406, 95);
    label(w, L"", 20, 445, 760, 36, BuildStatus);
    control(w, PROGRESS_CLASSW, L"", Progress, 20, 485, 760, 18, PBS_MARQUEE);
    control(w, L"EDIT", L"", BuildLog, 20, 518, 760, 190,
            ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL);
    button(w, L"Build", Build, 620, 745, 160);
    builderRefresh();
    builderLayout(w);
    paintWindow(w);
}
void openBuilder(bool patches = false) {
    if (builderWindow) {
        ShowWindow(builderWindow, SW_RESTORE);
        SetForegroundWindow(builderWindow);
        if (patches && !busy) {
            SendMessageW(item(builderWindow, Operation), CB_SETCURSEL, 0, 0);
            builderRefresh();
        }
        return;
    }
    CreateWindowExW(WS_EX_CONTROLPARENT, L"M2FixBuilder", L"M2FixGUI - Patch & Disc Builder",
                    WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, px(840),
                    px(830), settingsWindow, nullptr, GetModuleHandleW(nullptr), nullptr);
}
void log(HWND w, const std::string &message) {
    auto s = wide(message + "\r\n");
    auto h = item(w, BuildLog);
    SendMessageW(h, EM_SETSEL, (WPARAM)-1, -1);
    SendMessageW(h, EM_REPLACESEL, FALSE, (LPARAM)s.c_str());
}
struct Result {
    std::string error;
    fs::path package, installedTo, backup;
    bool images = false;
    bool installed = false;
};
void startBuild(HWND w) {
    need(!busy, "Build already running");
    Options o;
    o.steam = choice(w, Source) == 0;
    o.images = choice(w, Operation) == 1;
    o.source = wide(get(w, Input));
    o.destination = wide(get(w, Output));
    o.story = checked(w, Story);
    o.vr = checked(w, VR);
    o.grenade = checked(w, Grenade);
    o.english = checked(w, English);
    bool installAfter = !o.images && checked(w, InstallAfter);
    fs::path installTarget = installAfter ? wide(get(w, InstallTarget)) : fs::path{};
    if (installAfter && installTarget.empty() && o.steam)
        installTarget = o.source;
    int disc = choice(w, Disc);
    o.disc = disc == 0 ? "all" : disc == 1 ? "1" : disc == 2 ? "2" : "vr";
    need(!o.source.empty() && !o.destination.empty(), "Choose source and output folders");
    if (installAfter) {
        need(!installTarget.empty(), "Choose the MGS1 folder to install into");
        need(fs::is_regular_file(installTarget / L"METAL GEAR SOLID.exe") &&
                 fs::is_regular_file(installTarget / L"windata/alldata.bin"),
             "Install destination must contain METAL GEAR SOLID.exe and windata/alldata.bin");
    }
    busy = true;
    if (dark)
        SetTimer(item(w, Progress), 42, 50, nullptr);
    for (int id = Source; id <= Build; ++id)
        EnableWindow(item(w, id), FALSE);
    set(w, BuildLog, "");
    set(w, BuildStatus, "Building and verifying source files...");
    SendMessageW(item(w, Progress), PBM_SETMARQUEE, TRUE, 30);
    std::thread([w, o, installAfter, installTarget]() {
        auto result = new Result;
        result->images = o.images;
        try {
            result->package = build(o, [w](std::string message) {
                PostMessageW(w, LOG, 0, (LPARAM) new std::string(std::move(message)));
            });
            if (installAfter) {
                result->installedTo = installTarget;
                result->backup = install(result->package, installTarget);
                result->installed = true;
            }
        } catch (const std::exception &e) {
            result->error = e.what();
        }
        PostMessageW(w, DONE, 0, (LPARAM)result);
    }).detach();
}
void finishBuild(HWND w, Result &result) {
    busy = false;
    KillTimer(item(w, Progress), 42);
    SendMessageW(item(w, Progress), PBM_SETMARQUEE, FALSE, 0);
    for (int id = Source; id <= Build; ++id)
        EnableWindow(item(w, id), TRUE);
    builderRefresh();
    if (!result.error.empty()) {
        set(w, BuildStatus, result.package.empty()
                               ? "Build failed; details below."
                               : "PPF package built, but installation failed; details below.");
        if (!result.package.empty())
            log(w, "PPF package remains at " + str(result.package));
        log(w, result.error);
        return;
    }
    if (result.installed) {
        log(w, "Installed PPFs into " + str(result.installedTo) +
                  "\r\nBackup: " + str(result.backup));
        set(w, BuildStatus, "PPFs built and installed. Enable corresponding settings and restart MGS1.");
        updateNotice();
    } else if (result.images) {
        set(w, BuildStatus, "Disc images built successfully.");
    } else {
        set(w, BuildStatus, "PPF package built successfully.");
    }
}
LRESULT common(HWND w, UINT m, WPARAM a, LPARAM b, bool &handled) {
    handled = true;
    if (m == WM_CTLCOLORSTATIC || m == WM_CTLCOLOREDIT || m == WM_CTLCOLORLISTBOX ||
        m == WM_CTLCOLORBTN) {
        HDC dc = (HDC)a;
        SetTextColor(dc, fg);
        bool isField = m != WM_CTLCOLORSTATIC && m != WM_CTLCOLORBTN;
        wchar_t cls[32];
        GetClassNameW((HWND)b, cls, 32);
        isField |= wcscmp(cls, L"Edit") == 0;
        SetBkColor(dc, isField ? field : bg);
        return (LRESULT)(isField ? fieldBrush : bgBrush);
    }
    if (m == WM_ERASEBKGND) {
        RECT r;
        GetClientRect(w, &r);
        FillRect((HDC)a, &r, bgBrush);
        return 1;
    }
    if (m == WM_DRAWITEM) {
        auto d = (DRAWITEMSTRUCT *)b;
        if (d->CtlType == ODT_COMBOBOX) {
            auto brush = CreateSolidBrush((d->itemState & ODS_SELECTED) ? RGB(40, 90, 140) : field);
            FillRect(d->hDC, &d->rcItem, brush);
            DeleteObject(brush);
            SetBkMode(d->hDC, TRANSPARENT);
            SetTextColor(d->hDC, (d->itemState & ODS_DISABLED)   ? RGB(140, 140, 140)
                                 : (d->itemState & ODS_SELECTED) ? RGB(255, 255, 255)
                                                                 : fg);
            if (d->itemID != (UINT)-1) {
                int n = (int)SendMessageW(d->hwndItem, CB_GETLBTEXTLEN, d->itemID, 0);
                std::wstring text(n + 1, 0);
                SendMessageW(d->hwndItem, CB_GETLBTEXT, d->itemID, (LPARAM)text.data());
                RECT r = d->rcItem;
                r.left += px(5);
                DrawTextW(d->hDC, text.c_str(), n, &r,
                          DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            }
            return TRUE;
        }
        if (d->CtlType == ODT_BUTTON) {
            auto brush = CreateSolidBrush(dark ? RGB(55, 58, 64) : RGB(230, 230, 230));
            FillRect(d->hDC, &d->rcItem, brush);
            DeleteObject(brush);
            SetBkMode(d->hDC, TRANSPARENT);
            SetTextColor(d->hDC, (d->itemState & ODS_DISABLED) ? RGB(140, 140, 140) : fg);
            auto text = getW(d->hwndItem);
            RECT r = d->rcItem;
            DrawTextW(d->hDC, text.c_str(), -1, &r,
                      DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            FrameRect(d->hDC, &r, (HBRUSH)GetStockObject(GRAY_BRUSH));
            if (d->itemState & ODS_FOCUS) {
                InflateRect(&r, -4, -4);
                DrawFocusRect(d->hDC, &r);
            }
            return TRUE;
        }
    }
    handled = false;
    return 0;
}
LRESULT CALLBACK proc(HWND w, UINT m, WPARAM a, LPARAM b) {
    try {
        bool handled;
        auto c = common(w, m, a, b, handled);
        if (handled)
            return c;
        bool builder = w == builderWindow;
        if (m == WM_CREATE) {
            wchar_t name[64];
            GetClassNameW(w, name, 64);
            if (wcscmp(name, L"M2FixBuilder") == 0)
                createBuilder(w);
            else if (wcscmp(name, L"M2FixSettings") == 0)
                createSettings(w);
            return 0;
        }
        if (m == WM_GETMINMAXINFO) {
            auto p = (MINMAXINFO *)b;
            p->ptMinTrackSize = {px(builder ? 840 : 980), px(builder ? 750 : 760)};
            return 0;
        }
        if (m == WM_SIZE) {
            if (w == settingsWindow)
                settingsLayout(w);
            else if (builder)
                builderLayout(w);
            else {
                RECT r;
                GetClientRect(w, &r);
                MoveWindow(item(w, Help), px(12), px(12), r.right - px(24), r.bottom - px(24),
                           TRUE);
            }
            return 0;
        }
        if (m == WM_TIMER) {
            updateNotice();
            return 0;
        }
        if (m == WM_SETTINGCHANGE) {
            theme();
            return 0;
        }
        if (m == WM_NOTIFY && w == settingsWindow) {
            auto hdr = (NMHDR *)b;
            if (hdr->idFrom == List && hdr->code == NM_CUSTOMDRAW)
                return drawFeatureRow((NMLVCUSTOMDRAW *)b);
            if (hdr->idFrom == List && hdr->code == LVN_ITEMCHANGED && !filling)
                selectSetting();
            return 0;
        }
        if (m == LOG) {
            std::unique_ptr<std::string> s((std::string *)b);
            log(w, *s);
            return 0;
        }
        if (m == DONE) {
            std::unique_ptr<Result> r((Result *)b);
            finishBuild(w, *r);
            return 0;
        }
        if (m == WM_COMMAND) {
            int id = LOWORD(a), code = HIWORD(a);
            if (id == Theme && code == CBN_SELCHANGE) {
                setTheme(w);
                return 0;
            }
            if (builder) {
                switch (id) {
                case Source:
                    if (code == CBN_SELCHANGE) {
                        auto steam = choice(w, Source) == 0;
                        auto game = steam ? str(findGame()) : "";
                        set(w, Input, game);
                        if (steam && get(w, InstallTarget).empty())
                            set(w, InstallTarget, game);
                    }
                    break;
                case Operation:
                    if (code == CBN_SELCHANGE)
                        builderRefresh();
                    break;
                case InstallAfter:
                    if (code == BN_CLICKED)
                        builderRefresh();
                    break;
                case BrowseInput:
                case BrowseOutput:
                case BrowseInstall: {
                    int targetControl = id == BrowseInput ? Input
                                        : id == BrowseOutput ? Output
                                                             : InstallTarget;
                    auto p = pick(w, true, false, wide(get(w, targetControl)));
                    if (!p.empty()) {
                        set(w, targetControl, str(p));
                        if (targetControl == Input && choice(w, Source) == 0 &&
                            get(w, InstallTarget).empty())
                            set(w, InstallTarget, str(p));
                    }
                    break;
                }
                case Build:
                    startBuild(w);
                    break;
                }
                return 0;
            }
            switch (id) {
            case Section:
                if (code == LBN_SELCHANGE)
                    populateList();
                break;
            case Value:
                if (code == EN_CHANGE)
                    editValue(id, code);
                break;
            case Choice:
                if (code == CBN_SELCHANGE || code == CBN_EDITCHANGE)
                    editValue(id, code);
                break;
            case Builder:
                openBuilder();
                break;
            case BuildPPF:
                openBuilder(true);
                break;
            case Details:
                showDetails();
                break;
            case Open:
                if (discard()) {
                    auto p = pick(w, false);
                    if (!p.empty()) {
                        ini.load(p);
                        populateList();
                    }
                }
                break;
            case Reload:
                if (!ini.path.empty() && discard()) {
                    ini.load(ini.path);
                    populateList();
                }
                break;
            case Defaults:
                for (auto &d : definitions)
                    ini.values[d.key] = d.def;
                populateList();
                break;
            case Save:
                saveIni(false);
                break;
            case SaveAs:
                saveIni(true);
                break;
            }
            return 0;
        }
        if (m == WM_CLOSE) {
            if (busy && (builder || w == settingsWindow)) {
                MessageBoxW(w, L"Please wait for the build to finish.", L"Build in progress",
                            MB_OK);
                return 0;
            }
            if (w == settingsWindow) {
                if (!discard())
                    return 0;
                if (builderWindow)
                    DestroyWindow(builderWindow);
            }
            DestroyWindow(w);
            return 0;
        }
        if (m == WM_DESTROY) {
            if (builder) {
                builderWindow = nullptr;
                return 0;
            }
            if (w == settingsWindow) {
                settingsWindow = nullptr;
                PostQuitMessage(0);
            }
            return 0;
        }
    } catch (const std::exception &e) {
        error(w, e);
    }
    return DefWindowProcW(w, m, a, b);
}
} // namespace
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR, int) {
    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::unique_ptr<void, decltype(&LocalFree)> argvOwner(argv, LocalFree);
    try {
        need(argv != nullptr, "Cannot parse command line");
        if (argc > 1) {
            std::map<std::string, std::wstring> args;
            for (int i = 1; i < argc; ++i) {
                auto key = utf8(argv[i]);
                if (key == "--worker" || key.find("--no-") == 0)
                    args[key] = L"true";
                else if (i + 1 < argc)
                    args[key] = argv[++i];
            }
            Options o;
            o.steam = args["--source"] == L"steam";
            o.images = args["--operation"] != L"patches";
            o.source = args["--input"];
            o.destination = args["--destination"];
            o.disc = args.count("--discs") ? utf8(args["--discs"]) : "all";
            o.story = !args.count("--no-story");
            o.vr = !args.count("--no-vr");
            o.grenade = !args.count("--no-grenade");
            o.english = !args.count("--no-english-default");
            std::ofstream log;
            if (args.count("--log"))
                log.open(fs::path(args["--log"]), std::ios::binary);
            try {
                if (args.count("--install")) {
                    auto backup = install(fs::path(args["--install"]), fs::path(args["--game"]));
                    log << str(backup);
                } else
                    build(o, [&](std::string s) {
                        log << s << "\n";
                        log.flush();
                    });
                return 0;
            } catch (const std::exception &e) {
                log << "Build failed: " << e.what() << "\n";
                return 1;
            }
        }
        CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS};
        InitCommonControlsEx(&controls);
        HDC dc = GetDC(nullptr);
        scale = MulDiv(GetDeviceCaps(dc, LOGPIXELSX), 100, 96);
        ReleaseDC(nullptr, dc);
        font = CreateFontW(-px(14), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                           DEFAULT_PITCH, L"Segoe UI");
        boldFont = CreateFontW(-px(14), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                               OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                               DEFAULT_PITCH, L"Segoe UI");
        titleFont = CreateFontW(-px(24), 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                DEFAULT_PITCH, L"Segoe UI");
        DWORD mode = 0, n = 4;
        RegGetValueW(HKEY_CURRENT_USER, L"Software\\M2FixGUI", L"Appearance", RRF_RT_REG_DWORD,
                     nullptr, &mode, &n);
        themeMode = mode <= 2 ? mode : 0;
        theme(true);
        for (auto cls : {L"M2FixSettings", L"M2FixBuilder", L"M2FixDetails"}) {
            WNDCLASSEXW wc{sizeof(wc)};
            wc.lpfnWndProc = proc;
            wc.hInstance = instance;
            wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
            wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
            wc.lpszClassName = cls;
            RegisterClassExW(&wc);
        }
        CreateWindowExW(WS_EX_CONTROLPARENT, L"M2FixSettings", L"M2FixGUI - Settings",
                        WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, px(1000),
                        px(830), nullptr, nullptr, instance, nullptr);
        MSG message;
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            if ((builderWindow && IsDialogMessageW(builderWindow, &message)) ||
                (settingsWindow && IsDialogMessageW(settingsWindow, &message)))
                continue;
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        CoUninitialize();
        return 0;
    } catch (const std::exception &e) {
        MessageBoxW(nullptr, wide(e.what()).c_str(), L"M2FixGUI", MB_ICONERROR);
        return 1;
    }
}
