"""Library code for Qt D-Bus-related code generation.

The master copy of this library is in the telepathy-qt repository -
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

from sys import maxint, stderr
import re
from libtpcodegen import get_by_path, get_descendant_text, NS_TP, xml_escape

class Xzibit(Exception):
    def __init__(self, parent, child):
        self.parent = parent
        self.child = child

    def __str__(self):
        print """
    Nested <%s>s are forbidden.
    Parent:
        %s...
    Child:
        %s...
        """ % (self.parent.nodeName, self.parent.toxml()[:100],
               self.child.toxml()[:100])

class _QtTypeBinding:
    def __init__(self, val, inarg, outarg, array_val, custom_type, array_of,
            array_depth=None):
        self.val = val
        self.inarg = inarg
        self.outarg = outarg
        self.array_val = array_val
        self.custom_type = custom_type
        self.array_of = array_of
        self.array_depth = array_depth

        if array_depth is None:
            self.array_depth = int(bool(array_val))
        elif array_depth >= 1:
            assert array_val
        else:
            assert not array_val

class RefTarget(object):
    KIND_INTERFACE, KIND_METHOD, KIND_SIGNAL, KIND_PROPERTY = 'node', 'method', 'signal', 'property'

    def __init__(self, el):
        self.kind = el.localName
        assert self.kind in (self.KIND_INTERFACE, self.KIND_METHOD, self.KIND_SIGNAL, self.KIND_PROPERTY)

        if self.kind == self.KIND_INTERFACE:
            self.dbus_text = el.getAttribute('name').lstrip('/').replace('_', '') + 'Interface'
        else:
            self.member_text = el.getAttribute('name')

            assert el.parentNode.parentNode.localName == self.KIND_INTERFACE
            host_class = el.parentNode.parentNode.getAttribute('name').lstrip('/').replace('_', '') + 'Interface'

            if self.kind == self.KIND_PROPERTY:
                self.member_link = 'requestProperty%s()' % (self.member_text)
                self.dbus_link = '%s::%s' % (host_class, self.member_link)
            else:
                self.member_text = '%s()' % self.member_text

            self.dbus_text = '%s::%s' % (host_class, self.member_text)

class RefRegistry(object):
    def __init__(self, spec):
        self.targets = {}
        for node in spec.getElementsByTagName('node'):
            iface, = get_by_path(node, 'interface')
            iface_name = iface.getAttribute('name')

            self.targets[iface_name] = RefTarget(node)

            for method in iface.getElementsByTagName(RefTarget.KIND_METHOD):
                self.targets[iface_name + '.' + method.getAttribute('name')] = RefTarget(method)

            for signal in iface.getElementsByTagName(RefTarget.KIND_SIGNAL):
                self.targets[iface_name + '.' + signal.getAttribute('name')] = RefTarget(signal)

            for prop in iface.getElementsByTagName(RefTarget.KIND_PROPERTY):
                self.targets[iface_name + '.' + prop.getAttribute('name')] = RefTarget(prop)

    def process(self, ref):
        assert ref.namespaceURI == NS_TP

        def get_closest_parent(el, needle):
            node = el
            while node is not None and node.localName != needle:
                node = node.parentNode
            return node

        local = get_descendant_text(ref).strip()
        if ref.localName == 'member-ref':
            ns = get_closest_parent(ref, 'interface').getAttribute('name')
            path = ns + '.' + local.strip()
        else:
            if ref.hasAttribute('namespace'):
                ns = ref.getAttribute('namespace').replace('ofdT', 'org.freedesktop.Telepathy')
                path = ns + '.' + local.strip()
            else:
                path = local

        target = self.targets.get(path)

        if target is None:
            parent = get_closest_parent(ref, 'interface') or get_closest_parent(ref, 'error')
            parent_name = parent.getAttribute('name')
            if (path + parent_name).find('.DRAFT') == -1 and (path + parent_name).find('.FUTURE') == -1:
                print >> stderr, 'WARNING: Failed to resolve %s to "%s" in "%s"' % (
                        ref.localName, path, parent_name)
            return path

        if ref.localName == 'member-ref':
            if target.kind == target.KIND_PROPERTY:
                return '\\link %s %s \\endlink' % (target.member_link, target.member_text)
            else:
                return target.member_text
        else:
            if target.kind == target.KIND_PROPERTY:
                return '\\link %s %s \\endlink' % (target.dbus_link, target.dbus_text)
            else:
                return target.dbus_text

def binding_from_usage(sig, tptype, custom_lists, external=False, explicit_own_ns=None):
    # 'signature' : ('qt-type', 'pass-by-reference', 'array-type')
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
            'as' : ('QStringList', True, "StringListList"),
            'ay' : ('QByteArray', True, "ByteArrayList"),
            'av' : ('QVariantList', True, "VariantListList"),
            'a{sv}' : ('QVariantMap', True, None)
            }

    val, inarg = None, None
    custom_type = False
    array_of = None

    if natives.has_key(sig):
        typename, pass_by_ref, array_name = natives[sig]
        val = typename
        inarg = (pass_by_ref and ('const %s&' % val)) or val
    elif sig[0] == 'a' and natives.has_key(sig[1:]) and natives[sig[1:]][2]:
        val = natives[sig[1:]][2]
        if explicit_own_ns:
            val = explicit_own_ns + '::' + val
        inarg = 'const %s&' % val
        array_of = natives[sig[1:]][0]
    elif tptype:
        tptype = tptype.replace('_', '')
        custom_type = True

        if external:
            tptype = 'Tp::' + tptype
        elif explicit_own_ns:
            tptype = explicit_own_ns + '::' + tptype

        if tptype.endswith('[]'):
            tptype = tptype[:-2]
            extra_list_nesting = 0

            while tptype.endswith('[]'):
                extra_list_nesting += 1
                tptype = tptype[:-2]

            assert custom_lists.has_key(tptype), ('No array version of custom type %s in the spec, but array version used' % tptype)
            val = custom_lists[tptype] + 'List' * extra_list_nesting
        else:
            val = tptype

        inarg = 'const %s&' % val
    else:
        assert False, 'Don\'t know how to map type (%s, %s)' % (sig, tptype)

    outarg = val + '&'
    return _QtTypeBinding(val, inarg, outarg, None, custom_type, array_of)

def binding_from_decl(name, array_name, array_depth=None, external=False, explicit_own_ns=''):
    val = name.replace('_', '')
    if external:
        val = 'Tp::' + val
    elif explicit_own_ns:
        val = explicit_own_ns + '::' + val
    inarg = 'const %s&' % val
    outarg = '%s&' % val
    return _QtTypeBinding(val, inarg, outarg, array_name.replace('_', ''), True, None, array_depth)

def extract_arg_or_member_info(els, custom_lists, externals, typesns, refs, docstring_indent=' * ', docstring_brackets=None, docstring_maxwidth=80):
    names = []
    docstrings = []
    bindings = []

    for el in els:
        names.append(get_qt_name(el))
        docstrings.append(format_docstring(el, refs, docstring_indent, docstring_brackets, docstring_maxwidth))

        sig = el.getAttribute('type')
        tptype = el.getAttributeNS(NS_TP, 'type')
        bindings.append(binding_from_usage(sig, tptype, custom_lists, (sig, tptype) in externals, typesns))

    return names, docstrings, bindings

def format_docstring(el, refs, indent=' * ', brackets=None, maxwidth=80):
    docstring_el = None

    for x in el.childNodes:
        if x.namespaceURI == NS_TP and x.localName == 'docstring':
            docstring_el = x

    if not docstring_el:
        return ''

    lines = []

    # escape backslashes, so they won't be interpreted starting doxygen commands and we can later
    # insert doxygen commands we actually want
    def escape_slashes(x):
        if x.nodeType == x.TEXT_NODE:
            x.data = x.data.replace('\\', '\\\\')
        elif x.nodeType == x.ELEMENT_NODE:
            for y in x.childNodes:
                escape_slashes(y)
        else:
            return

    escape_slashes(docstring_el)
    doc = docstring_el.ownerDocument

    for n in docstring_el.getElementsByTagNameNS(NS_TP, 'rationale'):
        nested = n.getElementsByTagNameNS(NS_TP, 'rationale')
        if nested:
            raise Xzibit(n, nested[0])

        div = doc.createElement('div')
        div.setAttribute('class', 'rationale')

        for rationale_body in n.childNodes:
            div.appendChild(rationale_body.cloneNode(True))

        n.parentNode.replaceChild(div, n)

    if docstring_el.getAttribute('xmlns') == 'http://www.w3.org/1999/xhtml':
        for ref in docstring_el.getElementsByTagNameNS(NS_TP, 'member-ref') + docstring_el.getElementsByTagNameNS(NS_TP, 'dbus-ref'):
            nested = ref.getElementsByTagNameNS(NS_TP, 'member-ref') + ref.getElementsByTagNameNS(NS_TP, 'dbus-ref')
            if nested:
                raise Xzibit(n, nested[0])

            text = doc.createTextNode(' \\endhtmlonly ')
            text.data += refs.process(ref)
            text.data += ' \\htmlonly '

            ref.parentNode.replaceChild(text, ref)

        splitted = ''.join([el.toxml() for el in docstring_el.childNodes]).strip(' ').strip('\n').split('\n')
        level = min([not match and maxint or match.end() - 1 for match in [re.match('^ *[^ ]', line) for line in splitted]])
        assert level != maxint
        lines = ['\\htmlonly'] + [line[level:] for line in splitted] + ['\\endhtmlonly']
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
        tptype = ext.getAttribute('name')
        externals.append((sig, tptype))

    return externals

def gather_custom_lists(spec, typesns):
    custom_lists = {}
    structs = [(provider, typesns) for provider in spec.getElementsByTagNameNS(NS_TP, 'struct')]
    mappings = [(provider, typesns) for provider in spec.getElementsByTagNameNS(NS_TP, 'mapping')]
    exts = [(provider, 'Telepathy') for provider in spec.getElementsByTagNameNS(NS_TP, 'external-type')]

    for (provider, ns) in structs + mappings + exts:
        tptype = provider.getAttribute('name').replace('_', '')
        array_val = provider.getAttribute('array-name').replace('_', '')
        array_depth = provider.getAttribute('array-depth')
        if array_depth:
            array_depth = int(array_depth)
        else:
            array_depth = None

        if array_val:
            custom_lists[tptype] = array_val
            custom_lists[ns + '::' + tptype] = ns + '::' + array_val
            if array_depth >= 2:
                for i in xrange(array_depth):
                    custom_lists[tptype + ('[]' * (i+1))] = (
                            array_val + ('List' * i))
                    custom_lists[ns + '::' + tptype + ('[]' * (i+1))] = (
                            ns + '::' + array_val + ('List' * i))

    return custom_lists

def get_headerfile_cmd(realinclude, prettyinclude, indent=' * '):
    prettyinclude = prettyinclude or realinclude
    if realinclude:
        if prettyinclude:
            return indent + ('\\headerfile %s <%s>\n' % (realinclude, prettyinclude))
        else:
            return indent + ('\\headerfile %s <%s>\n' % (realinclude))
    else:
        return ''

def get_qt_name(el):
    name = el.getAttribute('name')

    if el.localName in ('method', 'signal', 'property'):
        bname = el.getAttributeNS(NS_TP, 'name-for-bindings')

        if bname:
            name = bname

    if not name:
        return None

    if name[0].isupper() and name[1].islower():
        name = name[0].lower() + name[1:]

    return qt_identifier_escape(name.replace('_', ''))

def qt_identifier_escape(str):
    built = (str[0].isdigit() and ['_']) or []

    for c in str:
        if c.isalnum():
            built.append(c)
        else:
            built.append('_')

    str = ''.join(built)

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

