# GRID BOARD APP

A Flutter app for sending customizable text and emoji grids via BLE (Bluetooth Low Energy) to ESP32-P4 Device.  
Create, save, and reload messages in a 12×5 grid—perfect for smart displays and IoT dashboards.

---

## Features

- 12×5 customizable grid (text and emoji supported)
- Fast BLE connection (uses [flutter_blue_plus](https://pub.dev/packages/flutter_blue_plus))
- Save/load unlimited grid messages (local storage)
- Emoji picker, clear grid, auto-uppercase, and more!
- Automatic BLE reconnect and robust error handling

---

## Getting Started

### 1. **Install dependencies**
```bash
flutter pub get
```

### 2. **(Re)generate platform folders (if needed)**
If the `ios/` or `android/` folders are missing, run:
```bash
flutter create .
```

---

## Permissions & Platform Setup

### iOS

**Required: Add Bluetooth Usage Description to Info.plist**

Open `ios/Runner/Info.plist` and add:
```xml
<key>NSBluetoothAlwaysUsageDescription</key>
<string>This app uses Bluetooth to connect to your Grid Board display.</string>
```
> ⚠️ Without this, iOS will deny BLE access and your app won't see any devices.

---

### Android

**Required: Bluetooth Permissions in AndroidManifest.xml**

Add these inside `<manifest><application>...</application></manifest>`:

```xml
<uses-permission android:name="android.permission.BLUETOOTH"/>
<uses-permission android:name="android.permission.BLUETOOTH_ADMIN"/>
<uses-permission android:name="android.permission.BLUETOOTH_SCAN"/>
<uses-permission android:name="android.permission.BLUETOOTH_CONNECT"/>
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION"/>
<uses-permission android:name="android.permission.ACCESS_COARSE_LOCATION"/>
```

For **Android 12+** you must also request runtime permissions; see [flutter_blue_plus docs](https://pub.dev/packages/flutter_blue_plus#android-permissions) for examples.

---

## Running the App

```bash
flutter run
```
- The app will scan for your Grid Board BLE device (default: `"Grid_Board"`).
- Tap any grid cell and start typing or long-press to insert emoji.
- Use the Send button to transmit the grid to your ESP32.
- Save and reload grid messages for later use!

---

## Notes

- All user input is forced to uppercase except for emoji.
- Emojis are limited to those supported by both your ESP32 font and app.
- The app automatically tries to reconnect if BLE connection drops.

---

## Dependencies

- [flutter_blue_plus](https://pub.dev/packages/flutter_blue_plus)
- [shared_preferences](https://pub.dev/packages/shared_preferences)

---

## License

MIT License.

---