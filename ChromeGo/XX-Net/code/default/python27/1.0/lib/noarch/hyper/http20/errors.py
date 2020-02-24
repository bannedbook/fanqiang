# -*- coding: utf-8 -*-
"""
hyper/http20/errors
~~~~~~~~~~~~~~~~~~~

Global error code registry containing the established HTTP/2 error codes.
The registry is based on a 32-bit space so we use the error code to index into
the array.

The current registry is available at:
https://tools.ietf.org/html/rfc7540#section-11.4
"""

NO_ERROR =            {'Name': 'NO_ERROR',
                       'Code': '0x0',
                       'Description': 'Graceful shutdown'}
PROTOCOL_ERROR =      {'Name': 'PROTOCOL_ERROR',
                       'Code': '0x1',
                       'Description': 'Protocol error detected'}
INTERNAL_ERROR =      {'Name': 'INTERNAL_ERROR',
                       'Code': '0x2',
                       'Description': 'Implementation fault'}
FLOW_CONTROL_ERROR =  {'Name': 'FLOW_CONTROL_ERROR',
                       'Code': '0x3',
                       'Description': 'Flow control limits exceeded'}
SETTINGS_TIMEOUT =    {'Name': 'SETTINGS_TIMEOUT',
                       'Code': '0x4',
                       'Description': 'Settings not acknowledged'}
STREAM_CLOSED =       {'Name': 'STREAM_CLOSED',
                       'Code': '0x5',
                       'Description': 'Frame received for closed stream'}
FRAME_SIZE_ERROR =    {'Name': 'FRAME_SIZE_ERROR',
                       'Code': '0x6',
                       'Description': 'Frame size incorrect'}
REFUSED_STREAM =      {'Name': 'REFUSED_STREAM ',
                       'Code': '0x7',
                       'Description': 'Stream not processed'}
CANCEL =              {'Name': 'CANCEL',
                       'Code': '0x8',
                       'Description': 'Stream cancelled'}
COMPRESSION_ERROR =   {'Name': 'COMPRESSION_ERROR',
                       'Code': '0x9',
                       'Description': 'Compression state not updated'}
CONNECT_ERROR =       {'Name': 'CONNECT_ERROR',
                       'Code': '0xa',
                       'Description': 
                       'TCP connection error for CONNECT method'}
ENHANCE_YOUR_CALM =   {'Name': 'ENHANCE_YOUR_CALM',
                       'Code': '0xb',
                       'Description': 'Processing capacity exceeded'}
INADEQUATE_SECURITY = {'Name': 'INADEQUATE_SECURITY',
                       'Code': '0xc',
                       'Description': 
                       'Negotiated TLS parameters not acceptable'}
HTTP_1_1_REQUIRED =   {'Name': 'HTTP_1_1_REQUIRED', 
                       'Code': '0xd',
                       'Description': 'Use HTTP/1.1 for the request'}

H2_ERRORS = [NO_ERROR, PROTOCOL_ERROR, INTERNAL_ERROR, FLOW_CONTROL_ERROR,
             SETTINGS_TIMEOUT, STREAM_CLOSED, FRAME_SIZE_ERROR, REFUSED_STREAM,
             CANCEL, COMPRESSION_ERROR, CONNECT_ERROR, ENHANCE_YOUR_CALM,
             INADEQUATE_SECURITY, HTTP_1_1_REQUIRED]

def get_data(error_code):
    """
    Lookup the error code description, if not available throw a value error
    """
    if error_code < 0 or error_code >= len(H2_ERRORS):
        raise ValueError("Error code is invalid")

    name = H2_ERRORS[error_code]['Name']
    number = H2_ERRORS[error_code]['Code']
    description = H2_ERRORS[error_code]['Description']

    return name, number, description
