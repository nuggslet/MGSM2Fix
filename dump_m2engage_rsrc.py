import pefile
import os
import sys
import re

TRANSLATIONS = {
    "繝ｪ繧ｽ繝ｼ繧ｹ繝輔か繝ｫ繝": "Resource folder\tdefault:revodate/",
    "リソースフォルダ(実行カレントからの相対パス）": "Resource folder (relative to current working dir)\tdefault:revodate/",
    "Backup繝輔か繝ｫ繝": "Backup folder\tdefault:winbackup",
    "Backupフォルダ(ディレクトリ名)": "Backup folder (directory name)\tdefault:winbackup",
    "繧ｦ繧｣繝ｳ繝峨え繧ｯ繝ｩ繧ｹ繝阪ｼ繝": "Window class name\t\t\tdefault:m2lib",
    "ウィンドウクラスネーム": "Window class name\t\t\tdefault:m2lib",
    "繧ｭ繝｣繝励す繝ｧ繝ｳ": "Window caption\t\t\t\tdefault:m2lib",
    "キャプション": "Window caption\t\t\t\tdefault:m2lib",
    "繧ｦ繧｣繝ｳ繝峨え繧ｵ繧､繧ｺ": "Window size\t\t\t\tdefault:640,480",
    "ウィンドウサイズ": "Window size\t\t\t\tdefault:640,480",
    "VSync": "Window VSync setting. 0 = off (FPS uncapped), 1 = on\t\tdefault:0",
    "Drag": "Window edge drag-resizing. 0 = off, 1 = on\t\tdefault:0",
    "フルスクリーン種類 0:ボーダーレス, 1:排他": "Fullscreen type: 0 = borderless, 1 = exclusive\t\tdefault:0",
    "フルスクリーンモード時のリフレッシュレート 0の場合は現在のモニタのリフレッシュレート": "Refresh rate in fullscreen mode; if 0, use current monitor’s refresh rate\t\tdefault:0"
}

def translate_comment(comment: str) -> str:
    for key, value in TRANSLATIONS.items():
        if key in comment:
            return value
    return comment.strip()

def bake_translations(text: str) -> str:
    lines = []
    for line in text.splitlines():
        if "//" in line:
            code, comment = line.split("//", 1)
            translated = translate_comment(comment)
            lines.append(f"{code.strip()} // {translated}")
        else:
            lines.append(line)
    return "\n".join(lines)

def clean_text_block(sliced_text: str) -> str:
    """
    Stop output on the first line that doesn't have ' = '.
    Trim off trailing CRLF from each line.
    """
    cleaned_lines = []
    for line in sliced_text.splitlines(True):
        stripped_line = line.rstrip("\r\n")

        if " = " in stripped_line:
            cleaned_lines.append(stripped_line)
        elif stripped_line.strip().startswith("//") or stripped_line.strip().startswith("#"):
            cleaned_lines.append(stripped_line)
        else:
            break

    return "\n".join(cleaned_lines)

def hex_dump_lines(raw_data: bytes, section_rva: int) -> str:
    """
    Find # SPEC_TXT in raw_data and dump each line of text plus raw bytes.
    Show \r, \n, and \t literally in the text section.
    Prefix each line with the actual RVA address in the exe.
    """
    marker = "# SPEC_TXT".encode("shift_jis", errors="ignore")
    start = raw_data.find(marker)
    if start == -1:
        return "[!] Could not locate # SPEC_TXT in raw_data"

    region = raw_data[start:]
    output = []
    buf = b""
    offset = start

    def make_visible(text: str) -> str:
        return (
            text.replace("\r", "\\r")
                .replace("\n", "\\n")
                .replace("\t", "\\t")
        )

    pos = 0
    for b in region:
        buf += bytes([b])
        pos += 1
        if buf.endswith(b"\r\n") or buf.endswith(b"\n"):
            text_line = buf.decode("shift_jis", errors="ignore")
            visible_line = make_visible(text_line)

            if not (" = " in text_line or text_line.strip().startswith(("//", "#"))):
                break

            hex_bytes = " ".join(f"{x:02X}" for x in buf)

            line_addr = section_rva + offset
            output.append(f"[0x{line_addr:08X}] {visible_line}\n  [RAW] {hex_bytes}\n")

            offset += len(buf)
            buf = b""

    return "\n".join(output)



def extract_classname(text: str) -> str:
    """
    Find the CLASSNAME field and return its value
    (up to first tab or newline).
    """
    for line in text.splitlines():
        if line.strip().startswith("CLASSNAME"):
            _, value = line.split("=", 1)
            value = value.strip()
            value = value.split("\t")[0]  # stop at tab
            value = value.split("\n")[0]  # stop at newline
            return value
    return "output"  # fallback if not found


def dump_rsrc_to_text(exe_path):
    try:
        pe = pefile.PE(exe_path)
    except Exception as e:
        print(f"[!] Failed to parse {exe_path}: {e}")
        return

    rsrc_section = None
    for section in pe.sections:
        name = section.Name.rstrip(b'\x00').decode(errors="ignore")
        if name == ".rsrc":
            rsrc_section = section
            break

    if not rsrc_section:
        print(f"[!] No .rsrc section in {exe_path}")
        return

    raw_data = rsrc_section.get_data()
    text = raw_data.decode("shift_jis", errors="ignore").replace("\r\n", "\n")

    marker = "# SPEC_TXT"
    idx = text.find(marker)
    if idx == -1:
        print(f"[!] Marker '{marker}' not found in {exe_path}")
        return

    sliced_text = text[idx:]
    cleaned_text = clean_text_block(sliced_text)
    translated_text = bake_translations(cleaned_text)

    # build output name
    exe_base = os.path.splitext(os.path.basename(exe_path))[0]
    classname = extract_classname(cleaned_text)
    out_name = f"({classname}) {exe_base}.rsrc.txt"

    with open(out_name, "w", encoding="utf-8") as f:
        f.write(cleaned_text)
        f.write("\n\n------------------------------ TRANSLATED ------------------------------\n\n")
        f.write(translated_text)
        f.write("\n\n------------------------------ RAW BYTES ------------------------------\n\n")
        f.write(hex_dump_lines(raw_data, rsrc_section.VirtualAddress))

    print(f"[+] Dumped .rsrc from {exe_path} → {out_name}")

def main():
    cwd = os.getcwd()
    for fname in os.listdir(cwd):
        if fname.lower().endswith((".exe")):
            if os.path.abspath(fname) == os.path.abspath(sys.argv[0]):
                continue
            dump_rsrc_to_text(fname)

if __name__ == "__main__":
    main()
