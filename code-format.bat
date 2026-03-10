@echo off
for /r %%f in (*.c *.h) do (
    clang-format -i -style=file "%%f"
)