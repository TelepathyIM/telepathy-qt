#!/usr/bin/python
#
# A small implementation of a Telepathy AccountManager.

import sys
import re

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


Connection_Status_Disconnected = dbus.UInt32(2)
Connection_Status_Reason_None_Specified = dbus.UInt32(0)
Connection_Presence_Type_Offline = dbus.UInt32(1)
Connection_Presence_Type_Available = dbus.UInt32(2)


VALID_CONNECTION_MANAGER_NAME = re.compile(r'^[A-Za-z0-9][_A-Za-z0-9]+$')
VALID_PROTOCOL_NAME = re.compile(r'^[A-Za-z0-9][-A-Za-z0-9]+$')


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
                signature='s'),
            'InvalidAccounts': dbus.Array(self._invalid_accounts.keys(),
                signature='s'),
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
            in_signature='ss',
            out_signature='v')
    def Get(self, iface, prop):
        if iface == AM_IFACE:
            props = self._am_props()
        else:
            raise ValueError('No such interface')

        if prop in props:
            return props[prop]
        else:
            raise ValueError('No such property')

    @method(dbus.PROPERTIES_IFACE,
            in_signature='ssv')
    def Set(self, iface, prop, value):
        raise NotImplementedError('No mutable properties')

    @signal(AM_IFACE, signature='ob')
    def AccountValidityChanged(self, path, valid):
        if valid:
            assert path in self._invalid_accounts
            assert path not in self._valid_accounts
            self._valid_accounts[path] = self._invalid_accounts.pop(path)
        else:
            assert path in self._valid_accounts
            assert path not in self._invalid_accounts
            self._invalid_accounts[path] = self._valid_accounts.pop(path)

    @signal(AM_IFACE, signature='o')
    def AccountRemoved(self, path):
        assert path in self._valid_accounts or path in self._invalid_accounts
        self._valid_accounts.pop(path, None)
        self._invalid_accounts.pop(path, None)

    @method(AM_IFACE, in_signature='sssa{sv}', out_signature='o')
    def CreateAccount(self, cm, protocol, display_name, parameters):

        if not VALID_CONNECTION_MANAGER_NAME.match(cm):
            raise ValueError('Invalid CM name')

        if not VALID_PROTOCOL_NAME.match(protocol):
            raise ValueError('Invalid protocol name')

        base = ACCOUNT_OBJECT_PATH_BASE + cm + '/' + protocol.replace('-', '_')

        # FIXME: This is a stupid way to generate the paths - we should
        # incorporate the display name somehow. However, it's spec-compliant
        i = 0
        while 1:
            path = '%s/Account%d' % (base, i)

            if (path not in self._valid_accounts and
                path not in self._invalid_accounts):
                account = Account(self, path,
                        '%s (account %d)' % (display_name, i), parameters)

                # put it in the wrong set and move it to the right one -
                # that's probably the simplest implementation
                if account._is_valid():
                    self._invalid_accounts[path] = account
                    self.AccountValidityChanged(path, True)
                    assert path not in self._invalid_accounts
                    assert path in self._valid_accounts
                else:
                    self._valid_accounts[path] = account
                    self.AccountValidityChanged(path, False)
                    assert path not in self._valid_accounts
                    assert path in self._invalid_accounts

                return path

            i += 1

        raise AssertionError('Not reached')

class Account(Object):
    def __init__(self, am, path, display_name, parameters):
        Object.__init__(self, am.connection, path)
        self._am = am

        self._display_name = display_name
        self._icon = u'bob.png'
        self._enabled = True
        self._nickname = u'Bob'
        self._parameters = parameters
        self._connect_automatically = True
        self._normalized_name = u'bob'
        self._automatic_presence = dbus.Struct(
                (Connection_Presence_Type_Available, 'available', ''),
                signature='uss')
        self._current_presence = dbus.Struct(
                (Connection_Presence_Type_Offline, 'offline', ''),
                signature='uss')
        self._requested_presence = dbus.Struct(
                (Connection_Presence_Type_Offline, 'offline', ''),
                signature='uss')

    def _is_valid(self):
        return True

    @method(ACCOUNT_IFACE, in_signature='a{sv}as', out_signature='')
    def UpdateParameters(self, set_, unset):
        for (key, value) in set_.iteritems():
            self._parameters[key] = value
        for key in unset:
            self._parameters.pop(key, None)

        AccountPropertyChanged({'Parameters': self._parameters})

    @signal(ACCOUNT_IFACE, signature='a{sv}')
    def AccountPropertyChanged(self, delta):
        pass

    @method(ACCOUNT_IFACE, in_signature='', out_signature='')
    def Remove(self):
        self.Removed()

    @signal(ACCOUNT_IFACE, signature='')
    def Removed(self):
        self._am.AccountRemoved(self.__dbus_object_path__)
        self.remove_from_connection()

    def _account_props(self):
        return dbus.Dictionary({
            'Interfaces': dbus.Array([], signature='s'),
            'DisplayName': self._display_name,
            'Icon': self._icon,
            'Valid': self._is_valid(),
            'Enabled': self._enabled,
            'Nickname': self._nickname,
            'Parameters': self._parameters,
            'AutomaticPresence': self._automatic_presence,
            'ConnectAutomatically': self._connect_automatically,
            'Connection': dbus.ObjectPath('/'),
            'ConnectionStatus': Connection_Status_Disconnected,
            'ConnectionStatusReason': Connection_Status_Reason_None_Specified,
            'CurrentPresence': self._current_presence,
            'RequestedPresence': self._requested_presence,
            'NormalizedName': self._normalized_name,
        }, signature='sv')

    @method(dbus.PROPERTIES_IFACE,
            in_signature='s',
            out_signature='a{sv}')
    def GetAll(self, iface):
        if iface == ACCOUNT_IFACE:
            return self._account_props()
        else:
            raise ValueError('No such interface')

    @method(dbus.PROPERTIES_IFACE,
            in_signature='ss',
            out_signature='v')
    def Get(self, iface, prop):
        if iface == ACCOUNT_IFACE:
            props = self._am_props()
        else:
            raise ValueError('No such interface')

        if prop in props:
            return props[prop]
        else:
            raise ValueError('No such property')

    @method(dbus.PROPERTIES_IFACE,
            in_signature='ssv')
    def Set(self, iface, prop, value):
        raise NotImplementedError('Not implemented')

if __name__ == '__main__':
    DBusGMainLoop(set_as_default=True)

    try:
        am = AccountManager()
    except dbus.NameExistsException:
        print >> sys.stderr, 'AccountManager already running'
        sys.exit(1)

    mainloop = MainLoop()
    mainloop.run()
