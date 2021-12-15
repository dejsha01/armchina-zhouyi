# armchina-zhouyi
ArmChina Zhouyi NPU Linux Driver

This repo contains the source code of Zhouyi NPU User Mode Driver (UMD)
and Kernel Mode Driver (KMD), and an test application example:

	UMD: driver/umd
	KMD: driver/kmd
	example: test

This driver supports the AI inference tasks on Zhouyi Z1 and Z2 NPUs, and
simulators of them.

To build the driver binaries and test application, you should setup correct
environment variables for your platform in env_setup.sh, including toolchains,
kernel info., etc.

Then execute the following build commands:

	$ source env_setup.sh
	$ ./build_all.sh -p [your platform] -k [kernel version] -v [NPU version]

Assumed that the devicetree and all low level configurations are done by the
SoC vendor BSP engineers as the dt-binding doc required.

After a successful building, copy the binaries in ./bin to the file system of
your target platform. Use the run.sh script to try the test application:

	$ source env_setup.sh
	$./run.sh -c [benchmark path]

For simulation tests, you should specify the path of simulator binaries in env_setup.sh
and try the above commands. We will try to upload the simulator binaries after
getting some internal review approvals.

This open source project is just a start and we will contribute to and improve
it soon to make the build/run flow of this driver be more user friendly.
