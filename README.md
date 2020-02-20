# gmpack

As far as I know, there are no [msgpack-RPC](https://github.com/msgpack-rpc/msgpack-rpc/blob/master/spec.md) libraries that use GLib.
I wanted to build a GTK-based Neovim frontend tailored to my tastes, and for that I would need an API client to talk to the nvim backend.
This library would have served as the backbone for communication. But then, I got bored halfway through the project and managed to
implement barely functional client/server classes before giving up. I am going to put this up here anyway, in case anyone wants to use
the code. There are some tests that demonstrate how the different classes may be used.

