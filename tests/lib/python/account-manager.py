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
ACCOUNT_IFACE_AVATAR_IFACE = ACCOUNT_IFACE + '.Interface.Avatar'
ACCOUNT_OBJECT_PATH_BASE = '/' + ACCOUNT_IFACE.replace('.', '/') + '/'


Connection_Status_Connected = dbus.UInt32(0)
Connection_Status_Connecting = dbus.UInt32(1)
Connection_Status_Disconnected = dbus.UInt32(2)
Connection_Status_Reason_None_Specified = dbus.UInt32(0)
Connection_Status_Reason_Requested = dbus.UInt32(1)
Connection_Status_Reason_Network_Error = dbus.UInt32(2)
Connection_Presence_Type_Offline = dbus.UInt32(1)
Connection_Presence_Type_Available = dbus.UInt32(2)


VALID_CONNECTION_MANAGER_NAME = re.compile(r'^[A-Za-z0-9][_A-Za-z0-9]+$')
VALID_PROTOCOL_NAME = re.compile(r'^[A-Za-z0-9][-A-Za-z0-9]+$')

TELEPATHY_ERROR = "org.freedesktop.Telepathy.Error"
TELEPATHY_ERROR_DISCONNECTED = TELEPATHY_ERROR + ".Disconnected"
TELEPATHY_ERROR_CANCELLED = TELEPATHY_ERROR + ".Cancelled"
TELEPATHY_ERROR_NETWORK_ERROR = TELEPATHY_ERROR + ".NetworkError"

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

    def _am_props(self):
        return dbus.Dictionary({
            'Interfaces': dbus.Array([], signature='s'),
            'ValidAccounts': dbus.Array(self._valid_accounts.keys(),
                signature='o'),
            'InvalidAccounts': dbus.Array(self._invalid_accounts.keys(),
                signature='o'),
            'SupportedAccountProperties': dbus.Array([ACCOUNT_IFACE + '.Enabled'], signature='s'),
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
        print "Emitting AccountValidityChanged(%s, %s)" % (path, valid)

    @signal(AM_IFACE, signature='o')
    def AccountRemoved(self, path):
        assert path in self._valid_accounts or path in self._invalid_accounts
        self._valid_accounts.pop(path, None)
        self._invalid_accounts.pop(path, None)
        print "Emitting AccountRemoved(%s)" % path

    @method(AM_IFACE, in_signature='sssa{sv}a{sv}', out_signature='o')
    def CreateAccount(self, cm, protocol, display_name, parameters,
            properties):

        if not VALID_CONNECTION_MANAGER_NAME.match(cm):
            raise ValueError('Invalid CM name')

        if not VALID_PROTOCOL_NAME.match(protocol):
            raise ValueError('Invalid protocol name')

        if properties:
            raise ValueError('This AM does not support setting properties at'
                    'account creation')

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

        self._connection = dbus.ObjectPath('/')
        self._connection_status = Connection_Status_Disconnected
        self._connection_status_reason = Connection_Status_Reason_None_Specified
        self._connection_error = u''
        self._connection_error_details = dbus.Dictionary({}, signature='sv')
        self._service = u''
        self._display_name = display_name
        self._icon = u'bob.png'
        self._enabled = True
        self._nickname = u'Bob'
        self._parameters = parameters
        self._has_been_online = False
        self._connect_automatically = False
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
        self._avatar = dbus.Struct(
                (dbus.ByteArray(''), 'image/png'),
                signature='ays')
        self._interfaces = [ACCOUNT_IFACE_AVATAR_IFACE,]

    def _is_valid(self):
        return True

    @method(ACCOUNT_IFACE, in_signature='a{sv}as', out_signature='as')
    def UpdateParameters(self, set_, unset):
        print ("%s: entering UpdateParameters(\n    %r,\n    %r    \n)"
            % (self.__dbus_object_path__, set_, unset))
        for (key, value) in set_.iteritems():
            self._parameters[key] = value
        for key in unset:
            self._parameters.pop(key, None)
        print ("%s: UpdateParameters(...) -> success"
                % self.__dbus_object_path__)

        self.AccountPropertyChanged({'Parameters': self._parameters})

        return []

    @signal(ACCOUNT_IFACE, signature='a{sv}')
    def AccountPropertyChanged(self, delta):
        print ("%s: emitting AccountPropertyChanged(\n    %r    \n)"
                % (self.__dbus_object_path__, delta))

    @signal(ACCOUNT_IFACE_AVATAR_IFACE, signature='')
    def AvatarChanged(self):
        print ("%s: emitting AvatarChanged"
                % (self.__dbus_object_path__))

    @method(ACCOUNT_IFACE, in_signature='', out_signature='')
    def Remove(self):
        print "%s: entering Remove()" % self.__dbus_object_path__
        self.Removed()
        self.remove_from_connection()
        print "%s: Remove() -> success" % self.__dbus_object_path__

    @signal(ACCOUNT_IFACE, signature='')
    def Removed(self):
        self._am.AccountRemoved(self.__dbus_object_path__)
        print "%s: Emitting Removed()" % self.__dbus_object_path__

    def _account_props(self):
        return dbus.Dictionary({
            'Interfaces': dbus.Array(self._interfaces, signature='s'),
            'Service': self._service,
            'DisplayName': self._display_name,
            'Icon': self._icon,
            'Valid': self._is_valid(),
            'Enabled': self._enabled,
            'Nickname': self._nickname,
            'Parameters': self._parameters,
            'AutomaticPresence': self._automatic_presence,
            'CurrentPresence': self._current_presence,
            'RequestedPresence': self._requested_presence,
            'HasBeenOnline': self._has_been_online,
            'ConnectAutomatically': self._connect_automatically,
            'Connection': self._connection,
            'ConnectionStatus': self._connection_status,
            'ConnectionStatusReason': self._connection_status_reason,
            'ConnectionError': self._connection_error,
            'ConnectionErrorDetails': self._connection_error_details,
            'NormalizedName': self._normalized_name,
        }, signature='sv')

    def _account_avatar_props(self):
        return dbus.Dictionary({
            'Avatar': self._avatar
        }, signature='sv')

    @method(dbus.PROPERTIES_IFACE,
            in_signature='s',
            out_signature='a{sv}')
    def GetAll(self, iface):
        if iface == ACCOUNT_IFACE:
            return self._account_props()
        elif iface == ACCOUNT_IFACE_AVATAR_IFACE:
            return self._account_avatar_props()
        else:
            raise ValueError('No such interface')

    @method(dbus.PROPERTIES_IFACE,
            in_signature='ss',
            out_signature='v')
    def Get(self, iface, prop):
        if iface == ACCOUNT_IFACE:
            props = self._account_props()
        elif iface == ACCOUNT_IFACE_AVATAR_IFACE:
            props = self._account_avatar_props()
        else:
            raise ValueError('No such interface')

        if prop in props:
            return props[prop]
        else:
            raise ValueError('No such property')

    @method(dbus.PROPERTIES_IFACE,
            in_signature='ssv', byte_arrays=True)
    def Set(self, iface, prop, value):
        if iface == ACCOUNT_IFACE:
            props = {}
            if prop == 'Service':
                self._service = unicode(value)
            elif prop == 'DisplayName':
                self._display_name = unicode(value)
            elif prop == 'Icon':
                self._icon = unicode(value)
            elif prop == 'Enabled':
                self._enabled = bool(value)
            elif prop == 'Nickname':
                self._nickname = unicode(value)
            elif prop == 'AutomaticPresence':
                self._automatic_presence = dbus.Struct(
                        (dbus.UInt32(value[0]), unicode(value[1]),
                            unicode(value[2])),
                        signature='uss')
            elif prop == 'RequestedPresence':
                self._requested_presence = dbus.Struct(
                        (dbus.UInt32(value[0]), unicode(value[1]),
                            unicode(value[2])),
                        signature='uss')
                # pretend to put the account online, if the presence != offline
                if value[0] != Connection_Presence_Type_Offline:
                    # simulate that we are connecting/changing presence
                    props["ChangingPresence"] = True

                    if self._connection_status == Connection_Status_Disconnected:
                        self._connection_status = Connection_Status_Connecting
                        props["ConnectionStatus"] = self._connection_status

                    props[prop] = self._account_props()[prop]
                    self.AccountPropertyChanged(props)

                    props["ChangingPresence"] = False
                    if "(deny)" in self._requested_presence[2]:
                        self._connection_status = Connection_Status_Disconnected
                        self._connection_status_reason = Connection_Status_Reason_Network_Error
                        self._connection_error = TELEPATHY_ERROR_NETWORK_ERROR
                        self._connection_error_details = dbus.Dictionary(
                                {'debug-message': u'You asked for it'},
                                signature='sv')
                        self._current_presence = dbus.Struct(
                                (Connection_Presence_Type_Offline, 'offline', ''),
                                signature='uss')
                    else:
                        self._connection_status = Connection_Status_Connected
                        self._connection_status_reason = Connection_Status_Reason_None_Specified
                        self._connection_error = u''
                        self._connection_error_details = dbus.Dictionary({}, signature='sv')
                        self._current_presence = self._requested_presence
                        if self._has_been_online == False:
                            self._has_been_online = True
                            props["HasBeenOnline"] = self._has_been_online
                else:
                    self._connection_status = Connection_Status_Disconnected
                    self._connection_status_reason = Connection_Status_Reason_Requested
                    self._connection_error = TELEPATHY_ERROR_CANCELLED
                    self._connection_error_details = dbus.Dictionary(
                                {'debug-message': u'You asked for it'},
                                signature='sv')
                    self._current_presence = dbus.Struct(
                            (Connection_Presence_Type_Offline, 'offline', ''),
                            signature='uss')

                props["ConnectionStatus"] = self._connection_status
                props["ConnectionStatusReason"] = self._connection_status_reason
                props["ConnectionError"] = self._connection_error
                props["ConnectionErrorDetails"] = self._connection_error_details
                props["CurrentPresence"] = self._current_presence
            elif prop == 'ConnectAutomatically':
                self._connect_automatically = bool(value)
            elif prop == 'Connection':
                self._connection = dbus.ObjectPath(value)
            else:
                raise ValueError('Read-only or nonexistent property')

            props[prop] = self._account_props()[prop]
            self.AccountPropertyChanged(props)
        elif iface == ACCOUNT_IFACE_AVATAR_IFACE:
            if prop == 'Avatar':
                self._avatar = dbus.Struct(
                        (dbus.ByteArray(value[0]), unicode(value[1])),
                        signature='ays')
                self.AvatarChanged()
            else:
                raise ValueError('Nonexistent property')
        else:
            raise ValueError('No such interface')

if __name__ == '__main__':
    DBusGMainLoop(set_as_default=True)

    try:
        am = AccountManager()
    except dbus.NameExistsException:
        print >> sys.stderr, 'AccountManager already running'
        sys.exit(1)

    print "AccountManager running..."
    mainloop = MainLoop()
    mainloop.run()
