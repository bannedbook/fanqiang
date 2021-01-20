#!/usr/bin/env python
# coding:utf-8

import os
import sys

current_path = os.path.dirname(os.path.abspath(__file__))
helper_path = os.path.join(current_path, os.pardir, os.pardir, os.pardir, 'data', 'launcher', 'helper')

if __name__ == "__main__":
    python_path = os.path.abspath( os.path.join(current_path, os.pardir, 'python27', '1.0'))
    noarch_lib = os.path.abspath( os.path.join(python_path, 'lib', 'noarch'))
    sys.path.append(noarch_lib)
    osx_lib = os.path.join(python_path, 'lib', 'darwin')
    sys.path.append(osx_lib)
    extra_lib = "/System/Library/Frameworks/Python.framework/Versions/2.7/Extras/lib/python/PyObjC"
    sys.path.append(extra_lib)

import config
import module_init
import subprocess
import webbrowser

from xlog import getLogger
xlog = getLogger("launcher")

import AppKit
import SystemConfiguration
from PyObjCTools import AppHelper

class MacTrayObject(AppKit.NSObject):
    def __init__(self):
        pass

    def applicationDidFinishLaunching_(self, notification):
        setupHelper()
        loadConfig()
        self.setupUI()
        self.registerObserver()

    def setupUI(self):
        self.statusbar = AppKit.NSStatusBar.systemStatusBar()
        self.statusitem = self.statusbar.statusItemWithLength_(AppKit.NSSquareStatusItemLength) #NSSquareStatusItemLength #NSVariableStatusItemLength

        # Set initial image icon
        icon_path = os.path.join(current_path, "web_ui", "favicon-mac.ico")
        image = AppKit.NSImage.alloc().initByReferencingFile_(icon_path.decode('utf-8'))
        image.setScalesWhenResized_(True)
        image.setSize_((20, 20))
        self.statusitem.setImage_(image)

        # Let it highlight upon clicking
        self.statusitem.setHighlightMode_(1)
        self.statusitem.setToolTip_("XX-Net")

        # Get current selected mode
        proxyState = getProxyState(currentService)

        # Build a very simple menu
        self.menu = AppKit.NSMenu.alloc().initWithTitle_('XX-Net')

        menuitem = AppKit.NSMenuItem.alloc().initWithTitle_action_keyEquivalent_('Config', 'config:', '')
        self.menu.addItem_(menuitem)

        menuitem = AppKit.NSMenuItem.alloc().initWithTitle_action_keyEquivalent_(getCurrentServiceMenuItemTitle(), None, '')
        self.menu.addItem_(menuitem)
        self.currentServiceMenuItem = menuitem

        menuitem = AppKit.NSMenuItem.alloc().initWithTitle_action_keyEquivalent_('Enable Auto GAEProxy', 'enableAutoProxy:', '')
        if proxyState == 'pac':
            menuitem.setState_(AppKit.NSOnState)
        self.menu.addItem_(menuitem)
        self.autoGaeProxyMenuItem = menuitem

        menuitem = AppKit.NSMenuItem.alloc().initWithTitle_action_keyEquivalent_('Enable Global GAEProxy', 'enableGlobalProxy:', '')
        if proxyState == 'gae':
            menuitem.setState_(AppKit.NSOnState)
        self.menu.addItem_(menuitem)
        self.globalGaeProxyMenuItem = menuitem

        menuitem = AppKit.NSMenuItem.alloc().initWithTitle_action_keyEquivalent_('Enable Global X-Tunnel', 'enableGlobalXTunnel:', '')
        if proxyState == 'x_tunnel':
            menuitem.setState_(AppKit.NSOnState)
        self.menu.addItem_(menuitem)
        self.globalXTunnelMenuItem = menuitem

        menuitem = AppKit.NSMenuItem.alloc().initWithTitle_action_keyEquivalent_('Enable Global Smart-Router', 'enableGlobalSmartRouter:', '')
        if proxyState == 'smart_router':
            menuitem.setState_(AppKit.NSOnState)
        self.menu.addItem_(menuitem)
        self.globalSmartRouterMenuItem = menuitem

        menuitem = AppKit.NSMenuItem.alloc().initWithTitle_action_keyEquivalent_('Disable GAEProxy', 'disableProxy:', '')
        if proxyState == 'disable':
            menuitem.setState_(AppKit.NSOnState)
        self.menu.addItem_(menuitem)
        self.disableGaeProxyMenuItem = menuitem

        # Reset Menu Item
        menuitem = AppKit.NSMenuItem.alloc().initWithTitle_action_keyEquivalent_('Reset Each Module', 'restartEachModule:', '')
        self.menu.addItem_(menuitem)
        # Default event
        menuitem = AppKit.NSMenuItem.alloc().initWithTitle_action_keyEquivalent_('Quit', 'windowWillClose:', '')
        self.menu.addItem_(menuitem)
        # Bind it to the status item
        self.statusitem.setMenu_(self.menu)

        # Hide dock icon
        AppKit.NSApp.setActivationPolicy_(AppKit.NSApplicationActivationPolicyProhibited)

    def updateStatusBarMenu(self):
        self.currentServiceMenuItem.setTitle_(getCurrentServiceMenuItemTitle())

        # Remove Tick before All Menu Items
        self.autoGaeProxyMenuItem.setState_(AppKit.NSOffState)
        self.globalGaeProxyMenuItem.setState_(AppKit.NSOffState)
        self.globalXTunnelMenuItem.setState_(AppKit.NSOffState)
        self.globalSmartRouterMenuItem.setState_(AppKit.NSOffState)
        self.disableGaeProxyMenuItem.setState_(AppKit.NSOffState)

        # Get current selected mode
        proxyState = getProxyState(currentService)

        # Update Tick before Menu Item
        if proxyState == 'pac':
            self.autoGaeProxyMenuItem.setState_(AppKit.NSOnState)
        elif proxyState == 'gae':
            self.globalGaeProxyMenuItem.setState_(AppKit.NSOnState)
        elif proxyState == 'x_tunnel':
            self.globalXTunnelMenuItem.setState_(AppKit.NSOnState)
        elif proxyState == 'smart_router':
            self.globalSmartRouterMenuItem.setState_(AppKit.NSOnState)
        elif proxyState == 'disable':
            self.disableGaeProxyMenuItem.setState_(AppKit.NSOnState)

        # Trigger autovalidation
        self.menu.update()

    def validateMenuItem_(self, menuItem):
        return currentService or (menuItem != self.autoGaeProxyMenuItem and
                                  menuItem != self.globalGaeProxyMenuItem and
                                  menuItem != self.globalXTunnelMenuItem and
                                  menuItem != self.globalSmartRouterMenuItem and
                                  menuItem != self.disableGaeProxyMenuItem)

    def presentAlert_withTitle_(self, msg, title):
        self.performSelectorOnMainThread_withObject_waitUntilDone_('presentAlertWithInfo:', [title, msg], True)
        return self.alertReturn

    def presentAlertWithInfo_(self, info):
        alert = AppKit.NSAlert.alloc().init()
        alert.setMessageText_(info[0])
        alert.setInformativeText_(info[1])
        alert.addButtonWithTitle_("OK")
        alert.addButtonWithTitle_("Cancel")
        self.alertReturn = alert.runModal() == AppKit.NSAlertFirstButtonReturn

    def registerObserver(self):
        nc = AppKit.NSWorkspace.sharedWorkspace().notificationCenter()
        nc.addObserver_selector_name_object_(self, 'windowWillClose:', AppKit.NSWorkspaceWillPowerOffNotification, None)

    def windowWillClose_(self, notification):
        executeResult = subprocess.check_output(['networksetup', '-listallnetworkservices'])
        services = executeResult.split('\n')
        services = filter(lambda service : service and service.find('*') == -1 and getProxyState(service) != 'disable', services) # Remove disabled services and empty lines

        if len(services) > 0:
            try:
                map(helperDisableAutoProxy, services)
                map(helperDisableGlobalProxy, services)
            except:
                disableAutoProxyCommand   = ';'.join(map(getDisableAutoProxyCommand, services))
                disableGlobalProxyCommand = ';'.join(map(getDisableGlobalProxyCommand, services))
                executeCommand            = 'do shell script "%s;%s" with administrator privileges' % (disableAutoProxyCommand, disableGlobalProxyCommand)

                xlog.info("try disable proxy:%s", executeCommand)
                subprocess.call(['osascript', '-e', executeCommand])

        module_init.stop_all()
        os._exit(0)
        AppKit.NSApp.terminate_(self)

    def config_(self, notification):
        host_port = config.get(["modules", "launcher", "control_port"], 8085)
        webbrowser.open_new("http://127.0.0.1:%s/" % host_port)

    def restartEachModule_(self, _):
        module_init.stop_all()
        module_init.start_all_auto()

    def enableAutoProxy_(self, _):
        try:
            helperDisableGlobalProxy(currentService)
            helperEnableAutoProxy(currentService)
        except:
            disableGlobalProxyCommand = getDisableGlobalProxyCommand(currentService)
            enableAutoProxyCommand    = getEnableAutoProxyCommand(currentService)
            executeCommand            = 'do shell script "%s;%s" with administrator privileges' % (disableGlobalProxyCommand, enableAutoProxyCommand)

            xlog.info("try enable auto proxy:%s", executeCommand)
            subprocess.call(['osascript', '-e', executeCommand])
        config.set(["modules", "launcher", "proxy"], "pac")
        config.save()
        self.updateStatusBarMenu()

    def enableGlobalProxy_(self, _):
        try:
            helperDisableAutoProxy(currentService)
            helperEnableGlobalProxy(currentService)
        except:
            disableAutoProxyCommand   = getDisableAutoProxyCommand(currentService)
            enableGlobalProxyCommand  = getEnableGlobalProxyCommand(currentService)
            executeCommand            = 'do shell script "%s;%s" with administrator privileges' % (disableAutoProxyCommand, enableGlobalProxyCommand)

            xlog.info("try enable global proxy:%s", executeCommand)
            subprocess.call(['osascript', '-e', executeCommand])
        config.set(["modules", "launcher", "proxy"], "gae")
        config.save()
        self.updateStatusBarMenu()

    def enableGlobalXTunnel_(self, _):
        try:
            helperDisableAutoProxy(currentService)
            helperEnableXTunnelProxy(currentService)
        except:
            disableAutoProxyCommand   = getDisableAutoProxyCommand(currentService)
            enableXTunnelProxyCommand  = getEnableXTunnelProxyCommand(currentService)
            executeCommand            = 'do shell script "%s;%s" with administrator privileges' % (disableAutoProxyCommand, enableXTunnelProxyCommand)

            xlog.info("try enable global x-tunnel proxy:%s", executeCommand)
            subprocess.call(['osascript', '-e', executeCommand])
        config.set(["modules", "launcher", "proxy"], "x_tunnel")
        config.save()
        self.updateStatusBarMenu()

    def enableGlobalSmartRouter_(self, _):
        try:
            helperDisableAutoProxy(currentService)
            helperEnableSmartRouterProxy(currentService)
        except:
            disableAutoProxyCommand   = getDisableAutoProxyCommand(currentService)
            enableSmartRouterCommand  = getEnableSmartRouterProxyCommand(currentService)
            executeCommand            = 'do shell script "%s;%s" with administrator privileges' % (disableAutoProxyCommand, enableSmartRouterCommand)

            xlog.info("try enable global smart-router proxy:%s", executeCommand)
            subprocess.call(['osascript', '-e', executeCommand])
        config.set(["modules", "launcher", "proxy"], "smart_router")
        config.save()
        self.updateStatusBarMenu()

    def disableProxy_(self, _):
        try:
            helperDisableAutoProxy(currentService)
            helperDisableGlobalProxy(currentService)
        except:
            disableAutoProxyCommand   = getDisableAutoProxyCommand(currentService)
            disableGlobalProxyCommand = getDisableGlobalProxyCommand(currentService)
            executeCommand            = 'do shell script "%s;%s" with administrator privileges' % (disableAutoProxyCommand, disableGlobalProxyCommand)
            
            xlog.info("try disable proxy:%s", executeCommand)
            subprocess.call(['osascript', '-e', executeCommand])
        config.set(["modules", "launcher", "proxy"], "disable")
        config.save()
        self.updateStatusBarMenu()


