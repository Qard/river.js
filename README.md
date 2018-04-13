# River.js

River.js is a Node.js-like thing, except focused on providing only lower-level
primitives and supporting only es modules.

This is a work-in-progress. It's probably crashy and not usable for anything
yet, but feel free to follow along as I try to make this into something maybe
usable eventually, or join in and help make something really cool. I'm happy
to mentor anyone that wants to contribute!

## Dependencies

You must have the following things installed before building:

- [V8](https://chromium.googlesource.com/v8/v8.git) 6.5.x
- [buck](https://buckbuild.com/)

```sh
git submodule update --init --recursive
```

Currently only libuv is vendored in `deps` as a git submodule. The plan is to
vendor V8 eventually too, but a buck build file is needed. The easiest route
is probably to just use `genrule` and exported flags to build V8 with GN, the
official way, and use some flag hackery to point to the libs and headers.

## Building

To build, simply do:

```sh
make test
```
