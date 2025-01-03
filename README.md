# test-monitor

Test project for monitoring GPS and Bluetooth temperature beacons on Particle Boron.

Code designed to be compiled using WSL by running `make`. Must have Windows `particle.exe`
CLI installed for flashing (`make flash`).

## Pre-requisites

```bash
sudo apt-get install -y git bash curl bats bzip2 isomd5sum jq libarchive-zip-perl make zip wget parallel gnupg cmake xxd
```

## Particle CLI

See [Particle CLI](https://docs.particle.io/getting-started/developer-tools/cli/).

For Windows, install [ParticleCLISetup.exe](https://binaries.particle.io/particle-cli/installer/win32/ParticleCLISetup.exe).
