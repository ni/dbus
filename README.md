
# DBus

*A lightweight interprocess communication system.*

---

 **[❮ Website ❯][Website]**
 **[❮ Contribute ❯][Contribute]**
 **[❮ FAQ ❯][FAQ]**
 **[❮ Mailing List ❯][Mailing List]**
 **[❮ Authors ❯][Authors]**

---

<br>

## Topics

**[<kbd> Introduction </kbd>](#Introduction)** 
**[<kbd> Use Cases </kbd>](#Use-Cases)** 
**[<kbd> API Levels </kbd>](#API-Levels)** 
**[<kbd> Versioning </kbd>](#Versioning)**

**[<kbd> Configuration </kbd>](#Configuration)** 
**[<kbd> Stability </kbd>](#Stability)** 
**[<kbd> Bootstrapping </kbd>](#Bootstrapping)**

<br>

---

<br>

## Introduction

**D-Bus** is a simple system for ***inter-process communication*** and ***coordination***.

The coordination part is provided in <br>
the form of a `bus daemon` that can:
- Start services on demand
- Support single-instance apps
- Notify others when an app exits

<br>

---

<br>

## Use Cases

**D-Bus** was designed as a `System Bus` / `Desktop Session Bus`.

If your use-case isn't one of these, **D-Bus** <br>
may still be useful, but only by accident.

***If so, should evaluate carefully whether*** <br>
***D-Bus makes sense for your project***.

For more information, check our the **[Specification][Specification]** / **[FAQ][FAQ]**

<br>

---

<br>

## Security


If you find a not yet discovered **Security Vulnerability**, <br>
please report it ***privately*** to:
- Either [`dbus-security@lists.freedesktop.org`](mailto:dbus-security@lists.freedesktop.org)
- Or as a **[Gitlab Issue][Issue]** and mark it as `confidential`

<br>

### On Unix

The **System Bus** *`dbus-daemon --system`* <br>
*is designed to be a security boundary* <br>
*between users with different privileges.*

The **Session Bus** *`dbus-daemon --session`* <br>
*is designed to be used by a single* <br>
*user and only accessible by that user.*

<br>

### On Windows

We do not currently consider **D-Bus** on **Windows** <br>
to be security-supported and do not recommend <br>
allowing untrusted users to access it via **TCP**.

<br>

---

<br>

## API Levels


A core concept of the **D-Bus Implementation** is <br>
that `libdbus` is intended to be a **Low-Level API**.

### Bindings

**Most Programmers** are intended to use the <br>
bindings to **GLib**, **Qt**, **Python**, **Mono**, **Java**, ...

*These bindings have varying levels of completeness and are* <br>
*maintained as separate projects from the main* ***D-Bus*** *package.*

### Main Package

The main **D-Bus Package** contains:
- Commandline tools such as `dbus-launch`
- The low-level `libdbus`
- The bus daemon

***If you use the low-level API directly, you're signing up for some pain.***

Think of the low-level API as analogous to **Xlib** or **GDI**, <br>
and the high-level API as analogous to **Qt** / **GTK+** / **HTML**.

<br>

---

<br>

## Versioning


**D-Bus** uses the common `Linux kernel` versioning <br>
system, where minor versions are distinguished by:

| Version Number |         Type         |               Example                 |
|----------------|----------------------|---------------------------------------|
|      Even      |        Stable        | `1.0` `1.0.1` `1.0.2` `1.2.1` `1.2.3` |
|      Odd       | Development Snapshot |    `1.1.1` `1.1.2` `1.1.3` `1.3.4`    |

All versions before `1.0` are considered **Development Snapshots**.

### Development Snapshots

These version make no ***ABI Stability Guarantees*** <br>
for new **ABI** introduced since the previous stable <br>
release and are likely to have more bugs.

<br>

---

<br>

## Configuration

**D-Bus** can be build with either **AutoTools** or **CMake**.

*Optionally use additional configuration flags.*

<br>

### AutoTools

**D-Bus** requires **GNU Make** or a `make` <br>
implementation with compatible extensions.

***BSD Systems*** *typically use `gmake`.*


##### Setup

Initiate the project with:

```sh
./configure
```

##### Flags

Check for available configuration options with:

```sh
./configure --help
```

<br>

### CMake


##### Setup

Initiate the project with:

```sh
cmake
```

##### Flags

Check the [`README.cmake`][CMake] file for <br>
available configuration options.

<br>

---

<br>

## Stability

As of version `1.0`, the objective has been to indefinitely sustain <br>
the working dynamic linking process of applications to `libdbus`.

- The protocol will not be modified.

- The protocol can be modified with extensions.

- If the library API is becomes incompatibly it will be **[renamed]** <br>
  to always be able to compile against and use the older API <br>
  and have apps be provided with the right version.

### Interfaces

Interfaces can be added that will provide both new <br>
functions as well as types in `libdbus`, as well as <br>
methods to applications by the bus daemon.

<br>
<br>

The above policy is intended to make **D-Bus** as ***API - stable*** <br>
as other widely used libraries, such as **GTK+**, **Qt** or **XLib**.

*If you have questions or concerns, you are* <br>
*welcome to post them on the* ***[Mailing List]*** *.*

<br>

### ABI Changes

**ABI**s found in stable releases are frozen.

`1.2.0` <- **No Change** -> `1.2.5`

`1.2.x` <- **Possible Change** -> `1.4.x`

<br>

### Static Linking

We are not yet firmly freezing all runtime <br>
dependencies of the `libdbus` library.

*As an example, the library may read certain* <br>
*files as part of its implementation, and these* <br>
*files may move around between versions.*

As a result, ***we don't yet recommend statically linking to `libdbus`***.

#### Reimplementations

Reimplementations of the protocol that <br>
are made from scratch might have to work <br>
to stay in sync with how `libdbus` behaves.

#### Locking Requirements

To lock things down and declare static linking and reimplementation <br>
to be safe, we'd like to see all the internal dependencies of `libdbus` <br>
well-documented in the specification, and we'd like to have a high <br>
degree of confidence that these dependencies are supportable <br>
over the long term and extensible where required.

<br>

### High-Level Bindings

Note that the high-level bindings are ***separate projects*** <br>
from the main **D-Bus Package**, and have their own:
- Release Cycles
- Levels of Maturity
- ABI Stability Policies

*Please consult the documentation for your binding.*

<br>

---

<br>

## Bootstrapping

*D-Bus on new platforms.*


A full build of **D-Bus**, with all regression <br>
tests enabled and run, depends on **GLib**.

A full build of **GLib**, with all regression <br>
tests enabled and run, depends on **D-Bus**.


*To break this cycle, don't enable full test coverage for* <br>
*at least one of the two projects during bootstrapping.*

You can rebuild with full test coverage after you <br>
have built both **D-Bus** and **GLib** at least once.


<!----------------------------------------------------------------------------->

[Specification]: https://dbus.freedesktop.org/doc/dbus-specification.html
[Mailing List]: http://lists.freedesktop.org/mailman/listinfo/dbus/
[Website]: http://www.freedesktop.org/software/dbus/
[FAQ]: https://dbus.freedesktop.org/doc/dbus-faq.html

[Contribute]: ./CONTRIBUTING.md
[Authors]: ./AUTHORS
[CMake]: ./README.cmake

[Renamed]: http://ometer.com/parallel.html
[Issue]: https://gitlab.freedesktop.org/dbus/dbus/issues/new
