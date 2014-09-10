Database Management Hints
=========================

Pre-requisites
--------------

``sqlite3`` installed


Create the database
-------------------

Set the attribute ``dbfile`` in the NDNS configuration file, e.g., ``${SYSCONFDIR}/ndn/ndns.conf`` (``/etc/ndn/ndns.conf``), to the desired path of the database file.
By default, ``dbfile`` is assigned to ``${LOCALSTATEDIR}/lib/ndns/ndns.db`` (``/var/lib/ndns/ndns.db``).

When NDNS started, an empty database will be automatically created.

Reset the database
------------------

Remove the database file.
