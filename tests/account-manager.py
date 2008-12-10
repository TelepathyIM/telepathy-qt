#!/usr/bin/python
#
# A small implementation of a Telepathy AccountManager.

import sys

import dbus
from dbus.bus import NAME_FLAG_DO_NOT_QUEUE, REQUEST_NAME_REPLY_EXISTS
from dbus.mainloop.glib import DBusGMainLoop
from dbus.service import Object, method, signal
from gobject import MainLoop

TP = 'org.freedesktop.Telepathy'

AM_IFACE = TP + '.AccountManager'
AM_BUS_NAME = AM_IFACE
AM_OBJECT_PATH = '/' + AM_IFACE.replace('.', '/')

ACCOUNT_IFACE = TP + '.Account'
ACCOUNT_OBJECT_PATH_BASE = '/' + ACCOUNT_IFACE.replace('.', '/') + '/'


class AccountManager(Object):
    def __init__(self, bus=None):
        #: map from object path to Account
        self._valid_accounts = {}
        #: map from object path to Account
        self._invalid_accounts = {}

        if bus is None:
            bus = dbus.SessionBus()

        ret = bus.request_name(AM_BUS_NAME, NAME_FLAG_DO_NOT_QUEUE)
        if ret == REQUEST_NAME_REPLY_EXISTS:
            raise dbus.NameExistsException(AM_BUS_NAME)

        Object.__init__(self, bus, AM_OBJECT_PATH)

    # overridden from Object, to have the Properties in introspection
    def Introspect(self, **kwargs):
        xml = super(AccountManager, self).Introspect(**kwargs)

        before, _ = xml.rsplit('</node>', 1)

        return before + """
            <property name="Interfaces" type="as" access="read"/>
            <property name="ValidAccounts" type="o" access="read"/>
            <property name="InvalidAccounts" type="o" access="read"/>
        </node>
        """

    def _am_props(self):
        return dbus.Dictionary({
            'Interfaces': dbus.Array([], signature='s'),
            'ValidAccounts': dbus.Array(self._valid_accounts.keys(),
                signature='o'),
            'InvalidAccounts': dbus.Array(self._invalid_accounts.keys(),
                signature='o'),
        }, signature='sv')

    @method(dbus.PROPERTIES_IFACE,
            in_signature='s',
            out_signature='a{sv}')
    def GetAll(self, iface):
        if iface == AM_IFACE:
            return self._am_props()
        else:
            raise ValueError('No such interface')

    @method(dbus.PROPERTIES_IFACE,
            in_signature='s',
            out_signature='v')
    def Get(self, iface_and_prop):
        if iface_and_prop.startswith(AM_IFACE + '.'):
            props = self._am_props()
            prop = iface_and_prop[(len(AM_IFACE) + 1):]
        else:
            raise ValueError('No such interface')

        if prop in props:
            return props[prop]
        else:
            raise ValueError('No such property')

    @method(dbus.PROPERTIES_IFACE,
            in_signature='sv')
    def Set(self, iface_and_prop, value):
        raise NotImplementedError('No mutable properties')

    @signal(AM_IFACE, signature='ob')
    def AccountValidityChanged(self, path, valid):
        if valid:
            assert path in self._valid_accounts
            assert path not in self._invalid_accounts
            self._invalid_accounts[path] = self._valid_accounts.pop(path)
        else:
            assert path in self._invalid_accounts
            assert path not in self._valid_accounts
            self._valid_accounts[path] = self._invalid_accounts.pop(path)

    @signal(AM_IFACE, signature='o')
    def AccountRemoved(self, path):
        assert path in self._valid_accounts or path in self._invalid_accounts
        self._valid_accounts.pop(path, None)
        self._invalid_accounts.pop(path, None)

    @method(AM_IFACE, in_signature='sssa{sv}', out_signature='o')
    def CreateAccount(self, cm, protocol, display_name, parameters):
        raise NotImplementedError


if __name__ == '__main__':
    DBusGMainLoop(set_as_default=True)

    try:
        am = AccountManager()
    except dbus.NameExistsException:
        print >> sys.stderr, 'AccountManager already running'
        sys.exit(1)

    mainloop = MainLoop()
    mainloop.run()