def setupHelper():
    try:
        with open(os.devnull) as devnull:
            subprocess.check_call(helper_path, stderr=devnull)
    except:
        rmCommand      = "rm \\\"%s\\\"" % helper_path
        cpCommand      = "cp \\\"%s\\\" \\\"%s\\\"" % (os.path.join(current_path, 'mac_helper'), helper_path)
        chownCommand   = "chown root \\\"%s\\\"" % helper_path
        chmodCommand   = "chmod 4755 \\\"%s\\\"" % helper_path
        executeCommand = 'do shell script "%s;%s;%s;%s" with administrator privileges' % (rmCommand, cpCommand, chownCommand, chmodCommand)

        xlog.info("try setup helper:%s", executeCommand)
        subprocess.call(['osascript', '-e', executeCommand])

def getCurrentServiceMenuItemTitle():
    if currentService:
        return 'Connection: %s' % currentService
    else:
        return 'Connection: None'

def getProxyState(service):
    if not service:
        return

    # Check if auto proxy is enabled
    executeResult = subprocess.check_output(['networksetup', '-getautoproxyurl', service])
    if ( executeResult.find('http://127.0.0.1:8086/proxy.pac\nEnabled: Yes') != -1 ):
        return "pac"

    # Check if global proxy is enabled
    executeResult = subprocess.check_output(['networksetup', '-getwebproxy', service])
    if ( executeResult.find('Enabled: Yes\nServer: 127.0.0.1\nPort: 8087') != -1 ):
        return "gae"

    # Check if global proxy is enabled
    if ( executeResult.find('Enabled: Yes\nServer: 127.0.0.1\nPort: 1080') != -1 ):
        return "x_tunnel"

    if ( executeResult.find('Enabled: Yes\nServer: 127.0.0.1\nPort: 8086') != -1 ):
        return "smart_router"

    return "disable"

