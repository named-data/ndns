# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

VERSION = '0.1.0'
APPNAME = 'ndns'
BUGREPORT = 'https://redmine.named-data.net/projects/ndns'
URL = 'http://named-data.net/doc/ndns/'
GIT_TAG_PREFIX = 'ndns-'

from waflib import Logs, Utils, Context
import os, subprocess

def options(opt):
    opt.load(['compiler_cxx', 'gnu_dirs'])
    opt.load(['boost', 'default-compiler-flags', 'doxygen', 'sphinx_build',
              'sqlite3', 'sanitizers', 'coverage'], tooldir=['.waf-tools'])

    ropt = opt.add_option_group('NDNS Options')
    ropt.add_option('--with-tests', action='store_true', default=False, help='build unit tests')

def configure(conf):
    conf.load(['compiler_cxx', 'gnu_dirs',
               'boost', 'default-compiler-flags', 'doxygen', 'sphinx_build',
               'sqlite3'])

    conf.env['WITH_TESTS'] = conf.options.with_tests

    if 'PKG_CONFIG_PATH' not in os.environ:
        os.environ['PKG_CONFIG_PATH'] = Utils.subst_vars('${LIBDIR}/pkgconfig', conf.env)
    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'],
                   uselib_store='NDN_CXX', mandatory=True)

    conf.check_sqlite3(mandatory=True)

    USED_BOOST_LIBS = ['system', 'filesystem', 'thread', 'log', 'log_setup']
    if conf.env['WITH_TESTS']:
        USED_BOOST_LIBS += ['unit_test_framework']
    conf.check_boost(lib=USED_BOOST_LIBS, mandatory=True, mt=True)

    conf.check_compiler_flags()

    # Loading "late" to prevent tests from being compiled with profiling flags
    conf.load('coverage')

    conf.load('sanitizers')

    conf.define_cond('HAVE_TESTS', conf.env['WITH_TESTS'])
    conf.define('CONFDIR', '%s/ndn/ndns' % conf.env['SYSCONFDIR'])
    conf.define('DEFAULT_DBFILE', '%s/lib/ndn/ndns/ndns.db' % conf.env['LOCALSTATEDIR'])
    conf.write_config_header('src/config.hpp', define_prefix='NDNS_')

def build (bld):
    version(bld)

    bld(features='subst',
        name='version.hpp',
        source='src/version.hpp.in',
        target='src/version.hpp',
        VERSION_STRING=VERSION_BASE,
        VERSION_BUILD=VERSION,
        VERSION=int(VERSION_SPLIT[0]) * 1000000 +
                int(VERSION_SPLIT[1]) * 1000 +
                int(VERSION_SPLIT[2]),
        VERSION_MAJOR=VERSION_SPLIT[0],
        VERSION_MINOR=VERSION_SPLIT[1],
        VERSION_PATCH=VERSION_SPLIT[2])

    bld.objects(
        name='ndns-objects',
        source=bld.path.ant_glob('src/**/*.cpp'),
        use='version NDN_CXX BOOST',
        includes='src',
        export_includes='src')

    bld.recurse('tests')
    bld.recurse('tools')

    bld(features='subst',
        name='conf-samples',
        source=['validator.conf.sample.in', 'ndns.conf.sample.in'],
        target=['validator.conf.sample', 'ndns.conf.sample'],
        install_path='${SYSCONFDIR}/ndn/ndns',
        ANCHORPATH='anchors/root.cert',
        CONFDIR='%s/ndn/ndns' % bld.env['SYSCONFDIR'],
        DEFAULT_DBFILE='%s/lib/ndn/ndns/ndns.db' % bld.env['LOCALSTATEDIR'])

    if Utils.unversioned_sys_platform() == 'linux':
        bld(features='subst',
            name='ndns.service',
            source='systemd/ndns.service.in',
            target='systemd/ndns.service')

    if bld.env.SPHINX_BUILD:
        bld(features='sphinx',
            name='manpages',
            builder='man',
            config='docs/conf.py',
            outdir='docs/manpages',
            source=bld.path.ant_glob('docs/manpages/**/*.rst'),
            install_path='${MANDIR}',
            VERSION=VERSION)

def docs(bld):
    from waflib import Options
    Options.commands = ['doxygen', 'sphinx'] + Options.commands

def doxygen(bld):
    version(bld)

    if not bld.env.DOXYGEN:
        bld.fatal('Cannot build documentation ("doxygen" not found in PATH)')

    bld(features='subst',
        name='doxygen.conf',
        source=['docs/doxygen.conf.in',
                'docs/named_data_theme/named_data_footer-with-analytics.html.in'],
        target=['docs/doxygen.conf',
                'docs/named_data_theme/named_data_footer-with-analytics.html'],
        VERSION=VERSION,
        HTML_FOOTER='../build/docs/named_data_theme/named_data_footer-with-analytics.html' \
                        if os.getenv('GOOGLE_ANALYTICS', None) \
                        else '../docs/named_data_theme/named_data_footer.html',
        GOOGLE_ANALYTICS=os.getenv('GOOGLE_ANALYTICS', ''))

    bld(features='doxygen',
        doxyfile='docs/doxygen.conf',
        use='doxygen.conf')

def sphinx(bld):
    version(bld)

    if not bld.env.SPHINX_BUILD:
        bld.fatal('Cannot build documentation ("sphinx-build" not found in PATH)')

    bld(features='sphinx',
        config='docs/conf.py',
        outdir='docs',
        source=bld.path.ant_glob('docs/**/*.rst'),
        VERSION=VERSION)

def version(ctx):
    # don't execute more than once
    if getattr(Context.g_module, 'VERSION_BASE', None):
        return

    Context.g_module.VERSION_BASE = Context.g_module.VERSION
    Context.g_module.VERSION_SPLIT = VERSION_BASE.split('.')

    # first, try to get a version string from git
    gotVersionFromGit = False
    try:
        cmd = ['git', 'describe', '--always', '--match', '%s*' % GIT_TAG_PREFIX]
        out = subprocess.check_output(cmd, universal_newlines=True).strip()
        if out:
            gotVersionFromGit = True
            if out.startswith(GIT_TAG_PREFIX):
                Context.g_module.VERSION = out.lstrip(GIT_TAG_PREFIX)
            else:
                # no tags matched
                Context.g_module.VERSION = '%s-commit-%s' % (VERSION_BASE, out)
    except (OSError, subprocess.CalledProcessError):
        pass

    versionFile = ctx.path.find_node('VERSION')
    if not gotVersionFromGit and versionFile is not None:
        try:
            Context.g_module.VERSION = versionFile.read()
            return
        except EnvironmentError:
            pass

    # version was obtained from git, update VERSION file if necessary
    if versionFile is not None:
        try:
            if versionFile.read() == Context.g_module.VERSION:
                # already up-to-date
                return
        except EnvironmentError as e:
            Logs.warn('%s exists but is not readable (%s)' % (versionFile, e.strerror))
    else:
        versionFile = ctx.path.make_node('VERSION')

    try:
        versionFile.write(Context.g_module.VERSION)
    except EnvironmentError as e:
        Logs.warn('%s is not writable (%s)' % (versionFile, e.strerror))

def dist(ctx):
    version(ctx)

def distcheck(ctx):
    version(ctx)
