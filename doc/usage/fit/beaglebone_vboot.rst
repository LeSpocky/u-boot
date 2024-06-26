.. SPDX-License-Identifier: GPL-2.0+

Verified Boot on the Beaglebone Black
=====================================

Introduction
------------

Before reading this, please read :doc:`verified-boot` and :doc:`signature`.
These instructions are for mainline U-Boot from v2014.07 onwards.

There is quite a bit of documentation in this directory describing how
verified boot works in U-Boot. There is also a test which runs through the
entire process of signing an image and running U-Boot (sandbox) to check it.
However, it might be useful to also have an example on a real board.

Beaglebone Black is a fairly common board so seems to be a reasonable choice
for an example of how to enable verified boot using U-Boot.

First a note that may to help avoid confusion. U-Boot and Linux both use
device tree. They may use the same device tree source, but it is seldom useful
for them to use the exact same binary from the same place. More typically,
U-Boot has its device tree packaged with it, and the kernel's device tree is
packaged with the kernel. In particular this is important with verified boot,
since U-Boot's device tree must be immutable. If it can be changed then the
public keys can be changed and verified boot is useless. An attacker can
simply generate a new key and put his public key into U-Boot so that
everything verifies. On the other hand the kernel's device tree typically
changes when the kernel changes, so it is useful to package an updated device
tree with the kernel binary. U-Boot supports the latter with its flexible FIT
format (Flat Image Tree).


Overview
--------

The steps are roughly as follows:

#. Build U-Boot for the board, with the verified boot options enabled.

#. Obtain a suitable Linux kernel

#. Create a Image Tree Source file (ITS) file describing how you want the
   kernel to be packaged, compressed and signed.

#. Create a key pair

#. Sign the kernel

#. Put the public key into U-Boot's image

#. Put U-Boot and the kernel onto the board

#. Try it


Step 1: Build U-Boot
--------------------

a. Set up the environment variable to point to your toolchain. You will need
   this for U-Boot and also for the kernel if you build it. For example if you
   installed a Linaro version manually it might be something like::

       export CROSS_COMPILE=/opt/linaro/gcc-linaro-arm-linux-gnueabihf-4.8-2013.08_linux/bin/arm-linux-gnueabihf-

   or if you just installed gcc-arm-linux-gnueabi then it might be::

       export CROSS_COMPILE=arm-linux-gnueabi-

b. Configure and build U-Boot with verified boot enabled. Note that we use the
am335x_evm target since it covers all boards based on the AM335x evaluation
board::

    export UBOOT=/path/to/u-boot
    cd $UBOOT
    # You can add -j10 if you have 10 CPUs to make it faster
    make O=b/am335x_evm am335x_evm_config all
    export UOUT=$UBOOT/b/am335x_evm

c. You will now have a U-Boot image::

    file b/am335x_evm/u-boot-dtb.img
    b/am335x_evm/u-boot-dtb.img: u-boot legacy uImage,
      U-Boot 2014.07-rc2-00065-g2f69f8, Firmware/ARM, Firmware Image
      (Not compressed), 395375 bytes, Sat May 31 16:19:04 2014,
      Load Address: 0x80800000, Entry Point: 0x00000000,
      Header CRC: 0x0ABD6ACA, Data CRC: 0x36DEF7E4


Step 2: Build Linux
-------------------

a. Find the kernel image ('Image') and device tree (.dtb) file you plan to
   use. In our case it is am335x-boneblack.dtb and it is built with the kernel.
   At the time of writing an SD Boot image can be obtained from here::

       http://www.elinux.org/Beagleboard:Updating_The_Software#Image_For_Booting_From_microSD

   You can write this to an SD card and then mount it to extract the kernel and
   device tree files.

   You can also build a kernel. Instructions for this are are here::

       http://elinux.org/Building_BBB_Kernel

   or you can use your favourite search engine. Following these instructions
   produces a kernel Image and device tree files. For the record the steps
   were::

        export KERNEL=/path/to/kernel
        cd $KERNEL
        git clone git://github.com/beagleboard/kernel.git .
        git checkout v3.14
        ./patch.sh
        cp configs/beaglebone kernel/arch/arm/configs/beaglebone_defconfig
        cd kernel
        make beaglebone_defconfig
        make uImage dtbs   # -j10 if you have 10 CPUs
        export OKERNEL=$KERNEL/kernel/arch/arm/boot

b. You now have the 'Image' and 'am335x-boneblack.dtb' files needed to boot.


Step 3: Create the ITS
----------------------

