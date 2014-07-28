# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-
VERSION='0.1'
APPNAME="ndns"

def options(opt):
    opt.load(['compiler_cxx', 'gnu_dirs'])
    opt.load(['boost', 'default-compiler-flags', 'doxygen', 'sphinx_build',
              'sqlite3', 'pch', 'coverage'], tooldir=['.waf-tools'])

    ropt = opt.add_option_group('NDNS Options')

    ropt.add_option('--with-tests', action='store_true', default=False, dest='with_tests',
                    help='''build unit tests''')

def configure(conf):
    conf.load(['compiler_cxx', 'gnu_dirs',
               'boost', 'default-compiler-flags', 'doxygen',
               'sqlite3', 'pch', 'coverage'])

    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'],
                   uselib_store='NDN_CXX', mandatory=True)

    conf.check_sqlite3(mandatory=True)

    if conf.options.with_tests:
        conf.env['WITH_TESTS'] = True

    USED_BOOST_LIBS = ['system']
    if conf.env['WITH_TESTS']:
        USED_BOOST_LIBS += ['unit_test_framework']
    conf.check_boost(lib=USED_BOOST_LIBS, mandatory=True)

    # conf.define('DEFAULT_CONFIG_FILE', '%s/ndn/ndns.conf' % conf.env['SYSCONFDIR'])

    if not conf.options.with_sqlite_locking:
        conf.define('DISABLE_SQLITE3_FS_LOCKING', 1)

    conf.write_config_header('src/config.hpp')

def build (bld):
    bld(
        features='cxx',
        name='ndns-objects',
#        source=bld.path.ant_glob(['src/**/*.cpp'], excl=['src/main.cpp']),
        source=bld.path.ant_glob(['src/rr.cpp', 'src/query.cpp',
                'src/response.cpp', 'src/zone.cpp', 'src/iterative-query.cpp',
                'src/db/db-mgr.cpp','src/db/zone-mgr.cpp', 'src/db/rr-mgr.cpp',
                'src/app/ndn-app.cpp', 'src/app/name-server.cpp',
                'src/app/name-caching-resolver.cpp', 'src/app/name-dig.cpp',
                ], 
           excl=['src/main.cpp']),
        use='NDN_CXX BOOST',
        includes='src',
        export_includes='src',
    )
    
    bld(
      features='cxx cxxprogram',
      source = "tools/name-server-daemon.cpp",
      target = "name-server-daemon",
      use = "ndns-objects"
    )
    
    bld(
      features='cxx cxxprogram',
      source = "tools/caching-resolver-daemon.cpp",
      target = "caching-resolver-daemon",
      use = "ndns-objects"
    )
    
    bld(
      features='cxx cxxprogram',
      source = "tools/dig.cpp",
      target = "dig",
      use = "ndns-objects"
    )
    
    
    
    
    
    bld.recurse('tests')

    # bld.install_files('${SYSCONFDIR}/ndn', 'ndns.conf.sample')
