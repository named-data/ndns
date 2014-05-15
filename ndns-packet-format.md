.. _packetFormat:

Query
-----                

Requests to the zone are made in the format of a standard NDN interest.

See :ref `Interest section <http://named-data.net/doc/ndn-tlv/interest.html>`_ for details.


Answer
------

.. _data:

    Data ::= DATA-TLV TLV-LENGTH
        Name
        MetaInfo
        Content
        Signature
        
Every response from the NDNS system comes in the same generic form.
Specialization of different types of NDNS responses is accomplished through
``NDNS Type`` field of MetaInfo.

See :ref `NDNS Type section` <NDNS-Type> for details


Name
~~~~
See :ref:`Name section <Name>` for details.


.. _MetaInfo

MetaInfo
~~~~~~~~

MetaInfo specified here represents portions that are defaultly specified in any 
NDN packet with NDNS specific additions. 

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
                      
For an NDNS packet the ContentType should always be BLOB (=0), specificity is 
accomplished by NDNS-Type.

See :ref: `NDNS-Type` for details


FreshnessPeriod
+++++++++++++++

::

        FreshnessPeriod ::= FRESHNESS-PERIOD-TLV TLV-LENGTH 
                          nonNegativeInteger
                          
The optional freshness period specifies how long after the arrive of the data 
the recieving node should wait before marking it stale.

See `NDN Packet Specification - Freshness Period <http://named-data.net/doc/ndn-tlv/data.html#freshnessperiod>`_


.. _NDNS-Type:

NDNS-Type
+++++++++

::
  
        NDNS-Type ::= NDNS-TYPE-TLV TLV-LENGTH
                    nonNegativeIntegear
                    
There are 3 distinct types of NDNS Responses:
+-----------+------------------------------------------------------------------+
|    Type   | Description                                                      |
+===========+==================================================================+    
| NDNS Data | indicates an answer was found, and that contains the requested   | 
|           | content                                                          |
+-----------+------------------------------------------------------------------+   
| NDNS NACK | indicates that the responding zone has no content matching the   |
|           | query.                                                           |
+-----------+------------------------------------------------------------------+
| NDNS AUTH | indicates that the zone has content, but a more specific         |
|           | question is need to determine the correct content to server.     |
+-----------+------------------------------------------------------------------+

.. _AnswerCount:

AnswerCount
++++++++++++

::
    
            AnswerCount ::= ANSWER-COUNT-TLV TLV-LENGTH
                    nonNegativeIntegear
                    
This field is used to specify the number of answer in a response.


Content
~~~~~~~

::

            Content ::= CONTENT-TYPE TLV-LENGTH BYTE*
            
Signature
~~~~~~~~~

::

            Signature ::= SignatureInfo
                          SignatureBits
                          
See `NDN Signature Specification - Signature <http://named-data.net/doc/ndn-tlv/signature.html>`_
            
            
