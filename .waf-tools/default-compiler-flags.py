# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

import platform
from waflib import Configure, Logs, Utils

def options(opt):
    opt.add_option('--debug', '--with-debug', action='store_true', default=False,
                   help='Compile in debugging mode with minimal optimizations (-Og)')

def configure(conf):
    conf.start_msg('Checking C++ compiler version')

    cxx = conf.env.CXX_NAME # generic name of the compiler
    ccver = tuple(int(i) for i in conf.env.CC_VERSION)
    ccverstr = '.'.join(conf.env.CC_VERSION)
    errmsg = ''
    warnmsg = ''
    if cxx == 'gcc':
        if ccver < (7, 4, 0):
            errmsg = ('The version of gcc you are using is too old.\n'
                      'The minimum supported gcc version is 7.4.')
        conf.flags = GccFlags()
    elif cxx == 'clang':
        if Utils.unversioned_sys_platform() == 'darwin':
            if ccver < (10, 0, 0):
                errmsg = ('The version of Xcode you are using is too old.\n'
                          'The minimum supported Xcode version is 11.3.')
            elif ccver < (11, 0, 0):
                warnmsg = ('Using a version of Xcode older than 11.3 is not '
                           'officially supported and may result in build failures.')
        elif ccver < (6, 0, 0):
            errmsg = ('The version of clang you are using is too old.\n'
                      'The minimum supported clang version is 6.0.')
        conf.flags = ClangFlags()
    else:
        warnmsg = f'{cxx} compiler is unsupported'
        conf.flags = CompilerFlags()

    if errmsg:
        conf.end_msg(ccverstr, color='RED')
        conf.fatal(errmsg)
    elif warnmsg:
        conf.end_msg(ccverstr, color='YELLOW')
        Logs.warn('WARNING: ' + warnmsg)
    else:
        conf.end_msg(ccverstr)

    conf.areCustomCxxflagsPresent = (len(conf.env.CXXFLAGS) > 0)

    # General flags are always applied (e.g., selecting C++ language standard)
    generalFlags = conf.flags.getGeneralFlags(conf)
    conf.add_supported_cxxflags(generalFlags['CXXFLAGS'])
    conf.add_supported_linkflags(generalFlags['LINKFLAGS'])
    conf.env.DEFINES += generalFlags['DEFINES']

@Configure.conf
def check_compiler_flags(conf):
    # Debug or optimized CXXFLAGS and LINKFLAGS are applied only if the
    # corresponding environment variables are not set.
    # DEFINES are always applied.
    if conf.options.debug:
        extraFlags = conf.flags.getDebugFlags(conf)
        if conf.areCustomCxxflagsPresent:
            missingFlags = [x for x in extraFlags['CXXFLAGS'] if x not in conf.env.CXXFLAGS]
            if missingFlags:
                Logs.warn('Selected debug mode, but CXXFLAGS is set to a custom value "%s"'
                          % ' '.join(conf.env.CXXFLAGS))
                Logs.warn('Default flags "%s" will not be used' % ' '.join(missingFlags))
    else:
        extraFlags = conf.flags.getOptimizedFlags(conf)

    if not conf.areCustomCxxflagsPresent:
        conf.add_supported_cxxflags(extraFlags['CXXFLAGS'])
        conf.add_supported_linkflags(extraFlags['LINKFLAGS'])

    conf.env.DEFINES += extraFlags['DEFINES']

@Configure.conf
def add_supported_cxxflags(self, cxxflags):
    """
    Check which cxxflags are supported by compiler and add them to env.CXXFLAGS variable
    """
    if len(cxxflags) == 0:
        return

    self.start_msg('Checking supported CXXFLAGS')

    supportedFlags = []
    for flags in cxxflags:
        flags = Utils.to_list(flags)
        if self.check_cxx(cxxflags=['-Werror'] + flags, mandatory=False):
            supportedFlags += flags

    self.end_msg(' '.join(supportedFlags))
    self.env.prepend_value('CXXFLAGS', supportedFlags)

@Configure.conf
def add_supported_linkflags(self, linkflags):
    """
    Check which linkflags are supported by compiler and add them to env.LINKFLAGS variable
    """
    if len(linkflags) == 0:
        return

    self.start_msg('Checking supported LINKFLAGS')

    supportedFlags = []
    for flags in linkflags:
        flags = Utils.to_list(flags)
        if self.check_cxx(linkflags=['-Werror'] + flags, mandatory=False):
            supportedFlags += flags

    self.end_msg(' '.join(supportedFlags))
    self.env.prepend_value('LINKFLAGS', supportedFlags)


