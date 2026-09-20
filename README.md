# lua-pdfio

A thin Lua wrapper over the [PDFio](https://www.msweet.org/pdfio/) C
library for reading and writing PDF files.

## Overview

`lua-pdfio` is a thin Lua wrapper over
[PDFio](https://www.msweet.org/pdfio/), the C library that does the actual
PDF reading/writing work. Most functions map directly onto their PDFio C
counterparts (e.g. `pdfioFileGetAuthor` becomes `file:get_author()`), so the
[PDFio documentation](https://www.msweet.org/pdfio/pdfio.html) is the best
reference for what each call does and why; this module mostly just adapts
argument passing and object lifetimes to Lua idioms.

## Installation

```bash
make
sudo make install
```

## Usage

```lua
local pdfio = require("lua-pdfio")
```

### Reading a PDF

```lua
local pdf, err = pdfio.open('input.pdf')
if not pdf then
	error('Failed to open PDF: ' .. tostring(err))
end

print('Title:  ' .. (pdf:get_title() or '(none)'))
print('Author: ' .. (pdf:get_author() or '(none)'))
print('Pages:  ' .. pdf:get_num_pages())

pdf:close()
```

### Creating a PDF

```lua
local pdf = pdfio.create('hello.pdf', '1.7')
local font = pdf:create_font_obj_from_base('Helvetica')

local dict = pdf:dict_create()
dict:add_font('F1', font)

local page = pdf:create_page(dict)
page:text_begin()
page:set_text_font('F1', 24)
page:text_move_to(72, 700)
page:text_show(false, 'Hello, World!')
page:text_end()
page:close()

pdf:close()
```

### More examples

The `example/` directory has fuller, runnable Lua ports of PDFio's own C
example programs:

- `lua-pdfinfo.lua` — read and print a PDF's metadata
- `lua-pdf2text.lua` — extract text from a PDF
- `lua-image2pdf.lua` — create a PDF page from an image with a caption
- `lua-md2pdf.lua` — convert a Markdown file to PDF

## Testing

```bash
make test       # functional tests
make memcheck    # valgrind: leaks, invalid reads/writes, double frees
make sanitize    # AddressSanitizer + UndefinedBehaviorSanitizer
make memtest     # both of the above
```

`memcheck` and `sanitize` build separate, unoptimized copies of the module
with debug symbols and run the same Lua test suite through valgrind /
ASan+UBSan respectively, so any memory-safety bug hit while exercising the
bindings fails the build with a real stack trace instead of surfacing as a
flaky test or a silent corruption.

`memcheck` relies on valgrind, which on some systems needs debug symbols
for glibc's dynamic linker to start up at all. If `make memcheck` fails
with an error like "a function redirection ... cannot be set up", install
your distribution's debug symbol package (e.g. `glibc-debug` on
Fedora/RHEL, `libc6-dbg` on Debian/Ubuntu) or point it at your distro's
debuginfod server, e.g.:

```bash
DEBUGINFOD_URLS=https://debuginfod.archlinux.org make memcheck   # Arch
DEBUGINFOD_URLS=https://debuginfod.fedoraproject.org make memcheck  # Fedora
DEBUGINFOD_URLS=https://debuginfod.debian.net make memcheck      # Debian
```

`sanitize` has no such requirement.

## License

This project is licensed under the MIT License.