# Generate commands for Apple Script
def getEnableAutoProxyCommand(service):
    return "networksetup -setautoproxyurl \\\"%s\\\" \\\"http://127.0.0.1:8086/proxy.pac\\\"" % service

def getDisableAutoProxyCommand(service):
    return "networksetup -setautoproxystate \\\"%s\\\" off" % service

def getEnableGlobalProxyCommand(service):
    enableHttpProxyCommand   = "networksetup -setwebproxy \\\"%s\\\" 127.0.0.1 8087" % service
    enableHttpsProxyCommand  = "networksetup -setsecurewebproxy \\\"%s\\\" 127.0.0.1 8087" % service
    return "%s;%s" % (enableHttpProxyCommand, enableHttpsProxyCommand)

def getEnableXTunnelProxyCommand(service):
    enableHttpProxyCommand   = "networksetup -setwebproxy \\\"%s\\\" 127.0.0.1 1080" % service
    enableHttpsProxyCommand  = "networksetup -setsecurewebproxy \\\"%s\\\" 127.0.0.1 1080" % service
    return "%s;%s" % (enableHttpProxyCommand, enableHttpsProxyCommand)

def getEnableSmartRouterProxyCommand(service):
    enableHttpProxyCommand   = "networksetup -setwebproxy \\\"%s\\\" 127.0.0.1 8086" % service
    enableHttpsProxyCommand  = "networksetup -setsecurewebproxy \\\"%s\\\" 127.0.0.1 8086" % service
    return "%s;%s" % (enableHttpProxyCommand, enableHttpsProxyCommand)

