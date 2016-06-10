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

import sys
import xml.dom.minidom
from getopt import gnu_getopt

from libtpcodegen import NS_TP, get_descendant_text, get_by_path
from libqtcodegen import binding_from_usage, binding_from_decl, extract_arg_or_member_info, format_docstring, gather_externals, gather_custom_lists, get_qt_name, get_headerfile_cmd, RefRegistry

class BrokenSpecException(Exception):
    pass

class MissingTypes(BrokenSpecException):
    def __init__(self, types):
        super(MissingTypes, self).__init__(self)
        self.types = types

    def __str__(self):
        typelist = ''.join(['    %s' % t for t in self.types])
        return "The following types were used, but not provided by the spec " \
            "or by <tp:external-type/> declarations in all.xml:\n%s" % typelist

class UnresolvedDependency(BrokenSpecException):
    def __init__(self, child, parent):
        super(UnresolvedDependency, self).__init__(self)
        self.child = child
        self.parent = parent

    def __str__(self):
        return 'Type %s has unresolved dependency on %s' % (
            self.child, self.parent)

class EmptyStruct(BrokenSpecException):
    def __init__(self, struct_name):
        super(EmptyStruct, self).__init__(self)
        self.struct_name = struct_name

    def __str__(self):
        return 'tp:struct %s should have some members' % self.struct_name

class MalformedMapping(BrokenSpecException):
    def __init__(self, mapping_name, members):
        super(MalformedMapping, self).__init__(self)
        self.mapping_name = mapping_name
        self.members = members

    def __str__(self):
        return 'tp:mapping %s should have 2 members, not %u' % (
            self.mapping_name, self.members)

class WTF(BrokenSpecException):
    def __init__(self, element_name):
        super(BrokenSpecException, self).__init__(self)
        self.element_name = element_name

    def __str__(self):
        return 'What the hell is a tp:%s?' % self.element_name


