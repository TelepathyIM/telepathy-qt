#!/usr/bin/env python2

from sys import argv, stdout, stderr
import xml.dom.minidom

from libtpcodegen import file_set_contents, u
from libglibcodegen import NS_TP, get_docstring, \
        get_descendant_text, get_by_path

class Generator(object):
    def __init__(self, prefix, dom, output_base):
        self.prefix = prefix + '_'
        self.spec = get_by_path(dom, "spec")[0]

        self.output_base = output_base
        self.__header = []
        self.__docs = []

    def __call__(self):
        self.do_header()
        self.do_body()
        self.do_footer()

        file_set_contents(self.output_base + '.h', u('').join(self.__header).encode('utf-8'))
        file_set_contents(self.output_base + '-gtk-doc.h', u('').join(self.__docs).encode('utf-8'))

    def write(self, code):
        self.__header.append(code)

    def d(self, code):
        self.__docs.append(code)

    # Header
    def do_header(self):
        self.write('/* Generated from ')
        self.write(get_descendant_text(get_by_path(self.spec, 'title')))
        version = get_by_path(self.spec, "version")
        if version:
            self.write(', version ' + get_descendant_text(version))
        self.write('\n\n')
        for copyright in get_by_path(self.spec, 'copyright'):
            self.write(get_descendant_text(copyright))
            self.write('\n')
        self.write(get_descendant_text(get_by_path(self.spec, 'license')))
        self.write('\n')
        self.write(get_descendant_text(get_by_path(self.spec, 'docstring')))
        self.write("""
 */

#ifdef __cplusplus
extern "C" {
#endif
\n""")

    # Body
    def do_body(self):
        for elem in self.spec.getElementsByTagNameNS(NS_TP, '*'):
            if elem.localName == 'flags':
                self.do_flags(elem)
            elif elem.localName == 'enum':
                self.do_enum(elem)

    def do_flags(self, flags):
        name = flags.getAttribute('plural') or flags.getAttribute('name')
        value_prefix = flags.getAttribute('singular') or \
                       flags.getAttribute('value-prefix') or \
                       flags.getAttribute('name')
        self.d("""\
/**
 * %s:
""" % (self.prefix + name).replace('_', ''))
        for flag in get_by_path(flags, 'flag'):
            self.do_gtkdoc(flag, value_prefix)
        self.d(' *\n')
        docstrings = get_by_path(flags, 'docstring')
        if docstrings:
            self.d("""\
 * <![CDATA[%s]]>
 *
""" % get_descendant_text(docstrings).replace('\n', ' '))
        self.d("""\
 * Bitfield/set of flags generated from the Telepathy specification.
 */
""")

        self.write("typedef enum /*< flags >*/ {\n")

        for flag in get_by_path(flags, 'flag'):
            self.do_val(flag, value_prefix)
        self.write("""\
} %s;

""" % (self.prefix + name).replace('_', ''))

    def do_enum(self, enum):
        name = enum.getAttribute('singular') or enum.getAttribute('name')
        value_prefix = enum.getAttribute('singular') or \
                       enum.getAttribute('value-prefix') or \
                       enum.getAttribute('name')
        name_plural = enum.getAttribute('plural') or \
                      enum.getAttribute('name') + 's'
        self.d("""\
/**
 * %s:
""" % (self.prefix + name).replace('_', ''))
        vals = get_by_path(enum, 'enumvalue')
        for val in vals:
            self.do_gtkdoc(val, value_prefix)
        self.d(' *\n')
        docstrings = get_by_path(enum, 'docstring')
        if docstrings:
            self.d("""\
 * <![CDATA[%s]]>
 *
""" % get_descendant_text(docstrings).replace('\n', ' '))
        self.d("""\
 * Bitfield/set of flags generated from the Telepathy specification.
 */
""")

        self.write("typedef enum {\n")

        for val in vals:
            self.do_val(val, value_prefix)
        self.write("} %s;\n" % (self.prefix + name).replace('_', ''))

        self.d("""\
/**
 * %(upper-prefix)sNUM_%(upper-plural)s:
 *
 * 1 higher than the highest valid value of #%(mixed-name)s.
 */

/**
 * NUM_%(upper-prefix)s%(upper-plural)s: (skip)
 *
 * 1 higher than the highest valid value of #%(mixed-name)s.
 * In new code, use %(upper-prefix)sNUM_%(upper-plural)s instead.
 */
""" % {'mixed-name' : (self.prefix + name).replace('_', ''),
       'upper-prefix' : self.prefix.upper(),
       'upper-plural' : name_plural.upper(),
       'last-val' : vals[-1].getAttribute('value')})

        self.write("""\
#define %(upper-prefix)sNUM_%(upper-plural)s (%(last-val)s+1)
#define NUM_%(upper-prefix)s%(upper-plural)s %(upper-prefix)sNUM_%(upper-plural)s

""" % {'mixed-name' : (self.prefix + name).replace('_', ''),
       'upper-prefix' : self.prefix.upper(),
       'upper-plural' : name_plural.upper(),
       'last-val' : vals[-1].getAttribute('value')})

    def do_val(self, val, value_prefix):
        name = val.getAttribute('name')
        suffix = val.getAttribute('suffix')
        use_name = (self.prefix + value_prefix + '_' + \
                (suffix or name)).upper()
        assert not (name and suffix) or name == suffix, \
                'Flag/enumvalue name %s != suffix %s' % (name, suffix)
        self.write('    %s = %s,\n' % (use_name, val.getAttribute('value')))

    def do_gtkdoc(self, node, value_prefix):
        self.d(' * @')
        self.d((self.prefix + value_prefix + '_' +
            node.getAttribute('suffix')).upper())
        self.d(': <![CDATA[')
        docstring = get_by_path(node, 'docstring')
        self.d(get_descendant_text(docstring).replace('\n', ' '))
        self.d(']]>\n')

    # Footer
    def do_footer(self):
        self.write("""
#ifdef __cplusplus
}
#endif
""")

if __name__ == '__main__':
    argv = argv[1:]
    Generator(argv[0], xml.dom.minidom.parse(argv[1]), argv[2])()
