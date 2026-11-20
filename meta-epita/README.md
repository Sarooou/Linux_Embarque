# Station météo sur Banana Pi M2 Zero (Yocto)


## Matériel

| Composant | Rôle | Bus | Adresse I2C |
|---|---|---|---|
| Banana Pi M2 Zero | Carte | – | – |
| BME280 | Température, humidité, pression | `i2c0` | 0x76 |
| Écran OLED SSD1306 | Affichage | `i2c1` | 0x3c |
| Adaptateur USB-série 3,3 V | Console de débogage | – | – |


### Câblage

Brochage du connecteur 40 broches de la carte (compatible Raspberry Pi) :

| Composant | Alimentation | Masse | SDA | SCL |
|---|---|---|---|---|
| BME280 | broche 1 | broche 6 | broche 3 | broche 5 |
| Écran SSD1306 | broche 17 | broche 9 | broche 27 | broche 28 |


## Contenu du layer `meta-epita`

```
meta-epita/
├── README.md
├── COPYING.MIT
├── conf/
│   ├── layer.conf
│   └── machine/
│       ├── bananapi-m2-zero.conf        Machine
│       └── include/{sun8i.inc, sunxi.inc}
├── files/wic/sunxi-sdcard-image.wks.in  Partitionnement de la carte SD
├── recipes-bsp/u-boot/
│   ├── u-boot_%.bbappend
│   └── files/arm/boot.cmd               Script de démarrage U-Boot
├── recipes-kernel/linux/
│   ├── linux.inc, linux-mainline.inc, linux-mainline_6.18.40.bb
│   └── linux-mainline/
│       ├── arm/defconfig
│       ├── axp20x.cfg, bme280.cfg       Fragments de configuration
│       └── patches/                     Correctifs (MMC, Device Tree)
├── recipes-support/libssd1306/          Bibliothèque de l'écran
├── recipes-apps/weather-station-app/    Programme et script d'init
└── recipes-core/images/weather-station-image.bb
```

### Correctifs du noyau

| Correctif | Rôle |
|---|---|
| `0003-dts-allwinner-bananapi-m2-zero-Enforce-consistent-MM.patch` | Fixe la numérotation MMC : sans lui, la carte SD peut être vue comme `mmcblk1`, ce qui empêche le démarrage |
| `0004-dts-enable-i2c0-bme280-and-i2c1.patch` | Déclare le BME280 sur `i2c0` et active le bus `i2c1` |


## Construire l'image

### Prérequis

Le dossier de travail contient les dépôts suivants, clonés côte à côte, et le conteneur fourni pour le cours :

Dépôts :

- openembedded-core
- bitbake
- meta-openembedded
- meta-arm
- meta-epita 


## Flasher la carte SD

En dehors du conteneur. Vérifier la lettre de la carte avec `lsblk`.

```
lsblk -o NAME,SIZE,MODEL,TRAN,RM
sudo umount /dev/sdX1 /dev/sdX2 2>/dev/null
cd build/tmp/deploy/images/bananapi-m2-zero
sudo bmaptool copy weather-station-image-bananapi-m2-zero.rootfs.wic.gz /dev/sdX
sync
sudo eject /dev/sdX
```


## Utilisation

### Console série

Adaptateur USB-série branché directement sur le PC :

```
sudo minicom -b 115200 -D /dev/ttyUSB0
```

Connexion : `root`, sans mot de passe.


### Vérifier le matériel

```
cat /sys/bus/iio/devices/iio:device0/name        # bme280
cat /sys/bus/iio/devices/iio:device0/in_temp_input
i2cdetect -y 0                                   # UU à 76 (capteur)
i2cdetect -y 1                                   # 3c (écran)
```


### Le programme

```
weather-station --preview                        # rendu en ASCII, sans écran
SSD1306_I2C_DEV=/dev/i2c-1 weather-station       # affichage sur l'écran, Ctrl-C pour arrêter
```

L'écran affiche quatre lignes : température, humidité, pression et heure.


### Démarrage automatique

Un script d'init BusyBox, `/etc/init.d/weather-station`, lance le programme en arrière-plan avec `start-stop-daemon`.