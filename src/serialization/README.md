# Native particle codec

`opensim::serialization` implements the accepted IO-001 native v1 byte contract.
It depends only on core/math/scene and builds with the app disabled. Decode bounds
the entire layout before allocation or record access, then delegates semantic
validation to scene. No filesystem or UI operations, compression or new dependency.

This is the first particle-format slice, not a SimPHY importer or finished product
format. The 4096-particle limit belongs to v1. Future versions require reviewed
evolution. Save/Open UI and safe file replacement are separate prerequisites.
