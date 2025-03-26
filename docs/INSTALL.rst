Installation Instructions
=========================

Prerequisites
-------------

Install the `ndn-cxx library <https://docs.named-data.net/ndn-cxx/current/INSTALL.html>`__
and its prerequisites.

Optionally, to build man pages and API documentation the following additional dependencies
need to be installed:

- doxygen
- graphviz
- sphinx
- sphinxcontrib-doxylink

Build
-----

The following commands should be used to build NDNS on Ubuntu:

.. code-block:: sh

    ./waf configure
    ./waf
    sudo ./waf install

Refer to ``./waf --help`` for more options that can be used during the ``configure`` stage.

Debug symbols
+++++++++++++

The default compiler flags include debug symbols in binaries. This should provide
more meaningful debugging information if NDNS or other tools happen to crash.

If this is not desired, the default flags can be overridden to disable debug symbols.
The following example shows how to completely disable debug symbols and configure
NDNS to be installed into ``/usr`` with configuration in the ``/etc`` directory.

.. code-block:: sh

    CXXFLAGS="-O2" ./waf configure --prefix=/usr --sysconfdir=/etc
    ./waf
    sudo ./waf install

Building documentation
----------------------

Tutorials and API documentation can be built using the following commands:

.. code-block:: sh

    # Full set of documentation (tutorials + API) in build/docs
    ./waf docs

    # Only tutorials in build/docs
    ./waf sphinx

    # Only API docs in build/docs/doxygen
    ./waf doxygen

If ``sphinx-build`` is detected during ``./waf configure``, man pages will automatically
be built and installed during the normal build process (i.e., during ``./waf`` and
``./waf install``). By default, man pages will be installed into ``${PREFIX}/share/man``
(the default value for ``PREFIX`` is ``/usr/local``). This location can be changed
during the ``./waf configure`` stage using the ``--prefix``, ``--datarootdir``, or
``--mandir`` options.

For further details, please refer to ``./waf --help``.