def getDisableGlobalProxyCommand(service):
    disableHttpProxyCommand  = "networksetup -setwebproxystate \\\"%s\\\" off" % service
    disableHttpsProxyCommand = "networksetup -setsecurewebproxystate \\\"%s\\\" off" % service
    return "%s;%s" % (disableHttpProxyCommand, disableHttpsProxyCommand)

# Call helper
def helperEnableAutoProxy(service):
    subprocess.check_call([helper_path, 'enableauto', service, 'http://127.0.0.1:8086/proxy.pac'])

def helperDisableAutoProxy(service):
    subprocess.check_call([helper_path, 'disableauto', service])

def helperEnableGlobalProxy(service):
    subprocess.check_call([helper_path, 'enablehttp', service, '127.0.0.1', '8087'])
    subprocess.check_call([helper_path, 'enablehttps', service, '127.0.0.1', '8087'])

def helperEnableXTunnelProxy(service):
    subprocess.check_call([helper_path, 'enablehttp', service, '127.0.0.1', '1080'])
    subprocess.check_call([helper_path, 'enablehttps', service, '127.0.0.1', '1080'])

def helperEnableSmartRouterProxy(service):
    subprocess.check_call([helper_path, 'enablehttp', service, '127.0.0.1', '8086'])
    subprocess.check_call([helper_path, 'enablehttps', service, '127.0.0.1', '8086'])

