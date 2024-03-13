from waflib.Configure import conf

def options(opt):
    opt.add_option('--with-sqlite3', type='string', default=None, dest='sqlite3_dir',
                   help='directory where SQLite3 is installed, e.g., /usr/local')
    opt.add_option('--without-sqlite-locking', action='store_false', default=True,
                   dest='with_sqlite_locking',
                   help='''Disable filesystem locking in sqlite3 database '''
                        '''(use unix-dot locking mechanism instead). '''
                        '''This option may be necessary if home directory is hosted on NFS.''')

@conf
def check_sqlite3(self, *k, **kw):
    root = k and k[0] or kw.get('path', self.options.sqlite3_dir)
    mandatory = kw.get('mandatory', True)
    var = kw.get('uselib_store', 'SQLITE3')

    if not self.options.with_sqlite_locking:
        conf.define('DISABLE_SQLITE3_FS_LOCKING', 1)

    if root:
        self.check_cxx(lib='sqlite3',
                       msg='Checking for SQLite3 library',
                       define_name='HAVE_%s' % var,
                       uselib_store=var,
                       mandatory=mandatory,
                       includes='%s/include' % root,
                       libpath='%s/lib' % root)
    else:
        try:
            self.check_cfg(package='sqlite3',
                           args=['--cflags', '--libs'],
                           uselib_store='SQLITE3',
                           mandatory=True)
        except:
            self.check_cxx(lib='sqlite3',
                           msg='Checking for SQLite3 library',
                           define_name='HAVE_%s' % var,
                           uselib_store=var,
                           mandatory=mandatory)