Set up a directory for your work::

   export WORK=/path/to/dir
   cd $WORK

Put this into a file in that directory called sign.its::

    /dts-v1/;

    / {
        description = "Beaglebone black";
        #address-cells = <1>;

        images {
            kernel {
                data = /incbin/("Image.lzo");
                type = "kernel";
                arch = "arm";
                os = "linux";
                compression = "lzo";
                load = <0x80008000>;
                entry = <0x80008000>;
                hash-1 {
                    algo = "sha256";
                };
            };
            fdt-1 {
                description = "beaglebone-black";
                data = /incbin/("am335x-boneblack.dtb");
                type = "flat_dt";
                arch = "arm";
                compression = "none";
                hash-1 {
                    algo = "sha256";
                };
            };
        };
        configurations {
            default = "conf-1";
            conf-1 {
                kernel = "kernel";
                fdt = "fdt-1";
                signature-1 {
                    algo = "sha256,rsa2048";
                    key-name-hint = "dev";
                    sign-images = "fdt", "kernel";
                };
            };
        };
    };


The explanation for this is all in the documentation you have already read.
But briefly it packages a kernel and device tree, and provides a single
configuration to be signed with a key named 'dev'. The kernel is compressed
with LZO to make it smaller.


Step 4: Create a key pair
-------------------------

See :doc:`signature` for details on this step::

   cd $WORK
   mkdir keys
   openssl genrsa -F4 -out keys/dev.key 2048
   openssl req -batch -new -x509 -key keys/dev.key -out keys/dev.crt

Note: keys/dev.key contains your private key and is very secret. If anyone
gets access to that file they can sign kernels with it. Keep it secure.


Step 5: Sign the kernel
-----------------------

We need to use mkimage (which was built when you built U-Boot) to package the
Linux kernel into a FIT (Flat Image Tree, a flexible file format that U-Boot
can load) using the ITS file you just created.

At the same time we must put the public key into U-Boot device tree, with the
'required' property, which tells U-Boot that this key must be verified for the
image to be valid. You will make this key available to U-Boot for booting in
step 6::

   ln -s $OKERNEL/dts/am335x-boneblack.dtb
   ln -s $OKERNEL/Image
   ln -s $UOUT/u-boot-dtb.img
   cp $UOUT/arch/arm/dts/am335x-boneblack.dtb am335x-boneblack-pubkey.dtb
   lzop Image
   $UOUT/tools/mkimage -f sign.its -K am335x-boneblack-pubkey.dtb -k keys -r image.fit

You should see something like this::

    FIT description: Beaglebone black
    Created:         Sun Jun  1 12:50:30 2014
     Image 0 (kernel)
      Description:  unavailable
      Created:      Sun Jun  1 12:50:30 2014
      Type:         Kernel Image
      Compression:  lzo compressed
      Data Size:    7790938 Bytes = 7608.34 kB = 7.43 MB
      Architecture: ARM
      OS:           Linux
      Load Address: 0x80008000
      Entry Point:  0x80008000
      Hash algo:    sha256
      Hash value:   51b2adf9c1016ed46f424d85dcc6c34c46a20b9bee7227e06a6b6320ca5d35c1
     Image 1 (fdt-1)
      Description:  beaglebone-black
      Created:      Sun Jun  1 12:50:30 2014
      Type:         Flat Device Tree
      Compression:  uncompressed
      Data Size:    31547 Bytes = 30.81 kB = 0.03 MB
      Architecture: ARM
      Hash algo:    sha256
      Hash value:   807d5842a04132261ba092373bd40c78991bc7ce173d1175cd976ec37858e7cd
     Default Configuration: 'conf-1'
     Configuration 0 (conf-1)
      Description:  unavailable
      Kernel:       kernel
      FDT:          fdt-1


Now am335x-boneblack-pubkey.dtb contains the public key and image.fit contains
the signed kernel. Jump to step 6 if you like, or continue reading to increase
your understanding.

You can also run fit_check_sign to check it::

   $UOUT/tools/fit_check_sign -f image.fit -k am335x-boneblack-pubkey.dtb

