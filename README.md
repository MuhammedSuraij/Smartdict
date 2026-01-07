# SmartDict  
A high-performance, version-aware dictionary implemented as a CPython C extension.

SmartDict behaves like a normal Python dictionary, but stores **all historical
values per key**, allowing time-travel style access with near-native dict speed.

---

## ✨ Features

- ⚡ **C-level performance**
- 🧠 **Built on CPython's native dict**
- 📦 **Automatic version tracking**
- 🔍 Access latest or specific versions
- 🧹 Controlled deletion (key or version)
- 🛡 Memory-safe & GC-friendly

---

## 📦 Installation

Build using MSVC (Python 3.12+):

```bash
cl /I "C:\Program Files\Python312\include" smart_dict.c ^
   /link /LIBPATH:"C:\Program Files\Python312\libs" python312.lib ^
   /OUT:smart_dict.pyd




*Day-1:
    Made the file structures and make a sample module for Python in C

*Day-2:
    Made a sample dict object and check whether it can use in Python

*Day-3:
    Add the versioning in insertion and deletion of the dictionary

*Day-4:
    made proper CPython-compliant custom mapping type with versioned values, dict-like behavior, and correct memory management.