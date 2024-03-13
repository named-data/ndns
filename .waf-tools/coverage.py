from waflib import TaskGen

def options(opt):
    opt.add_option('--with-coverage', action='store_true', default=False,
                   help='Add compiler flags to enable code coverage information')

def configure(conf):
    if conf.options.with_coverage:
        if not conf.options.debug:
            conf.fatal('Code coverage flags require debug mode compilation (add --debug)')
        conf.check_cxx(cxxflags=['-fprofile-arcs', '-ftest-coverage', '-fPIC'],
                       linkflags=['-fprofile-arcs'], uselib_store='GCOV', mandatory=True)

@TaskGen.feature('cxx','cc')
@TaskGen.after('process_source')
def add_coverage(self):
    if getattr(self, 'use', ''):
        self.use += ' GCOV'
    else:
        self.use = 'GCOV'
