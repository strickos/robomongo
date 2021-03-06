TEMPLATE = subdirs

SUBDIRS = shell \
          core \
          gui  \
          app

# Print OS and CPU architecture
contains(QMAKE_HOST.arch, x86_64) {
    win32:message("Windows x64/x86_64 (64bit) build")
    unix:message("Linux x64/x86_64 (64bit) build")
} else {
    win32:message("Windows x86 (32bit) build.")
    unix:message("Linux x86 (32bit) build.")
}
