#!/usr/bin/python

# Generate GLib GInterfaces from the Telepathy specification.
# The master copy of this program is in the telepathy-glib repository -
# please make any changes there.
#
# Copyright (C) 2006, 2007 Collabora Limited
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

from libglibcodegen import escape_as_identifier, \
                           get_docstring, \
                           NS_TP, \
                           Signature, \
                           type_to_gtype, \
                           xml_escape


def types_to_gtypes(types):
    return [type_to_gtype(t)[1] for t in types]


class GTypesGenerator(object):
    def __init__(self, dom, output, mixed_case_prefix):
        self.dom = dom
        self.Prefix = mixed_case_prefix
        self.PREFIX_ = self.Prefix.upper() + '_'
        self.prefix_ = self.Prefix.lower() + '_'

        self.header = open(output + '.h', 'w')
        self.body = open(output + '-body.h', 'w')

        for f in (self.header, self.body):
            f.write('/* Auto-generated, do not edit.\n *\n'
                    ' * This file may be distributed under the same terms\n'
                    ' * as the specification from which it was generated.\n'
                    ' */\n\n')

        # keys are e.g. 'sv', values are the key escaped
        self.need_mappings = {}
        # keys are the contents of the struct (e.g. 'sssu'), values are the
        # key escaped
        self.need_structs = {}
        # keys are the contents of the struct (e.g. 'sssu'), values are the
        # key escaped
        self.need_struct_arrays = {}

        # keys are the contents of the array (unlike need_struct_arrays!),
        # values are the key escaped
        self.need_other_arrays = {}

    def h(self, code):
        self.header.write(code.encode("utf-8"))

    def c(self, code):
        self.body.write(code.encode("utf-8"))

    def do_mapping_header(self, mapping):
        members = mapping.getElementsByTagNameNS(NS_TP, 'member')
        assert len(members) == 2

        impl_sig = ''.join([elt.getAttribute('type')
                            for elt in members])

        esc_impl_sig = escape_as_identifier(impl_sig)

        name = (self.PREFIX_ + 'HASH_TYPE_' +
                mapping.getAttribute('name').upper())
        impl = self.prefix_ + 'type_dbus_hash_' + esc_impl_sig

        docstring = get_docstring(mapping) or '(Undocumented)'

        self.h('/**\n * %s:\n *\n' % name)
        self.h(' * %s\n' % xml_escape(docstring))
        self.h(' *\n')
        self.h(' * This macro expands to a call to a function\n')
        self.h(' * that returns the #GType of a #GHashTable\n')
        self.h(' * appropriate for representing a D-Bus\n')
        self.h(' * dictionary of signature\n')
        self.h(' * <literal>a{%s}</literal>.\n' % impl_sig)
        self.h(' *\n')

        key, value = members

        self.h(' * Keys (D-Bus type <literal>%s</literal>,\n'
                          % key.getAttribute('type'))
        tp_type = key.getAttributeNS(NS_TP, 'type')
        if tp_type:
            self.h(' * type <literal>%s</literal>,\n' % tp_type)
        self.h(' * named <literal>%s</literal>):\n'
                          % key.getAttribute('name'))
        docstring = get_docstring(key) or '(Undocumented)'
        self.h(' * %s\n' % xml_escape(docstring))
        self.h(' *\n')

        self.h(' * Values (D-Bus type <literal>%s</literal>,\n'
                          % value.getAttribute('type'))
        tp_type = value.getAttributeNS(NS_TP, 'type')
        if tp_type:
            self.h(' * type <literal>%s</literal>,\n' % tp_type)
        self.h(' * named <literal>%s</literal>):\n'
                          % value.getAttribute('name'))
        docstring = get_docstring(value) or '(Undocumented)'
        self.h(' * %s\n' % xml_escape(docstring))
        self.h(' *\n')

        self.h(' */\n')

        self.h('#define %s (%s ())\n\n' % (name, impl))
        self.need_mappings[impl_sig] = esc_impl_sig

        array_name = mapping.getAttribute('array-name')
        if array_name:
            gtype_name = self.PREFIX_ + 'ARRAY_TYPE_' + array_name.upper()
            contents_sig = 'a{' + impl_sig + '}'
            esc_contents_sig = escape_as_identifier(contents_sig)
            impl = self.prefix_ + 'type_dbus_array_of_' + esc_contents_sig
            self.h('/**\n * %s:\n\n' % gtype_name)
            self.h(' * Expands to a call to a function\n')
            self.h(' * that returns the #GType of a #GPtrArray\n')
            self.h(' * of #%s.\n' % name)
            self.h(' */\n')
            self.h('#define %s (%s ())\n\n' % (gtype_name, impl))
            self.need_other_arrays[contents_sig] = esc_contents_sig

    def do_struct_header(self, struct):
        members = struct.getElementsByTagNameNS(NS_TP, 'member')
        impl_sig = ''.join([elt.getAttribute('type') for elt in members])
        esc_impl_sig = escape_as_identifier(impl_sig)

        name = (self.PREFIX_ + 'STRUCT_TYPE_' +
                struct.getAttribute('name').upper())
        impl = self.prefix_ + 'type_dbus_struct_' + esc_impl_sig
        docstring = struct.getElementsByTagNameNS(NS_TP, 'docstring')
        if docstring:
            docstring = docstring[0].toprettyxml()
            if docstring.startswith('<tp:docstring>'):
                docstring = docstring[14:]
            if docstring.endswith('</tp:docstring>\n'):
                docstring = docstring[:-16]
            if docstring.strip() in ('<tp:docstring/>', ''):
                docstring = '(Undocumented)'
        else:
            docstring = '(Undocumented)'
        self.h('/**\n * %s:\n\n' % name)
        self.h(' * %s\n' % xml_escape(docstring))
        self.h(' *\n')
        self.h(' * This macro expands to a call to a function\n')
        self.h(' * that returns the #GType of a #GValueArray\n')
        self.h(' * appropriate for representing a D-Bus struct\n')
        self.h(' * with signature <literal>(%s)</literal>.\n'
                          % impl_sig)
        self.h(' *\n')

        for i, member in enumerate(members):
            self.h(' * Member %d (D-Bus type '
                              '<literal>%s</literal>,\n'
                              % (i, member.getAttribute('type')))
            tp_type = member.getAttributeNS(NS_TP, 'type')
            if tp_type:
                self.h(' * type <literal>%s</literal>,\n' % tp_type)
            self.h(' * named <literal>%s</literal>):\n'
                              % member.getAttribute('name'))
            docstring = get_docstring(member) or '(Undocumented)'
            self.h(' * %s\n' % xml_escape(docstring))
            self.h(' *\n')

        self.h(' */\n')
        self.h('#define %s (%s ())\n\n' % (name, impl))

        array_name = struct.getAttribute('array-name')
        if array_name != '':
            array_name = (self.PREFIX_ + 'ARRAY_TYPE_' + array_name.upper())
            impl = self.prefix_ + 'type_dbus_array_' + esc_impl_sig
            self.h('/**\n * %s:\n\n' % array_name)
            self.h(' * Expands to a call to a function\n')
            self.h(' * that returns the #GType of a #GPtrArray\n')
            self.h(' * of #%s.\n' % name)
            self.h(' */\n')
            self.h('#define %s (%s ())\n\n' % (array_name, impl))
            self.need_struct_arrays[impl_sig] = esc_impl_sig

        self.need_structs[impl_sig] = esc_impl_sig

    def __call__(self):
        mappings = self.dom.getElementsByTagNameNS(NS_TP, 'mapping')
        structs = self.dom.getElementsByTagNameNS(NS_TP, 'struct')

        for mapping in mappings:
            self.do_mapping_header(mapping)

        for sig in self.need_mappings:
            self.h('GType %stype_dbus_hash_%s (void);\n\n' %
                              (self.prefix_, self.need_mappings[sig]))
            self.c('GType\n%stype_dbus_hash_%s (void)\n{\n' %
                              (self.prefix_, self.need_mappings[sig]))
            self.c('  static GType t = 0;\n\n')
            self.c('  if (G_UNLIKELY (t == 0))\n')
            # FIXME: translate sig into two GTypes
            items = tuple(Signature(sig))
            gtypes = types_to_gtypes(items)
            self.c('    t = dbus_g_type_get_map ("GHashTable", '
                            '%s, %s);\n' % (gtypes[0], gtypes[1]))
            self.c('  return t;\n')
            self.c('}\n\n')

        for struct in structs:
            self.do_struct_header(struct)

        for sig in self.need_structs:
            self.h('GType %stype_dbus_struct_%s (void);\n\n' %
                              (self.prefix_, self.need_structs[sig]))
            self.c('GType\n%stype_dbus_struct_%s (void)\n{\n' %
                              (self.prefix_, self.need_structs[sig]))
            self.c('  static GType t = 0;\n\n')
            self.c('  if (G_UNLIKELY (t == 0))\n')
            self.c('    t = dbus_g_type_get_struct ("GValueArray",\n')
            items = tuple(Signature(sig))
            gtypes = types_to_gtypes(items)
            for gtype in gtypes:
                self.c('        %s,\n' % gtype)
            self.c('        G_TYPE_INVALID);\n')
            self.c('  return t;\n')
            self.c('}\n\n')

        for sig in self.need_struct_arrays:
            self.h('GType %stype_dbus_array_%s (void);\n\n' %
                              (self.prefix_, self.need_struct_arrays[sig]))
            self.c('GType\n%stype_dbus_array_%s (void)\n{\n' %
                              (self.prefix_, self.need_struct_arrays[sig]))
            self.c('  static GType t = 0;\n\n')
            self.c('  if (G_UNLIKELY (t == 0))\n')
            self.c('    t = dbus_g_type_get_collection ("GPtrArray", '
                            '%stype_dbus_struct_%s ());\n' %
                            (self.prefix_, self.need_struct_arrays[sig]))
            self.c('  return t;\n')
            self.c('}\n\n')

        for sig in self.need_other_arrays:
            self.h('GType %stype_dbus_array_of_%s (void);\n\n' %
                              (self.prefix_, self.need_other_arrays[sig]))
            self.c('GType\n%stype_dbus_array_of_%s (void)\n{\n' %
                              (self.prefix_, self.need_other_arrays[sig]))
            self.c('  static GType t = 0;\n\n')
            self.c('  if (G_UNLIKELY (t == 0))\n')

            if sig[:2] == 'a{' and sig[-1:] == '}':
                # array of mappings
                self.c('    t = dbus_g_type_get_collection ('
                            '"GPtrArray", '
                            '%stype_dbus_hash_%s ());\n' %
                            (self.prefix_, escape_as_identifier(sig[2:-1])))
            elif sig[:2] == 'a(' and sig[-1:] == ')':
                # array of arrays of struct
                self.c('    t = dbus_g_type_get_collection ('
                            '"GPtrArray", '
                            '%stype_dbus_array_%s ());\n' %
                            (self.prefix_, escape_as_identifier(sig[2:-1])))
            elif sig[:1] == 'a':
                # array of arrays of non-struct
                self.c('    t = dbus_g_type_get_collection ('
                            '"GPtrArray", '
                            '%stype_dbus_array_of_%s ());\n' %
                            (self.prefix_, escape_as_identifier(sig[1:])))
            else:
                raise AssertionError("array of '%s' not supported" % sig)

            self.c('  return t;\n')
            self.c('}\n\n')

if __name__ == '__main__':
    argv = sys.argv[1:]

    dom = xml.dom.minidom.parse(argv[0])

    GTypesGenerator(dom, argv[1], argv[2])()
