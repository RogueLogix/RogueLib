#General
comments

#Translation

##Optimizations
Non-x86 platforms dont use special fast paths, because i dont have them to run on.

##Swap endianness durring serialization
could be done as extra step

#AutoSerialization

##Optional, required, and conditionally required
Currently, all values are optional, and no error will be raised if one is missing

If there are additional values, an error will not be raised

Sometimes things may not be always required or always optional, but required/optional/must-not-be-set based on other conditions

##Extensions
There is no good way to handle extensions where it can be casted to a lower struct/class 