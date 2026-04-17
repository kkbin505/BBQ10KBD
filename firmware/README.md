# BBQ10KBD Firmware (ESP32-S3)

This firmware turns an ESP32-S3 board into a BLE keyboard for the BlackBerry Q10 keyboard matrix.

## Implemented Today

- BLE HID keyboard output using ESP32 BLE Keyboard.
- NimBLE backend enabled for better ESP32-S3 compatibility.
- Matrix scan for 7 rows x 5 columns.
- Long-press fallback mapping for broken ROW7 keys:
  - long press `D` -> `F`
  - long press `H` -> `J`
  - long press `L` -> `K`
- Shift support (left or right shift state affects output letters).
- Enter key mapping to HID Return.
- Backspace key mapping fixed to matrix `ROW4/COL5`.
- Symbol key support using the symbol layer table.

## Hardware Notes

- If ROW7 is physically open (no continuity), `F/J/K` cannot be read directly.
- The long-press fallback is a software workaround for that hardware issue.

## Build and Upload

From this `firmware` folder:

```bash
pio run
pio run --target upload
```

## BLE Pairing

1. Upload firmware and reboot the board.
2. Pair with BLE device name: `BBQ10-BLE2`.
3. Open a text box and test typing.
4. If connected but typing does not work, remove old pairing and pair again.
