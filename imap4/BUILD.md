# Build

This folder keeps the original Visual Studio 2005 project files intact.
For current Visual Studio/MSBuild, use the added modern solution:

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" imap4-modern.sln /p:Configuration=ReleaseNoSSL /p:Platform=x64
```

The verified configuration is `ReleaseNoSSL|x64`. It disables SSL and LDAP integration so that the source can be built with the dependencies currently present in this folder. It builds the executable to:

```text
ReleaseNoSSL\Epstimap4s.exe
```

Notes:

- `WIN32_LEAN_AND_MEAN` is defined to avoid Windows SDK 10.0.26100 anonymous-union errors from unused headers.
- `NO_LDAP` is defined to avoid ADSI/LDAP SDK headers in the no-SSL build. Remove it if you need LDAP support and are ready to address the current SDK header requirements.
- `USE_SSL` is intentionally not defined in this configuration because the repository currently has OpenSSL libraries under `..\rel64` / `..\openssl-3.5.6`, but it does not contain the matching generated public headers such as `include\openssl\ssl.h`.
- To restore the SSL build, install or place matching OpenSSL public headers next to the libraries, then add `USE_SSL` plus the OpenSSL include/library paths to `imap4-modern.vcxproj`.
