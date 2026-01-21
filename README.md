🚀 SmartDict — Versioned Dictionary in C (CPython Extension)

📌 Overview

SmartDict is a custom CPython C-extension that enhances Python’s built-in dictionary by supporting versioned values, time-travel access, and snapshot-based state restoration.

It behaves like a normal dictionary but internally stores all historical values per key.


✨ Features

* Versioned assignments

* Access any previous value by index

* Snapshot support

* Fast C-level performance

* Memory-efficient design

* CPython-compliant reference handling


🏗 Internal Design

* Implemented as a CPython C-extension

* Uses PyDict + PyList internally

* Strict reference counting discipline

* Borrowed vs owned references handled explicitly


🧠 Learning Outcomes

* CPython object model

* Reference counting

* Borrowed vs owned references

* C-extension design

* Data structure engineering