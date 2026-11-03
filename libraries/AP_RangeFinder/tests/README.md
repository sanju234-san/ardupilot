Existing tests cover V1 (6‑byte) frame parsing and generic range‑finder behaviour.

## New Tests

* **test_usd1_v0_parsing.cpp** – verifies that the USD1 driver correctly parses the legacy V0 (3‑byte) protocol, including handling of frames that are split across multiple driver invocations. This test ensures that the recent bug‑fix (persistent header detection and correct frame‑length handling) works as intended.
