#!/usr/bin/env python
# coding:utf-8
# Contributor:
#      Phus Lu        <phus.lu@gmail.com>
import os
import sys
import webbrowser

from xlog import getLogger
xlog = getLogger("launcher")

import config
if __name__ == "__main__":
    current_path = os.path.dirname(os.path.abspath(__file__))
    python_path = os.path.abspath( os.path.join(current_path, os.pardir, 'python27', '1.0'))
    noarch_lib = os.path.abspath( os.path.join(python_path, 'lib', 'noarch'))
    sys.path.append(noarch_lib)

#Only enable AppIndicator in the DEs that are Unity and QT-based
enable_appind = False
if 'XDG_CURRENT_DESKTOP' in os.environ:
    cur_desktops = os.environ['XDG_CURRENT_DESKTOP'].split(':')
    if {'Unity', 'KDE', 'LXQt', 'ENLIGHTENMENT'}.intersection(cur_desktops):
        enable_appind = True

try:
    import pygtk
    pygtk.require('2.0')
    import gtk
    import gtk.gdk as gdk
    use_gi = False
    xlog.info('Using PyGTK as the GUI Backend.')
except:
    import gi
    gi.require_version('Gtk', '3.0')
    gi.require_version('Gdk', '3.0')
    from gi.repository import Gtk as gtk
    from gi.repository import Gdk as gdk
    use_gi = True
    xlog.info('Using PyGObject as the GUI Backend.')

gdk.threads_init()

import module_init

if use_gi:
    try:
        gi.require_version('Notify', '0.7')
        from gi.repository import Notify as notify
        notify.init('XX-Net Notify')
        new_notification = notify.Notification.new
    except:
        xlog.warn("import Notify fail, please install libnotify if possible.")
        notify = None

    try:
        assert(enable_appind)
        gi.require_version('AppIndicator3', '0.1')
        from gi.repository import AppIndicator3 as appindicator
        new_appindicator = appindicator.Indicator.new
        appind_category = appindicator.IndicatorCategory.APPLICATION_STATUS
        appind_status = appindicator.IndicatorStatus.ACTIVE
    except:
        appindicator = None
        popup_trayicon_menu = lambda m, s, b, t: m.popup(None, None, gtk.StatusIcon.position_menu, s, b, t)
else:
    try:
        import pynotify as notify
        notify.init('XX-Net Notify')
        new_notification = notify.Notification
    except:
        xlog.warn("import pynotify fail, please install python-notify if possible.")
        notify = None

    try:
        assert(enable_appind)
        import appindicator
        new_appindicator = appindicator.Indicator
        appind_category = appindicator.CATEGORY_APPLICATION_STATUS
        appind_status = appindicator.STATUS_ACTIVE
    except:
        appindicator = None
        popup_trayicon_menu = lambda m, s, b, t: m.popup(None, None, gtk.status_icon_position_menu, b, t, s)


class Gtk_tray():
    notify_list = []
    def __init__(self):
        logo_filename = os.path.join(os.path.abspath(os.path.dirname(__file__)), 'web_ui', 'favicon.ico')

        if appindicator:
            self.trayicon = self.appind_trayicon(logo_filename)
            xlog.info('AppIndicator found and used.')
        else:
            self.trayicon = self.gtk_trayicon(logo_filename)
            xlog.info('Gtk.StatusIcon used.')

    def appind_trayicon(self, logo_filename):
        trayicon = new_appindicator('XX-Net', 'indicator-messages', appind_category)
        trayicon.set_status(appind_status)
        trayicon.set_attention_icon('indicator-messages-new')
        trayicon.set_icon(logo_filename)
        trayicon.set_menu(self.make_menu())
        try:  #this method does not exist when using pygtk and python2-appindicator
            trayicon.set_title('XX-Net')
        except:
            pass

        return trayicon

    def gtk_trayicon(self, logo_filename):
        trayicon = gtk.StatusIcon()
        trayicon.set_from_file(logo_filename)

        trayicon.connect('popup-menu', lambda i, b, t: popup_trayicon_menu(self.make_menu(), trayicon, b, t))
        trayicon.connect('activate', self.show_control_web)
        trayicon.set_tooltip_text('XX-Net')
        trayicon.set_title('XX-Net')
        trayicon.set_visible(True)

        return trayicon

    def make_menu(self):
        menu = gtk.Menu()
        itemlist = [(u'Config', self.on_show),
                    (u'Reset Each Module', self.on_restart_each_module),
                    (u'Quit', self.on_quit)]
        for text, callback in itemlist:
            item = gtk.MenuItem(text)
            item.connect('activate', callback)
            item.show()
            menu.append(item)
        menu.show()
        return menu

    def on_show(self, widget=None, data=None):
        self.show_control_web()


    def notify_general(self, msg="msg", title="Title", buttons={}, timeout=3600):
        if not notify:
            return False

        n = new_notification('Test', msg)
        for k in buttons:
            data = buttons[k]["data"]
            label = buttons[k]["label"]
            callback = buttons[k]["callback"]
            n.add_action(data, label, callback)
        n.set_timeout(timeout)
        n.show()
        self.notify_list.append(n)
        return True

    def show_control_web(self, widget=None, data=None):
        host_port = config.get(["modules", "launcher", "control_port"], 8085)
        webbrowser.open_new("http://127.0.0.1:%s/" % host_port)

    def on_restart_each_module(self, widget=None, data=None):
        module_init.stop_all()
        module_init.start_all_auto()

    def on_quit(self, widget, data=None):
        module_init.stop_all()
        os._exit(0)
        gtk.main_quit()

    def serve_forever(self):
        gdk.threads_enter()
        gtk.main()
        gdk.threads_leave()

sys_tray = Gtk_tray()

def main():
    sys_tray.serve_forever()

if __name__ == '__main__':
    main()