class DepInfo:
    def __init__(self, el, externals, custom_lists):
        self.el = el
        name = get_by_path(el, '@name')
        array_name = get_by_path(el, '@array-name')
        array_depth = get_by_path(el, '@array-depth')
        if array_depth:
            array_depth = int(array_depth)
        else:
            array_depth = None
        self.binding = binding_from_decl(name, array_name, array_depth)
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
            self.prettyinclude = opts.get('--prettyinclude', self.realinclude)
            self.extraincludes = opts.get('--extraincludes', None)
            self.must_define = opts.get('--must-define', None)
            self.visibility = opts.get('--visibility', '')
            dom = xml.dom.minidom.parse(opts['--specxml'])
        except KeyError, k:
            assert False, 'Missing required parameter %s' % k.args[0]

        self.decls = []
        self.impls = []
        self.spec = get_by_path(dom, "spec")[0]
        self.externals = gather_externals(self.spec)
        self.custom_lists = gather_custom_lists(self.spec, self.namespace)
        self.required_custom = []
        self.required_arrays = []
        self.to_declare = []
        self.depinfos = {}
        self.refs = RefRegistry(self.spec)

    def __call__(self):
        # Emit comment header

        self.both('/* Generated from ')
        self.both(get_descendant_text(get_by_path(self.spec, 'title')))
        version = get_by_path(self.spec, "version")

        if version:
            self.both(', version ' + get_descendant_text(version))

        self.both(' */\n')

        # Gather info on available and required types

        self.gather_required()

        if self.must_define:
            self.decl('\n')
            self.decl('#ifndef %s\n' % self.must_define)
            self.decl('#error %s\n' % self.must_define)
            self.decl('#endif')

        self.decl('\n')

        if self.extraincludes:
            for include in self.extraincludes.split(','):
                self.decl('#include %s\n' % include)

        self.decl("""
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

#include <TelepathyQt/Global>

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

        if self.must_define:
            self.impl("""
#define %s""" % self.must_define)

        self.impl("""
#include "%s"
""" % self.realinclude)

        self.both("""
namespace %s
{
""" % self.namespace)

        # Emit type definitions for types provided in the spec

        self.provide_all()

        # Emit type registration function

        self.decl("""
} // namespace %s

""" % self.namespace)

        self.impl("""\
TP_QT_NO_EXPORT void _registerTypes()
{
    static bool registered = false;
    if (registered)
        return;
    registered = true;

""")

        # Emit Qt metatype declarations

        self.to_declare.sort()

        for metatype in self.to_declare:
            self.decl('Q_DECLARE_METATYPE(%s)\n' % metatype)
            self.impl('    qDBusRegisterMetaType<%s>();\n' % ((metatype.endswith('>') and metatype + ' ') or metatype))

        self.impl("""\
}

} // namespace %s
""" % self.namespace)

        # Write output to files

        open(self.declfile, 'w').write(''.join(self.decls).encode("utf-8"))
        open(self.implfile, 'w').write(''.join(self.impls).encode("utf-8"))

    def decl(self, str):
        self.decls.append(str)

    def impl(self, str):
        self.impls.append(str)

    def both(self, str):
        self.decl(str)
        self.impl(str)

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
%s\
 *
 * Generic list type with %s elements. Convertible with
 * %s, but needed to have a discrete type in the Qt type system.
 */
""" % (val, get_headerfile_cmd(self.realinclude, self.prettyinclude), array_of, real))
            self.decl(self.faketype(val, real))
            self.to_declare.append(self.namespace + '::' + val)

            self.both('%s QDBusArgument& operator<<(QDBusArgument& arg, const %s &list)' %
                    (self.visibility, val))
            self.decl(';\n')
            self.impl("""
{
    int id = qMetaTypeId<%s>();
    arg.beginArray(id);
    for (int i = 0; i < list.count(); ++i) {
        arg << list.at(i);
    }
    arg.endArray();
    return arg;
}

""" % (array_of))

            self.both('%s const QDBusArgument& operator>>(const QDBusArgument& arg, %s &list)' %
                    (self.visibility, val))
            self.decl(';\n\n')
            self.impl("""
{
    arg.beginArray();
    list.clear();
    while (!arg.atEnd()) {
        %s item;
        arg >> item;
        list.append(item);
    }
    arg.endArray();
    return arg;
}

""" % (array_of))

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
                if not self.depinfos.has_key(dep):
                    raise UnresolvedDependency(val, dep)

                leaf = False
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
            array_depth = get_by_path(provider, '@array-depth')
            if array_depth:
                array_depth = int(array_depth)
            else:
                array_depth = None
            sig = provider.getAttribute('type')
            tptype = provider.getAttribute('name')
            external = (sig, tptype) in self.externals
            binding = binding_from_decl(name, array_name, array_depth, external)
            self.provide(binding.val)

            if binding.array_val:
                self.provide(binding.array_val)

            d = binding.array_depth
            while d > 1:
                d -= 1
                self.provide(binding.array_val + ('List' * d))

        if self.required_custom:
            raise MissingTypes(self.required_custom)

    def provide(self, type):
        if type in self.required_custom:
            self.required_custom.remove(type)

    def output_by_depinfo(self, depinfo):
        names, docstrings, bindings = extract_arg_or_member_info(get_by_path(depinfo.el, 'member'), self.custom_lists, self.externals, None, self.refs, '     * ', ('    /**', '     */'))
        members = len(names)

        if depinfo.el.localName == 'struct':
            if members == 0:
                raise EmptyStruct(depinfo.binding.val)

            self.decl("""\
/**
 * \\struct %(name)s
 * \\ingroup struct
%(headercmd)s\
 *
 * Structure type generated from the specification.
%(docstring)s\
 */
struct %(visibility)s %(name)s
{
""" % {
        'name' : depinfo.binding.val,
        'headercmd': get_headerfile_cmd(self.realinclude, self.prettyinclude),
        'docstring' : format_docstring(depinfo.el, self.refs),
        'visibility': self.visibility,
        })

            for i in xrange(members):
                self.decl("""\
%s\
    %s %s;
""" % (docstrings[i], bindings[i].val, names[i]))

            self.decl("""\
};

""")

            self.both('%s bool operator==(%s v1, %s v2)' %
                    (self.visibility,
                     depinfo.binding.inarg,
                     depinfo.binding.inarg))
            self.decl(';\n')
            self.impl("""
{""")
            if (bindings[0].val != 'QDBusVariant'):
                self.impl("""
    return ((v1.%s == v2.%s)""" % (names[0], names[0]))
            else:
                self.impl("""
    return ((v1.%s.variant() == v2.%s.variant())""" % (names[0], names[0]))
            for i in xrange(1, members):
                if (bindings[i].val != 'QDBusVariant'):
                    self.impl("""
            && (v1.%s == v2.%s)""" % (names[i], names[i]))
                else:
                    self.impl("""
            && (v1.%s.variant() == v2.%s.variant())""" % (names[i], names[i]))
            self.impl("""
            );
}

""")

            self.decl('inline bool operator!=(%s v1, %s v2)' %
                      (depinfo.binding.inarg, depinfo.binding.inarg))
            self.decl("""
{
    return !operator==(v1, v2);
}
""")

            self.both('%s QDBusArgument& operator<<(QDBusArgument& arg, %s val)' %
                    (self.visibility, depinfo.binding.inarg))
            self.decl(';\n')
            self.impl("""
{
    arg.beginStructure();
    arg << %s;
    arg.endStructure();
    return arg;
}

""" % ' << '.join(['val.' + name for name in names]))

            self.both('%s const QDBusArgument& operator>>(const QDBusArgument& arg, %s val)' %
                    (self.visibility, depinfo.binding.outarg))
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
            if members != 2:
                raise MalformedMapping(depinfo.binding.val, members)

            realtype = 'QMap<%s, %s>' % (bindings[0].val, (bindings[1].val.endswith('>') and bindings[1].val + ' ') or bindings[1].val)
            self.decl("""\
/**
 * \\struct %s
 * \\ingroup mapping
%s\
 *
 * Mapping type generated from the specification. Convertible with
 * %s, but needed to have a discrete type in the Qt type system.
%s\
 */
""" % (depinfo.binding.val, get_headerfile_cmd(self.realinclude, self.prettyinclude), realtype, format_docstring(depinfo.el, self.refs)))
            self.decl(self.faketype(depinfo.binding.val, realtype))
        else:
            raise WTF(depinfo.el.localName)

        self.to_declare.append(self.namespace + '::' + depinfo.binding.val)

        if depinfo.binding.array_val:
            self.to_declare.append('%s::%s' % (self.namespace, depinfo.binding.array_val))
            self.decl("""\
/**
 * \\ingroup list
%s\
 *
 * Array of %s values.
 */
typedef %s %s;

""" % (get_headerfile_cmd(self.realinclude, self.prettyinclude), depinfo.binding.val, 'QList<%s>' % depinfo.binding.val, depinfo.binding.array_val))

        i = depinfo.binding.array_depth
        while i > 1:
            i -= 1
            self.to_declare.append('%s::%s%s' % (self.namespace, depinfo.binding.array_val, ('List' * i)))
            list_of = depinfo.binding.array_val + ('List' * (i-1))
            self.decl("""\
/**
 * \\ingroup list
%s\
 *
 * Array of %s values.
 */
typedef QList<%s> %sList;

""" % (get_headerfile_cmd(self.realinclude, self.prettyinclude), list_of, list_of, list_of))

    def faketype(self, fake, real):
        return """\
struct %(visibility)s %(fake)s : public %(real)s
{
    %(fake)s() : %(real)s() {}
    %(fake)s(const %(real)s& a) : %(real)s(a) {}

    %(fake)s& operator=(const %(real)s& a)
    {
        *(static_cast<%(real)s*>(this)) = a;
        return *this;
    }
};

""" % {'fake' : fake, 'real' : real, 'visibility': self.visibility}

if __name__ == '__main__':
    options, argv = gnu_getopt(sys.argv[1:], '',
            ['declfile=',
             'implfile=',
             'realinclude=',
             'prettyinclude=',
             'extraincludes=',
             'must-define=',
             'namespace=',
             'specxml=',
             'visibility=',
             ])

    try:
        Generator(dict(options))()
    except BrokenSpecException as e:
        print >> sys.stderr, 'Your spec is broken, dear developer! %s' % e
        sys.exit(42)
