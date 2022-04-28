# iVisor

## Environment

Ubuntu 21.10 on VMware Workstation 16 Player

## Package

```shell
$ sudo apt install -y make gcc mingw-w64
```

## Virtualization Technology

```shell
$ grep -E 'svm|vmx' /proc/cpuinfo
```

## Build

```shell
$ make
$ make -C boot/loader
$ make -C boot/uefi-loader
$ ls bitvisor.elf
$ ls boot/loader/bootloader
$ ls boot/loader/bootloaderusb
$ ls boot/uefi-loader/loadvmm.efi
```

## Install

```shell
$ sudo cp bitvisor.elf /boot/
$ sudo cp 99_bitvisor /etc/grub.d/99_bitvisor
$ sudo chmod +x /etc/grub.d/99_bitvisor
$ sudo update-grub2
```

## Test

```shell
$ cd tools/dbgsh
$ make
$ ./dbgsh
> log
> exit
```

## References

### BitVisor[^BitVisor]

BitVisor is a tiny hypervisor initially designed for mediating I/O access from a single guest OS. Its implementation is mature enough to run Windows and Linux, and can be used as a generic platform for various research and development projects.

https://github.com/matsu/bitvisor

[^BitVisor]: Takahiro Shinagawa, Hideki Eiraku, Kouichi Tanimoto, Kazumasa Omote, Shoichi Hasegawa, Takashi Horie, Manabu Hirano, Kenichi Kourai, Yoshihiro Oyama, Eiji Kawai, Kenji Kono, Shigeru Chiba, Yasushi Shinjo, and Kazuhiko Kato. 2009. BitVisor: a thin hypervisor for enforcing i/o device security. In Proceedings of the 2009 ACM SIGPLAN/SIGOPS international conference on Virtual execution environments (VEE '09). Association for Computing Machinery, New York, NY, USA, 121–130. https://doi.org/10.1145/1508293.1508311
