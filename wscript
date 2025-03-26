# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

import os
import subprocess
from waflib import Context, Logs, Utils

VERSION = '0.1'
APPNAME = 'ndns'
GIT_TAG_PREFIX = 'ndns-'

def options(opt):
    opt.load(['compiler_cxx', 'gnu_dirs'])
    opt.load(['default-compiler-flags',
              'coverage', 'sanitizers', 'boost', 'sqlite3',
              'doxygen', 'sphinx'],
             tooldir=['.waf-tools'])

    optgrp = opt.add_option_group('NDNS Options')
    optgrp.add_option('--with-tests', action='store_true', default=False,
                      help='Build unit tests')

def configure(conf):
    conf.load(['compiler_cxx', 'gnu_dirs',
               'default-compiler-flags', 'boost', 'sqlite3',
               'doxygen', 'sphinx'])

    conf.env.WITH_TESTS = conf.options.with_tests

    conf.find_program('dot', mandatory=False)

    # Prefer pkgconf if it's installed, because it gives more correct results
    # on Fedora/CentOS/RHEL/etc. See https://bugzilla.redhat.com/show_bug.cgi?id=1953348
    # Store the result in env.PKGCONFIG, which is the variable used inside check_cfg()
    conf.find_program(['pkgconf', 'pkg-config'], var='PKGCONFIG')

    pkg_config_path = os.environ.get('PKG_CONFIG_PATH', f'{conf.env.LIBDIR}/pkgconfig')
    conf.check_cfg(package='libndn-cxx', args=['libndn-cxx >= 0.8.1', '--cflags', '--libs'],
                   uselib_store='NDN_CXX', pkg_config_path=pkg_config_path)

    conf.check_sqlite3()

    boost_libs = ['filesystem', 'program_options']
    if conf.env.WITH_TESTS:
        boost_libs.append('unit_test_framework')

    conf.check_boost(lib=boost_libs, mt=True)

    conf.check_compiler_flags()

    # Loading "late" to prevent tests from being compiled with profiling flags
    conf.load('coverage')
    conf.load('sanitizers')

    conf.define_cond('HAVE_TESTS', conf.env.WITH_TESTS)
    conf.define('CONFDIR', f'{conf.env.SYSCONFDIR}/ndn/ndns')
    conf.define('DEFAULT_DBFILE', f'{conf.env.LOCALSTATEDIR}/lib/ndn/ndns/ndns.db')
    conf.write_config_header('src/config.hpp', define_prefix='NDNS_')

def build(bld):
    version(bld)

    vmajor = int(VERSION_SPLIT[0])
    vminor = int(VERSION_SPLIT[1]) if len(VERSION_SPLIT) >= 2 else 0
    vpatch = int(VERSION_SPLIT[2]) if len(VERSION_SPLIT) >= 3 else 0

    bld(features='subst',
        name='version.hpp',
        source='src/version.hpp.in',
        target='src/version.hpp',
        VERSION_STRING=VERSION_BASE,
        VERSION_BUILD=VERSION,
        VERSION=vmajor * 1000000 + vminor * 1000 + vpatch,
        VERSION_MAJOR=str(vmajor),
        VERSION_MINOR=str(vminor),
        VERSION_PATCH=str(vpatch))

    bld.objects(
        name='ndns-objects',
        source=bld.path.ant_glob('src/**/*.cpp'),
        use='version.hpp NDN_CXX BOOST',
        includes='src',
        export_includes='src')

    bld.recurse('tests')
    bld.recurse('tools')

    # Install sample configs
    bld(features='subst',
        name='conf-samples',
        source=['validator.conf.sample.in', 'ndns.conf.sample.in'],
        target=['validator.conf.sample', 'ndns.conf.sample'],
        install_path='${SYSCONFDIR}/ndn/ndns',
        ANCHORPATH='anchors/root.cert',
        CONFDIR='%s/ndn/ndns' % bld.env.SYSCONFDIR,
        DEFAULT_DBFILE='%s/lib/ndn/ndns/ndns.db' % bld.env.LOCALSTATEDIR)

    if Utils.unversioned_sys_platform() == 'linux':
        bld(features='subst',
            name='systemd-units',
            source='systemd/ndns.service.in',
            target='systemd/ndns.service')

    if bld.env.SPHINX_BUILD:
        bld(features='sphinx',
            name='manpages',
            builder='man',
            config='docs/conf.py',
            outdir='docs/manpages',
            source=bld.path.ant_glob('docs/manpages/*.rst'),
            install_path='${MANDIR}',
            version=VERSION_BASE,
            release=VERSION)

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
        HAVE_DOT='YES' if bld.env.DOT else 'NO',
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
        version=VERSION_BASE,
        release=VERSION)

def version(ctx):
    # don't execute more than once
    if getattr(Context.g_module, 'VERSION_BASE', None):
        return

    Context.g_module.VERSION_BASE = Context.g_module.VERSION
    Context.g_module.VERSION_SPLIT = VERSION_BASE.split('.')

    # first, try to get a version string from git
    version_from_git = ''
    try:
        cmd = ['git', 'describe', '--abbrev=8', '--always', '--match', f'{GIT_TAG_PREFIX}*']
        version_from_git = subprocess.run(cmd, capture_output=True, check=True, text=True).stdout.strip()
        if version_from_git:
            if GIT_TAG_PREFIX and version_from_git.startswith(GIT_TAG_PREFIX):
                Context.g_module.VERSION = version_from_git[len(GIT_TAG_PREFIX):]
            elif not GIT_TAG_PREFIX and ('.' in version_from_git or '-' in version_from_git):
                Context.g_module.VERSION = version_from_git
            else:
                # no tags matched (or we are in a shallow clone)
                Context.g_module.VERSION = f'{VERSION_BASE}+git.{version_from_git}'
    except (OSError, subprocess.SubprocessError):
        pass

    # fallback to the VERSION.info file, if it exists and is not empty
    version_from_file = ''
    version_file = ctx.path.find_node('VERSION.info')
    if version_file is not None:
        try:
            version_from_file = version_file.read().strip()
        except OSError as e:
            Logs.warn(f'{e.filename} exists but is not readable ({e.strerror})')
    if version_from_file and not version_from_git:
        Context.g_module.VERSION = version_from_file
        return

    # update VERSION.info if necessary
    if version_from_file == Context.g_module.VERSION:
        # already up-to-date
        return
    if version_file is None:
        version_file = ctx.path.make_node('VERSION.info')
    try:
        version_file.write(Context.g_module.VERSION)
    except OSError as e:
        Logs.warn(f'{e.filename} is not writable ({e.strerror})')

def dist(ctx):
    ctx.algo = 'tar.xz'
    version(ctx)

def distcheck(ctx):
    ctx.algo = 'tar.xz'
    version(ctx)
