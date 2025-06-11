import psutil
import win32gui
import win32con
import win32api
import win32process
import subprocess
import time

pid = 0
for proc in psutil.process_iter(['pid', 'name']):
    if proc.info['name'].lower() != 'steam.exe': continue
    pid = proc.info['pid']
    break

launcher = psutil.Process(pid)
exe = launcher.exe()

apps = [
    2131630,
    2306740,
    1018020,
    1018010,
    1552550,
    2369900,
    2478020,
    1638330,
    1640160,
]

for id in apps:
    subprocess.run([exe, '-applaunch', str(id)])

    app = None
    while not app:
        children = proc.children(recursive=False)
        for child in children:
            try:
                if child.name().lower() == 'steamwebhelper.exe': continue
                if child.name().lower() == 'gameoverlayui.exe': continue
                app = child
            except psutil.NoSuchProcess: continue
            break

    def GetWindows(pid):
        def _GetWindows(hwnd, hwnds):
            if win32gui.IsWindowVisible(hwnd) and win32gui.IsWindowEnabled(hwnd):
                _, _pid = win32process.GetWindowThreadProcessId(hwnd)
                if _pid == pid: hwnds.append(hwnd)
            return True

        hwnds = []
        win32gui.EnumWindows(_GetWindows, hwnds)
        return hwnds

    def CloseWindow(hwnd):
        win32gui.ShowWindow(hwnd, win32con.SW_RESTORE)
        win32gui.SetForegroundWindow(hwnd)

        win32api.PostMessage(hwnd, win32con.WM_SYSKEYDOWN, win32con.VK_MENU, 0x20000000)
        win32api.PostMessage(hwnd, win32con.WM_SYSKEYDOWN, win32con.VK_F4,   0x203D0001)
        win32api.PostMessage(hwnd, win32con.WM_SYSKEYUP,   win32con.VK_F4,   0xC03D0001)
        win32api.PostMessage(hwnd, win32con.WM_SYSKEYUP,   win32con.VK_MENU, 0xC0000000)

    hwnds = GetWindows(app.pid)
    while not hwnds: hwnds = GetWindows(app.pid)

    time.sleep(10)
    CloseWindow(hwnds[0])

    try: result = app.wait(10)
    except Exception as e:
        result = e
        app.kill()
        app.wait(10)

    status = "PASS"
    if result != 0: status = "FAIL (%s)" % result
    print("Title %u: %s" % (id, status))
