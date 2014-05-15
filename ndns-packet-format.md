.. _packetFormat:

Query
-----                

    Requests to the server are made in the format of a standard NDN interest packet. The question is replaced with a standard request for content. See :ref `Interest section <Interest>` for details.

Answer
------

.. _data:
    Data ::= DATA-TLV TLV-LENGTH
        Name
        MetaInfo
        Content
        Signature
        
Every response from the NDNS system comes in the same generic form.
The specialization is accomplished by setting values in the MetaInfo structure.


Name
~~~~
See :ref:`Name section <Name>` for details.

.. _MetaInfo

MetaInfo
~~~~~~~~

MetaInfo specified here represents portions that are defaultly specified in any NDN packet, which NDNS specific additions. 
See `NDN Packet Specification - Meta-Info <http://named-data.net/doc/ndn-tlv/data.html#metainfo>`_

MetaInfo ::= META-INFO-TYPE TLV-LENGTH
        ContentType
        FreshnessPeriod?
        NDNS-Type
        AnswerCount
        
ContentType
+++++++++++

::

        ContentType ::= CONTENT-TYPE-TYPE TLV-LENGTH 
                      nonNegativeInteger
                      
For an NDNS packet the ContentType should always be BLOB (=0), specificity is accomplished by NDNS-Type.

See :ref: `NDNS-Type` for details

FreshnessPeriod
+++++++++++++++

::


        FreshnessPeriod ::= FRESHNESS-PERIOD-TLV TLV-LENGTH 
                          nonNegativeInteger
                          
The optional freshness period specifies how long after the arrive of the data the recieving node should wait before marking it stale.

See `NDN Packet Specification - Freshness Period <http://named-data.net/doc/ndn-tlv/data.html#freshnessperiod>`_

.. _NDNS-Type:

NDNS-Type
+++++++++

::

    
        NDNS-Type ::= NDNS-TYPE-TLV TLV-LENGTH
                    nonNegativeIntegear
                    
There are 4 distinct types of NDNS Responses:
    
    *NDNS Data*- a traditional response which answers which contains the data corresponding to the NDNS Query.
    *NDNS NACK* - indicates that the responding zone has no content matching the query
    *NDNS AUTH* - indicates that the zone has content, but a more specific question is need to determine the correct content to server.
    
.. _Content-Length:

AnswerCount
++++++++++++

::

    
            AnswerCount ::= ANSWER-COUNT-TLV TLV-LENGTH
                    nonNegativeIntegear
                    
This field is used to specify the number of answer in a response.


Content
~~~~~~~
