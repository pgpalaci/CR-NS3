# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

# def options(opt):
#     pass

# def configure(conf):
#     conf.check_nonfatal(header_name='stdint.h', define_name='HAVE_STDINT_H')

def build(bld):
    module = bld.create_ns3_module('laa-wifi-coexistence', ['lte','spectrum', 'wifi'])
    module.source = [
        # 'model/laa-wifi-coexistence.cc',
        # 'helper/laa-wifi-coexistence-helper.cc',
        ]

    module_test = bld.create_ns3_module_test_library('laa-wifi-coexistence')
    module_test.source = [
        'test/test-lte-unlicensed-interference.cc',
        'test/test-lte-interference-abs.cc',
        ]

    headers = bld(features='ns3header')
    headers.module = 'laa-wifi-coexistence'
    headers.source = [
#        'model/laa-wifi-coexistence.h',
#        'helper/laa-wifi-coexistence-helper.h',
        ]

    if bld.env.ENABLE_EXAMPLES:
        bld.recurse('examples')

    # bld.ns3_python_bindings()