which results in::

    Verifying Hash Integrity ... sha256,rsa2048:dev+
    ## Loading kernel from FIT Image at 7fc6ee469000 ...
       Using 'conf-1' configuration
       Verifying Hash Integrity ...
    sha256,rsa2048:dev+
    OK

       Trying 'kernel' kernel subimage
         Description:  unavailable
         Created:      Sun Jun  1 12:50:30 2014
         Type:         Kernel Image
         Compression:  lzo compressed
         Data Size:    7790938 Bytes = 7608.34 kB = 7.43 MB
         Architecture: ARM
         OS:           Linux
         Load Address: 0x80008000
         Entry Point:  0x80008000
         Hash algo:    sha256
         Hash value:   51b2adf9c1016ed46f424d85dcc6c34c46a20b9bee7227e06a6b6320ca5d35c1
       Verifying Hash Integrity ...
    sha256+
    OK

    Unimplemented compression type 4
    ## Loading fdt from FIT Image at 7fc6ee469000 ...
       Using 'conf-1' configuration
       Trying 'fdt-1' fdt subimage
         Description:  beaglebone-black
         Created:      Sun Jun  1 12:50:30 2014
         Type:         Flat Device Tree
         Compression:  uncompressed
         Data Size:    31547 Bytes = 30.81 kB = 0.03 MB
         Architecture: ARM
         Hash algo:    sha256
         Hash value:   807d5842a04132261ba092373bd40c78991bc7ce173d1175cd976ec37858e7cd
       Verifying Hash Integrity ...
    sha256+
    OK

       Loading Flat Device Tree ... OK

    ## Loading ramdisk from FIT Image at 7fc6ee469000 ...
       Using 'conf-1' configuration
    Could not find subimage node

    Signature check OK


At the top, you see "sha256,rsa2048:dev+". This means that it checked an RSA key
of size 2048 bits using SHA256 as the hash algorithm. The key name checked was
'dev' and the '+' means that it verified. If it showed '-' that would be bad.

Once the configuration is verified it is then possible to rely on the hashes
in each image referenced by that configuration. So fit_check_sign goes on to
load each of the images. We have a kernel and an FDT but no ramkdisk. In each
case fit_check_sign checks the hash and prints sha256+ meaning that the SHA256
hash verified. This means that none of the images has been tampered with.

There is a test in test/vboot which uses U-Boot's sandbox build to verify that
the above flow works.

But it is fun to do this by hand, so you can load image.fit into a hex editor
like ghex, and change a byte in the kernel::

    $UOUT/tools/fit_info -f image.fit -n /images/kernel -p data
    NAME: kernel
    LEN: 7790938
    OFF: 168

This tells us that the kernel starts at byte offset 168 (decimal) in image.fit
and extends for about 7MB. Try changing a byte at 0x2000 (say) and run
fit_check_sign again. You should see something like::

    Verifying Hash Integrity ... sha256,rsa2048:dev+
    ## Loading kernel from FIT Image at 7f5a39571000 ...
       Using 'conf-1' configuration
       Verifying Hash Integrity ...
    sha256,rsa2048:dev+
    OK

       Trying 'kernel' kernel subimage
         Description:  unavailable
         Created:      Sun Jun  1 13:09:21 2014
         Type:         Kernel Image
         Compression:  lzo compressed
         Data Size:    7790938 Bytes = 7608.34 kB = 7.43 MB
         Architecture: ARM
         OS:           Linux
         Load Address: 0x80008000
         Entry Point:  0x80008000
         Hash algo:    sha256
         Hash value:   51b2adf9c1016ed46f424d85dcc6c34c46a20b9bee7227e06a6b6320ca5d35c1
       Verifying Hash Integrity ...
    sha256 error
    Bad hash value for 'hash-1' hash node in 'kernel' image node
    Bad Data Hash

    ## Loading fdt from FIT Image at 7f5a39571000 ...
       Using 'conf-1' configuration
       Trying 'fdt-1' fdt subimage
         Description:  beaglebone-black
         Created:      Sun Jun  1 13:09:21 2014
         Type:         Flat Device Tree
         Compression:  uncompressed
         Data Size:    31547 Bytes = 30.81 kB = 0.03 MB
         Architecture: ARM
         Hash algo:    sha256
         Hash value:   807d5842a04132261ba092373bd40c78991bc7ce173d1175cd976ec37858e7cd
       Verifying Hash Integrity ...
    sha256+
    OK

       Loading Flat Device Tree ... OK

    ## Loading ramdisk from FIT Image at 7f5a39571000 ...
       Using 'conf-1' configuration
    Could not find subimage node

    Signature check Bad (error 1)


It has detected the change in the kernel.

