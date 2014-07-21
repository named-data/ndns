# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
VERSION='0.1'
APPNAME="ndn-tlv-ping"

def options(opt):
    opt.load('compiler_cxx gnu_dirs')

def configure(conf):
    conf.load("compiler_cxx gnu_dirs")
    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'],
                   uselib_store='NDN_CXX', mandatory=True)

def build (bld):
    
    bld.program (
        features = 'cxx',
        target = 'rr-test',
        source = 'tests/RR_test.cpp',
        use = 'rr',
        )
    bld.stlib(source="src/rr.cpp", target="rr", use="NDN_CXX")


