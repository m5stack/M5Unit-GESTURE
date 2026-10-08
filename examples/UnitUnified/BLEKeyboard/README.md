# Example BLEKeyboard

## Overview

This is a sample that detects gestures and sends the specified key code via BLE.
The BLE HID keyboard is implemented directly on NimBLE-Arduino (`NimBLEHIDDevice`), so no other keyboard library is needed.
Clockwise sends the down arrow key and counterclockwise sends the up arrow key.

Note: Pairing uses "Just Works" bonding without a PIN, so any nearby device can pair with it while it is advertising.

## Required libraries
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)


---

## 概要
ジェスチャーを検知し、指定したキーコードをBLE経由で送信するサンプルです。
BLE HID キーボードは NimBLE-Arduino (`NimBLEHIDDevice`) で直接実装しているため、他のキーボードライブラリは不要です。
時計回りで下矢印キー、反時計回りで上矢印キーを送信します。

注意: PIN なしの "Just Works" ボンディングでペアリングするため、アドバタイズ中は近くにある任意の機器からペアリングできます。

## 必要なライブラリ
- [NimBLE-Arduino](https://github.com/h2zero/NimBLE-Arduino)
