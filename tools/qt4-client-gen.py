#!/usr/bin/python
#
# Copyright (C) 2008 Collabora Limited <http://www.collabora.co.uk>
# Copyright (C) 2008 Nokia Corporation
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

from sys import argv
import xml.dom.minidom
from getopt import gnu_getopt

from libtpcodegen import NS_TP, get_descendant_text, get_by_path
from libqt4codegen import binding_from_usage, cxx_identifier_escape, format_docstring, gather_externals, gather_custom_lists

class Generator(object):
    def __init__(self, opts):
        try:
            self.group = opts['--group']
            self.outputfile = opts['--outputfile']
            self.namespace = opts['--namespace']
            self.realinclude = opts['--realinclude']
            self.prettyinclude = opts['--prettyinclude']
            self.typesinclude = opts['--typesinclude']
            self.mainiface = opts.get('--mainiface', None)
            ifacedom = xml.dom.minidom.parse(opts['--ifacexml'])
            specdom = xml.dom.minidom.parse(opts['--specxml'])
        except KeyError, k:
            assert False, 'Missing required parameter %s' % k.args[0]

        self.output = []
        self.ifacenodes = ifacedom.getElementsByTagName('node')
        self.spec, = get_by_path(specdom, "spec")
        self.custom_lists = gather_custom_lists(self.spec)
        self.externals = gather_externals(self.spec)
        self.mainifacename = self.mainiface and self.mainiface.replace('/', '').replace('_', '') + 'Interface'

    def __call__(self):
        # Output info header and includes
        self.o("""\
/*
 * This file contains D-Bus client proxy classes generated by qt4-client-gen.py.
 *
 * This file can be distributed under the same terms as the specification from
 * which it was generated.
 */

#include <QDBusAbstractInterface>
#include <QDBusConnection>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QString>
#include <QVariant>

#include <%s>
""" % self.typesinclude)

        # Begin namespace
        for ns in self.namespace.split('::'):
            self.o("""
namespace %s
{
""" % ns)

        # Output interface proxies
        def ifacenodecmp(x, y):
            xname, yname = x.getAttribute('name'), y.getAttribute('name')

            if xname == self.mainiface:
                return -1
            elif yname == self.mainiface:
                return 1
            else:
                return cmp(xname, yname)

        self.ifacenodes.sort(cmp=ifacenodecmp)
        for ifacenode in self.ifacenodes:
            self.do_ifacenode(ifacenode)

        # End namespace
        self.o(''.join(['}\n' for ns in self.namespace.split('::')]))

        # Write output to file
        open(self.outputfile, 'w').write(''.join(self.output))

    def do_ifacenode(self, ifacenode):
        # Extract info
        name = ifacenode.getAttribute('name').replace('/', '').replace('_', '') + 'Interface'
        iface, = get_by_path(ifacenode, 'interface')
        dbusname = iface.getAttribute('name')

        # Begin class, constructors
        self.o("""
class %(name)s : public QDBusAbstractInterface
{
    Q_OBJECT

public:
    static inline const char *staticInterfaceName()
    {
        return "%(dbusname)s";
    }

    inline %(name)s(
        const QString& serviceName,
        const QString& objectPath,
        QObject* parent = 0
    ) : QDBusAbstractInterface(serviceName, objectPath, staticInterfaceName(), QDBusConnection::sessionBus(), parent) {}

    inline %(name)s(
        const QDBusConnection& connection,
        const QString& serviceName,
        const QString& objectPath,
        QObject* parent = 0
    ) : QDBusAbstractInterface(serviceName, objectPath, staticInterfaceName(), connection, parent) {}
""" % {'name' : name,
       'dbusname' : dbusname})

        # Main interface
        mainifacename = self.mainifacename or 'QDBusAbstractInterface'

        if self.mainifacename != name:
            self.o("""
    inline %(name)s(const %(mainifacename)s& mainInterface)
        : QDBusAbstractInterface(mainInterface.service(), mainInterface.path(), staticInterfaceName(), mainInterface.connection(), mainInterface.parent()) {}

    inline %(name)s(
        const %(mainifacename)s& mainInterface,
        QObject* parent
    ) : QDBusAbstractInterface(mainInterface.service(), mainInterface.path(), staticInterfaceName(), mainInterface.connection(), parent) {}
""" % {'name' : name,
       'mainifacename' : mainifacename})

        # Properties
        for prop in get_by_path(iface, 'property'):
            # Skip tp:properties
            if not prop.namespaceURI:
                self.do_prop(prop)

        # Close class
        self.o("""\
};
""")

    def do_prop(self, prop):
        propname = prop.getAttribute('name')
        access = prop.getAttribute('access')
        gettername = cxx_identifier_escape(propname[0].lower() + propname[1:])
        settername = None

        sig = prop.getAttribute('type')
        tptype = prop.getAttributeNS(NS_TP, 'type')
        binding = binding_from_usage(sig, tptype, self.custom_lists, (sig, tptype) in self.externals)

        if 'write' in access:
            settername = cxx_escape('set' + propname[0].upper() + propname[1:])

        self.o("""
    Q_PROPERTY(%(val)s %(propname-escaped)s READ %(gettername)s%(maybesettername)s)

    inline %(val)s %(gettername)s() const
    {
        return %(getter-return)s;
    }
""" % {'val' : binding.val,
       'propname-escaped' : cxx_identifier_escape(propname),
       'propname' : propname,
       'gettername' : gettername,
       'maybesettername' : settername and (' WRITE ' + settername) or '',
       'getter-return' : 'read' in access and ('qvariant_cast<%s>(internalPropGet("%s"))' % (binding.val, propname)) or binding.val + '()'})

        if settername:
            self.o("""
    inline void %s(%s newValue)
    {
        internalPropSet("%s", QVariant::fromValue(newValue));
    }
""" % (settername, binding.inarg, propname))

    def o(self, str):
        self.output.append(str)


if __name__ == '__main__':
    options, argv = gnu_getopt(argv[1:], '',
            ['group=',
             'namespace=',
             'outputfile=',
             'ifacexml=',
             'specxml=',
             'realinclude=',
             'prettyinclude=',
             'typesinclude=',
             'mainiface='])

    Generator(dict(options))()
