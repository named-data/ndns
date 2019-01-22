Database Management Hints
=========================

Pre-requisites
--------------

``sqlite3`` installed

Create the database
-------------------

Set the attribute ``dbFile`` in the NDNS configuration file, e.g., ``${SYSCONFDIR}/ndn/ndns/ndns.conf``
(``/etc/ndn/ndns/ndns.conf``), to the desired path of the database file. By default, ``dbFile`` is equal
to ``${LOCALSTATEDIR}/lib/ndn/ndns/ndns.db`` (``/var/lib/ndn/ndns/ndns.db``).

When NDNS started, an empty database will be automatically created.

Reset the database
------------------

Remove the database file.
