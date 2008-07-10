#!/usr/bin/python

from sys import argv, stdout, stderr
import xml.dom.minidom

from libtpcodegen import NS_TP, get_descendant_text, get_by_path
from libqt4codegen import format_docstring

class Generator(object):
    def __init__(self, namespace, dom):
        self.namespace = namespace
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

        stdout.write(' */')
        stdout.write("""

#include <QFlags>

/**
 * \\addtogroup types Types and constants
 *
 * Enumerated, flag, structure, list and mapping types generated from the
 * specification.
 */

/**
 * \\defgroup flags Flag type constants
 * \\ingroup types
 *
 * Types generated from the specification representing bit flag constants and
 * combinations of them (bitfields).
 */

/**
 * \\defgroup enum Enumerated type constants
 * \\ingroup types
 *
 * Types generated from the specification representing enumerated types ie.
 * types the values of which are mutually exclusive.
 */

namespace %s
{
""" % self.namespace)

    # Body
    def do_body(self):
        for flags in self.spec.getElementsByTagNameNS(NS_TP, 'flags'):
            self.do_flags(flags)

        for enum in self.spec.getElementsByTagNameNS(NS_TP, 'enum'):
            self.do_enum(enum)

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
 * \\ingroup flags
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
 * \\ingroup flags
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
 * \\ingroup enum
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
 * \\ingroup enum
 *
 * 1 higher than the highest valid value of %(singular)s.
 */
#define NUM_%(upper-plural)s (%(last-val)s+1)

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
    argv = argv[1:]
    Generator(argv[0], xml.dom.minidom.parse(argv[1]))()
