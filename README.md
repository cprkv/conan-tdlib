# Conan Telegram Database library package

## Installation

Add remote to get ckristen packages from bintray:  
`conan remote add ckristen https://api.bintray.com/conan/ckristen/conan`

Add dependency to your project:  
`tdlib/1.3.0@ckristen/master`

## Extra dependencies

On linux requires tools: makedepend, gperf. Commands to install it:  
`sudo apt-get install xutils-dev gperf`

### Environment (Linux)

Ensure you has properly defined flags in conan profile (default profile located in `~/.conan/profiles/default`):
```
compiler.libcxx=libstdc++11
cppstd=14
```

`libstdc++11` is required, because telegram core library requires `-D_GLIBCXX_USE_CXX11_ABI=1`.  
If you preffer not to set it in profile, you should define compilation flag yourself.

## ToDo

- Tests on different OS
