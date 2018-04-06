# River.js

River.js is a Node.js-like thing, except focused on providing only lower-level
primitives and supporting only es modules.

This is a work-in-progress. It's probably crashy and not usable for anything
yet, but feel free to follow along as I try to make this into something maybe
usable eventually.

## Building

You must have the following things installed before building:

- [libuv](http://libuv.org/) 1.19+
- [V8](https://chromium.googlesource.com/v8/v8.git) 6.5.x
- [buck](https://buckbuild.com/)

To build, simply do:

```sh
make test
```
