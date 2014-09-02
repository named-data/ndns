.. _NDNS Installation Instructions:

NDNS Installation Instructions
==============================

Prerequisites
-------------

-  Install the `ndn-cxx library <http://named-data.net/doc/ndn-cxx/current/INSTALL.html>`_
   and its requirements

To build manpages and API documentation:

-  ``doxygen``
-  ``graphviz``
-  ``python-sphinx``

   On OS X 10.8 and 10.9 with MacPorts:

   ::

       sudo port install doxygen graphviz py27-sphinx sphinx_select
       sudo port select sphinx py27-sphinx

   On Ubuntu >= 12.04:

   ::

       sudo apt-get install doxygen graphviz python-sphinx

Build
-----

The following basic commands should be used to build NDNS on Ubuntu:

::

    ./waf configure
    ./waf
    sudo ./waf install

Some platforms, such as OS X with MacPorts and certain Linux distributions (e.g., Fedora)
require setting the ``PKG_CONFIG_PATH`` before running configure.

For example, on OS X with MacPorts (assuming the default ``/opt/local`` MacPorts path):

::

    export PKG_CONFIG_PATH=/opt/local/lib/pkgconfig:/usr/local/lib/pkgconfig:/usr/lib/pkgconfig:$PKG_CONFIG_PATH
    ./waf configure
    ./waf
    sudo ./waf install

or some Linux distributions:

::

    export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig:/usr/lib/pkgconfig:/usr/lib64/pkgconfig:$PKG_CONFIG_PATH
    ./waf configure
    ./waf
    sudo ./waf install


Refer to ``./waf --help`` for more options that can be used during ``configure`` stage and
how to properly configure and run NFD.

Debug symbols
+++++++++++++

The default compiler flags enable debug symbols to be included in binaries.  This
potentially allows more meaningful debugging if NDNS or other tools happen to crash.

If it is undesirable, default flags can be easily overridden.  The following example shows
how to completely disable debug symbols and configure NDNS to be installed into ``/usr``
with configuration in ``/etc`` folder.

::

    CXXFLAGS="-O2" ./waf configure --prefix=/usr --sysconfdir=/etc
    ./waf
    sudo ./waf install

Building documentation
----------------------

NDNS tutorials and API documentation can be built using the following commands:

::

    # Full set of documentation (tutorials + API) in build/docs
    ./waf docs

    # Only tutorials in `build/docs`
    ./waf sphinx

    # Only API docs in `build/docs/doxygen`
    ./waf doxgyen


Manpages are automatically created and installed during the normal build process
(e.g., during ``./waf`` and ``./waf install``), if ``python-sphinx`` module is detected
during ``./waf configure`` stage.  By default, manpages are installed into
``${PREFIX}/share/man`` (where default value for ``PREFIX`` is ``/usr/local``). This
location can be changed during ``./waf configure`` stage using ``--prefix``,
``--datarootdir``, or ``--mandir`` options.

For more details, refer to ``./waf --help``.
