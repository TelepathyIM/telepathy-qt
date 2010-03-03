#!/usr/bin/python

from sys import argv, stdout, stderr
import xml.dom.minidom

from libglibcodegen import NS_TP, get_docstring, \
        get_descendant_text, get_by_path

class Generator(object):
    def __init__(self, prefix, implfile, declfile, dom):
        self.prefix = prefix + '_'
        self.impls = open(implfile, 'w')
        self.decls = open(declfile, 'w')
        self.spec = get_by_path(dom, "spec")[0]

    def h(self, code):
        self.decls.write(code.encode('utf-8'))

    def c(self, code):
        self.impls.write(code.encode('utf-8'))

    def __call__(self):
        for f in self.h, self.c:
            self.do_header(f)
        self.do_body()

    # Header
    def do_header(self, f):
        f('/* Generated from: ')
        f(get_descendant_text(get_by_path(self.spec, 'title')))
        version = get_by_path(self.spec, "version")
        if version:
            f(' version ' + get_descendant_text(version))
        f('\n\n')
        for copyright in get_by_path(self.spec, 'copyright'):
            f(get_descendant_text(copyright))
            f('\n')
        f('\n')
        f(get_descendant_text(get_by_path(self.spec, 'license')))
        f(get_descendant_text(get_by_path(self.spec, 'docstring')))
        f("""
 */

""")

    # Body
    def do_body(self):
        for iface in self.spec.getElementsByTagName('interface'):
            self.do_iface(iface)

    def do_iface(self, iface):
        parent_name = get_by_path(iface, '../@name')
        self.h("""\
/**
 * %(IFACE_DEFINE)s:
 *
 * The interface name "%(name)s"
 */
#define %(IFACE_DEFINE)s \\
"%(name)s"
""" % {'IFACE_DEFINE' : (self.prefix + 'IFACE_' + \
            parent_name).upper().replace('/', ''),
       'name' : iface.getAttribute('name')})

        self.h("""
/**
 * %(IFACE_QUARK_DEFINE)s:
 *
 * Expands to a call to a function that returns a quark for the interface \
name "%(name)s"
 */
#define %(IFACE_QUARK_DEFINE)s \\
  (%(iface_quark_func)s ())

GQuark %(iface_quark_func)s (void);

""" % {'IFACE_QUARK_DEFINE' : (self.prefix + 'IFACE_QUARK_' + \
            parent_name).upper().replace('/', ''),
       'iface_quark_func' : (self.prefix + 'iface_quark_' + \
            parent_name).lower().replace('/', ''),
       'name' : iface.getAttribute('name')})

        self.c("""\
GQuark
%(iface_quark_func)s (void)
{
  static GQuark quark = 0;

  if (G_UNLIKELY (quark == 0))
    {
      quark = g_quark_from_static_string ("%(name)s");
    }

  return quark;
}

""" % {'iface_quark_func' : (self.prefix + 'iface_quark_' + \
            parent_name).lower().replace('/', ''),
       'name' : iface.getAttribute('name')})

        for prop in iface.getElementsByTagNameNS(None, 'property'):
            self.decls.write("""
/**
 * %(IFACE_PREFIX)s_%(PROP_UC)s:
 *
 * The fully-qualified property name "%(name)s.%(prop)s"
 */
#define %(IFACE_PREFIX)s_%(PROP_UC)s \\
"%(name)s.%(prop)s"
""" % {'IFACE_PREFIX' : (self.prefix + 'PROP_' + \
                parent_name).upper().replace('/', ''),
           'PROP_UC': prop.getAttributeNS(NS_TP, "name-for-bindings").upper(),
           'name' : iface.getAttribute('name'),
           'prop' : prop.getAttribute('name'),
           })

if __name__ == '__main__':
    argv = argv[1:]
    Generator(argv[0], argv[1], argv[2], xml.dom.minidom.parse(argv[3]))()
