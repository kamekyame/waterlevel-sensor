# Water Level Sensor

## 使用マイコン

ESP-WROOM-02

### メーカプロダクトページ

https://www.espressif.com/en/support/documents/technical-documents

### データシート

https://www.espressif.com/en/support/documents/technical-documents?keys=esp-wroom-02

## 書き込み方法

### USBシリアルで書き込み

IO0をPull downにしてリセットすると書き込める

### Arduino OTA(Wifi経由)で書き込み

`arduino.json`の`port`をESPのIPアドレスに変更して書き込み

## 起動方法

IO0をPull upしてリセットするとフラッシュの内容が実行される。

## Tips

- IO2をプルアップするとシリアル通信が安定する（内部プルアップになってるはずなんだけど…）
- あとの回路はデータシートのまますればOK
