# Security Policy

## Supported versions

ext-comrec is pre-1.0. No 0.x release receives security fixes or advisories; fixes land in
the next release line. Security support starts with 1.0.

| Version | Security fixes |
|---------|----------------|
| < 1.0   | No             |

## Reporting a vulnerability

Please don't open a public issue for a security problem.

Report it privately through GitHub: the **Report a vulnerability** button on the
[php-io-extensions/comrec](https://github.com/php-io-extensions/comrec) repository's
**Security** tab. If that isn't available, email **info@projectsaturnstudios.com**.

Include what you found, the affected version, your platform and PHP build (NTS or ZTS), the
calls that trigger it, and steps to reproduce. Reports are read
by the maintainers and answered as soon as we can.

## What the extension touches

ext-comrec is plain C on the Zend API. It opens no file, socket or device, links no system
library, and reads and writes no memory but its own and the PHP values it is given. It draws
nothing: it records commands as PHP lists and answers rects.

- **Input.** Every number of a shape is checked as it is read: finite, and at most
  `PIXEL_QUEUE_LIMIT` once transformed. A surface, a clip, a unit and a rect are whole numbers
  within the same limit. Anything else is refused with a `PixelQueueException`, an `Error` or a
  `ValueError`, and nothing is recorded.
- **Values it holds.** The clip and the fill rule are stored and handed back untouched: the
  extension never reads a property of one or calls a method on one. Two commands are compared
  with PHP's own `==`.
- **Commands made elsewhere.** `append()` keeps the list it is given. Where it writes is read
  from its second entry only when the caller says so, and an entry that is not what was said
  gives the command no box; it is not read past.
- **Memory.** The stack of pushes, the layers and the lists of rects grow as they are used and
  are freed with the queue. A turned curve is flattened to at most 1,028 points.

A report is welcome for any way to make the queue read or write outside its own memory, or to
crash it with an argument it should have refused.
