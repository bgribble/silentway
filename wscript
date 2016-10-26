#! /usr/bin/env python

from __future__ import print_function

APPNAME = "silentway"
VERSION = "0.01"
WAFTOOLS = "compiler_c gcc python glib2"

top = '.'
out = 'wafbuild'
pkgconf_libs = ["glib-2.0", "jack"]

from waflib.Configure import conf
import waflib

import os, os.path

def git_version():
    import subprocess

    vers = subprocess.check_output(['git', 'rev-parse', '--verify', '--short', 'HEAD'])
    if not isinstance(vers, str):
        vers = vers.decode()

    vers = vers.strip()

    return 'git_' + format(vers)

from waflib.Build import BuildContext, CleanContext, CFG_FILES

class MFPBuildContext (BuildContext):
    fun = 'install_deps'
    cmd = 'install_deps'

class MFPCleanContext (CleanContext):
    cmd = 'clean'

    def execute(self):
        self.restore()
        if not self.all_envs:
            self.load_envs()

        self.recurse([self.run_dir])
        try:
            self.clean()
        finally:
            self.store()

    def clean(self):
        if self.bldnode != self.srcnode:
            # would lead to a disaster if top == out
            lst=[]
            symlinks = []
            for e in self.all_envs.values():
                exclfiles = '.lock* *conf_check_*/** config.log c4che/*'
                allfiles = self.bldnode.ant_glob('**/*', dir=True, excl=exclfiles, quiet=True)
                allfiles.sort(key=lambda n: n.abspath())
                lst.extend(self.root.find_or_declare(f) for f in e[CFG_FILES])

                for n in allfiles:
                    if n in lst:
                        continue
                    else:
                        is_symlinked = False
                        for link in symlinks:
                            if os.path.dirname(n.abspath()).startswith(link):
                                is_symlinked = True
                                break
                        if is_symlinked:
                            continue
                    if os.path.islink(n.abspath()) and hasattr(n, 'children'):
                        symlinks.append(n.abspath())
                        delattr(n, "children")
                    elif os.path.isdir(n.abspath()):
                        continue
                    n.delete()
                    self.root.children = {}

        for v in ['node_deps', 'task_sigs', 'raw_deps']:
            setattr(self, v, {})


def options(opt):
    from waflib.Options import options
    opt.load(WAFTOOLS)
    optgrp = opt.get_option_group('Configuration options')
    optgrp.add_option("--virtualenv", action="store_true", dest="USE_VIRTUALENV",
                   help="Install into a virtualenv")

    # "egg" targets race to update the .pth file.  Must build them one at a time.
    options['jobs'] = 1

def configure(conf):
    conf.load(WAFTOOLS)

    # C libraries with pkg-config support (listed at top of file)
    uselibs = []

    for l in pkgconf_libs:
        uname = l.split("-")[0].upper()
        conf.check_cfg(package=l, args="--libs --cflags",
                       uselib_store=uname)
        uselibs.append(uname)
    conf.env.PKGCONF_LIBS = uselibs

    conf.env.GITVERSION = VERSION + "_" + git_version()
    print()
    print("silentway version", conf.env.GITVERSION, "configured.")

def build(bld):
    bld.program(source=bld.path.ant_glob("silentway/*.c"), target="silentway/silentway",
                cflags=["-std=gnu99", "-fpic", "-g", "-D_GNU_SOURCE", "-DMFP_USE_SSE"],
                uselib = bld.env.PKGCONF_LIBS,
                use=['silentway'])

