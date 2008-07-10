#!/usr/bin/python

from sys import argv, stdout, stderr
import codecs, locale
import xml.dom.minidom

stdout = codecs.getwriter('utf-8')(stdout)

NS_XI = 'http://www.w3.org/2001/XInclude'

def xincludate(dom, dropns = []):
    remove_attrs = []
    for i in xrange(dom.documentElement.attributes.length):
        attr = dom.documentElement.attributes.item(i)
        if attr.prefix == 'xmlns':
            if attr.localName in dropns:
                remove_attrs.append(attr)
            else:
                dropns.append(attr.localName)
    for attr in remove_attrs:
        dom.documentElement.removeAttributeNode(attr)
    for include in dom.getElementsByTagNameNS(NS_XI, 'include'):
        href = include.getAttribute('href')
        subdom = xml.dom.minidom.parse(href)
        xincludate(subdom, dropns)
        if './' in href:
            subdom.documentElement.setAttribute('xml:base', href)
        include.parentNode.replaceChild(subdom.documentElement, include)

if __name__ == '__main__':
    argv = argv[1:]
    dom = xml.dom.minidom.parse(argv[0])
    xincludate(dom)
    xml = dom.toxml()
    stdout.write(xml)
    stdout.write('\n')
