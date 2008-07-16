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
import xml.dom.minidom
from getopt import gnu_getopt

from libtpcodegen import NS_TP, get_descendant_text, get_by_path
from libqt4codegen import format_docstring

class Generator(object):
    def __init__(self, opts):
        try:
            self.namespace = opts['--namespace']
            self.prefix = opts['--str-constant-prefix']
            dom = xml.dom.minidom.parse(opts['--specxml'])
        except KeyError, k:
            assert False, 'Missing required parameter %s' % k.args[0]

        self.spec = get_by_path(dom, "spec")[0]

    def __call__(self):
        self.do_header()
        self.do_body()
        self.do_footer()

    # Header
    def do_header(self):
        stdout.write('/* Generated from ')
        stdout.write(get_descendant_text(get_by_path(self.spec, 'title')))
        version = get_by_path(self.spec, "version")

        if version:
            stdout.write(', version ' + get_descendant_text(version))

        stdout.write("""
 */

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

    # Body
    def do_body(self):
        # Begin namespace
        stdout.write("""
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
        stdout.write("""\
}

""")

        # Interface names
        for iface in self.spec.getElementsByTagName('interface'):
            stdout.write("""\
/**
 * \\ingroup ifacestrconsts
 *
 * The interface name "%(name)s".
 */
#define %(DEFINE)s "%(name)s"

""" % {'name' : iface.getAttribute('name'),
       'DEFINE' : self.prefix + 'IFACE_' + get_by_path(iface, '../@name').upper().replace('/', '')})

        # Error names
        for error in get_by_path(self.spec, 'errors/error'):
            name = error.getAttribute('name')
            fullname = get_by_path(error, '../@namespace') + '.' + name.replace(' ', '')
            define = self.prefix + 'ERROR_' + name.replace(' ', '_').replace('.', '_').upper()
            stdout.write("""\
/**
 * \\ingroup errorstrconsts
 *
 * The error name "%(fullname)s".
%(docstring)s\
 */
#define %(DEFINE)s "%(fullname)s"

""" % {'fullname' : fullname,
       'docstring': format_docstring(error),
       'DEFINE' : define})

    def do_flags(self, flags):
        singular = flags.getAttribute('singular') or \
                   flags.getAttribute('value-prefix') or \
                   flags.getAttribute('name')

        if singular.endswith('lags'):
            singular = singular[:-1]

        singular = singular.replace('_', '')
        plural = (flags.getAttribute('plural') or flags.getAttribute('name') or singular + 's').replace('_', '')
        stdout.write("""\
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

        stdout.write("""\
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

""" % {'singular' : singular, 'plural' : plural, 'docstring' : format_docstring(flags)})

    def do_enum(self, enum):
        singular = enum.getAttribute('singular') or \
                   enum.getAttribute('value-prefix') or \
                   enum.getAttribute('name')

        if singular.endswith('lags'):
            singular = singular[:-1]

        plural = enum.getAttribute('plural') or singular + 's'
        singular = singular.replace('_', '')
        vals = get_by_path(enum, 'enumvalue')

        stdout.write("""\
/**
 * \\enum %(singular)s
 * \\ingroup enumtypeconsts
 *
 * Enumerated type generated from the specification.
%(docstring)s\
 */
enum %(singular)s
{
""" % {'singular' : singular, 'docstring' : format_docstring(enum)})

        for val in vals:
            self.do_val(val, singular, val == vals[-1])

        stdout.write("""\
};

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
        stdout.write("""\
%s\
     %s = %s%s
""" % (format_docstring(val, indent='     * ', brackets=('    /**', '     */')), prefix + name, val.getAttribute('value'), (not last and ',\n') or ''))

    # Footer
    def do_footer(self):
        stdout.write('}\n')

if __name__ == '__main__':
    options, argv = gnu_getopt(argv[1:], '',
            ['namespace=',
             'str-constant-prefix=',
             'specxml='])

    Generator(dict(options))()
