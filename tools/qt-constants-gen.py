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

from sys import argv, stdout, stderr
import codecs
import xml.dom.minidom
from getopt import gnu_getopt

from libtpcodegen import NS_TP, get_descendant_text, get_by_path
from libqtcodegen import format_docstring, RefRegistry

class Generator(object):
    def __init__(self, opts):
        try:
            self.namespace = opts['--namespace']
            self.must_define = opts.get('--must-define', None)
            dom = xml.dom.minidom.parse(opts['--specxml'])
        except KeyError, k:
            assert False, 'Missing required parameter %s' % k.args[0]

        self.define_prefix = None
        if '--define-prefix' in opts:
            self.define_prefix = opts['--define-prefix']

        self.old_prefix = None
        if '--str-constant-prefix' in opts:
            self.old_prefix = opts['--str-constant-prefix']

        self.spec = get_by_path(dom, "spec")[0]
        self.out = codecs.getwriter('utf-8')(stdout)
        self.refs = RefRegistry(self.spec)

    def h(self, code):
        self.out.write(code)

    def __call__(self):
        # Header
        self.h('/* Generated from ')
        self.h(get_descendant_text(get_by_path(self.spec, 'title')))
        version = get_by_path(self.spec, "version")

        if version:
            self.h(', version ' + get_descendant_text(version))

        self.h("""
 */
 """)

        if self.must_define:
            self.h("""
#ifndef %s
#error %s
#endif
""" % (self.must_define, self.must_define))

        self.h("""
#include <QFlags>

/**
 * \\addtogroup typesconstants Types and constants
 *
 * Enumerated, flag, structure, list and mapping types and utility constants.
 */

/**
 * \\defgroup flagtypeconsts Flag type constants
 * \\ingroup typesconstants
 *
 * Types generated from the specification representing bit flag constants and
 * combinations of them (bitfields).
 */

/**
 * \\defgroup enumtypeconsts Enumerated type constants
 * \\ingroup typesconstants
 *
 * Types generated from the specification representing enumerated types ie.
 * types the values of which are mutually exclusive integral constants.
 */

/**
 * \\defgroup ifacestrconsts Interface string constants
 * \\ingroup typesconstants
 *
 * D-Bus interface names of the interfaces in the specification.
 */

/**
 * \\defgroup errorstrconsts Error string constants
 * \\ingroup typesconstants
 *
 * Names of the D-Bus errors in the specification.
 */
""")

        # Begin namespace
        self.h("""
namespace %s
{
""" % self.namespace)

        # Flags
        for flags in self.spec.getElementsByTagNameNS(NS_TP, 'flags'):
            self.do_flags(flags)

        # Enums
        for enum in self.spec.getElementsByTagNameNS(NS_TP, 'enum'):
            self.do_enum(enum)

        # End namespace
        self.h("""\
}

""")

        # Interface names
        for iface in self.spec.getElementsByTagName('interface'):
            if self.old_prefix:
                self.h("""\
/**
 * \\ingroup ifacestrconsts
 *
 * The interface name "%(name)s".
 */
#define %(DEFINE)s "%(name)s"

""" % {'name' : iface.getAttribute('name'),
       'DEFINE' : self.old_prefix + 'INTERFACE_' + get_by_path(iface, '../@name').upper().replace('/', '')})

            if self.define_prefix:
                self.h("""\
/**
 * \\ingroup ifacestrconsts
 *
 * The interface name "%(name)s" as a QLatin1String, usable in QString requiring contexts even when
 * building with Q_NO_CAST_FROM_ASCII defined.
 */
#define %(DEFINE)s (QLatin1String("%(name)s"))

""" % {'name' : iface.getAttribute('name'),
       'DEFINE' : self.define_prefix + 'IFACE_' + get_by_path(iface, '../@name').upper().replace('/', '')})

        # Error names
        for error in get_by_path(self.spec, 'errors/error'):
            name = error.getAttribute('name')
            fullname = get_by_path(error, '../@namespace') + '.' + name.replace(' ', '')

            if self.old_prefix:
                define = self.old_prefix + 'ERROR_' + name.replace(' ', '_').replace('.', '_').upper()
                self.h("""\
/**
 * \\ingroup errorstrconsts
 *
 * The error name "%(fullname)s".
%(docstring)s\
 */
#define %(DEFINE)s "%(fullname)s"

""" % {'fullname' : fullname,
       'docstring': format_docstring(error, self.refs),
       'DEFINE' : define})

            if self.define_prefix:
                define = self.define_prefix + 'ERROR_' + name.replace(' ', '_').replace('.', '_').upper()
                self.h("""\
/**
 * \\ingroup errorstrconsts
 *
 * The error name "%(fullname)s" as a QLatin1String, usable in QString requiring contexts even when
 * building with Q_NO_CAST_FROM_ASCII defined.
%(docstring)s\
 */
#define %(DEFINE)s QLatin1String("%(fullname)s")

""" % {'fullname' : fullname,
       'docstring': format_docstring(error, self.refs),
       'DEFINE' : define})

    def do_flags(self, flags):
        singular = flags.getAttribute('singular') or \
                   flags.getAttribute('value-prefix')

        using_name = False
        if not singular:
            using_name = True
            singular = flags.getAttribute('name')

        if singular.endswith('lags'):
            singular = singular[:-1]

        if using_name and singular.endswith('s'):
            singular = singular[:-1]

        singular = singular.replace('_', '')
        plural = (flags.getAttribute('plural') or flags.getAttribute('name') or singular + 's').replace('_', '')
        self.h("""\
/**
 * \\ingroup flagtypeconsts
 *
 * Flag type generated from the specification.
 */
enum %(singular)s
{
""" % {'singular' : singular})

        flagvalues = get_by_path(flags, 'flag')

        for flag in flagvalues:
            self.do_val(flag, singular, flag == flagvalues[-1])

        self.h("""\
    %s = 0xffffffffU
""" % ("_" + singular + "Padding"))

        self.h("""\
};

/**
 * \\typedef QFlags<%(singular)s> %(plural)s
 * \\ingroup flagtypeconsts
 *
 * Type representing combinations of #%(singular)s values.
%(docstring)s\
 */
typedef QFlags<%(singular)s> %(plural)s;
Q_DECLARE_OPERATORS_FOR_FLAGS(%(plural)s)

""" % {'singular' : singular, 'plural' : plural, 'docstring' : format_docstring(flags, self.refs)})

    def do_enum(self, enum):
        singular = enum.getAttribute('singular') or \
                   enum.getAttribute('name')
        value_prefix = enum.getAttribute('singular') or \
                       enum.getAttribute('value-prefix') or \
                       enum.getAttribute('name')

        if singular.endswith('lags'):
            singular = singular[:-1]

        plural = enum.getAttribute('plural') or singular + 's'
        singular = singular.replace('_', '')
        value_prefix = value_prefix.replace('_', '')
        vals = get_by_path(enum, 'enumvalue')

        self.h("""\
/**
 * \\enum %(singular)s
 * \\ingroup enumtypeconsts
 *
 * Enumerated type generated from the specification.
%(docstring)s\
 */
enum %(singular)s
{
""" % {'singular' : singular, 'docstring' : format_docstring(enum, self.refs)})

        for val in vals:
            self.do_val(val, value_prefix, val == vals[-1])

        self.h("""\
    %s = 0xffffffffU
};

""" % ("_" + singular + "Padding"))

        self.h("""\
/**
 * \\ingroup enumtypeconsts
 *
 * 1 higher than the highest valid value of %(singular)s.
 */
const int NUM_%(upper-plural)s = (%(last-val)s+1);

""" % {'singular' : singular,
       'upper-plural' : plural.upper(),
       'last-val' : vals[-1].getAttribute('value')})

    def do_val(self, val, prefix, last):
        name = (val.getAttribute('suffix') or val.getAttribute('name')).replace('_', '')
        self.h("""\
%s\
    %s = %s,

""" % (format_docstring(val, self.refs, indent='     * ', brackets=('    /**', '     */')), prefix + name, val.getAttribute('value')))

if __name__ == '__main__':
    options, argv = gnu_getopt(argv[1:], '',
            ['namespace=',
             'str-constant-prefix=',
             'define-prefix=',
             'must-define=',
             'specxml='])

    Generator(dict(options))()
