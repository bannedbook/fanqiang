import locale
import os
import subprocess
import sys


class SimpleI18N:
    def __init__(self, lang=None):
        if lang:
            self.lang = lang
        else:
            self.lang = self.get_os_language()

        if not self.lang:
            self.lang = "en_US"

    def get_os_language(self):
        try:
            lang_code, code_page = locale.getdefaultlocale()
            # ('en_GB', 'cp1252'), en_US,
            self.lang_code = lang_code
            return lang_code
        except:
            # Mac fail to run this
            pass

        if sys.platform == "darwin":
            try:
                oot = os.pipe()
                p = subprocess.Popen(["/usr/bin/defaults", 'read', 'NSGlobalDomain', 'AppleLanguages'], stdout=oot[1])
                p.communicate()
                lang_code = self.get_default_language_code_for_mac(os.read(oot[0], 10000))
                self.lang_code = lang_code
                return lang_code
            except:
                pass

        lang_code = 'Unknown'
        return lang_code

    def get_valid_languages(self):
        # return ['de_DE', 'en_US', 'es_VE', 'fa_IR', 'ja_JP', 'zh_CN']
        return ['en_US', 'fa_IR', 'zh_CN']

    def get_default_language_code_for_mac(self, lang_code):
        if 'zh' in lang_code:
            return 'zh_CN'
        elif 'en' in lang_code:
            return 'en_US'
        elif 'fa' in lang_code:
            return 'fa_IR'
        else:
            return 'Unknown'

    def po_loader(self, file):
        po_dict = {}
        fp = open(file, "r")
        while True:
            line = fp.readline()
            if not line:
                break

            if len(line) < 2:
                continue

            if line.startswith("#"):
                continue

            if line.startswith("msgid "):
                key = line[7:-2]
                value = ""
                while True:
                    line = fp.readline()
                    if not line:
                        break

                    if line.startswith("\""):
                        key += line[1:-2]
                    elif line.startswith("msgstr "):
                        value += line[8:-2]
                        break
                    else:
                        break

                while True:
                    line = fp.readline()
                    if not line:
                        break

                    if line.startswith("\""):
                        value += line[1:-2]
                    else:
                        break

                if key == "":
                    continue

                po_dict[key] = value

        return po_dict

    def _render(self, po_dict, file):
        fp = open(file, "r")
        content = fp.read()

        out_arr = []

        cp = 0
        while True:
            bp = content.find("{{", cp)
            if bp == -1:
                break

            ep = content.find("}}", bp)
            if ep == -1:
                print(content[bp:])
                break

            b1p = content.find("_(", bp, ep)
            if b1p == -1:
                print(content[bp:])
                continue
            b2p = content.find("\"", b1p + 2, b1p + 4)
            if b2p == -1:
                print(content[bp:])
                continue

            e1p = content.find(")", ep - 2, ep)
            if e1p == -1:
                print(content[bp:])
                continue

            e2p = content.find("\"", e1p - 2, e1p)
            if e2p == -1:
                print(content[bp:])
                continue

            out_arr.append(content[cp:bp])
            key = content[b2p + 1:e2p]
            if po_dict.get(key, "") == "":
                out_arr.append(key)
            else:
                out_arr.append(po_dict[key])

            cp = ep + 2

        out_arr.append(content[cp:])

        return "".join(out_arr)

    def render(self, lang_path, template_file):
        po_file = os.path.join(lang_path, self.lang, "LC_MESSAGES", "messages.po")
        if not os.path.isfile(po_file):
            return self._render(dict(), template_file)
        else:
            po_dict = self.po_loader(po_file)
            return self._render(po_dict, template_file)
