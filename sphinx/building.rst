Building the firmware
=====================

See the MicroPython `Getting Started <https://docs.micropython.org/en/latest/develop/gettingstarted.html>`_
for more detailed information on building the MicroPython firmware.


Clone the Repositories
----------------------

.. code-block:: console

   $ git clone git@github.com:micropython/micropython.git
   $ git clone https://github.com/russhughes/gc9a01_mpy.git

Compile the cross compiler if you haven't already

.. code-block:: console

   $ make -C micropython/mpy-cross

.. _buildingr-micropython-114-thru-119:

ESP32 MicroPython 1.14 thru 1.19
--------------------------------

Change to the ESP32 port directory

.. code-block:: console

   $ cd micropython/ports/esp32

Compile the module with specified USER_C_MODULES dir

.. code-block:: console

   $ make USER_C_MODULES=../../../../gc9a01_mpy/src/micropython.cmake all

Erase the target device if this is the first time uploading this
firmware

.. code-block:: console

   $ make USER_C_MODULES=../../../../gc9a01_mpy/src/micropython.cmake erase

Upload the new firmware

.. code-block:: console

   $ make USER_C_MODULES=../../../../gc9a01_mpy/src/micropython.cmake deploy

.. _building-micropython-120-and-later:

ESP32 MicroPython 1.20 and later
--------------------------------

Change to the ESP32 port directory, and build the firmware

.. code-block:: console

   $ cd micropython/ports/esp32

   $ make \
       BOARD=ESP32_GENERIC \
       BOARD_VARIANT=SPIRAM \
       USER_C_MODULES=../../../../gc9a01_mpy/src/micropython.cmake \
       FROZEN_MANIFEST=../../../../gc9a01_mpy/manifest.py \
       clean submodules all

Erase the flash and deploy on your device

.. code-block:: console

   $ make \
       BOARD=ESP32_GENERIC \
       BOARD_VARIANT=SPIRAM \
       USER_C_MODULES=../../../../gc9a01_mpy/src/micropython.cmake \
       FROZEN_MANIFEST=../../../../gc9a01_mpy/manifest.py \
       erase deploy

RP2040 MicroPython 1.20 and later
---------------------------------

Change to the RP2 port directory, and build the firmware

.. code-block:: console

    $ cd micropython/ports/rp2
    $ make \
        BOARD=RPI_PICO \
        FROZEN_MANIFEST=../../../../gc9a01c/manifest.py \
        USER_C_MODULES=../../../gc9a01c/src/micropython.cmake \
        submodules clean all

Flash the firmware.uf2 file from the build-${BOARD} directory to your device.
