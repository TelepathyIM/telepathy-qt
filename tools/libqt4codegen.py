"""Library code for Qt4 D-Bus-related code generation.

The master copy of this library is in the telepathy-qt4 repository -
please make any changes there.
"""

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

from sys import maxint
import re
from libtpcodegen import escape_as_identifier, get_by_path, get_descendant_text, NS_TP, xml_escape


class _Qt4TypeBinding:
    def __init__(self, val, inarg, outarg, array_val, custom_type, array_of):
        self.val = val
        self.inarg = inarg
        self.outarg = outarg
        self.array_val = array_val
        self.custom_type = custom_type
        self.array_of = array_of

def binding_from_usage(sig, tptype, custom_lists, external=False):
    # 'signature' : ('qt-type', 'pass-by-reference')
    natives = {
            'y' : ('uchar', False, None),
            'b' : ('bool', False, 'BoolList'),
            'n' : ('short', False, 'ShortList'),
            'q' : ('ushort', False, 'UShortList'),
            'i' : ('int', False, 'IntList'),
            'u' : ('uint', False, 'UIntList'),
            'x' : ('qlonglong', False, 'LongLongList'),
            't' : ('qulonglong', False, 'ULongLongList'),
            'd' : ('double', False, 'DoubleList'),
            's' : ('QString', True, None),
            'v' : ('QDBusVariant', True, None),
            'o' : ('QDBusObjectPath', True, 'ObjectPathList'),
            'g' : ('QDBusSignature', True, 'SignatureList'),
            'as' : ('QStringList', True, None),
            'ay' : ('QByteArray', True, None),
            'av' : ('QVariantList', True, None),
            'a{sv}' : ('QVariantMap', True, None)
            }

    val, inarg = None, None
    custom_type = False
    array_of = None

    if natives.has_key(sig):
        typename, pass_by_ref, array_name = natives[sig]
        val = typename
        inarg = (pass_by_ref and ('const %s&' % val)) or val
    elif sig[0] == 'a' and natives.has_key(sig[1]) and natives[sig[1]][2]:
        val = natives[sig[1]][2]
        inarg = 'const %s&' % val
        array_of = natives[sig[1]][0]
    elif tptype:
        tptype = tptype.replace('_', '')
        custom_type = True

        if external:
            tptype = 'Telepathy::' + tptype

        if tptype.endswith('[]'):
            tptype = tptype[:-2]
            assert custom_lists.has_key(tptype), ('No array version of custom type %s in the spec, but array version used' % tptype) + str(custom_lists)
            val = custom_lists[tptype]
        else:
            val = tptype

        inarg = 'const %s&' % val
    else:
        assert False, 'Don\'t know how to map type (%s, %s)' % (sig, tptype)

    outarg = val + '&'
    return _Qt4TypeBinding(val, inarg, outarg, None, custom_type, array_of)

def binding_from_decl(name, array_name):
    val = name.replace('_', '')
    inarg = 'const %s&' % val
    outarg = '%s&' % val
    return _Qt4TypeBinding(val, inarg, outarg, array_name.replace('_', ''), True, None)

def cxx_identifier_escape(str):
    str = escape_as_identifier(str)

    # List of reserved identifiers
    # Initial list from http://cs.smu.ca/~porter/csc/ref/cpp_keywords.html

    # Keywords inherited from C90
    reserved = ['auto',
                'const',
                'double',
                'float',
                'int',
                'short',
                'struct',
                'unsigned',
                'break',
                'continue',
                'else',
                'for',
                'long',
                'signed',
                'switch',
                'void',
                'case',
                'default',
                'enum',
                'goto',
                'register',
                'sizeof',
                'typedef',
                'volatile',
                'char',
                'do',
                'extern',
                'if',
                'return',
                'static',
                'union',
                'while',
    # C++-only keywords
                'asm',
                'dynamic_cast',
                'namespace',
                'reinterpret_cast',
                'try',
                'bool',
                'explicit',
                'new',
                'static_cast',
                'typeid',
                'catch',
                'false',
                'operator',
                'template',
                'typename',
                'class',
                'friend',
                'private',
                'this',
                'using',
                'const_cast',
                'inline',
                'public',
                'throw',
                'virtual',
                'delete',
                'mutable',
                'protected',
                'true',
                'wchar_t',
    # Operator replacements
                'and',
                'bitand',
                'compl',
                'not_eq',
                'or_eq',
                'xor_eq',
                'and_eq',
                'bitor',
                'not',
                'or',
                'xor',
    # Predefined identifiers
                'INT_MIN',
                'INT_MAX',
                'MAX_RAND',
                'NULL',
    # Qt
                'SIGNAL',
                'SLOT',
                'signals',
                'slots']

    while str in reserved:
        str = str + '_'

    return str

def format_docstring(el, indent=' * ', brackets=None, maxwidth=80):
    docstring_el = None

    for x in el.childNodes:
        if x.namespaceURI == NS_TP and x.localName == 'docstring':
            docstring_el = x

    if not docstring_el:
        return ''

    lines = []

    if docstring_el.getAttribute('xmlns') == 'http://www.w3.org/1999/xhtml':
        splitted = ''.join([el.toxml() for el in docstring_el.childNodes]).strip(' ').strip('\n').split('\n')
        level = min([not match and maxint or match.end() - 1 for match in [re.match('^ *[^ ]', line) for line in splitted]])
        assert level != maxint
        lines = [line[level:] for line in splitted]
    else:
        content = xml_escape(get_descendant_text(docstring_el).replace('\n', ' ').strip())

        while content.find('  ') != -1:
            content = content.replace('  ', ' ')

        left = maxwidth - len(indent) - 1
        line = ''

        while content:
            step = (content.find(' ') + 1) or len(content)

            if step > left:
                lines.append(line)
                line = ''
                left = maxwidth - len(indent) - 1

            left = left - step
            line = line + content[:step]
            content = content[step:]

        if line:
            lines.append(line)

    output = []

    if lines:
        if brackets:
            output.append(brackets[0])
        else:
            output.append(indent)
 
        output.append('\n')

    for line in lines:
        output.append(indent)
        output.append(line)
        output.append('\n')

    if lines and brackets:
        output.append(brackets[1])
        output.append('\n')

    return ''.join(output)

def gather_externals(spec):
    externals = []

    for ext in spec.getElementsByTagNameNS(NS_TP, 'external-type'):
        sig = ext.getAttribute('type')
        tptype = ext.getAttributeNS(NS_TP, 'type')
        externals.append((sig, tptype))

    return externals

def gather_custom_lists(spec):
    custom_lists = {}
    structs = spec.getElementsByTagNameNS(NS_TP, 'struct')
    mappings = spec.getElementsByTagNameNS(NS_TP, 'mapping')
    exts = spec.getElementsByTagNameNS(NS_TP, 'external-type')

    for provider in structs + mappings + exts:
        tptype = provider.getAttribute('name').replace('_', '')
        array_name = provider.getAttribute('array-name')

        if array_name:
            custom_lists[tptype] = array_name.replace('_', '')

    return custom_lists

def get_qt4_name(el):
    name = el.getAttribute('name')
    bname = el.getAttributeNS(NS_TP, 'name-for-bindings')

    if bname:
        bname = bname[0].lower() + bname[1:]
        ret = bname.replace('_', '')
    else:
        ret = name[0].lower() + name[1:]

    return cxx_identifier_escape(ret)