You can also be sneaky and try to switch images, using the libfdt utilities
that come with dtc (package name is device-tree-compiler but you will need a
recent version like 1.4::

    dtc -v
    Version: DTC 1.4.0

First we can check which nodes are actually hashed by the configuration::

    $ fdtget -l image.fit /
    images
    configurations

    $ fdtget -l image.fit /configurations
    conf-1
    fdtget -l image.fit /configurations/conf-1
    signature-1

    $ fdtget -p image.fit /configurations/conf-1/signature-1
    hashed-strings
    hashed-nodes
    timestamp
    signer-version
    signer-name
    value
    algo
    key-name-hint
    sign-images

    $ fdtget image.fit /configurations/conf-1/signature-1 hashed-nodes
    / /configurations/conf-1 /images/fdt-1 /images/fdt-1/hash /images/kernel /images/kernel/hash-1

This gives us a bit of a look into the signature that mkimage added. Note you
can also use fdtdump to list the entire device tree.

Say we want to change the kernel that this configuration uses
(/images/kernel). We could just put a new kernel in the image, but we will
need to change the hash to match. Let's simulate that by changing a byte of
the hash::

    fdtget -tx image.fit /images/kernel/hash-1 value
    51b2adf9 c1016ed4 6f424d85 dcc6c34c 46a20b9b ee7227e0 6a6b6320 ca5d35c1
    fdtput -tx image.fit /images/kernel/hash-1 value 51b2adf9 c1016ed4 6f424d85 dcc6c34c 46a20b9b ee7227e0 6a6b6320 ca5d35c8

Now check it again::

    $UOUT/tools/fit_check_sign -f image.fit -k am335x-boneblack-pubkey.dtb
    Verifying Hash Integrity ... sha256,rsa2048:devrsa_verify_with_keynode: RSA failed to verify: -13
    rsa_verify_with_keynode: RSA failed to verify: -13
    -
    Failed to verify required signature 'key-dev'
    Signature check Bad (error 1)

This time we don't even get as far as checking the images, since the
configuration signature doesn't match. We can't change any hashes without the
signature check noticing. The configuration is essentially locked. U-Boot has
a public key for which it requires a match, and will not permit the use of any
configuration that does not match that public key. The only way the
configuration will match is if it was signed by the matching private key.

It would also be possible to add a new signature node that does match your new
configuration. But that won't work since you are not allowed to change the
configuration in any way. Try it with a fresh (valid) image if you like by
running the mkimage link again. Then::

    fdtput -p image.fit /configurations/conf-1/signature-1 value fred
    $UOUT/tools/fit_check_sign -f image.fit -k am335x-boneblack-pubkey.dtb
    Verifying Hash Integrity ... -
    sha256,rsa2048:devrsa_verify_with_keynode: RSA failed to verify: -13
    rsa_verify_with_keynode: RSA failed to verify: -13
    -
    Failed to verify required signature 'key-dev'
    Signature check Bad (error 1)


Of course it would be possible to add an entirely new configuration and boot
with that, but it still needs to be signed, so it won't help.


6. Put the public key into U-Boot's image
-----------------------------------------

Having confirmed that the signature is doing its job, let's try it out in
U-Boot on the board. U-Boot needs access to the public key corresponding to
the private key that you signed with so that it can verify any kernels that
you sign::

    cd $UBOOT
    make O=b/am335x_evm EXT_DTB=${WORK}/am335x-boneblack-pubkey.dtb

Here we are overriding the normal device tree file with our one, which
contains the public key.

Now you have a special U-Boot image with the public key. It can verify can
kernel that you sign with the private key as in step 5.

If you like you can take a look at the public key information that mkimage
added to U-Boot's device tree::

    fdtget -p am335x-boneblack-pubkey.dtb /signature/key-dev
    required
    algo
    rsa,r-squared
    rsa,modulus
    rsa,n0-inverse
    rsa,num-bits
    key-name-hint

This has information about the key and some pre-processed values which U-Boot
can use to verify against it. These values are obtained from the public key
certificate by mkimage, but require quite a bit of code to generate. To save
code space in U-Boot, the information is extracted and written in raw form for
U-Boot to easily use. The same mechanism is used in Google's Chrome OS.

Notice the 'required' property. This marks the key as required - U-Boot will
not boot any image that does not verify against this key.


7. Put U-Boot and the kernel onto the board
-------------------------------------------

The method here varies depending on how you are booting. For this example we
are booting from an micro-SD card with two partitions, one for U-Boot and one
for Linux. Put it into your machine and write U-Boot and the kernel to it.
Here the card is /dev/sde::

    cd $WORK
    export UDEV=/dev/sde1   # Change thes two lines to the correct device
    export KDEV=/dev/sde2
    sudo mount $UDEV /mnt/tmp && sudo cp $UOUT/u-boot-dtb.img /mnt/tmp/u-boot.img  && sleep 1 && sudo umount $UDEV
    sudo mount $KDEV /mnt/tmp && sudo cp $WORK/image.fit /mnt/tmp/boot/image.fit && sleep 1 && sudo umount $KDEV


8. Try it
---------

Boot the board using the commands below::

    setenv bootargs console=ttyO0,115200n8 quiet root=/dev/mmcblk0p2 ro rootfstype=ext4 rootwait
    ext2load mmc 0:2 82000000 /boot/image.fit
    bootm 82000000

You should then see something like this::

    U-Boot# setenv bootargs console=ttyO0,115200n8 quiet root=/dev/mmcblk0p2 ro rootfstype=ext4 rootwait
    U-Boot# ext2load mmc 0:2 82000000 /boot/image.fit
    7824930 bytes read in 589 ms (12.7 MiB/s)
    U-Boot# bootm 82000000
    ## Loading kernel from FIT Image at 82000000 ...
       Using 'conf-1' configuration
       Verifying Hash Integrity ... sha256,rsa2048:dev+ OK
       Trying 'kernel' kernel subimage
         Description:  unavailable
         Created:      2014-06-01  19:32:54 UTC
         Type:         Kernel Image
         Compression:  lzo compressed
         Data Start:   0x820000a8
         Data Size:    7790938 Bytes = 7.4 MiB
         Architecture: ARM
         OS:           Linux
         Load Address: 0x80008000
         Entry Point:  0x80008000
         Hash algo:    sha256
         Hash value:   51b2adf9c1016ed46f424d85dcc6c34c46a20b9bee7227e06a6b6320ca5d35c1
       Verifying Hash Integrity ... sha256+ OK
    ## Loading fdt from FIT Image at 82000000 ...
       Using 'conf-1' configuration
       Trying 'fdt-1' fdt subimage
         Description:  beaglebone-black
         Created:      2014-06-01  19:32:54 UTC
         Type:         Flat Device Tree
         Compression:  uncompressed
         Data Start:   0x8276e2ec
         Data Size:    31547 Bytes = 30.8 KiB
         Architecture: ARM
         Hash algo:    sha256
         Hash value:   807d5842a04132261ba092373bd40c78991bc7ce173d1175cd976ec37858e7cd
       Verifying Hash Integrity ... sha256+ OK
       Booting using the fdt blob at 0x8276e2ec
       Uncompressing Kernel Image ... OK
       Loading Device Tree to 8fff5000, end 8ffffb3a ... OK

    Starting kernel ...

    [    0.582377] omap_init_mbox: hwmod doesn't have valid attrs
    [    2.589651] musb-hdrc musb-hdrc.0.auto: Failed to request rx1.
    [    2.595830] musb-hdrc musb-hdrc.0.auto: musb_init_controller failed with status -517
    [    2.606470] musb-hdrc musb-hdrc.1.auto: Failed to request rx1.
    [    2.612723] musb-hdrc musb-hdrc.1.auto: musb_init_controller failed with status -517
    [    2.940808] drivers/rtc/hctosys.c: unable to open rtc device (rtc0)
    [    7.248889] libphy: PHY 4a101000.mdio:01 not found
    [    7.253995] net eth0: phy 4a101000.mdio:01 not found on slave 1
    systemd-fsck[83]: Angstrom: clean, 50607/218160 files, 306348/872448 blocks

    .---O---.
    |       |                  .-.           o o
    |   |   |-----.-----.-----.| |   .----..-----.-----.
    |       |     | __  |  ---'| '--.|  .-'|     |     |
    |   |   |  |  |     |---  ||  --'|  |  |  '  | | | |
    '---'---'--'--'--.  |-----''----''--'  '-----'-'-'-'
                    -'  |
                    '---'

    The Angstrom Distribution beaglebone ttyO0

    Angstrom v2012.12 - Kernel 3.14.1+

    beaglebone login:

At this point your kernel has been verified and you can be sure that it is one
that you signed. As an exercise, try changing image.fit as in step 5 and see
what happens.


Further Improvements
--------------------

Several of the steps here can be easily automated. In particular it would be
capital if signing and packaging a kernel were easy, perhaps a simple make
target in the kernel. A starting point for this is the 'make image.fit' target
for ARM64 in Linux from v6.9 onwards.

Some mention of how to use multiple .dtb files in a FIT might be useful.

Perhaps the verified boot feature could be integrated into the Amstrom
distribution.


.. sectionauthor:: Simon Glass <sjg@chromium.org>, 2-June-14
