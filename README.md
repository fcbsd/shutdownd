###shutdownd

A small daemon for OpenBSD that will watch apm status and run commands when
the machine is off A/C power and the battery's "minutes remaining" calculation
falls below certain values.

By default, when the battery reports 25 minutes remaining, `shutdownd` will
run [yad](http://yad.googlecode.com/), an X11 dialog utility, warning about the
low battery.  When the battery reports 5 minutes remaining, `shutdownd` will
run `sudo halt -p`.

While this utility does daemonize by default, it should be run as a normal user
during an X session in order for `yad` to properly display an X window.

####Options

* `-v`: Verbose - don't daemonize, report progress to STDOUT.  Commands are
still run.

* `-w [minutes]`: Minutes remaining when warn command is run.  Defaults to 25.

* `-W [command]`: Command to run when warning, passed to `sh -c`.  Defaults to
a `yad` command displaying the amount of time left before shutdown.

	`$battery_minutes` will be expanded in the command text to the actual minutes
	remaining, followed by "minute" or "minutes".

	`$shutdown_minutes` will be expanded in the command text to the amount of
	minutes before the shutdown command is run, followed by "minute" or "minutes".

* `-s [minutes]`: Minutes remaining when shutdown command is run.  Defaults to
5.

* `-S [command]`: Command to run when shutting down, passed to `sh -c`.
Defaults to `doas halt -p`.  `doas.conf` should allow access to halt without a
password (`permit nopass :%wheel cmd /sbin/halt`) for this to work.

####License

	Copyright (c) 2012 joshua stein <jcs@jcs.org>

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	1. Redistributions of source code must retain the above copyright
	   notice, this list of conditions and the following disclaimer.
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	3. The name of the author may not be used to endorse or promote products
	   derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
	IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
	IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
	NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
	THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
