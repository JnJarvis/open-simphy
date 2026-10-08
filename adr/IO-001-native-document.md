# IO-001 acceptance and product requirements

User approved the native document contract and affected-consumer RFC on 2026-10-07.
Merged as 3a055ba012edeb315bc110bae9f35962a98c5425.

The user explicitly requires opening existing .sim files and every other file
format SimPHY can open. This is a product goal, not a claim of current support.
Research must establish the actual format/version inventory and behavior; do not
invent supported extensions or silently narrow the goal to the particle demo.
Importers remain separate from the native codec and need evidence-backed tests.

The user also requires a custom Open Simphy format designed with performance in
mind. The accepted compact binary v1 is the first particle-only slice, not the
finished product format or a permanent 4096-object product limit. Future scene
kinds and scale need reviewed version evolution. Implementation should measure
encode/decode time, file size and peak memory on representative workloads before
claiming performance; no benchmark target or result has been established yet.

See spec/contracts/serialization.md and rfcs/IO-001-native-document.md. No file
codec or importer implementation is authorized by this acceptance record alone;
the coordinator must scope and claim the corresponding implementation tasks.
