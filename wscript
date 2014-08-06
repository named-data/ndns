# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

VERSION='0.1.0'
APPNAME="ndns"
BUGREPORT = "http://redmine.named-data.net/projects/ndns"
URL = "http://named-data.net/doc/NDNS/"

from waflib import Logs, Utils, Context

def options(opt):
    opt.load(['compiler_cxx', 'gnu_dirs'])
    opt.load(['boost', 'default-compiler-flags', 'doxygen', 'sphinx_build',
              'sqlite3', 'pch', 'coverage'], tooldir=['.waf-tools'])

    ropt = opt.add_option_group('NDNS Options')

    ropt.add_option('--with-tests', action='store_true', default=False, dest='with_tests',
                    help='''build unit tests''')

def configure(conf):
    conf.load(['compiler_cxx', 'gnu_dirs',
               'boost', 'default-compiler-flags', 'doxygen', 'sphinx_build',
               'sqlite3', 'pch', 'coverage'])

    conf.check_cfg(package='liblog4cxx', args=['--cflags', '--libs'],
                  uselib_store='LOG4CXX', mandatory=True)

    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'],
                   uselib_store='NDN_CXX', mandatory=True)

    conf.check_sqlite3(mandatory=True)

    if conf.options.with_tests:
        conf.env['WITH_TESTS'] = True

    USED_BOOST_LIBS = ['system']
    if conf.env['WITH_TESTS']:
        USED_BOOST_LIBS += ['unit_test_framework']
    conf.check_boost(lib=USED_BOOST_LIBS, mandatory=True)

    if not conf.options.with_sqlite_locking:
        conf.define('DISABLE_SQLITE3_FS_LOCKING', 1)

    conf.write_config_header('src/config.hpp')

def build (bld):
    version(bld)

    bld(features="subst",
        name='version',
        source='src/version.hpp.in',
        target='src/version.hpp',
        install_path=None,
        VERSION_STRING=VERSION_BASE,
        VERSION_BUILD=VERSION,
        VERSION=int(VERSION_SPLIT[0]) * 1000000 +
                int(VERSION_SPLIT[1]) * 1000 +
                int(VERSION_SPLIT[2]),
        VERSION_MAJOR=VERSION_SPLIT[0],
        VERSION_MINOR=VERSION_SPLIT[1],
        VERSION_PATCH=VERSION_SPLIT[2],
        )

    bld(
        features='cxx',
        name='ndns-objects',
        source=bld.path.ant_glob(['src/**/*.cpp'],
                                 excl=['src/main.cpp']),
        use='version NDN_CXX BOOST',
        includes='src',
        export_includes='src',
    )

    for app in bld.path.ant_glob('tools/*.cpp'):
        bld(features=['cxx', 'cxxprogram'],
            target='bin/%s' % (str(app.change_ext(''))),
            source=['tools/%s' % (str(app))],
            use='ndns-objects',
            )

    bld.recurse('tests')

    # bld.install_files('${SYSCONFDIR}/ndn', 'ndns.conf.sample')

def docs(bld):
    from waflib import Options
    Options.commands = ['doxygen', 'sphinx'] + Options.commands

def doxygen(bld):
    version(bld)

    if not bld.env.DOXYGEN:
        Logs.error("ERROR: cannot build documentation (`doxygen' is not found in $PATH)")
    else:
        bld(features="subst",
            name="doxygen-conf",
            source="docs/doxygen.conf.in",
            target="docs/doxygen.conf",
            VERSION=VERSION_BASE,
            )

        bld(features="doxygen",
            doxyfile='docs/doxygen.conf',
            use="doxygen-conf")

def sphinx(bld):
    version(bld)

    if not bld.env.SPHINX_BUILD:
        bld.fatal("ERROR: cannot build documentation (`sphinx-build' is not found in $PATH)")
    else:
        bld(features="sphinx",
            outdir="docs",
            source=bld.path.ant_glob('docs/**/*.rst'),
            config="docs/conf.py",
            VERSION=VERSION_BASE)

def version(ctx):
    if getattr(Context.g_module, 'VERSION_BASE', None):
        return

    Context.g_module.VERSION_BASE = Context.g_module.VERSION
    Context.g_module.VERSION_SPLIT = [v for v in VERSION_BASE.split('.')]

    try:
        cmd = ['git', 'describe', '--match', 'ndns-*']
        p = Utils.subprocess.Popen(cmd, stdout=Utils.subprocess.PIPE,
                                   stderr=None, stdin=None)
        out = p.communicate()[0].strip()
        if p.returncode == 0 and out != "":
            Context.g_module.VERSION = out[4:]
    except:
        pass

def dist(ctx):
    version(ctx)

def distcheck(ctx):
    version(ctx)
