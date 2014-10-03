==============
vmod_saintmode
==============

----------------------
Varnish Example Module
----------------------

:Author: Dag Haavi Finstad
:Date: 2014-10-03
:Version: 1.0
:Manual section: 3

SYNOPSIS
========

import saintmode;

DESCRIPTION
===========

This VMOD implements saintmode functionality for Varnish 4.0.

Saintmode is used by instantiating a 'saintmode' object, and using
that in place of the decorated backend.

Example::

  backend b0 {
	.host = "foo";
	.port = "8080";
  }

  backend b1 {
  	.host = "bar";
  	.port = "8080";
  }

  sub vcl_init {
  	# Instantiate sm0, sm1 for backends b0, b1
  	new sm0 = saintmode.saintmode(b0, 10);
  	new sm1 = saintmode.saintmode(b1, 10);

	# Add both to a director. Use sm0, sm1 in place of b0, b1
  	new mydir = directors.random();
  	mydir.add_backend(sm0.backend(), 1);
  	mydir.add_backend(sm1.backend(), 1);
  }

  sub vcl_backend_fetch {
  	set bereq.backend = mydir.backend();
  }

  sub vcl_backend_response {
  	if (beresp.status == 500) {
  		# This marks the backend as sick for this specific
  		# object for the next 20s.
  		saintmode.blacklist(20s);
  		return (retry);
  	}
  }


FUNCTIONS
=========

saintmode.saintmode
-------------------

Prototype
        ::

	saintmode.saintmode(BACKEND b, INT threshold)
Description
	Constructs a saintmode object. The ``threshold`` argument sets
	the saintmode threshold, which is the maximum number of items
	that can be blacklisted before the whole backend is regarded
	as sick. Corresponds with the ``saintmode_threshold`` parameter
	of Varnish 3.0.

Example
        ::

                sub vcl_init {
			new sm = saintmode.saintmode(b, 10);
		}


saintmode.backend()
-------------------

Prototype
	::
	   BACKEND saintmode.backend()

Description
	Used for assigning the backend from the saintmode object.

Example
	::

		sub vcl_backend_fetch {
			set bereq.backend = sm.backend();
		}

blacklist()
-----------

Prototype
	::
	   VOID blacklist(DURATION expires)

Description
	Marks the object as sick for a specific object. Used in
	vcl_backend_response.

Example
	::

		sub vcl_backend_response {
			if (beresp.http.broken-app) {
				saintmode.blacklist(20s);
				return (retry);
			}

		}

INSTALLATION
============

This is an saintmode skeleton for developing out-of-tree Varnish
vmods available from the 3.0 release. It implements the "Hello, World!" 
as a vmod callback. Not particularly useful in good hello world 
tradition,but demonstrates how to get the glue around a vmod working.

The source tree is based on autotools to configure the building, and
does also have the necessary bits in place to do functional unit tests
using the varnishtest tool.

Usage::

 ./configure VARNISHSRC=DIR [VMODDIR=DIR]

`VARNISHSRC` is the directory of the Varnish source tree for which to
compile your vmod. Both the `VARNISHSRC` and `VARNISHSRC/include`
will be added to the include search paths for your module.

Optionally you can also set the vmod install directory by adding
`VMODDIR=DIR` (defaults to the pkg-config discovered directory from your
Varnish installation).

Make targets:

* make - builds the vmod
* make install - installs your vmod in `VMODDIR`
* make check - runs the unit tests in ``src/tests/*.vtc``

In your VCL you could then use this vmod along the following lines::
        
        import saintmode;

        sub vcl_deliver {
                # This sets resp.http.hello to "Hello, World"
                set resp.http.hello = saintmode.hello("World");
        }

HISTORY
=======

This manual page was released as part of the libvmod-saintmode package,
demonstrating how to create an out-of-tree Varnish vmod.

For further saintmodes and inspiration check out the vmod directory:

    https://www.varnish-cache.org/vmods

COPYRIGHT
=========

This document is licensed under the same license as the
libvmod-saintmode project. See LICENSE for details.

* Copyright (c) 2011-2014 Varnish Software
