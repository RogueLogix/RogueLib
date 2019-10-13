# While this is C++, the idea is that it can be decoded in any language with the correct decoder, and on any CPU endianness
add_lib(GenericBinary)
add_lib_dependency(Exceptions)
add_external_lib_dependency(Boost::boost)