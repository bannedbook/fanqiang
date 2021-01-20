# -*- coding: utf-8 -*-
"""
hyper/http20/window
~~~~~~~~~~~~~~~~~~~

Objects that understand flow control in hyper.

HTTP/2 implements connection- and stream-level flow control. This flow
control is mandatory. Unfortunately, it's difficult for hyper to be
all that intelligent about how it manages flow control in a general case.

This module defines an interface for pluggable flow-control managers. These
managers will define a flow-control policy. This policy will determine when to
send WINDOWUPDATE frames.
"""
class BaseFlowControlManager(object):
    """
    The abstract base class for flow control managers.

    This class defines the interface for pluggable flow-control managers. A
    flow-control manager defines a flow-control policy, which basically boils
    down to deciding when to increase the flow control window.

    This decision can be based on a number of factors:

    - the initial window size,
    - the size of the document being retrieved,
    - the size of the received data frames,
    - any other information the manager can obtain

    A flow-control manager may be defined at the connection level or at the
    stream level. If no stream-level flow-control manager is defined, an
    instance of the connection-level flow control manager is used.

    A class that inherits from this one must not adjust the member variables
    defined in this class. They are updated and set by methods on this class.
    """
    def __init__(self, initial_window_size, document_size=None):
        #: The initial size of the connection window in bytes. This is set at
        #: creation time.
        self.initial_window_size = initial_window_size

        #: The current size of the connection window. Any methods overridden
        #: by the user must not adjust this value.
        self.window_size = initial_window_size

        #: The size of the document being retrieved, in bytes. This is
        #: retrieved from the Content-Length header, if provided. Note that
        #: the total number of bytes that will be received may be larger than
        #: this value due to HTTP/2 padding. It should not be assumed that
        #: simply because the the document size is smaller than the initial
        #: window size that there will never be a need to increase the window
        #: size.
        self.document_size = document_size

    def increase_window_size(self, frame_size):
        """
        Determine whether or not to emit a WINDOWUPDATE frame.

        This method should be overridden to determine, based on the state of
        the system and the size of the received frame, whether or not a
        WindowUpdate frame should be sent for the stream.

        This method should *not* adjust any of the member variables of this
        class.

        Note that this method is called before the window size is decremented
        as a result of the frame being handled.

        :param frame_size: The size of the received frame. Note that this *may*
          be zero. When this parameter is zero, it's possible that a
          WINDOWUPDATE frame may want to be emitted anyway. A zero-length frame
          size is usually associated with a change in the size of the receive
          window due to a SETTINGS frame.
        :returns: The amount to increase the receive window by. Return zero if
          the window should not be increased.
        """
        raise NotImplementedError(
            "FlowControlManager is an abstract base class"
        )

    def blocked(self):
        """
        Called whenever the remote endpoint reports that it is blocked behind
        the flow control window.

        When this method is called the remote endpoint is signaling that it
        has more data to send and that the transport layer is capable of
        transmitting it, but that the HTTP/2 flow control window prevents it
        being sent.

        This method should return the size by which the window should be
        incremented, which may be zero. This method should *not* adjust any
        of the member variables of this class.

        :returns: The amount to increase the receive window by. Return zero if
          the window should not be increased.
        """
        raise NotImplementedError(
            "FlowControlManager is an abstract base class"
        )

    def _handle_frame(self, frame_size):
        """
        This internal method is called by the connection or stream that owns
        the flow control manager. It handles the generic behaviour of flow
        control managers: namely, keeping track of the window size.
        """
        rc = self.increase_window_size(frame_size)
        self.window_size -= frame_size
        self.window_size += rc
        return rc

    def _blocked(self):
        """
        This internal method is called by the connection or stream that owns
        the flow control manager. It handles the generic behaviour of receiving
        BLOCKED frames.
        """
        rc = self.blocked()
        self.window_size += rc
        return rc


class FlowControlManager(BaseFlowControlManager):
    """
    ``hyper``'s default flow control manager.

    This implements hyper's flow control algorithms. This algorithm attempts to
    reduce the number of WINDOWUPDATE frames we send without blocking the remote
    endpoint behind the flow control window.

    This algorithm will become more complicated over time. In the current form,
    the algorithm is very simple:
        - When the flow control window gets less than 1/4 of the maximum size,
          increment back to the maximum.
        - Otherwise, if the flow control window gets to less than 1kB, increment
          back to the maximum.
    """
    def increase_window_size(self, frame_size):
        future_window_size = self.window_size - frame_size

        if ((future_window_size < (self.initial_window_size / 4)) or
            (future_window_size < 1000)):
            return self.initial_window_size - future_window_size

        return 0

    def blocked(self):
        return self.initial_window_size - self.window_size
