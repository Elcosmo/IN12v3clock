# Changelog

## Unreleased / RC9

- Added DS3231 weekday support with convention `1=lunedi` through `7=domenica`.
- Added configurable weekday/weekend display schedule on menu `3..10`.
- Removed the legacy night mode menu and behavior.
- Reused the old night-mode EEPROM bytes for the 8 schedule hours.
- Added schedule EEPROM version byte at logical address `17` / physical `0x4011`.
- Added migration from the old EEPROM format to RC9 schedule defaults.
- Kept normal brightness, RGB global state, RGB colors, colon mode and unrelated
  saved settings across migration.
- Added HC595 safe boot so visible outputs are latched off before enable.
- Added strict RTC validation so invalid DS3231 data is not displayed.
- Added true digital colon output: menu `11 = 0` is 1 second ON / 1 second OFF.
- Added schedule gates for Nixie, RGB and colon outputs.
- Added host tests for schedule, EEPROM migration/menu writes, DS3231 protocol,
  S19 validation, visible-output gates, safe boot and RTC recovery.
- Added Docker/SDCC CI workflow for host tests, firmware build, S19 validation
  and Flash-size limit checking.
