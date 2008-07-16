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
from libqt4codegen import binding_from_usage, binding_from_decl, format_docstring

class DepInfo:
    def __init__(self, el, externals, custom_lists):
        self.el = el
        name = get_by_path(el, '@name')
        array_name = get_by_path(el, '@array-name')
        self.binding = binding_from_decl(name, array_name)
        self.deps = []

        for member in get_by_path(el, 'member'):
            sig = member.getAttribute('type')
            tptype = member.getAttributeNS(NS_TP, 'type')

            if (sig, tptype) in externals:
                continue

            if tptype.endswith('[]'):
                tptype = tptype[:-2]

            binding = binding_from_usage(sig, tptype, custom_lists)

            if binding.custom_type:
                self.deps.append(binding.val)

        self.revdeps = []

class Generator(object):
    def __init__(self, opts):
        try:
            self.namespace = opts['--namespace']
            self.declfile = opts['--declfile']
            self.implfile = opts['--implfile']
            self.realinclude = opts['--realinclude']
            self.prettyinclude = opts.get('--prettyinclude' or self.realinclude)
            dom = xml.dom.minidom.parse(opts['--specxml'])
        except KeyError, k:
            assert False, 'Missing required parameter %s' % k.args[0]

        self.decls = []
        self.impls = []
        self.spec = get_by_path(dom, "spec")[0]
        self.externals = []
        self.custom_lists = {}
        self.required_custom = []
        self.required_arrays = []
        self.to_declare = []
        self.depinfos = {}

    def __call__(self):
        # Emit comment header

        self.both('/* Generated from ')
        self.both(get_descendant_text(get_by_path(self.spec, 'title')))
        version = get_by_path(self.spec, "version")

        if version:
            self.both(', version ' + get_descendant_text(version))

        self.both(' */\n')

        # Gather info on available and required types

        self.gather_externals()
        self.gather_custom_lists()
        self.gather_required()

        self.decl("""
#include <TelepathyQt4/Types>

#include <QtGlobal>

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include <QDBusArgument>
#include <QDBusMetaType>
#include <QDBusObjectPath>
#include <QDBusSignature>
#include <QDBusVariant>

/**
 * \\addtogroup typesconstants Types and constants
 *
 * Enumerated, flag, structure, list and mapping types and utility constants.
 */

/**
 * \\defgroup struct Structure types
 * \\ingroup typesconstants
 *
 * Structure types generated from the specification.
 */

/**
 * \\defgroup list List types
 * \\ingroup typesconstants
 *
 * List types generated from the specification.
 */

/**
 * \\defgroup mapping Mapping types
 * \\ingroup typesconstants
 *
 * Mapping types generated from the specification.
 */

""")
        self.impl("""
#include <%s>
""" % self.prettyinclude)
        self.both("""
namespace %s
{
""" % self.namespace)

        # Emit type definitions for types provided in the spec

        self.provide_all()

        # Emit type registration function

        self.decl("""\
/**
 * \\ingroup types
 * \\headerfile %s <%s>
 *
 * Register the types used by the library with the QtDBus type system.
 *
 * Call this function to register the types used before using anything else in
 * the library.
 */
void registerTypes();
}

""" % (self.realinclude, self.prettyinclude))
        self.impl("""\
void registerTypes()
{
    static bool registered = false;
    if (registered)
        return;
    registered = true;

""")

        # Emit Qt4 metatype declarations

        self.to_declare.sort()

        for metatype in self.to_declare:
            self.decl('Q_DECLARE_METATYPE(%s)\n' % metatype)
            self.impl('    qDBusRegisterMetaType<%s>();\n' % ((metatype.endswith('>') and metatype + ' ') or metatype))

        self.impl("""\
}
}
""")

        # Write output to files

        open(self.declfile, 'w').write(''.join(self.decls))
        open(self.implfile, 'w').write(''.join(self.impls))

    def decl(self, str):
        self.decls.append(str)

    def impl(self, str):
        self.impls.append(str)

    def both(self, str):
        self.decl(str)
        self.impl(str)

    def gather_externals(self):
        for ext in self.spec.getElementsByTagNameNS(NS_TP, 'external-type'):
            sig = ext.getAttribute('type')
            tptype = ext.getAttributeNS(NS_TP, 'type')
            externals.append((sig, tptype))

    def gather_custom_lists(self):
        structs = self.spec.getElementsByTagNameNS(NS_TP, 'struct')
        mappings = self.spec.getElementsByTagNameNS(NS_TP, 'mapping')
        exts = self.spec.getElementsByTagNameNS(NS_TP, 'external-type')

        for provider in structs + mappings + exts:
            tptype = provider.getAttribute('name').replace('_', '')
            array_name = provider.getAttribute('array-name')

            if array_name:
                self.custom_lists[tptype] = array_name.replace('_', '')

    def gather_required(self):
        members = self.spec.getElementsByTagNameNS(NS_TP, 'member')
        args = self.spec.getElementsByTagName('arg')
        props = self.spec.getElementsByTagName('property')
        tp_props = self.spec.getElementsByTagNameNS(NS_TP, 'property')

        for requirer in members + args + props + tp_props:
            sig = requirer.getAttribute('type')
            tptype = requirer.getAttributeNS(NS_TP, 'type')
            external = (sig, tptype) in self.externals
            binding = binding_from_usage(sig, tptype, self.custom_lists, external)

            if binding.custom_type and binding.val not in self.required_custom:
                self.required_custom.append(binding.val)

            if not binding.custom_type and binding.array_of and (binding.val, binding.array_of) not in self.required_arrays:
                self.required_arrays.append((binding.val, binding.array_of))

    def provide_all(self):
        self.required_arrays.sort()
        for (val, array_of) in self.required_arrays:
            real = 'QList<%s>' % array_of
            self.decl("""\
 /**
  * \\struct %s
  * \\ingroup list
  * \\headerfile %s <%s>
  * 
  * Generic list type with %s elements. Convertible with
  * %s, but needed to have a discrete type in the Qt4 type system.
  */
""" % (val, self.realinclude, self.prettyinclude, array_of, real))
            self.decl(self.faketype(val, real))
            self.to_declare.append(self.namespace + '::' + val)

        structs = self.spec.getElementsByTagNameNS(NS_TP, 'struct')
        mappings = self.spec.getElementsByTagNameNS(NS_TP, 'mapping')
        exts = self.spec.getElementsByTagNameNS(NS_TP, 'external-type')

        for deptype in structs + mappings:
            info = DepInfo(deptype, self.externals, self.custom_lists)
            self.depinfos[info.binding.val] = info

        leaves = []
        next_leaves = []

        for val, depinfo in self.depinfos.iteritems():
            leaf = True
            for dep in depinfo.deps:
                leaf = False
                assert self.depinfos.has_key(dep), 'Type %s has unresolved dependency on %s' % (val, dep)
                self.depinfos[dep].revdeps.append(val)
            if leaf:
                next_leaves.append(val)

        while leaves or next_leaves:
            if not leaves:
                leaves = next_leaves
                leaves.sort()
                next_leaves = []

            val = leaves.pop(0)
            depinfo = self.depinfos[val]
            self.output_by_depinfo(depinfo)

            for revdep in depinfo.revdeps:
                revdepinfo = self.depinfos[revdep]
                revdepinfo.deps.remove(val)

                if not revdepinfo.deps:
                    next_leaves.append(revdep)

            del self.depinfos[val]

        for provider in structs + mappings + exts:
            name = get_by_path(provider, '@name')
            array_name = get_by_path(provider, '@array-name')
            binding = binding_from_decl(name, array_name)
            self.provide(binding.val)

            if binding.array_val:
                self.provide(binding.array_val)

        assert not self.required_custom, 'These required types were not provided by the spec: ' + ', '.join(self.required_custom)

    def provide(self, type):
        if type in self.required_custom:
            self.required_custom.remove(type)

    def output_by_depinfo(self, depinfo):
        members = 0
        names = []
        bindings = []
        docstrings = []

        for member in get_by_path(depinfo.el, 'member'):
            members = members + 1

            name = member.getAttribute('name')
            if name[0].isupper():
                name = name[0].lower() + name[1:]
            names.append(name.replace('_', ''))

            sig = member.getAttribute('type')
            tptype = member.getAttributeNS(NS_TP, 'type')
            external = (sig, tptype) in self.externals
            bindings.append(binding_from_usage(sig, tptype, self.custom_lists, external))

            docstrings.append(format_docstring(member, '     * ', ('    /**', '     */')))

        if depinfo.el.localName == 'struct':
            assert members > 0, 'tp:struct %s should have some members' % depinfo.binding.val
            self.decl("""\
/**
 * \\struct %(name)s
 * \\ingroup struct
 * \\headerfile %(realinclude)s <%(prettyinclude)s>
 *
 * Structure type generated from the specification.
%(docstring)s\
 */
struct %(name)s
{
""" % {'name' : depinfo.binding.val, 'realinclude' : self.realinclude, 'prettyinclude' : self.prettyinclude, 'docstring' : format_docstring(depinfo.el)})

            for i in xrange(members):
                self.decl("""\
%s\
    %s %s;
""" % (docstrings[i], bindings[i].val, names[i]))

            self.decl("""\
};

""")

            self.both('QDBusArgument& operator<<(QDBusArgument& arg, %s val)' % depinfo.binding.inarg)
            self.decl(';\n')
            self.impl("""
{
    arg.beginStructure();
    arg << %s;
    arg.endStructure();
    return arg;
}

""" % ' << '.join(['val.' + name for name in names]))

            self.both('const QDBusArgument& operator>>(const QDBusArgument& arg, %s val)' % depinfo.binding.outarg)
            self.decl(';\n\n')
            self.impl("""
{
    arg.beginStructure();
    arg >> %s;
    arg.endStructure();
    return arg;
}

""" % ' >> '.join(['val.' + name for name in names]))
        elif depinfo.el.localName == 'mapping':
            assert members == 2, 'tp:mapping %s should have 2 members' % depinfo.binding.val
            realtype = 'QMap<%s, %s>' % (bindings[0].val, (bindings[1].val.endswith('>') and bindings[1].val + ' ') or bindings[1].val)
            self.decl("""\
/**
 * \\struct %s
 * \\ingroup mapping
 * \\headerfile %s <%s>
 *
 * Mapping type generated from the specification. Convertible with
 * %s, but needed to have a discrete type in the Qt4 type system.
%s\
 */
""" % (depinfo.binding.val, self.realinclude, self.prettyinclude, realtype, format_docstring(depinfo.el)))
            self.decl(self.faketype(depinfo.binding.val, realtype))
        else:
            assert False

        self.to_declare.append(self.namespace + '::' + depinfo.binding.val)

        if depinfo.binding.array_val:
            self.to_declare.append('%s::%s' % (self.namespace, depinfo.binding.array_val))
            self.decl("""\
/**
 * \\ingroup list
 * \\headerfile %s <%s>
 *
 * Array of %s values.
 */
typedef %s %s;

""" % (self.realinclude, self.prettyinclude, depinfo.binding.val, 'QList<%s>' % depinfo.binding.val, depinfo.binding.array_val))

    def faketype(self, fake, real):
        return """\
struct %(fake)s : public %(real)s
{
    inline %(fake)s() : %(real)s() {}
    inline %(fake)s(const %(real)s& a) : %(real)s(a) {}

    inline %(fake)s& operator=(const %(real)s& a)
    {
        *(static_cast<%(real)s*>(this)) = a;
        return *this;
    }
};

""" % {'fake' : fake, 'real' : real}

if __name__ == '__main__':
    options, argv = gnu_getopt(argv[1:], '',
            ['declfile=',
             'implfile=',
             'realinclude=',
             'prettyinclude=',
             'namespace=',
             'specxml='])

    Generator(dict(options))()

