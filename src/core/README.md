# Core values

Implements accepted core v1 using only the standard library. Include
opensim/core/values.hpp and link opensim::core. EntityId{0} is invalid;
IdAllocator belongs to one document, never a global service. reserve_through
does not validate uniqueness; scene must reject duplicate entities separately.

Result<T>::success and failure create explicit branches; value()/error() return
nullable const pointers owned by the result. Do not retain pointers beyond its
lifetime. Copy construction follows T's copyability; moving transfers ownership.
Assignment is deliberately unavailable for Result<T>, avoiding a variant becoming
valueless if payload replacement throws. Construct a new result to propagate a
diagnostic. Allocation exceptions propagate normally. Diagnostic messages/path
strings own UTF-8 bytes; producers are responsible for supplying valid UTF-8.
Code names, not enum ordinals, form the stable diagnostic vocabulary.