def helperDisableGlobalProxy(service):
    subprocess.check_call([helper_path, 'disablehttp', service])
    subprocess.check_call([helper_path, 'disablehttps', service])

def loadConfig():
    if not currentService:
        return
    proxy_setting = config.get(["modules", "launcher", "proxy"], "smart_router")
    if getProxyState(currentService) == proxy_setting:
        return
    try:
        if proxy_setting == "pac":
            helperDisableGlobalProxy(currentService)
            helperEnableAutoProxy(currentService)
        elif proxy_setting == "gae":
            helperDisableAutoProxy(currentService)
            helperEnableGlobalProxy(currentService)
        elif proxy_setting == "x_tunnel":
            helperDisableAutoProxy(currentService)
            helperEnableXTunnelProxy(currentService)
        elif proxy_setting == "smart_router":
            helperDisableAutoProxy(currentService)
            helperEnableSmartRouterProxy(currentService)
        elif proxy_setting == "disable":
            helperDisableAutoProxy(currentService)
            helperDisableGlobalProxy(currentService)
        else:
            xlog.warn("proxy_setting:%r", proxy_setting)
    except:
        xlog.warn("helper failed, please manually reset proxy settings after switching connection")


sys_tray = MacTrayObject.alloc().init()
currentService = None

def fetchCurrentService(protocol):
    global currentService
    status = SystemConfiguration.SCDynamicStoreCopyValue(None, "State:/Network/Global/" + protocol)
    if not status:
        currentService = None
        return
    serviceID = status['PrimaryService']
    service = SystemConfiguration.SCDynamicStoreCopyValue(None, "Setup:/Network/Service/" + serviceID)
    if not service:
        currentService = None
        return
    currentService = service['UserDefinedName']

@AppKit.objc.callbackFor(AppKit.CFNotificationCenterAddObserver)
def networkChanged(center, observer, name, object, userInfo):
    fetchCurrentService('IPv4')
    loadConfig()
    sys_tray.updateStatusBarMenu()

# Note: the following code can't run in class
def serve_forever():
    app = AppKit.NSApplication.sharedApplication()
    app.setDelegate_(sys_tray)

    # Listen for network change
    nc = AppKit.CFNotificationCenterGetDarwinNotifyCenter()
    AppKit.CFNotificationCenterAddObserver(nc, None, networkChanged, "com.apple.system.config.network_change", None, AppKit.CFNotificationSuspensionBehaviorDeliverImmediately)

    fetchCurrentService('IPv4')
    AppHelper.runEventLoop()

def main():
    serve_forever()

if __name__ == '__main__':
    main()