class CompilerFlags(object):
    def getCompilerVersion(self, conf):
        return tuple(int(i) for i in conf.env.CC_VERSION)

    def getGeneralFlags(self, conf):
        """Get dict of CXXFLAGS, LINKFLAGS, and DEFINES that are always needed"""
        return {'CXXFLAGS': [], 'LINKFLAGS': [], 'DEFINES': []}

    def getDebugFlags(self, conf):
        """Get dict of CXXFLAGS, LINKFLAGS, and DEFINES that are needed only in debug mode"""
        return {'CXXFLAGS': [], 'LINKFLAGS': [], 'DEFINES': ['_DEBUG']}

    def getOptimizedFlags(self, conf):
        """Get dict of CXXFLAGS, LINKFLAGS, and DEFINES that are needed only in optimized mode"""
        return {'CXXFLAGS': [], 'LINKFLAGS': [], 'DEFINES': ['NDEBUG']}

class GccBasicFlags(CompilerFlags):
    """
    This class defines basic flags that work for both gcc and clang compilers
    """
    def getGeneralFlags(self, conf):
        flags = super(GccBasicFlags, self).getGeneralFlags(conf)
        flags['CXXFLAGS'] += ['-std=c++17']
        if Utils.unversioned_sys_platform() != 'darwin':
            flags['LINKFLAGS'] += ['-fuse-ld=lld']
        return flags

    def getDebugFlags(self, conf):
        flags = super(GccBasicFlags, self).getDebugFlags(conf)
        flags['CXXFLAGS'] += ['-Og',
                              '-g3',
                              '-pedantic',
                              '-Wall',
                              '-Wextra',
                              '-Werror',
                              '-Wcatch-value=2',
                              '-Wextra-semi',
                              '-Wnon-virtual-dtor',
                              '-Wno-error=deprecated-declarations', # Bug #3795
                              '-Wno-error=maybe-uninitialized', # Bug #1615
                              '-Wno-unused-parameter',
                              ]
        flags['LINKFLAGS'] += ['-Wl,-O1']
        return flags

    def getOptimizedFlags(self, conf):
        flags = super(GccBasicFlags, self).getOptimizedFlags(conf)
        flags['CXXFLAGS'] += ['-O2',
                              '-g',
                              '-pedantic',
                              '-Wall',
                              '-Wextra',
                              '-Wcatch-value=2',
                              '-Wextra-semi',
                              '-Wnon-virtual-dtor',
                              '-Wno-unused-parameter',
                              ]
        flags['LINKFLAGS'] += ['-Wl,-O1']
        return flags

class GccFlags(GccBasicFlags):
    def getDebugFlags(self, conf):
        flags = super(GccFlags, self).getDebugFlags(conf)
        flags['CXXFLAGS'] += ['-fdiagnostics-color',
                              '-Wredundant-tags',
                              ]
        if platform.machine() == 'armv7l':
            flags['CXXFLAGS'] += ['-Wno-psabi'] # Bug #5106
        return flags

    def getOptimizedFlags(self, conf):
        flags = super(GccFlags, self).getOptimizedFlags(conf)
        flags['CXXFLAGS'] += ['-fdiagnostics-color',
                              '-Wredundant-tags',
                              ]
        if platform.machine() == 'armv7l':
            flags['CXXFLAGS'] += ['-Wno-psabi'] # Bug #5106
        return flags

class ClangFlags(GccBasicFlags):
    def getGeneralFlags(self, conf):
        flags = super(ClangFlags, self).getGeneralFlags(conf)
        if Utils.unversioned_sys_platform() == 'darwin':
            # Bug #4296
            brewdir = '/opt/homebrew' if platform.machine() == 'arm64' else '/usr/local'
            flags['CXXFLAGS'] += [['-isystem', f'{brewdir}/include'], # for Homebrew
                                  ['-isystem', '/opt/local/include']] # for MacPorts
        elif Utils.unversioned_sys_platform() == 'freebsd':
            # Bug #4790
            flags['CXXFLAGS'] += [['-isystem', '/usr/local/include']]
        return flags

    def getDebugFlags(self, conf):
        flags = super(ClangFlags, self).getDebugFlags(conf)
        flags['CXXFLAGS'] += ['-fcolor-diagnostics',
                              '-Wundefined-func-template',
                              '-Wno-unused-local-typedef', # Bugs #2657 and #3209
                              ]
        return flags

    def getOptimizedFlags(self, conf):
        flags = super(ClangFlags, self).getOptimizedFlags(conf)
        flags['CXXFLAGS'] += ['-fcolor-diagnostics',
                              '-Wundefined-func-template',
                              '-Wno-unused-local-typedef', # Bugs #2657 and #3209
                              ]
        return flags
